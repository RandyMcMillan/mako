/*!
 * mainnet.c - mainnet for libsatoshi
 * Copyright (c) 2021, Christopher Jeffrey (MIT License).
 * https://github.com/chjj/libsatoshi
 */

#include <stddef.h>
#include <stdint.h>
#include <satoshi/network.h>
#include "internal.h"

static const char *mainnet_seeds[] = {
  "seed.bitcoin.sipa.be", /* Pieter Wuille */
  "dnsseed.bluematt.me", /* Matt Corallo */
  "dnsseed.bitcoin.dashjr.org", /* Luke Dashjr */
  "seed.bitcoinstats.com", /* Christian Decker */
  "seed.bitcoin.jonasschnelli.ch", /* Jonas Schnelli */
  "seed.btc.petertodd.org" /* Peter Todd */
};

static const btc_checkpoint_t mainnet_checkpoints[] = {
  {
    11111,
    {
      0x1d, 0x7c, 0x6e, 0xb2, 0xfd, 0x42, 0xf5, 0x59,
      0x25, 0xe9, 0x2e, 0xfa, 0xd6, 0x8b, 0x61, 0xed,
      0xd2, 0x2f, 0xba, 0x29, 0xfd, 0xe8, 0x78, 0x3d,
      0xf7, 0x44, 0xe2, 0x69, 0x00, 0x00, 0x00, 0x00
    }
  },
  {
    33333,
    {
      0xa6, 0xd0, 0xb5, 0xdf, 0x7d, 0x0d, 0xf0, 0x69,
      0xce, 0xb1, 0xe7, 0x36, 0xa2, 0x16, 0xad, 0x18,
      0x7a, 0x50, 0xb0, 0x7a, 0xaa, 0x4e, 0x78, 0x74,
      0x8a, 0x58, 0xd5, 0x2d, 0x00, 0x00, 0x00, 0x00
    }
  },
  {
    74000,
    {
      0x20, 0x1a, 0x66, 0xb8, 0x53, 0xf9, 0xe7, 0x81,
      0x4a, 0x82, 0x0e, 0x2a, 0xf5, 0xf5, 0xdc, 0x79,
      0xc0, 0x71, 0x44, 0xe3, 0x1c, 0xe4, 0xc9, 0xa3,
      0x93, 0x39, 0x57, 0x00, 0x00, 0x00, 0x00, 0x00
    }
  },
  {
    105000,
    {
      0x97, 0xdc, 0x6b, 0x1d, 0x15, 0xfb, 0xee, 0xf3,
      0x73, 0xa7, 0x44, 0xfe, 0xe0, 0xb2, 0x54, 0xb0,
      0xd2, 0xc8, 0x20, 0xa3, 0xae, 0x7f, 0x02, 0x28,
      0xce, 0x91, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00
    }
  },
  {
    134444,
    {
      0xfe, 0xb0, 0xd2, 0x42, 0x0d, 0x4a, 0x18, 0x91,
      0x4c, 0x81, 0xac, 0x30, 0xf4, 0x94, 0xa5, 0xd4,
      0xff, 0x34, 0xcd, 0x15, 0xd3, 0x4c, 0xfd, 0x2f,
      0xb1, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    }
  },
  {
    168000,
    {
      0x63, 0xb7, 0x03, 0x83, 0x5c, 0xb7, 0x35, 0xcb,
      0x9a, 0x89, 0xd7, 0x33, 0xcb, 0xe6, 0x6f, 0x21,
      0x2f, 0x63, 0x79, 0x5e, 0x01, 0x72, 0xea, 0x61,
      0x9e, 0x09, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    }
  },
  {
    193000,
    {
      0x17, 0x13, 0x8b, 0xca, 0x83, 0xbd, 0xc3, 0xe6,
      0xf6, 0x0f, 0x01, 0x17, 0x7c, 0x38, 0x77, 0xa9,
      0x82, 0x66, 0xde, 0x40, 0x73, 0x5f, 0x2a, 0x45,
      0x9f, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    }
  },
  {
    210000,
    {
      0x2e, 0x34, 0x71, 0xa1, 0x9b, 0x8e, 0x22, 0xb7,
      0xf9, 0x39, 0xc6, 0x36, 0x63, 0x07, 0x66, 0x03,
      0xcf, 0x69, 0x2f, 0x19, 0x83, 0x7e, 0x34, 0x95,
      0x8b, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    }
  },
  {
    216116,
    {
      0x4e, 0xdf, 0x23, 0x1b, 0xf1, 0x70, 0x23, 0x4e,
      0x6a, 0x81, 0x14, 0x60, 0xf9, 0x5c, 0x94, 0xaf,
      0x94, 0x64, 0xe4, 0x1e, 0xe8, 0x33, 0xb4, 0xf4,
      0xb4, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    }
  },
  {
    225430,
    {
      0x32, 0x59, 0x57, 0x30, 0xb1, 0x65, 0xf0, 0x97,
      0xe7, 0xb8, 0x06, 0xa6, 0x79, 0xcf, 0x7f, 0x3e,
      0x43, 0x90, 0x40, 0xf7, 0x50, 0x43, 0x38, 0x08,
      0xc1, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    }
  },
  {
    250000,
    {
      0x14, 0xd2, 0xf2, 0x4d, 0x29, 0xbe, 0xd7, 0x53,
      0x54, 0xf3, 0xf8, 0x8a, 0x5f, 0xb5, 0x00, 0x22,
      0xfc, 0x06, 0x4b, 0x02, 0x29, 0x1f, 0xdf, 0x87,
      0x38, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    }
  },
  {
    279000,
    {
      0x40, 0x7e, 0xbd, 0xe9, 0x58, 0xe4, 0x41, 0x90,
      0xfa, 0x9e, 0x81, 0x0e, 0xa1, 0xfc, 0x3a, 0x7e,
      0xf6, 0x01, 0xc3, 0xb0, 0xa0, 0x72, 0x8c, 0xae,
      0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    }
  },
  {
    295000,
    {
      0x83, 0xa9, 0x32, 0x46, 0xc6, 0x70, 0x03, 0x10,
      0x5a, 0xf3, 0x3a, 0xe0, 0xb2, 0x9d, 0xd6, 0x6f,
      0x68, 0x9d, 0x0f, 0x0f, 0xf5, 0x4e, 0x9b, 0x4d,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    }
  },
  {
    300255,
    {
      0xb2, 0xf3, 0xa0, 0xf0, 0xde, 0x41, 0x20, 0xc1,
      0x08, 0x9d, 0x5f, 0x52, 0x80, 0xa2, 0x63, 0x05,
      0x9f, 0x9b, 0x6e, 0x7c, 0x52, 0x04, 0x28, 0x16,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    }
  },
  {
    319400,
    {
      0x3b, 0xf1, 0x15, 0xfd, 0x05, 0x73, 0x91, 0x58,
      0x7c, 0xa3, 0x9a, 0x53, 0x1c, 0x5d, 0x49, 0x89,
      0xe1, 0xad, 0xec, 0x9b, 0x2e, 0x05, 0xc6, 0x21,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    }
  },
  {
    343185,
    {
      0x54, 0x85, 0x36, 0xd4, 0x8e, 0x76, 0x78, 0xfc,
      0xfa, 0x03, 0x42, 0x02, 0xdd, 0x45, 0xd4, 0xa7,
      0x6b, 0x1a, 0xd0, 0x61, 0xf3, 0x8b, 0x2b, 0x07,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    }
  },
  {
    352940,
    {
      0xff, 0xc9, 0x52, 0x01, 0x43, 0xe4, 0x1c, 0x94,
      0xb6, 0xe0, 0x3c, 0x2f, 0xa3, 0xe6, 0x2b, 0xb7,
      0x6b, 0x55, 0xba, 0x2d, 0xf4, 0x5d, 0x75, 0x10,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    }
  },
  {
    382320,
    {
      0xb2, 0x8a, 0xfd, 0xde, 0x92, 0xb0, 0x89, 0x97,
      0x15, 0xe4, 0x03, 0x62, 0xf5, 0x6a, 0xfd, 0xb2,
      0x0e, 0x3d, 0x13, 0x5b, 0xed, 0xc6, 0x8d, 0x0a,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    }
  },
  {
    401465,
    {
      0xee, 0xd1, 0x6c, 0xb3, 0xe8, 0x93, 0xed, 0x93,
      0x66, 0xf2, 0x7c, 0x39, 0xa9, 0xec, 0xd9, 0x54,
      0x65, 0xd0, 0x2e, 0x3e, 0xf4, 0x0e, 0x45, 0x01,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    }
  },
  {
    420000,
    {
      0xa1, 0xff, 0x74, 0x6b, 0x2d, 0x42, 0xb8, 0x34,
      0xcb, 0x7d, 0x6b, 0x89, 0x81, 0xb0, 0x9c, 0x26,
      0x5c, 0x2c, 0xab, 0xc0, 0x16, 0xe8, 0xcc, 0x02,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    }
  },
  {
    440000,
    {
      0x9b, 0xf2, 0x96, 0xb8, 0xde, 0x5f, 0x83, 0x4f,
      0x76, 0x35, 0xd5, 0xe2, 0x58, 0xa4, 0x34, 0xad,
      0x51, 0xb4, 0xdb, 0xbc, 0xf7, 0xc0, 0x8c, 0x03,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    }
  },
  {
    450000,
    {
      0x0b, 0xa2, 0x07, 0x0c, 0x62, 0xcd, 0x9d, 0xa1,
      0xf8, 0xce, 0xf8, 0x8a, 0x06, 0x48, 0xc6, 0x61,
      0xa4, 0x11, 0xd3, 0x3e, 0x72, 0x83, 0x40, 0x01,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    }
  },
  {
    460000,
    {
      0x8c, 0x25, 0xfc, 0x7e, 0x41, 0x4d, 0x3e, 0x86,
      0x8d, 0x6c, 0xe0, 0xec, 0x47, 0x3c, 0x30, 0xad,
      0x44, 0xe7, 0xe8, 0xbc, 0x1b, 0x75, 0xef, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    }
  },
  {
    470000,
    {
      0x89, 0x75, 0x6d, 0x1e, 0xd7, 0x59, 0x01, 0x43,
      0x73, 0x00, 0xaf, 0x10, 0xd5, 0xab, 0x69, 0x07,
      0x0a, 0x28, 0x2e, 0x72, 0x9c, 0x53, 0x6c, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    }
  },
  {
    480000,
    {
      0xb1, 0xa8, 0x96, 0xfd, 0x31, 0xe6, 0x39, 0xe0,
      0xc7, 0x4d, 0x1a, 0xbe, 0xb1, 0xdb, 0xc9, 0x3f,
      0x17, 0x6b, 0x76, 0x7a, 0x5d, 0x4c, 0x02, 0x01,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    }
  },
  {
    490000,
    {
      0x90, 0xde, 0xc4, 0xd0, 0x15, 0x3f, 0x20, 0xfb,
      0xdc, 0xb2, 0x45, 0xb1, 0xd5, 0xfb, 0x3d, 0x5a,
      0x8d, 0x7b, 0xb1, 0x37, 0x91, 0x06, 0xde, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    }
  },
  {
    500000,
    {
      0x04, 0x5d, 0x94, 0xa1, 0xc3, 0x33, 0x54, 0xc3,
      0x75, 0x9c, 0xc0, 0x51, 0x2d, 0xcc, 0x49, 0xfd,
      0x81, 0xbf, 0x4c, 0x36, 0x37, 0xfb, 0x24, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    }
  },
  {
    510000,
    {
      0x29, 0x73, 0x01, 0xb8, 0xca, 0x28, 0x58, 0x4c,
      0xb0, 0xc3, 0x1c, 0x7e, 0x3f, 0xed, 0x51, 0x69,
      0x6b, 0xc3, 0x3e, 0xf8, 0x78, 0x26, 0x15, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    }
  },
  {
    525000,
    {
      0x1d, 0xde, 0x8e, 0x3f, 0xb4, 0x9b, 0xbd, 0x5a,
      0xb6, 0x6a, 0x1b, 0x84, 0x75, 0x44, 0xd6, 0x7f,
      0xff, 0x10, 0xb1, 0x08, 0xa1, 0xfa, 0x2f, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    }
  }
};

static const uint8_t mainnet_genesis[] = {
  0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x3b, 0xa3, 0xed, 0xfd,
  0x7a, 0x7b, 0x12, 0xb2, 0x7a, 0xc7, 0x2c, 0x3e,
  0x67, 0x76, 0x8f, 0x61, 0x7f, 0xc8, 0x1b, 0xc3,
  0x88, 0x8a, 0x51, 0x32, 0x3a, 0x9f, 0xb8, 0xaa,
  0x4b, 0x1e, 0x5e, 0x4a, 0x29, 0xab, 0x5f, 0x49,
  0xff, 0xff, 0x00, 0x1d, 0x1d, 0xac, 0x2b, 0x7c,
  0x01, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff,
  0xff, 0xff, 0x4d, 0x04, 0xff, 0xff, 0x00, 0x1d,
  0x01, 0x04, 0x45, 0x54, 0x68, 0x65, 0x20, 0x54,
  0x69, 0x6d, 0x65, 0x73, 0x20, 0x30, 0x33, 0x2f,
  0x4a, 0x61, 0x6e, 0x2f, 0x32, 0x30, 0x30, 0x39,
  0x20, 0x43, 0x68, 0x61, 0x6e, 0x63, 0x65, 0x6c,
  0x6c, 0x6f, 0x72, 0x20, 0x6f, 0x6e, 0x20, 0x62,
  0x72, 0x69, 0x6e, 0x6b, 0x20, 0x6f, 0x66, 0x20,
  0x73, 0x65, 0x63, 0x6f, 0x6e, 0x64, 0x20, 0x62,
  0x61, 0x69, 0x6c, 0x6f, 0x75, 0x74, 0x20, 0x66,
  0x6f, 0x72, 0x20, 0x62, 0x61, 0x6e, 0x6b, 0x73,
  0xff, 0xff, 0xff, 0xff, 0x01, 0x00, 0xf2, 0x05,
  0x2a, 0x01, 0x00, 0x00, 0x00, 0x43, 0x41, 0x04,
  0x67, 0x8a, 0xfd, 0xb0, 0xfe, 0x55, 0x48, 0x27,
  0x19, 0x67, 0xf1, 0xa6, 0x71, 0x30, 0xb7, 0x10,
  0x5c, 0xd6, 0xa8, 0x28, 0xe0, 0x39, 0x09, 0xa6,
  0x79, 0x62, 0xe0, 0xea, 0x1f, 0x61, 0xde, 0xb6,
  0x49, 0xf6, 0xbc, 0x3f, 0x4c, 0xef, 0x38, 0xc4,
  0xf3, 0x55, 0x04, 0xe5, 0x1e, 0xc1, 0x12, 0xde,
  0x5c, 0x38, 0x4d, 0xf7, 0xba, 0x0b, 0x8d, 0x57,
  0x8a, 0x4c, 0x70, 0x2b, 0x6b, 0xf1, 0x1d, 0x5f,
  0xac, 0x00, 0x00, 0x00, 0x00
};

static const btc_checkpoint_t mainnet_bip30[] = {
  {
    91842,
    {
      0xec, 0xca, 0xe0, 0x00, 0xe3, 0xc8, 0xe4, 0xe0,
      0x93, 0x93, 0x63, 0x60, 0x43, 0x1f, 0x3b, 0x76,
      0x03, 0xc5, 0x63, 0xc1, 0xff, 0x61, 0x81, 0x39,
      0x0a, 0x4d, 0x0a, 0x00, 0x00, 0x00, 0x00, 0x00
    }
  },
  {
    91880,
    {
      0x21, 0xd7, 0x7c, 0xcb, 0x4c, 0x08, 0x38, 0x6a,
      0x04, 0xac, 0x01, 0x96, 0xae, 0x10, 0xf6, 0xa1,
      0xd2, 0xc2, 0xa3, 0x77, 0x55, 0x8c, 0xa1, 0x90,
      0xf1, 0x43, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00
    }
  }
};

static const btc_deployment_t mainnet_deployments[] = {
  {
    /* .name = */ "csv",
    /* .bit = */ 0,
    /* .start_time = */ 1462060800, /* May 1st, 2016 */
    /* .timeout = */ 1493596800, /* May 1st, 2017 */
    /* .threshold = */ -1,
    /* .window = */ -1,
    /* .required = */ 0,
    /* .force = */ 1
  },
  {
    /* .name = */ "segwit",
    /* .bit = */ 1,
    /* .start_time = */ 1479168000, /* November 15th, 2016. */
    /* .timeout = */ 1510704000, /* November 15th, 2017. */
    /* .threshold = */ -1,
    /* .window = */ -1,
    /* .required = */ 1,
    /* .force = */ 0
  },
  {
    /* .name = */ "segsignal",
    /* .bit = */ 4,
    /* .start_time = */ 1496275200, /* June 1st, 2017. */
    /* .timeout = */ 1510704000, /* November 15th, 2017. */
    /* .threshold = */ 269, /* 80% */
    /* .window = */ 336, /* ~2.33 days */
    /* .required = */ 0,
    /* .force = */ 0
  },
  {
    /* .name = */ "testdummy",
    /* .bit = */ 28,
    /* .start_time = */ 1199145601, /* January 1, 2008 */
    /* .timeout = */ 1230767999, /* December 31, 2008 */
    /* .threshold = */ -1,
    /* .window = */ -1,
    /* .required = */ 0,
    /* .force = */ 1
  }
};

static const btc_network_t mainnet = {
  /* .type = */ BTC_NETWORK_MAINNET,
  /* .name = */ "mainnet",
  /* .seeds = */ {
    /* .items = */ mainnet_seeds,
    /* .length = */ lengthof(mainnet_seeds)
  },
  /* .magic = */ 0xd9b4bef9,
  /* .port = */ 8333,
  /* .checkpoints = */ {
    /* .items = */ mainnet_checkpoints,
    /* .length = */ lengthof(mainnet_checkpoints)
  },
  /* .last_checkpoint */ 525000,
  /* .halving_interval = */ 210000,
  /* .genesis = */ {
    /* .hash = */ {
      0x6f, 0xe2, 0x8c, 0x0a, 0xb6, 0xf1, 0xb3, 0x72,
      0xc1, 0xa6, 0xa2, 0x46, 0xae, 0x63, 0xf7, 0x4f,
      0x93, 0x1e, 0x83, 0x65, 0xe1, 0x5a, 0x08, 0x9c,
      0x68, 0xd6, 0x19, 0x00, 0x00, 0x00, 0x00, 0x00
    },
    /* .header = */ {
      /* .version = */ 1,
      /* .prev_block = */ {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
      },
      /* .merkle_root = */ {
        0x3b, 0xa3, 0xed, 0xfd, 0x7a, 0x7b, 0x12, 0xb2,
        0x7a, 0xc7, 0x2c, 0x3e, 0x67, 0x76, 0x8f, 0x61,
        0x7f, 0xc8, 0x1b, 0xc3, 0x88, 0x8a, 0x51, 0x32,
        0x3a, 0x9f, 0xb8, 0xaa, 0x4b, 0x1e, 0x5e, 0x4a
      },
      /* .time = */ 1231006505,
      /* .bits = */ 486604799,
      /* .nonce = */ 2083236893
    },
    /* .data = */ mainnet_genesis,
    /* .length = */ sizeof(mainnet_genesis)
  },
  /* .pow = */ {
    /* .limit = */ {
      0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
      0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
      0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
      0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00
    },
    /* .bits = */ 486604799,
    /* .chainwork = */ {
      0x8f, 0x18, 0xa1, 0x29, 0x9d, 0x77, 0xc7, 0xd8,
      0xb7, 0xc9, 0x59, 0x02, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    },
    /* .target_timespan = */ 14 * 24 * 60 * 60,
    /* .target_spacing = */ 10 * 60,
    /* .retarget_interval = */ 2016,
    /* .target_reset = */ 0,
    /* .no_retargeting = */ 0
  },
  /* .block = */ {
    /* .prune_after_height = */ 1000,
    /* .keep_blocks = */ 288,
    /* .max_tip_age = */ 24 * 60 * 60,
    /* .slow_height = */ 325000
  },
  /* .softforks = */ {
    /* .bip30 = */ {
      /* .items = */ mainnet_bip30,
      /* .length = */ lengthof(mainnet_bip30)
    },
    /* .bip34 = */ {
      227931,
      {
        0xb8, 0x08, 0x08, 0x9c, 0x75, 0x6a, 0xdd, 0x15,
        0x91, 0xb1, 0xd1, 0x7b, 0xab, 0x44, 0xbb, 0xa3,
        0xfe, 0xd9, 0xe0, 0x2f, 0x94, 0x2a, 0xb4, 0x89,
        0x4b, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
      }
    },
    /* .bip65 = */ {
      388381,
      {
        0xf0, 0x35, 0x47, 0x6c, 0xfa, 0xeb, 0x9f, 0x67,
        0x7c, 0x2c, 0xda, 0xd0, 0x0f, 0xd9, 0x08, 0xc5,
        0x56, 0x77, 0x5d, 0xed, 0x24, 0xb6, 0xc2, 0x04,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
      }
    },
    /* .bip66 = */ {
      363725,
      {
        0x31, 0x09, 0xb5, 0x88, 0x94, 0x11, 0x88, 0xa9,
        0xf1, 0xc2, 0x57, 0x6a, 0xae, 0x46, 0x2d, 0x72,
        0x9b, 0x8c, 0xce, 0x9d, 0xa1, 0xea, 0x79, 0x03,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
      }
    }
  },
  /* .activation_threshold = */ 1916, /* 95% of 2016 */
  /* .miner_window = */ 2016, /* pow.target_timespan / pow.target_spacing */
  /* .deployments = */ {
    /* .items = */ mainnet_deployments,
    /* .length = */ lengthof(mainnet_deployments)
  },
  /* .key = */ {
    /* .privkey = */ 0x80,
    /* .xpubkey = */ {
      0x0488b21e, /* xpub (p2pkh or p2sh) */
      0x049d7cb2, /* ypub (nested p2wpkh) */
      0x04b24746, /* zpub (native p2wpkh) */
      0x0295b43f, /* Ypub (nested p2wsh) */
      0x02aa7ed3  /* Zpub (native p2wsh) */
    },
    /* .xprvkey = */ {
      0x0488ade4, /* xprv (p2pkh or p2sh) */
      0x049d7878, /* yprv (nested p2wpkh) */
      0x04b2430c, /* zprv (native p2wpkh) */
      0x0295b005, /* Yprv (nested p2wsh) */
      0x02aa7a99  /* Zprv (native p2wsh) */
    },
    /* .coin_type = */ 0
  },
  /* .address = */ {
    /* .p2pkh = */ 0x00,
    /* .p2sh = */ 0x05,
    /* .bech32 = */ "bc"
  },
  /* .require_standard = */ 1,
  /* .rpc_port = */ 8332,
  /* .min_relay = */ 1000,
  /* .fee_rate = */ 100000,
  /* .max_fee_rate = */ 400000,
  /* .self_connect = */ 0,
  /* .request_mempool = */ 0
};

const btc_network_t *btc_mainnet = &mainnet;
