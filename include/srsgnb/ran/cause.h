/*
 *
 * Copyright 2013-2022 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#pragma once

namespace srsgnb {

enum class cause_t : uint8_t {
  radio_network = 0,
  transport     = 1,
  nas           = 2,
  protocol      = 3,
  misc          = 4,
  choice_exts   = 5,
  nulltype      = 6
};

} // namespace srsgnb