/*!
 * block.c - block for libsatoshi
 * Copyright (c) 2021, Christopher Jeffrey (MIT License).
 * https://github.com/chjj/libsatoshi
 */

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <satoshi/block.h>
#include <satoshi/consensus.h>
#include <satoshi/crypto.h>
#include <satoshi/header.h>
#include <satoshi/script.h>
#include <satoshi/tx.h>
#include <torsion/hash.h>
#include "impl.h"
#include "internal.h"

/*
 * Block
 */

DEFINE_SERIALIZABLE_OBJECT(btc_block, SCOPE_EXTERN)

void
btc_block_init(btc_block_t *z) {
  btc_header_init(&z->header);
  btc_txvec_init(&z->txs);
}

void
btc_block_clear(btc_block_t *z) {
  btc_header_clear(&z->header);
  btc_txvec_clear(&z->txs);
}

void
btc_block_copy(btc_block_t *z, const btc_block_t *x) {
  btc_header_copy(&z->header, &x->header);
  btc_txvec_copy(&z->txs, &x->txs);
}

int
btc_block_has_witness(const btc_block_t *blk) {
  size_t i;

  for (i = 0; i < blk->txs.length; i++) {
    if (btc_tx_has_witness(blk->txs.items[i]))
      return 1;
  }

  return 0;
}

int
btc_block_merkle_root(uint8_t *root, const btc_block_t *blk) {
  size_t length = blk->txs.length;
  uint8_t *hashes = (uint8_t *)malloc(length * 32);
  size_t i;
  int ret;

  CHECK(hashes != NULL);

  for (i = 0; i < length; i++)
    btc_tx_txid(&hashes[i * 32], blk->txs.items[i]);

  ret = btc_merkle_root(root, hashes, length);

  free(hashes);

  return ret;
}

int
btc_block_witness_root(uint8_t *root, const btc_block_t *blk) {
  size_t length = blk->txs.length;
  uint8_t *hashes = (uint8_t *)malloc(length * 32);
  size_t i;
  int ret;

  CHECK(hashes != NULL);

  memset(hashes, 0, 32);

  for (i = 1; i < length; i++)
    btc_tx_wtxid(&hashes[i * 32], blk->txs.items[i]);

  ret = btc_merkle_root(root, hashes, length);

  free(hashes);

  return ret;
}

const uint8_t *
btc_block_witness_nonce(const btc_block_t *blk) {
  const btc_input_t *input;
  const btc_tx_t *tx;

  if (blk->txs.length == 0)
    return NULL;

  tx = blk->txs.items[0];

  if (tx->inputs.length != 1)
    return NULL;

  input = tx->inputs.items[0];

  if (input->witness.length != 1)
    return NULL;

  if (input->witness.items[0]->length != 32)
    return NULL;

  return input->witness.items[0]->data;
}

int
btc_block_create_commitment_hash(uint8_t *hash, const btc_block_t *blk) {
  const uint8_t *nonce = btc_block_witness_nonce(blk);
  uint8_t root[32];
  hash256_t ctx;

  if (nonce == NULL)
    return 0;

  if (!btc_block_witness_root(root, blk))
    return 0;

  hash256_init(&ctx);
  hash256_update(&ctx, root, 32);
  hash256_update(&ctx, nonce, 32);
  hash256_final(&ctx, hash);

  return 1;
}

int
btc_block_get_commitment_hash(uint8_t *hash /* maybe do zero copy */, const btc_block_t *blk) {
  const btc_output_t *output;
  const btc_tx_t *tx;
  size_t i;

  if (blk->txs.length == 0)
    return 0;

  tx = blk->txs.items[0];

  for (i = tx->outputs.length - 1; i != (size_t)-1; i--) {
    output = tx->outputs.items[i];

    if (btc_script_get_commitment(hash, &output->script))
      return 1;
  }

  return 0;
}

#define THROW(m, s) do { \
  if (err != NULL) {     \
    err->msg = (m);      \
    err->score = (s);    \
  }                      \
  return 0;              \
} while (0)

int
btc_block_check_body(btc_verify_error_t *err, const btc_block_t *blk) {
  const btc_tx_t *tx;
  uint8_t root[32];
  int sigops = 0;
  size_t i;

  /* Check base size. */
  if (blk->txs.length == 0
      || blk->txs.length > BTC_MAX_BLOCK_SIZE
      || btc_block_base_size(blk) > BTC_MAX_BLOCK_SIZE) {
    THROW("bad-blk-length", 100);
  }

  /* First TX must be a coinbase. */
  if (blk->txs.length == 0 || !btc_tx_is_coinbase(blk->txs.items[0]))
    THROW("bad-cb-missing", 100);

  /* If the merkle is mutated, we have duplicate txs. */
  if (!btc_block_merkle_root(root, blk))
    THROW("bad-txns-duplicate", 100);

  /* Check merkle root. */
  if (memcmp(blk->header.merkle_root, root, 32) != 0)
    THROW("bad-txnmrklroot", 100);

  /* Test all transactions. */
  sigops = 0;

  for (i = 0; i < blk->txs.length; i++) {
    tx = blk->txs.items[i];

    /* The rest of the txs must not be coinbases. */
    if (i > 0 && btc_tx_is_coinbase(tx))
      THROW("bad-cb-multiple", 100);

    /* Sanity checks. */
    if (!btc_tx_check_sanity(err, tx))
      return 0;

    /* Count legacy sigops (do not count scripthash or witness). */
    sigops += btc_tx_legacy_sigops(tx);

    if (sigops * BTC_WITNESS_SCALE_FACTOR > BTC_MAX_BLOCK_SIGOPS_COST)
      THROW("bad-blk-sigops", 100);
  }

  return 1;
}

#undef THROW

int
btc_block_coinbase_height(uint32_t *height, const btc_block_t *blk) {
  const btc_tx_t *tx;

  *height = (uint32_t)-1;

  if (blk->header.version < 2)
    return 0;

  if (blk->txs.length == 0)
    return 0;

  tx = blk->txs.items[0];

  if (tx->inputs.length == 0)
    return 0;

  return btc_script_get_height(height, &tx->inputs.items[0]->script);
}

int64_t
btc_block_claimed(const btc_block_t *blk) {
  CHECK(blk->txs.length > 0);
  CHECK(btc_tx_is_coinbase(blk->txs.items[0]));

  return btc_tx_output_value(blk->txs.items[0]);
}

size_t
btc_block_base_size(const btc_block_t *blk) {
  size_t size = 0;
  size_t i;

  size += btc_header_size(&blk->header);
  size += btc_size_size(blk->txs.length);

  for (i = 0; i < blk->txs.length; i++)
    size += btc_tx_base_size(blk->txs.items[i]);

  return size;
}

size_t
btc_block_witness_size(const btc_block_t *blk) {
  size_t size = 0;
  size_t i;

  for (i = 0; i < blk->txs.length; i++)
    size += btc_tx_witness_size(blk->txs.items[i]);

  return size;
}

size_t
btc_block_size(const btc_block_t *blk) {
  return btc_block_base_size(blk) + btc_block_witness_size(blk);
}

size_t
btc_block_weight(const btc_block_t *blk) {
  size_t base = btc_block_base_size(blk);
  size_t wit = btc_block_witness_size(blk);
  return (base * BTC_WITNESS_SCALE_FACTOR) + wit;
}

size_t
btc_block_virtual_size(const btc_block_t *blk) {
  size_t weight = btc_block_weight(blk);
  return (weight + BTC_WITNESS_SCALE_FACTOR - 1) / BTC_WITNESS_SCALE_FACTOR;
}

uint8_t *
btc_block_write(uint8_t *zp, const btc_block_t *x) {
  zp = btc_header_write(zp, &x->header);
  zp = btc_txvec_write(zp, &x->txs);
  return zp;
}

int
btc_block_read(btc_block_t *z, const uint8_t **xp, size_t *xn) {
  if (!btc_header_read(&z->header, xp, xn))
    return 0;

  if (!btc_txvec_read(&z->txs, xp, xn))
    return 0;

  return 1;
}