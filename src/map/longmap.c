/*!
 * longmap.c - long integer map for mako
 * Copyright (c) 2021, Christopher Jeffrey (MIT License).
 * https://github.com/chjj/mako
 */

#include "map.h"
#include "../../include/wallet/types.h"

/*
 * Long Integer Map
 */

DEFINE_UINT64_MAP(btc_longmap, void *, NULL, MAP_EXTERN)
