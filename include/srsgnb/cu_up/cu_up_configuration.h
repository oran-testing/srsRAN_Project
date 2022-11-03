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

#include "srsgnb/f1_interface/cu/f1c_cu.h"
#include "srsgnb/support/executors/task_executor.h"

namespace srsgnb {
namespace srs_cu_up {

/// Configuration passed to CU-UP.
struct cu_up_configuration {
  task_executor*       cu_up_executor = nullptr;
  e1_message_notifier* e1_notifier    = nullptr; /// Callback for incoming E1 messages.
};

} // namespace srs_cu_up
} // namespace srsgnb