/*!
 * netmap.c - network address map for mako
 * Copyright (c) 2021, Christopher Jeffrey (MIT License).
 * https://github.com/chjj/mako
 */

#include "map.h"
#include "netaddr.h"
#include "map.h"

/*
 * Network Address Map
 */

DEFINE_NETADDR_MAP(btc_netmap, void *, NULL, MAP_EXTERN)
