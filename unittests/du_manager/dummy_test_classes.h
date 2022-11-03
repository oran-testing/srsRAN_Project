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

#include "srsgnb/du_manager/du_manager_params.h"
#include "srsgnb/support/async/async_test_utils.h"

namespace srsgnb {
namespace srs_du {

class dummy_ue_executor_mapper : public du_high_ue_executor_mapper
{
public:
  explicit dummy_ue_executor_mapper(task_executor& exec_) : exec(exec_) {}
  task_executor& rebind_executor(du_ue_index_t ue_index, du_cell_index_t pcell_index) override { return exec; }
  task_executor& executor(du_ue_index_t ue_index) override { return exec; }

  task_executor& exec;
};

class dummy_cell_executor_mapper : public du_high_cell_executor_mapper
{
public:
  explicit dummy_cell_executor_mapper(task_executor& exec_) : exec(exec_) {}
  task_executor& executor(du_cell_index_t cell_index) override { return exec; }

  task_executor& exec;
};

class f1ap_test_dummy : public f1ap_connection_manager,
                        public f1ap_ue_context_manager,
                        public f1c_message_handler,
                        public f1ap_rrc_message_transfer_procedure_handler
{
public:
  wait_manual_event_tester<f1_setup_response_message>                     wait_f1_setup;
  optional<f1ap_ue_create_request>                                        last_ue_create{};
  f1ap_ue_create_response                                                 next_ue_create_response;
  wait_manual_event_tester<f1ap_ue_context_modification_response_message> wait_ue_mod;

  async_task<f1_setup_response_message> handle_f1ap_setup_request(const f1_setup_request_message& request) override
  {
    return wait_f1_setup.launch();
  }

  /// Initiates creation of UE context in F1.
  f1ap_ue_create_response handle_ue_creation_request(const f1ap_ue_create_request& msg) override
  {
    last_ue_create = msg;
    return next_ue_create_response;
  }

  void handle_ue_context_release_request(const f1ap_ue_context_release_request_message& request) override {}

  async_task<f1ap_ue_context_modification_response_message>
  handle_ue_context_modification_required(const f1ap_ue_context_modification_required_message& msg) override
  {
    return wait_ue_mod.launch();
  }

  void handle_ue_inactivity_notification(const f1ap_ue_inactivity_notification_message& msg) override {}

  void handle_notify(const f1ap_notify_message& msg) override {}

  void handle_message(const f1c_message& msg) override {}

  void handle_rrc_delivery_report(const f1ap_rrc_delivery_report_msg& report) override {}
};

class mac_test_dummy : public mac_cell_manager, public mac_ue_configurator, public mac_ue_control_information_handler
{
public:
  class mac_cell_dummy : public mac_cell_controller
  {
    wait_manual_event_tester<void> wait_start;
    wait_manual_event_tester<void> wait_stop;

    async_task<void> start() override { return wait_start.launch(); }
    async_task<void> stop() override { return wait_stop.launch(); }
  };

  mac_cell_dummy mac_cell;

  optional<mac_ue_create_request_message>                           last_ue_create_msg{};
  optional<mac_ue_delete_request_message>                           last_ue_delete_msg{};
  byte_buffer                                                       last_pushed_ul_ccch_msg;
  wait_manual_event_tester<mac_ue_create_response_message>          wait_ue_create;
  wait_manual_event_tester<mac_ue_reconfiguration_response_message> wait_ue_reconf;
  wait_manual_event_tester<mac_ue_delete_response_message>          wait_ue_delete;

  void                 add_cell(const mac_cell_creation_request& cell_cfg) override {}
  void                 remove_cell(du_cell_index_t cell_index) override {}
  mac_cell_controller& get_cell_controller(du_cell_index_t cell_index) override { return mac_cell; }

  async_task<mac_ue_create_response_message> handle_ue_create_request(const mac_ue_create_request_message& msg) override
  {
    last_ue_create_msg = msg;
    return wait_ue_create.launch();
  }
  async_task<mac_ue_reconfiguration_response_message>
  handle_ue_reconfiguration_request(const mac_ue_reconfiguration_request_message& msg) override
  {
    return wait_ue_reconf.launch();
  }
  async_task<mac_ue_delete_response_message> handle_ue_delete_request(const mac_ue_delete_request_message& msg) override
  {
    last_ue_delete_msg = msg;
    return wait_ue_delete.launch();
  }
  void handle_ul_ccch_msg(du_ue_index_t ue_index, byte_buffer pdu) override
  {
    last_pushed_ul_ccch_msg = std::move(pdu);
  }

  void handle_dl_buffer_state_update_required(const mac_dl_buffer_state_indication_message& dl_bs) override {}
};

} // namespace srs_du
} // namespace srsgnb