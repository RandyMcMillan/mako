/*!
 * mempool.c - mempool for mako
 * Copyright (c) 2021, Christopher Jeffrey (MIT License).
 * https://github.com/chjj/mako
 */

#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include <io/core.h>

#include <node/chain.h>
#include <node/logger.h>
#include <node/mempool.h>
#include <node/timedata.h>

#include <mako/block.h>
#include <mako/bloom.h>
#include <mako/coins.h>
#include <mako/consensus.h>
#include <mako/crypto/hash.h>
#include <mako/crypto/rand.h>
#include <mako/entry.h>
#include <mako/error.h>
#include <mako/header.h>
#include <mako/heap.h>
#include <mako/map.h>
#include <mako/network.h>
#include <mako/policy.h>
#include <mako/script.h>
#include <mako/tx.h>
#include <mako/util.h>
#include <mako/vector.h>

#include "../impl.h"
#include "../internal.h"

/*
 * Orphan Transaction
 */

typedef struct btc_orphan_s {
  const uint8_t *hash;
  btc_tx_t *tx;
  int missing;
  unsigned int id;
} btc_orphan_t;

DEFINE_OBJECT(btc_orphan, SCOPE_STATIC)

static void
btc_orphan_init(btc_orphan_t *orphan) {
  memset(orphan, 0, sizeof(*orphan));
}

static void
btc_orphan_clear(btc_orphan_t *orphan) {
  if (orphan->tx != NULL)
    btc_tx_destroy(orphan->tx);

  orphan->tx = NULL;
}

static void
btc_orphan_copy(btc_orphan_t *z, const btc_orphan_t *x) {
  *z = *x;
}

/**
 * Mempool Entry
 */

DEFINE_SERIALIZABLE_OBJECT(btc_mpentry, SCOPE_STATIC)

static void
btc_mpentry_init(btc_mpentry_t *entry) {
  entry->tx = NULL;
  entry->hash = NULL;
  entry->whash = NULL;
  entry->height = -1;
  entry->size = 0;
  entry->sigops = 0;
  entry->fee = 0;
  entry->delta_fee = 0;
  entry->time = 0;
  entry->coinbase = 0;
  entry->locks = 0;
  entry->desc_fee = 0;
  entry->desc_size = 0;
}

static void
btc_mpentry_clear(btc_mpentry_t *entry) {
  if (entry->tx != NULL)
    btc_tx_destroy(entry->tx);

  entry->tx = NULL;
}

static void
btc_mpentry_copy(btc_mpentry_t *z, const btc_mpentry_t *x) {
  z->tx = btc_tx_ref(x->tx);
  z->hash = z->tx->hash;
  z->whash = z->tx->whash;
  z->height = x->height;
  z->size = x->size;
  z->sigops = x->sigops;
  z->fee = x->fee;
  z->delta_fee = x->delta_fee;
  z->time = x->time;
  z->coinbase = x->coinbase;
  z->locks = x->locks;
  z->desc_fee = x->desc_fee;
  z->desc_size = x->desc_size;
}

static void
btc_mpentry_set(btc_mpentry_t *entry,
                const btc_tx_t *tx,
                const btc_view_t *view,
                int32_t height,
                int64_t fee) {
  unsigned int flags = BTC_SCRIPT_STANDARD_VERIFY_FLAGS;
  int sigops = btc_tx_sigops_cost(tx, view, flags);
  size_t size = btc_tx_sigops_size(tx, sigops);
  int coinbase = 0;
  int locks = 0;
  size_t i;

  for (i = 0; i < tx->inputs.length; i++) {
    const btc_input_t *input = tx->inputs.items[i];
    const btc_coin_t *coin = btc_view_get(view, &input->prevout);

    CHECK(coin != NULL);

    if (coin->coinbase)
      coinbase = 1;

    if (!(input->sequence & BTC_SEQUENCE_DISABLE_FLAG))
      locks = (tx->version >= 2);
  }

  entry->tx = btc_tx_refconst(tx);
  entry->hash = entry->tx->hash;
  entry->whash = entry->tx->whash;
  entry->height = height;
  entry->size = size;
  entry->sigops = sigops;
  entry->fee = fee;
  entry->delta_fee = fee;
  entry->time = btc_now();
  entry->coinbase = coinbase;
  entry->locks = locks;
  entry->desc_fee = fee;
  entry->desc_size = size;
}

static size_t
btc_mpentry_size(const btc_mpentry_t *x) {
  return btc_tx_size(x->tx) + 30;
}

static uint8_t *
btc_mpentry_write(uint8_t *zp, const btc_mpentry_t *x) {
  zp = btc_tx_write(zp, x->tx);
  zp = btc_int32_write(zp, x->height);
  zp = btc_uint32_write(zp, x->size);
  zp = btc_uint32_write(zp, x->sigops);
  zp = btc_int64_write(zp, x->fee);
  zp = btc_int64_write(zp, x->time);
  zp = btc_uint8_write(zp, x->coinbase);
  zp = btc_uint8_write(zp, x->locks);
  return zp;
}

static int
btc_mpentry_read(btc_mpentry_t *z, const uint8_t **xp, size_t *xn) {
  z->tx = btc_tx_create();

  if (!btc_tx_read(z->tx, xp, xn))
    return 0;

  if (!btc_int32_read(&z->height, xp, xn))
    return 0;

  if (!btc_uint32_read(&z->size, xp, xn))
    return 0;

  if (!btc_uint32_read(&z->sigops, xp, xn))
    return 0;

  if (!btc_int64_read(&z->fee, xp, xn))
    return 0;

  if (!btc_int64_read(&z->time, xp, xn))
    return 0;

  if (!btc_uint8_read(&z->coinbase, xp, xn))
    return 0;

  if (!btc_uint8_read(&z->locks, xp, xn))
    return 0;

  z->hash = z->tx->hash;
  z->whash = z->tx->whash;

  return 1;
}

/*
 * Mempool
 */

struct btc_mempool_s {
  const btc_network_t *network;
  btc_logger_t *logger;
  const btc_timedata_t *timedata;
  btc_chain_t *chain;
  size_t size;
  btc_hashmap_t *map;
  btc_hashmap_t *waiting;
  btc_hashmap_t *orphans;
  btc_outmap_t *spents;
  btc_filter_t rejects;
  unsigned int flags;
  char file[BTC_PATH_MAX];
  btc_mempool_tx_cb *on_tx;
  btc_mempool_badorphan_cb *on_badorphan;
  void *arg;
};

BTC_DEFINE_LOGGER(btc_log, btc_mempool_t, "mempool");

btc_mempool_t *
btc_mempool_create(const btc_network_t *network, btc_chain_t *chain) {
  btc_mempool_t *mp = (btc_mempool_t *)btc_malloc(sizeof(btc_mempool_t));

  memset(mp, 0, sizeof(*mp));

  mp->network = network;
  mp->chain = chain;
  mp->map = btc_hashmap_create();
  mp->waiting = btc_hashmap_create(); /* orphan prevout hashes */
  mp->orphans = btc_hashmap_create();
  mp->spents = btc_outmap_create(); /* mempool entry's outpoints */
  mp->flags = BTC_MEMPOOL_DEFAULT_FLAGS;
  mp->file[0] = '\0';

  btc_filter_init(&mp->rejects);
  btc_filter_set(&mp->rejects, 120000, 0.000001);

  return mp;
}

void
btc_mempool_destroy(btc_mempool_t *mp) {
  btc_hashmapiter_t iter;

  btc_hashmap_iterate(&iter, mp->map);

  while (btc_hashmap_next(&iter))
    btc_mpentry_destroy(iter.val);

  btc_hashmap_iterate(&iter, mp->waiting);

  while (btc_hashmap_next(&iter)) {
    btc_free(iter.key);
    btc_hashset_destroy(iter.val);
  }

  btc_hashmap_iterate(&iter, mp->orphans);

  while (btc_hashmap_next(&iter))
    btc_orphan_destroy(iter.val);

  btc_hashmap_destroy(mp->map);
  btc_hashmap_destroy(mp->waiting);
  btc_hashmap_destroy(mp->orphans);
  btc_outmap_destroy(mp->spents);
  btc_filter_clear(&mp->rejects);

  btc_free(mp);
}

void
btc_mempool_set_logger(btc_mempool_t *mp, btc_logger_t *logger) {
  mp->logger = logger;
}

void
btc_mempool_set_timedata(btc_mempool_t *mp, const btc_timedata_t *td) {
  mp->timedata = td;
}

void
btc_mempool_on_tx(btc_mempool_t *mp, btc_mempool_tx_cb *handler) {
  mp->on_tx = handler;
}

void
btc_mempool_on_badorphan(btc_mempool_t *mp,
                         btc_mempool_badorphan_cb *handler) {
  mp->on_badorphan = handler;
}

void
btc_mempool_set_context(btc_mempool_t *mp, void *arg) {
  mp->arg = arg;
}

int
btc_mempool_open(btc_mempool_t *mp, const char *prefix, unsigned int flags) {
  mp->flags = flags;

  if (prefix != NULL) {
    if (!btc_path_resolve(mp->file, sizeof(mp->file),
                          prefix, "mempool.dat", NULL)) {
      return 0;
    }
  }

  btc_log_info(mp, "Opening mempool.");

  return 1;
}

void
btc_mempool_close(btc_mempool_t *mp) {
  btc_log_info(mp, "Closing mempool.");
}

/*
 * Orphan Handling
 */

static int
btc_mempool_remove_orphan(btc_mempool_t *mp, const uint8_t *hash) {
  btc_orphan_t *orphan = btc_hashmap_get(mp->orphans, hash);
  const btc_tx_t *tx;
  size_t i;

  if (orphan == NULL)
    return 0;

  tx = orphan->tx;

  for (i = 0; i < tx->inputs.length; i++) {
    const btc_input_t *input = tx->inputs.items[i];
    const btc_outpoint_t *prevout = &input->prevout;
    btc_hashset_t *set = btc_hashmap_get(mp->waiting, prevout->hash);

    if (set == NULL)
      continue;

    btc_hashset_del(set, hash);

    if (btc_hashset_size(set) == 0) {
      btc_free(btc_hashmap_del(mp->waiting, prevout->hash));
      btc_hashset_destroy(set);
    }
  }

  btc_hashmap_del(mp->orphans, hash);
  btc_orphan_destroy(orphan);

  return 1;
}

static int
btc_mempool_limit_orphans(btc_mempool_t *mp) {
  const uint8_t *hash = NULL;
  btc_hashmapiter_t iter;
  size_t index;

  if (btc_hashmap_size(mp->orphans) < BTC_MEMPOOL_MAX_ORPHANS)
    return 0;

  index = btc_uniform(btc_hashmap_size(mp->orphans));

  btc_hashmap_iterate(&iter, mp->orphans);

  while (btc_hashmap_next(&iter)) {
    hash = iter.key;

    if (index == 0)
      break;

    index--;
  }

  CHECK(hash != NULL);

  btc_log_debug(mp, "Removing orphan %H from mempool.", hash);

  btc_mempool_remove_orphan(mp, hash);

  return 1;
}

static int
btc_tx_has_coins(const btc_tx_t *tx, const btc_view_t *view) {
  size_t i;

  for (i = 0; i < tx->inputs.length; i++) {
    const btc_input_t *input = tx->inputs.items[i];

    if (!btc_view_has(view, &input->prevout))
      return 0;
  }

  return 1;
}

static btc_errno_t
btc_mempool_check_orphan(btc_mempool_t *mp,
                         const btc_tx_t *tx,
                         const btc_view_t *view) {
  size_t i;

  for (i = 0; i < tx->inputs.length; i++) {
    const btc_input_t *input = tx->inputs.items[i];
    const btc_outpoint_t *prevout = &input->prevout;

    if (btc_view_has(view, prevout))
      continue;

    if (btc_mempool_has_reject(mp, prevout->hash)) {
      btc_log_debug(mp, "Not storing orphan %H (rejected parents).",
                        tx->hash);
      return BTC_ERR_DUPLICATE; /* Need better error here (with 0, 0). */
    }

    if (btc_mempool_has(mp, prevout->hash)) {
      btc_log_debug(mp, "Not storing orphan %H (non-existent output).",
                        tx->hash);
      return BTC_ERR_INPUTS_MISSING;
    }
  }

  /* Weight limit for orphans. */
  if (btc_tx_weight(tx) > BTC_MAX_TX_WEIGHT) {
    btc_log_debug(mp, "Ignoring large orphan %H.", tx->hash);
    return BTC_ERR_TX_WEIGHT;
  }

  return BTC_OK;
}

static void
btc_mempool_add_orphan(btc_mempool_t *mp,
                       const btc_tx_t *tx,
                       const btc_view_t *view,
                       unsigned int id) {
  btc_orphan_t *orphan = btc_orphan_create();
  btc_hashset_t *hashes = btc_hashset_create();
  btc_hashsetiter_t iter;
  size_t i;

  orphan->tx = btc_tx_refconst(tx);
  orphan->hash = orphan->tx->hash;
  orphan->missing = 0;
  orphan->id = id;

  btc_mempool_limit_orphans(mp);

  for (i = 0; i < orphan->tx->inputs.length; i++) {
    const btc_input_t *input = orphan->tx->inputs.items[i];

    if (!btc_view_has(view, &input->prevout))
      btc_hashset_put(hashes, input->prevout.hash);
  }

  btc_hashset_iterate(&iter, hashes);

  while (btc_hashset_next(&iter)) {
    const uint8_t *prev = iter.key;

    if (!btc_hashmap_has(mp->waiting, prev))
      btc_hashmap_put(mp->waiting, btc_hash_clone(prev), btc_hashset_create());

    btc_hashset_put(btc_hashmap_get(mp->waiting, prev), orphan->hash);

    orphan->missing++;
  }

  btc_hashset_destroy(hashes);

  CHECK(btc_hashmap_put(mp->orphans, orphan->hash, orphan));

  btc_log_debug(mp, "Added orphan %H to mempool.", tx->hash);
}

static btc_vector_t *
btc_mempool_resolve_orphans(btc_mempool_t *mp, const uint8_t *parent) {
  btc_hashset_t *set = btc_hashmap_get(mp->waiting, parent);
  btc_vector_t *resolved;
  btc_hashsetiter_t iter;

  if (set == NULL)
    return NULL;

  CHECK(btc_hashset_size(set) > 0);

  resolved = btc_vector_create();

  btc_hashset_iterate(&iter, set);

  while (btc_hashset_next(&iter)) {
    const uint8_t *hash = iter.key;
    btc_orphan_t *orphan = btc_hashmap_get(mp->orphans, hash);

    CHECK(orphan != NULL);

    if (--orphan->missing == 0) {
      btc_hashmap_del(mp->orphans, hash);
      btc_vector_push(resolved, orphan);
    }
  }

  btc_free(btc_hashmap_del(mp->waiting, parent));

  btc_hashset_destroy(set);

  return resolved;
}

static void
btc_mempool_handle_orphans(btc_mempool_t *mp, const uint8_t *parent) {
  btc_vector_t *resolved = btc_mempool_resolve_orphans(mp, parent);
  uint8_t hash[32];
  size_t i;
  int rc;

  if (resolved == NULL)
    return;

  for (i = 0; i < resolved->length; i++) {
    btc_orphan_t *orphan = resolved->items[i];

    if ((rc = btc_mempool_add(mp, orphan->tx, orphan->id))) {
      btc_log_debug(mp, "Could not resolve orphan %H: %s.",
                        orphan->hash, btc_error_reason(rc));

      if (mp->on_badorphan != NULL)
        mp->on_badorphan(orphan->hash, rc, orphan->id, mp->arg);

      btc_orphan_destroy(orphan);

      continue;
    }

    btc_hash_copy(hash, orphan->hash);

    btc_orphan_destroy(orphan);

    /* Can happen if an existing parent is
       evicted in the interim between fetching
       the non-present parents. */
    if (btc_hashmap_has(mp->orphans, hash)) {
      btc_log_debug(mp, "Transaction %H was double-orphaned in mempool.",
                        hash);
      btc_mempool_remove_orphan(mp, hash);
      continue;
    }

    btc_log_debug(mp, "Resolved orphan %H in mempool.", hash);
  }

  btc_vector_destroy(resolved);
}

/*
 * UTXO Handling
 */

static btc_view_t *
btc_mempool_view(btc_mempool_t *mp, const btc_tx_t *tx) {
  btc_view_t *view = btc_view_create();
  size_t i;

  for (i = 0; i < tx->inputs.length; i++) {
    const btc_input_t *input = tx->inputs.items[i];
    const btc_outpoint_t *prevout = &input->prevout;
    btc_mpentry_t *parent = btc_hashmap_get(mp->map, prevout->hash);
    btc_coin_t *coin;

    if (parent == NULL)
      continue;

    if (prevout->index >= parent->tx->outputs.length)
      continue;

    coin = btc_tx_coin(parent->tx, prevout->index, -1);

    btc_view_put(view, prevout, coin);
  }

  btc_chain_get_coins(mp->chain, view, tx);

  return view;
}

/*
 * Entry Handling
 */

static void
add_fee(btc_mpentry_t *parent, const btc_mpentry_t *child) {
  parent->desc_fee += child->delta_fee;
  parent->desc_size += child->size;
}

static void
remove_fee(btc_mpentry_t *parent, const btc_mpentry_t *child) {
  parent->desc_fee -= child->desc_fee;
  parent->desc_size -= child->desc_size;
}

BTC_UNUSED static void
preprioritise(btc_mpentry_t *parent, const btc_mpentry_t *child) {
  parent->desc_fee -= child->delta_fee;
}

BTC_UNUSED static void
postprioritise(btc_mpentry_t *parent, const btc_mpentry_t *child) {
  parent->desc_fee += child->delta_fee;
}

static size_t
traverse_ancestors(btc_mempool_t *mp,
                   const btc_mpentry_t *entry,
                   btc_hashset_t *set,
                   const btc_mpentry_t *child,
                   void (*map)(btc_mpentry_t *,
                               const btc_mpentry_t *)) {
  const btc_tx_t *tx = entry->tx;
  size_t i;

  for (i = 0; i < tx->inputs.length; i++) {
    const btc_input_t *input = tx->inputs.items[i];
    const btc_outpoint_t *prevout = &input->prevout;
    btc_mpentry_t *parent = btc_hashmap_get(mp->map, prevout->hash);

    if (parent == NULL)
      continue;

    if (btc_hashset_has(set, parent->hash))
      continue;

    btc_hashset_put(set, parent->hash);

    if (map != NULL)
      map(parent, child);

    if (btc_hashset_size(set) > BTC_MEMPOOL_MAX_ANCESTORS)
      break;

    traverse_ancestors(mp, parent, set, child, map);

    if (btc_hashset_size(set) > BTC_MEMPOOL_MAX_ANCESTORS)
      break;
  }

  return btc_hashset_size(set);
}

static size_t
btc_mempool_update_ancestors(btc_mempool_t *mp,
                             const btc_mpentry_t *entry,
                             void (*map)(btc_mpentry_t *,
                                         const btc_mpentry_t *)) {
  btc_hashset_t *set = btc_hashset_create();
  size_t count = traverse_ancestors(mp, entry, set, entry, map);

  btc_hashset_destroy(set);

  return count;
}

static size_t
btc_mempool_count_ancestors(btc_mempool_t *mp,
                            const btc_mpentry_t *entry) {
  return btc_mempool_update_ancestors(mp, entry, NULL);
}

static int
btc_mempool_exists(btc_mempool_t *mp, const uint8_t *hash) {
  if (btc_hashmap_has(mp->orphans, hash))
    return 1;

  return btc_hashmap_has(mp->map, hash);
}

static int
btc_mempool_is_double_spend(btc_mempool_t *mp, const btc_tx_t *tx) {
  size_t i;

  for (i = 0; i < tx->inputs.length; i++) {
    const btc_input_t *input = tx->inputs.items[i];

    if (btc_outmap_has(mp->spents, &input->prevout))
      return 1;
  }

  return 0;
}

static void
btc_mempool_track_entry(btc_mempool_t *mp, btc_mpentry_t *entry) {
  const btc_tx_t *tx = entry->tx;
  size_t i;

  CHECK(!btc_tx_is_coinbase(tx));
  CHECK(btc_hashmap_put(mp->map, entry->hash, entry));

  for (i = 0; i < tx->inputs.length; i++) {
    const btc_input_t *input = tx->inputs.items[i];

    btc_outmap_put(mp->spents, &input->prevout, entry);
  }

  mp->size += entry->size;
}

static void
btc_mempool_add_entry(btc_mempool_t *mp,
                      btc_mpentry_t *entry,
                      const btc_view_t *view) {
  btc_mempool_track_entry(mp, entry);
  btc_mempool_update_ancestors(mp, entry, add_fee);

  if (mp->on_tx != NULL)
    mp->on_tx(entry, view, mp->arg);

  btc_log_debug(mp, "Added %H to mempool (txs=%zu).",
                    entry->hash, btc_hashmap_size(mp->map));

  btc_mempool_handle_orphans(mp, entry->hash);
}

static void
btc_mempool_untrack_entry(btc_mempool_t *mp,
                          const btc_mpentry_t *entry) {
  const btc_tx_t *tx = entry->tx;
  size_t i;

  CHECK(!btc_tx_is_coinbase(tx));
  CHECK(btc_hashmap_del(mp->map, entry->hash));

  for (i = 0; i < tx->inputs.length; i++) {
    const btc_input_t *input = tx->inputs.items[i];

    CHECK(btc_outmap_del(mp->spents, &input->prevout));
  }

  mp->size -= entry->size;
}

static void
btc_mempool_remove_entry(btc_mempool_t *mp, btc_mpentry_t *entry) {
  btc_mempool_untrack_entry(mp, entry);
  btc_mpentry_destroy(entry);
}

static void
btc_mempool_remove_spenders(btc_mempool_t *mp,
                            const btc_mpentry_t *entry) {
  btc_mpentry_t *spender;
  btc_outpoint_t prevout;
  size_t i;

  for (i = 0; i < entry->tx->outputs.length; i++) {
    btc_outpoint_set(&prevout, entry->hash, i);

    spender = btc_outmap_get(mp->spents, &prevout);

    if (spender == NULL)
      continue;

    btc_mempool_remove_spenders(mp, spender);
    btc_mempool_remove_entry(mp, spender);
  }
}

static void
btc_mempool_evict_entry(btc_mempool_t *mp, btc_mpentry_t *entry) {
  btc_mempool_remove_spenders(mp, entry);
  btc_mempool_update_ancestors(mp, entry, remove_fee);
  btc_mempool_remove_entry(mp, entry);
}

static void
btc_mempool_remove_double_spends(btc_mempool_t *mp, const btc_tx_t *tx) {
  size_t i;

  for (i = 0; i < tx->inputs.length; i++) {
    const btc_input_t *input = tx->inputs.items[i];
    btc_mpentry_t *spent = btc_outmap_get(mp->spents, &input->prevout);

    if (spent == NULL)
      continue;

    btc_log_debug(mp, "Removing double spender from mempool: %H.",
                      spent->hash);

    btc_mempool_evict_entry(mp, spent);
  }
}

static int
btc_mempool_has_dependencies(btc_mempool_t *mp, const btc_tx_t *tx) {
  size_t i;

  for (i = 0; i < tx->inputs.length; i++) {
    const btc_input_t *input = tx->inputs.items[i];

    if (btc_hashmap_has(mp->map, input->prevout.hash))
      return 1;
  }

  return 0;
}

static int
use_desc(const btc_mpentry_t *a) {
  int64_t x = a->delta_fee * a->desc_size;
  int64_t y = a->desc_fee * a->size;
  return y > x;
}

static int64_t
cmp_rate(const void *ap, const void *bp) {
  const btc_mpentry_t *a = ap;
  const btc_mpentry_t *b = bp;

  int64_t xf = a->delta_fee;
  int64_t xs = a->size;
  int64_t yf = b->delta_fee;
  int64_t ys = b->size;
  int64_t x, y;

  if (use_desc(a)) {
    xf = a->desc_fee;
    xs = a->desc_size;
  }

  if (use_desc(b)) {
    yf = b->desc_fee;
    ys = b->desc_size;
  }

  x = xf * ys;
  y = xs * yf;

  if (x == y) {
    x = a->time;
    y = b->time;
  }

  return x - y;
}

static int
btc_mempool_limit_size(btc_mempool_t *mp, const uint8_t *added) {
  btc_hashmapiter_t iter;
  btc_vector_t queue;
  int64_t now;

  if (mp->size <= BTC_MEMPOOL_MAX_SIZE)
    return 0;

  now = btc_now();

  btc_vector_init(&queue);

  btc_hashmap_iterate(&iter, mp->map);

  while (btc_hashmap_next(&iter)) {
    btc_mpentry_t *entry = iter.val;

    if (btc_mempool_has_dependencies(mp, entry->tx))
      continue;

    if (now >= entry->time + BTC_MEMPOOL_EXPIRY_TIME) {
      btc_log_debug(mp, "Removing package %H from mempool (too old).",
                        entry->hash);

      btc_mempool_evict_entry(mp, entry);

      continue;
    }

    btc_heap_insert(&queue, entry, cmp_rate);
  }

  while (queue.length > 0 && mp->size > BTC_MEMPOOL_THRESHOLD) {
    btc_mpentry_t *entry = btc_heap_shift(&queue, cmp_rate);

    btc_log_debug(mp, "Removing package %H from mempool (low fee).",
                      entry->hash);

    btc_mempool_evict_entry(mp, entry);
  }

  btc_vector_clear(&queue);

  return !btc_hashmap_has(mp->map, added);
}

/*
 * TX Handling
 */

static btc_errno_t
btc_mpentry_verify(const btc_mpentry_t *entry,
                   const btc_view_t *view,
                   unsigned int flags) {
  const btc_tx_t *tx = entry->tx;

  if (btc_tx_verify(tx, view, flags))
    return BTC_OK;

  if (flags & BTC_SCRIPT_ONLY_STANDARD_VERIFY_FLAGS) {
    flags &= ~BTC_SCRIPT_ONLY_STANDARD_VERIFY_FLAGS;

    if (btc_tx_verify(tx, view, flags))
      return BTC_ERR_SCRIPT_POLICY;
  }

  return BTC_ERR_SCRIPT_CONSENSUS;
}

static btc_errno_t
btc_mempool_verify(btc_mempool_t *mp,
                   const btc_mpentry_t *entry,
                   const btc_view_t *view) {
  unsigned int lock_flags = BTC_STANDARD_LOCKTIME_FLAGS;
  const btc_deployment_state_t *state = btc_chain_state(mp->chain);
  const btc_entry_t *tip = btc_chain_tip(mp->chain);
  const btc_tx_t *tx = entry->tx;
  unsigned int flags;
  int64_t minfee;
  int rc;

  /* Verify sequence locks. */
  if (!btc_chain_verify_locks(mp->chain, tip, tx, view, lock_flags))
    return BTC_ERR_FINALITY;

  /* Check input and witness standardness. */
  if (mp->network->require_standard) {
    if (!btc_tx_has_standard_inputs(tx, view))
      return BTC_ERR_INPUTS_NONSTANDARD;

    if (state->flags & BTC_SCRIPT_VERIFY_WITNESS) {
      if (!btc_tx_has_standard_witness(tx, view))
        return BTC_ERR_WITNESS_NONSTANDARD;
    }
  }

  /* Annoying process known as sigops counting. */
  if (entry->sigops > BTC_MAX_TX_SIGOPS_COST)
    return BTC_ERR_TX_SIGOPS;

  /* Make sure this guy gave a decent fee. */
  minfee = btc_get_fee(mp->network->min_relay, entry->size);

  if (entry->fee < minfee)
    return BTC_ERR_FEE_LOW;

  /* Important safety feature. */
  if (entry->fee > minfee * 10000)
    return BTC_ERR_FEE_HIGH;

  /* Check ancestor depth. */
  if (btc_mempool_count_ancestors(mp, entry) + 1 > BTC_MEMPOOL_MAX_ANCESTORS)
    return BTC_ERR_MEMPOOL_CHAIN;

  /* Script verification. */
  flags = BTC_SCRIPT_STANDARD_VERIFY_FLAGS;

  if ((rc = btc_mpentry_verify(entry, view, flags))) {
    if (btc_tx_has_witness(tx))
      return rc;

    /* Try without segwit and cleanstack. */
    flags &= ~BTC_SCRIPT_VERIFY_WITNESS;
    flags &= ~BTC_SCRIPT_VERIFY_CLEANSTACK;

    /* If it failed, the first verification
       was the only result we needed. */
    if (!btc_tx_verify(tx, view, flags))
      return rc;

    /* If it succeeded, segwit may be causing the
       failure. Try with segwit but without cleanstack. */
    flags |= BTC_SCRIPT_VERIFY_WITNESS;

    /* Cleanstack was causing the failure. */
    if (btc_tx_verify(tx, view, flags))
      return rc;

    /* Do not insert into reject cache. */
    return -rc;
  }

  /* Paranoid checks. */
  if (mp->flags & BTC_MEMPOOL_PARANOID)
    CHECK(btc_tx_verify(tx, view, BTC_SCRIPT_MANDATORY_VERIFY_FLAGS));

  return BTC_OK;
}

static btc_errno_t
btc_mempool_insert(btc_mempool_t *mp, const btc_tx_t *tx, unsigned int id) {
  const btc_deployment_state_t *state = btc_chain_state(mp->chain);
  unsigned int lock_flags = BTC_STANDARD_LOCKTIME_FLAGS;
  const btc_entry_t *tip = btc_chain_tip(mp->chain);
  int32_t height = tip->height;
  btc_mpentry_t *entry;
  btc_view_t *view;
  int64_t fee;
  int rc;

  /* Basic sanity checks. */
  if ((rc = btc_tx_check_sanity(tx)))
    return rc;

  /* Coinbases are an insta-ban. */
  if (btc_tx_is_coinbase(tx))
    return BTC_ERR_COINBASE;

  /* Do not allow CSV until it's activated. */
  if (mp->network->require_standard) {
    if (!(state->flags & BTC_SCRIPT_VERIFY_CHECKSEQUENCEVERIFY)) {
      if (tx->version >= 2)
        return BTC_ERR_PREMATURE_CSV;
    }
  }

  /* Do not allow segwit until it's activated. */
  if (!(state->flags & BTC_SCRIPT_VERIFY_WITNESS)) {
    if (btc_tx_has_witness(tx))
      return BTC_ERR_PREMATURE_WITNESS;
  }

  /* Non-contextual standardness checks. */
  if (mp->network->require_standard) {
    if ((rc = btc_tx_check_standard(tx)))
      return rc;
  }

  /* Verify transaction finality. */
  if (!btc_chain_verify_final(mp->chain, tip, tx, lock_flags))
    return BTC_ERR_FINALITY;

  /* We can maybe ignore this. */
  if (btc_mempool_exists(mp, tx->hash))
    return BTC_ERR_TX_IN_MEMPOOL;

  /* We can test whether this is an
     non-fully-spent transaction on
     the chain. */
  if (btc_chain_has_coins(mp->chain, tx))
    return BTC_ERR_TX_KNOWN;

  /* Quick and dirty test to verify we're
     not double-spending an output in the
     mempool. */
  if (btc_mempool_is_double_spend(mp, tx)) {
    if (btc_tx_is_rbf(tx))
      return BTC_ERR_REPLACEMENT;

    return BTC_ERR_INPUTS_SPENT;
  }

  /* Get coin viewpoint as it pertains to the mempool. */
  view = btc_mempool_view(mp, tx);

  /* Maybe store as an orphan. */
  if (!btc_tx_has_coins(tx, view)) {
    /* Preliminary orphan checks. */
    if ((rc = btc_mempool_check_orphan(mp, tx, view))) {
      btc_view_destroy(view);
      return rc;
    }

    btc_mempool_add_orphan(mp, tx, view, id);
    btc_view_destroy(view);

    return BTC_OK;
  }

  /* Contextual sanity checks. */
  if ((rc = btc_tx_check_inputs(tx, view, height + 1, &fee))) {
    btc_view_destroy(view);
    return rc;
  }

  /* Create a new mempool entry at current chain height. */
  entry = btc_mpentry_create();

  btc_mpentry_set(entry, tx, view, height, fee);

  /* Contextual verification. */
  if ((rc = btc_mempool_verify(mp, entry, view))) {
    btc_view_destroy(view);
    btc_mpentry_destroy(entry);
    return rc;
  }

  /* Add and index the entry. */
  btc_mempool_add_entry(mp, entry, view);
  btc_view_destroy(view);

  /* Trim size if we're too big. */
  if (btc_mempool_limit_size(mp, tx->hash))
    return BTC_ERR_MEMPOOL_FULL;

  return BTC_OK;
}

btc_errno_t
btc_mempool_add(btc_mempool_t *mp, const btc_tx_t *tx, unsigned int id) {
  int rc = btc_mempool_insert(mp, tx, id);

  if (rc != 0) {
    int mutated = btc_error_malleable(rc);

    if (rc < 0) {
      mutated = 1;
      rc = -rc;
    }

    if (rc == BTC_ERR_SCRIPT_CONSENSUS || rc == BTC_ERR_SCRIPT_POLICY) {
      if (!btc_tx_has_witness(tx) && !mutated)
        btc_filter_add(&mp->rejects, tx->hash, 32);
    } else {
      if (!mutated)
        btc_filter_add(&mp->rejects, tx->hash, 32);
    }

    btc_log_warn(mp, "Verification error: %s (score=%d hash=%H)",
                     btc_error_reason(rc), btc_error_score(rc),
                     tx->hash);

    return rc;
  }

  return BTC_OK;
}

/*
 * Block Handling
 */

void
btc_mempool_add_block(btc_mempool_t *mp,
                      const btc_entry_t *entry,
                      const btc_block_t *block) {
  int total = 0;
  size_t i;

  if (btc_hashmap_size(mp->map) == 0)
    return;

  CHECK(block->txs.length > 0);

  for (i = block->txs.length - 1; i != 0; i--) {
    const btc_tx_t *tx = block->txs.items[i];
    btc_mpentry_t *ent;

    ent = btc_hashmap_get(mp->map, tx->hash);

    if (ent == NULL) {
      btc_mempool_remove_orphan(mp, tx->hash);
      btc_mempool_remove_double_spends(mp, tx);
      btc_mempool_handle_orphans(mp, tx->hash);
      continue;
    }

    btc_mempool_remove_entry(mp, ent);

    total += 1;
  }

  /* We need to reset the rejects filter periodically. */
  /* There may be a locktime in a TX that is now valid. */
  btc_filter_reset(&mp->rejects);

  if (total > 0) {
    btc_log_debug(mp, "Removed %d txs from mempool for block %d.",
                      total, entry->height);
  }
}

void
btc_mempool_remove_block(btc_mempool_t *mp,
                         const btc_entry_t *entry,
                         const btc_block_t *block) {
  int total = 0;
  size_t i;

  if (btc_hashmap_size(mp->map) == 0)
    return;

  for (i = 1; i < block->txs.length; i++) {
    const btc_tx_t *tx = block->txs.items[i];

    if (btc_hashmap_has(mp->map, tx->hash))
      continue;

    total += (btc_mempool_insert(mp, tx, -1) == 0);
  }

  btc_filter_reset(&mp->rejects);

  if (total > 0) {
    btc_log_debug(mp, "Added %d txs back into the mempool for block %d.",
                      total, entry->height);
  }
}

void
btc_mempool_handle_reorg(btc_mempool_t *mp) {
  unsigned int flags = BTC_STANDARD_LOCKTIME_FLAGS;
  const btc_entry_t *tip = btc_chain_tip(mp->chain);
  int64_t mtp = btc_entry_median_time(tip);
  int32_t height = tip->height + 1;
  btc_hashmapiter_t iter;

  btc_hashmap_iterate(&iter, mp->map);

  while (btc_hashmap_next(&iter)) {
    btc_mpentry_t *entry = iter.val;
    btc_tx_t *tx = entry->tx;
    btc_view_t *view;

    if (!btc_tx_is_final(tx, height, mtp)) {
      btc_mempool_evict_entry(mp, entry);
      continue;
    }

    if (!entry->coinbase && !entry->locks)
      continue;

    view = btc_mempool_view(mp, tx);

    if (!btc_chain_verify_locks(mp->chain, tip, tx, view, flags)) {
      btc_mempool_evict_entry(mp, entry);
      btc_view_destroy(view);
      continue;
    }

    if (entry->coinbase) {
      int invalid = 0;
      size_t i;

      for (i = 0; i < tx->inputs.length; i++) {
        const btc_input_t *input = tx->inputs.items[i];
        const btc_coin_t *coin = btc_view_get(view, &input->prevout);

        if (coin == NULL || !coin->coinbase)
          continue;

        if (height < coin->height + BTC_COINBASE_MATURITY) {
          invalid = 1;
          break;
        }
      }

      if (invalid) {
        btc_mempool_evict_entry(mp, entry);
        btc_view_destroy(view);
        continue;
      }
    }

    btc_view_destroy(view);
  }
}

/*
 * API
 */

size_t
btc_mempool_size(btc_mempool_t *mp) {
  return btc_hashmap_size(mp->map);
}

int
btc_mempool_has(btc_mempool_t *mp, const uint8_t *hash) {
  return btc_hashmap_has(mp->map, hash);
}

const btc_mpentry_t *
btc_mempool_get(btc_mempool_t *mp, const uint8_t *hash) {
  return btc_hashmap_get(mp->map, hash);
}

int
btc_mempool_has_orphan(btc_mempool_t *mp, const uint8_t *hash) {
  return btc_hashmap_has(mp->orphans, hash);
}

int
btc_mempool_has_reject(btc_mempool_t *mp, const uint8_t *hash) {
  return btc_filter_has(&mp->rejects, hash, 32);
}

btc_vector_t *
btc_mempool_missing(btc_mempool_t *mp, const btc_tx_t *tx) {
  btc_vector_t *missing = btc_vector_create();
  size_t i;

  for (i = 0; i < tx->inputs.length; i++) {
    const btc_input_t *input = tx->inputs.items[i];

    if (!btc_hashmap_has(mp->waiting, input->prevout.hash))
      continue;

    if (btc_hashmap_has(mp->orphans, input->prevout.hash))
      continue;

    btc_vector_push(missing, input->prevout.hash);
  }

  return missing;
}

void
btc_mempool_iterate(btc_mpiter_t *iter, btc_mempool_t *mp) {
  btc_hashmap_iterate(iter, mp->map);
}

int
btc_mempool_next(const btc_mpentry_t **entry, btc_mpiter_t *iter) {
  if (btc_hashmap_next(iter)) {
    *entry = iter->val;
    return 1;
  }
  return 0;
}
