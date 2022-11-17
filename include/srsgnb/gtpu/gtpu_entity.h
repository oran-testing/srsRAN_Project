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

#include "srsgnb/gtpu/gtpu_dl.h"
#include "srsgnb/gtpu/gtpu_ul.h"

namespace srsgnb {

/// Class used to interface with an RLC entity.
/// It will contain getters for the TX and RX entities interfaces.
class gtpu_entity
{
public:
  gtpu_entity()                               = default;
  virtual ~gtpu_entity()                      = default;
  gtpu_entity(const gtpu_entity&)             = delete;
  gtpu_entity& operator=(const gtpu_entity&)  = delete;
  gtpu_entity(const gtpu_entity&&)            = delete;
  gtpu_entity& operator=(const gtpu_entity&&) = delete;

  virtual gtpu_dl_upper_layer_interface* get_dl_upper_layer_interface() = 0;
  virtual gtpu_ul_lower_layer_interface* get_ul_lower_layer_interface() = 0;
};

} // namespace srsgnb