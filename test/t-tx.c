/*!
 * t-tx.c - tx test for mako
 * Copyright (c) 2021, Christopher Jeffrey (MIT License).
 * https://github.com/chjj/mako
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mako/coins.h>
#include <mako/network.h>
#include <mako/script.h>
#include <mako/tx.h>
#include <mako/util.h>
#include "data/tx_valid_vectors.h"
#include "data/tx_invalid_vectors.h"
#include "lib/tests.h"

static void
test_tx_valid_vector(const test_valid_vector_t *vec, size_t index) {
  uint8_t hash[32];
  uint8_t whash[32];
  btc_coin_t *coin;
  btc_view_t *view;
  btc_tx_t tx;
  size_t i;

  printf("tx valid vector #%d: %s\n", (int)index, vec->comments);

  btc_tx_init(&tx);

  view = btc_view_create();

  ASSERT(btc_tx_import(&tx, vec->tx_raw, vec->tx_len));

  btc_tx_txid(hash, &tx);
  btc_tx_wtxid(whash, &tx);

  ASSERT(btc_hash_equal(tx.hash, hash));
  ASSERT(btc_hash_equal(tx.whash, whash));

  btc_tx_refresh(&tx);

  ASSERT(btc_hash_equal(tx.hash, hash));
  ASSERT(btc_hash_equal(tx.whash, whash));

  for (i = 0; i < vec->coins_len; i++) {
    coin = btc_coin_create();

    ASSERT(btc_output_import(&coin->output, vec->coins[i].output_raw,
                                            vec->coins[i].output_len));

    btc_view_put(view, &vec->coins[i].outpoint, coin);
  }

  if (strstr(vec->comments, "Coinbase") == vec->comments)
    ASSERT(btc_tx_check_sanity(&tx) == 0);
  else
    ASSERT(btc_tx_verify(&tx, view, vec->flags));

  btc_tx_clear(&tx);
  btc_view_destroy(view);
}

static void
test_tx_invalid_vector(const test_invalid_vector_t *vec, size_t index) {
  uint8_t hash[32];
  uint8_t whash[32];
  btc_coin_t *coin;
  btc_view_t *view;
  btc_tx_t tx;
  size_t i;

  printf("tx invalid vector #%d: %s\n", (int)index, vec->comments);

  btc_tx_init(&tx);

  view = btc_view_create();

  ASSERT(btc_tx_import(&tx, vec->tx_raw, vec->tx_len));

  btc_tx_txid(hash, &tx);
  btc_tx_wtxid(whash, &tx);

  ASSERT(btc_hash_equal(tx.hash, hash));
  ASSERT(btc_hash_equal(tx.whash, whash));

  btc_tx_refresh(&tx);

  ASSERT(btc_hash_equal(tx.hash, hash));
  ASSERT(btc_hash_equal(tx.whash, whash));

  for (i = 0; i < vec->coins_len; i++) {
    coin = btc_coin_create();

    ASSERT(btc_output_import(&coin->output, vec->coins[i].output_raw,
                                            vec->coins[i].output_len));

    btc_view_put(view, &vec->coins[i].outpoint, coin);
  }

  if (strcmp(vec->comments, "Duplicate inputs") == 0) {
    ASSERT(btc_tx_verify(&tx, view, vec->flags));
    ASSERT(btc_tx_check_sanity(&tx) != 0);
  } else if (strcmp(vec->comments, "Negative output") == 0) {
    ASSERT(btc_tx_verify(&tx, view, vec->flags));
    ASSERT(btc_tx_check_sanity(&tx) != 0);
  } else if (strstr(vec->comments, "Coinbase") == vec->comments) {
    ASSERT(btc_tx_check_sanity(&tx) != 0);
  } else {
    ASSERT(!btc_tx_verify(&tx, view, vec->flags));
  }

  btc_tx_clear(&tx);
  btc_view_destroy(view);
}

int
main(void) {
  size_t i;

  for (i = 0; i < lengthof(test_valid_vectors); i++)
    test_tx_valid_vector(&test_valid_vectors[i], i);

  for (i = 0; i < lengthof(test_invalid_vectors); i++)
    test_tx_invalid_vector(&test_invalid_vectors[i], i);

  return 0;
}
