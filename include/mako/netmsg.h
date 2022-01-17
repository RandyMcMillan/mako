/*!
 * netmsg.h - network messages for mako
 * Copyright (c) 2020, Christopher Jeffrey (MIT License).
 * https://github.com/chjj/mako
 */

#ifndef BTC_NETMSG_H
#define BTC_NETMSG_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>
#include "common.h"
#include "impl.h"
#include "types.h"

/*
 * Constants
 */

enum btc_msgtype {
  BTC_MSG_ADDR,
  BTC_MSG_BLOCK,
  BTC_MSG_BLOCKTXN,
  BTC_MSG_CMPCTBLOCK,
  BTC_MSG_FEEFILTER,
  BTC_MSG_FILTERADD,
  BTC_MSG_FILTERCLEAR,
  BTC_MSG_FILTERLOAD,
  BTC_MSG_GETADDR,
  BTC_MSG_GETBLOCKS,
  BTC_MSG_GETBLOCKTXN,
  BTC_MSG_GETDATA,
  BTC_MSG_GETHEADERS,
  BTC_MSG_HEADERS,
  BTC_MSG_INV,
  BTC_MSG_MEMPOOL,
  BTC_MSG_MERKLEBLOCK,
  BTC_MSG_NOTFOUND,
  BTC_MSG_PING,
  BTC_MSG_PONG,
  BTC_MSG_SENDCMPCT,
  BTC_MSG_SENDHEADERS,
  BTC_MSG_TX,
  BTC_MSG_VERACK,
  BTC_MSG_VERSION,
  /* Internal */
  BTC_MSG_BLOCKTXN_BASE,
  BTC_MSG_BLOCK_BASE,
  BTC_MSG_CMPCTBLOCK_BASE,
  BTC_MSG_GETDATA_FULL,
  BTC_MSG_INV_FULL,
  BTC_MSG_NOTFOUND_FULL,
  BTC_MSG_TX_BASE,
  BTC_MSG_UNKNOWN
};

enum btc_invtype {
  BTC_INV_UNDEFINED = 0,
  BTC_INV_TX = 1,
  BTC_INV_BLOCK = 2,
  BTC_INV_FILTERED_BLOCK = 3,
  BTC_INV_CMPCT_BLOCK = 4,
  BTC_INV_WTX = 5,
  BTC_INV_WITNESS_TX = 1 | (1 << 30),
  BTC_INV_WITNESS_BLOCK = 2 | (1 << 30),
  BTC_INV_WITNESS_FILTERED_BLOCK = 3 | (1 << 30),
  BTC_INV_WITNESS_FLAG = 1 << 30
};

/*
 * Types
 */

typedef struct btc_version_s {
  int32_t version;
  uint64_t services;
  int64_t time;
  btc_netaddr_t remote;
  btc_netaddr_t local;
  uint64_t nonce;
  char agent[256 + 1];
  int32_t height;
  uint8_t relay;
} btc_version_t;

typedef struct btc_ping_s {
  uint64_t nonce;
} btc_ping_t;

typedef btc_ping_t btc_pong_t;

typedef struct btc_addrs_s {
  btc_netaddr_t **items;
  size_t alloc;
  size_t length;
} btc_addrs_t;

typedef struct btc_invitem_s {
  uint32_t type;
  uint8_t hash[32];
  struct btc_invitem_s *next;
} btc_invitem_t;

typedef struct btc_inv_s {
  btc_invitem_t **items;
  size_t alloc;
  size_t length;
} btc_inv_t;

typedef struct btc_zinvitem_s {
  uint32_t type;
  const uint8_t *hash;
} btc_zinvitem_t;

typedef struct btc_zinv_s {
  btc_zinvitem_t *items;
  size_t alloc;
  size_t length;
} btc_zinv_t;

typedef struct btc_getblocks_s {
  uint32_t version;
  btc_vector_t locator;
  const uint8_t *stop;
} btc_getblocks_t;

typedef struct btc_headers_s {
  btc_header_t **items;
  size_t alloc;
  size_t length;
} btc_headers_t;

typedef struct btc_filteradd_s {
  const uint8_t *data;
  size_t length;
} btc_filteradd_t;

typedef struct btc_feefilter_s {
  int64_t rate;
} btc_feefilter_t;

typedef struct btc_sendcmpct_s {
  uint8_t mode;
  uint64_t version;
} btc_sendcmpct_t;

typedef struct btc_unknown_s {
  const uint8_t *data;
  size_t length;
} btc_unknown_t;

typedef struct btc_msg_s {
  enum btc_msgtype type;
  char cmd[12];
  void *body;
} btc_msg_t;

/*
 * Version
 */

BTC_DEFINE_SERIALIZABLE_OBJECT(btc_version, BTC_EXTERN)

BTC_EXTERN void
btc_version_init(btc_version_t *msg);

BTC_EXTERN void
btc_version_clear(btc_version_t *msg);

BTC_EXTERN void
btc_version_copy(btc_version_t *z, const btc_version_t *x);

BTC_EXTERN size_t
btc_version_size(const btc_version_t *x);

BTC_EXTERN uint8_t *
btc_version_write(uint8_t *zp, const btc_version_t *x);

BTC_EXTERN int
btc_version_read(btc_version_t *z, const uint8_t **xp, size_t *xn);

/*
 * Verack
 */

/* empty message */

/*
 * Ping
 */

BTC_DEFINE_SERIALIZABLE_OBJECT(btc_ping, BTC_EXTERN)

BTC_EXTERN void
btc_ping_init(btc_ping_t *msg);

BTC_EXTERN void
btc_ping_clear(btc_ping_t *msg);

BTC_EXTERN void
btc_ping_copy(btc_ping_t *z, const btc_ping_t *x);

BTC_EXTERN size_t
btc_ping_size(const btc_ping_t *x);

BTC_EXTERN uint8_t *
btc_ping_write(uint8_t *zp, const btc_ping_t *x);

BTC_EXTERN int
btc_ping_read(btc_ping_t *z, const uint8_t **xp, size_t *xn);

/*
 * Pong
 */

BTC_DEFINE_SERIALIZABLE_OBJECT(btc_pong, BTC_EXTERN)

BTC_EXTERN void
btc_pong_init(btc_pong_t *msg);

BTC_EXTERN void
btc_pong_clear(btc_pong_t *msg);

BTC_EXTERN void
btc_pong_copy(btc_pong_t *z, const btc_pong_t *x);

BTC_EXTERN size_t
btc_pong_size(const btc_pong_t *x);

BTC_EXTERN uint8_t *
btc_pong_write(uint8_t *zp, const btc_pong_t *x);

BTC_EXTERN int
btc_pong_read(btc_pong_t *z, const uint8_t **xp, size_t *xn);

/*
 * GetAddr
 */

/* empty message */

/*
 * Addr
 */

BTC_DEFINE_SERIALIZABLE_VECTOR(btc_addrs, btc_netaddr, BTC_EXTERN)

/*
 * Inv Item
 */

BTC_DEFINE_SERIALIZABLE_OBJECT(btc_invitem, BTC_EXTERN)

BTC_EXTERN void
btc_invitem_init(btc_invitem_t *item);

BTC_EXTERN void
btc_invitem_clear(btc_invitem_t *item);

BTC_EXTERN void
btc_invitem_copy(btc_invitem_t *z, const btc_invitem_t *x);

BTC_EXTERN void
btc_invitem_set(btc_invitem_t *z, uint32_t type, const uint8_t *hash);

BTC_EXTERN uint32_t
btc_invitem_hash(const btc_invitem_t *x);

BTC_EXTERN int
btc_invitem_equal(const btc_invitem_t *x, const btc_invitem_t *y);

BTC_EXTERN size_t
btc_invitem_size(const btc_invitem_t *x);

BTC_EXTERN uint8_t *
btc_invitem_write(uint8_t *zp, const btc_invitem_t *x);

BTC_EXTERN int
btc_invitem_read(btc_invitem_t *z, const uint8_t **xp, size_t *xn);

/*
 * Inv
 */

BTC_DEFINE_SERIALIZABLE_VECTOR(btc_inv, btc_invitem, BTC_EXTERN)

BTC_EXTERN void
btc_inv_push_item(btc_inv_t *inv, uint32_t type, const uint8_t *hash);

/*
 * Inv (zero copy)
 */

BTC_DEFINE_OBJECT(btc_zinv, BTC_EXTERN)

BTC_EXTERN void
btc_zinv_init(btc_zinv_t *z);

BTC_EXTERN void
btc_zinv_clear(btc_zinv_t *z);

BTC_EXTERN void
btc_zinv_copy(btc_zinv_t *z, const btc_zinv_t *x);

BTC_EXTERN void
btc_zinv_reset(btc_zinv_t *z);

BTC_EXTERN void
btc_zinv_grow(btc_zinv_t *z, size_t zn);

BTC_EXTERN void
btc_zinv_push(btc_zinv_t *z, uint32_t type, const uint8_t *hash);

BTC_EXTERN btc_invitem_t *
btc_zinv_get(const btc_zinv_t *z, size_t index);

/*
 * GetData
 */

/* inherits btc_inv_t */

/*
 * NotFound
 */

/* inherits btc_inv_t */

/*
 * GetBlocks
 */

BTC_DEFINE_SERIALIZABLE_OBJECT(btc_getblocks, BTC_EXTERN)

BTC_EXTERN void
btc_getblocks_init(btc_getblocks_t *msg);

BTC_EXTERN void
btc_getblocks_clear(btc_getblocks_t *msg);

BTC_EXTERN void
btc_getblocks_copy(btc_getblocks_t *z, const btc_getblocks_t *x);

BTC_EXTERN size_t
btc_getblocks_size(const btc_getblocks_t *x);

BTC_EXTERN uint8_t *
btc_getblocks_write(uint8_t *zp, const btc_getblocks_t *x);

BTC_EXTERN int
btc_getblocks_read(btc_getblocks_t *z, const uint8_t **xp, size_t *xn);

/*
 * GetHeaders
 */

/* inherits btc_getblocks_t */

/*
 * Headers
 */

BTC_DEFINE_SERIALIZABLE_VECTOR(btc_headers, btc_header, BTC_EXTERN)

/*
 * SendHeaders
 */

/* empty message */

/*
 * Block
 */

/* already implemented */

/*
 * TX
 */

/* already implemented */

/*
 * Mempool
 */

/* empty message */

/*
 * FilterLoad
 */

/* TODO */

/*
 * FilterAdd
 */

BTC_DEFINE_SERIALIZABLE_OBJECT(btc_filteradd, BTC_EXTERN)

BTC_EXTERN void
btc_filteradd_init(btc_filteradd_t *msg);

BTC_EXTERN void
btc_filteradd_clear(btc_filteradd_t *msg);

BTC_EXTERN void
btc_filteradd_copy(btc_filteradd_t *z, const btc_filteradd_t *x);

BTC_EXTERN size_t
btc_filteradd_size(const btc_filteradd_t *x);

BTC_EXTERN uint8_t *
btc_filteradd_write(uint8_t *zp, const btc_filteradd_t *x);

BTC_EXTERN int
btc_filteradd_read(btc_filteradd_t *z, const uint8_t **xp, size_t *xn);

/*
 * FilterClear
 */

/* empty message */

/*
 * MerkleBlock
 */

/* TODO */

/*
 * FeeFilter
 */

BTC_DEFINE_SERIALIZABLE_OBJECT(btc_feefilter, BTC_EXTERN)

BTC_EXTERN void
btc_feefilter_init(btc_feefilter_t *msg);

BTC_EXTERN void
btc_feefilter_clear(btc_feefilter_t *msg);

BTC_EXTERN void
btc_feefilter_copy(btc_feefilter_t *z, const btc_feefilter_t *x);

BTC_EXTERN size_t
btc_feefilter_size(const btc_feefilter_t *x);

BTC_EXTERN uint8_t *
btc_feefilter_write(uint8_t *zp, const btc_feefilter_t *x);

BTC_EXTERN int
btc_feefilter_read(btc_feefilter_t *z, const uint8_t **xp, size_t *xn);

/*
 * SendCmpct
 */

BTC_DEFINE_SERIALIZABLE_OBJECT(btc_sendcmpct, BTC_EXTERN)

BTC_EXTERN void
btc_sendcmpct_init(btc_sendcmpct_t *msg);

BTC_EXTERN void
btc_sendcmpct_clear(btc_sendcmpct_t *msg);

BTC_EXTERN void
btc_sendcmpct_copy(btc_sendcmpct_t *z, const btc_sendcmpct_t *x);

BTC_EXTERN size_t
btc_sendcmpct_size(const btc_sendcmpct_t *x);

BTC_EXTERN uint8_t *
btc_sendcmpct_write(uint8_t *zp, const btc_sendcmpct_t *x);

BTC_EXTERN int
btc_sendcmpct_read(btc_sendcmpct_t *z, const uint8_t **xp, size_t *xn);

/*
 * CmpctBlock
 */

/* TODO */

/*
 * GetBlockTxn
 */

/* TODO */

/*
 * BlockTxn
 */

/* TODO */

/*
 * Unknown
 */

BTC_DEFINE_SERIALIZABLE_OBJECT(btc_unknown, BTC_EXTERN)

BTC_EXTERN void
btc_unknown_init(btc_unknown_t *msg);

BTC_EXTERN void
btc_unknown_clear(btc_unknown_t *msg);

BTC_EXTERN void
btc_unknown_copy(btc_unknown_t *z, const btc_unknown_t *x);

BTC_EXTERN size_t
btc_unknown_size(const btc_unknown_t *x);

BTC_EXTERN uint8_t *
btc_unknown_write(uint8_t *zp, const btc_unknown_t *x);

BTC_EXTERN int
btc_unknown_read(btc_unknown_t *z, const uint8_t **xp, size_t *xn);

/*
 * Message
 */

BTC_DEFINE_SERIALIZABLE_OBJECT(btc_msg, BTC_EXTERN)

BTC_EXTERN void
btc_msg_init(btc_msg_t *msg);

BTC_EXTERN void
btc_msg_clear(btc_msg_t *msg);

BTC_EXTERN void
btc_msg_copy(btc_msg_t *z, const btc_msg_t *x);

BTC_EXTERN void
btc_msg_set_type(btc_msg_t *msg, enum btc_msgtype type);

BTC_EXTERN void
btc_msg_set_cmd(btc_msg_t *msg, const char *cmd);

BTC_EXTERN void
btc_msg_alloc(btc_msg_t *msg);

BTC_EXTERN size_t
btc_msg_size(const btc_msg_t *x);

BTC_EXTERN uint8_t *
btc_msg_write(uint8_t *zp, const btc_msg_t *x);

BTC_EXTERN int
btc_msg_read(btc_msg_t *z, const uint8_t **xp, size_t *xn);

#ifdef __cplusplus
}
#endif

#endif /* BTC_NETMSG_H */
