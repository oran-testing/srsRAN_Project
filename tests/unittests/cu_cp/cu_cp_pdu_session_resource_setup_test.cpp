/*
 *
 * Copyright 2021-2024 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "cu_cp_test_environment.h"
#include "tests/test_doubles/f1ap/f1ap_test_message_validators.h"
#include "tests/test_doubles/f1ap/f1ap_test_messages.h"
#include "tests/test_doubles/ngap/ngap_test_message_validators.h"
#include "tests/test_doubles/rrc/rrc_test_message_validators.h"
#include "tests/unittests/e1ap/common/e1ap_cu_cp_test_messages.h"
#include "tests/unittests/f1ap/common/f1ap_cu_test_messages.h"
#include "tests/unittests/ngap/ngap_test_messages.h"
#include "srsran/e1ap/common/e1ap_types.h"
#include "srsran/f1ap/common/f1ap_message.h"
#include "srsran/ngap/ngap_message.h"
#include "srsran/ran/cu_types.h"
#include "srsran/ran/lcid.h"
#include <gtest/gtest.h>
#include <optional>

using namespace srsran;
using namespace srs_cu_cp;

class cu_cp_pdu_session_resource_setup_test : public cu_cp_test_environment, public ::testing::Test
{
public:
  cu_cp_pdu_session_resource_setup_test() : cu_cp_test_environment(cu_cp_test_env_params{8, 8, 8192, create_mock_amf()})
  {
    // Run NG setup to completion.
    run_ng_setup();

    // Setup DU.
    std::optional<unsigned> ret = connect_new_du();
    EXPECT_TRUE(ret.has_value());
    du_idx = ret.value();
    EXPECT_TRUE(this->run_f1_setup(du_idx));

    // Setup CU-UP.
    ret = connect_new_cu_up();
    EXPECT_TRUE(ret.has_value());
    cu_up_idx = ret.value();
    EXPECT_TRUE(this->run_e1_setup(cu_up_idx));

    // Connect UE 0x4601.
    EXPECT_TRUE(connect_new_ue(du_idx, du_ue_id, crnti));
    EXPECT_TRUE(authenticate_ue(du_idx, du_ue_id, amf_ue_id));
    EXPECT_TRUE(setup_ue_security(du_idx, du_ue_id));
    ue_ctx = this->find_ue_context(du_idx, du_ue_id);

    EXPECT_TRUE(finish_ue_registration(du_idx, cu_up_idx, du_ue_id));
    EXPECT_TRUE(request_pdu_session_resource_setup(du_idx, cu_up_idx, du_ue_id));

    EXPECT_NE(ue_ctx, nullptr);
  }

  void setup_pdu_session(pdu_session_id_t psi_,
                         drb_id_t         drb_id_,
                         qos_flow_id_t    qfi_,
                         byte_buffer      rrc_reconfiguration_complete = make_byte_buffer("00070e00cc6fcda5").value(),
                         bool             is_initial_session_          = true)
  {
    EXPECT_TRUE(cu_cp_test_environment::setup_pdu_session(du_idx,
                                                          cu_up_idx,
                                                          du_ue_id,
                                                          crnti,
                                                          cu_up_e1ap_id,
                                                          psi_,
                                                          drb_id_,
                                                          qfi_,
                                                          std::move(rrc_reconfiguration_complete),
                                                          is_initial_session_));
  }

  ngap_message generate_pdu_session_resource_setup_request_with_unconfigured_fiveqi() const
  {
    ngap_message request = generate_valid_pdu_session_resource_setup_request_message(
        ue_ctx->amf_ue_id.value(), ue_ctx->ran_ue_id.value(), {{psi, {{qfi, 99}}}});

    return request;
  }

  void send_pdu_session_resource_setup_request_and_await_pdu_session_setup_response(
      const ngap_message& pdu_session_resource_setup_request)
  {
    srsran_assert(not this->get_amf().try_pop_rx_pdu(ngap_pdu), "there are still NGAP messages to pop from AMF");
    srsran_assert(not this->get_du(du_idx).try_pop_dl_pdu(f1ap_pdu), "there are still F1AP DL messages to pop from DU");
    srsran_assert(not this->get_cu_up(cu_up_idx).try_pop_rx_pdu(e1ap_pdu),
                  "there are still E1AP messages to pop from CU-UP");

    // Inject PDU Session Resource Setup Request and wait for PDU Session Resource Setup Response
    get_amf().push_tx_pdu(pdu_session_resource_setup_request);
    bool result = this->wait_for_ngap_tx_pdu(ngap_pdu);
    report_fatal_error_if_not(result, "Failed to receive PDU Session Resource Setup Response");
    report_fatal_error_if_not(test_helpers::is_valid_pdu_session_resource_setup_response(ngap_pdu),
                              "Invalid PDU Session Resource Setup Response");
    report_fatal_error_if_not(test_helpers::is_expected_pdu_session_resource_setup_response(ngap_pdu, {}, {psi}),
                              "Unsuccessful PDU Session Resource Setup Response");
  }

  void send_pdu_session_resource_setup_request_and_await_bearer_context_setup_request(
      const ngap_message& pdu_session_resource_setup_request)
  {
    ASSERT_TRUE(cu_cp_test_environment::send_pdu_session_resource_setup_request_and_await_bearer_context_setup_request(
        pdu_session_resource_setup_request, du_idx, cu_up_idx, du_ue_id));
  }

  void send_pdu_session_resource_setup_request_and_await_bearer_context_modification_request(
      const ngap_message& pdu_session_resource_setup_request)
  {
    ASSERT_TRUE(
        cu_cp_test_environment::send_pdu_session_resource_setup_request_and_await_bearer_context_modification_request(
            pdu_session_resource_setup_request, du_idx));
  }

  void send_bearer_context_setup_failure_and_await_pdu_session_setup_response()
  {
    // Inject Bearer Context Setup Failure and wait for PDU Session Resource Setup Response
    get_cu_up(cu_up_idx).push_tx_pdu(
        generate_bearer_context_setup_failure(ue_ctx->cu_cp_e1ap_id.value(), cu_up_e1ap_id));
    bool result = this->wait_for_ngap_tx_pdu(ngap_pdu);
    report_fatal_error_if_not(result, "Failed to receive PDU Session Resource Setup Response");
    report_fatal_error_if_not(test_helpers::is_valid_pdu_session_resource_setup_response(ngap_pdu),
                              "Invalid PDU Session Resource Setup Response");
    report_fatal_error_if_not(test_helpers::is_expected_pdu_session_resource_setup_response(ngap_pdu, {}, {psi}),
                              "Unsuccessful PDU Session Resource Setup Response");
  }

  void send_bearer_context_setup_response_and_await_ue_context_modification_request()
  {
    ASSERT_TRUE(cu_cp_test_environment::send_bearer_context_setup_response_and_await_ue_context_modification_request(
        du_idx, cu_up_idx, du_ue_id, cu_up_e1ap_id, psi, qfi));
  }

  void send_bearer_context_modification_response_and_await_ue_context_modification_request()
  {
    ASSERT_TRUE(
        cu_cp_test_environment::send_bearer_context_modification_response_and_await_ue_context_modification_request(
            du_idx, cu_up_idx, du_ue_id, psi2, drb_id_t::drb2, qfi2));
  }

  void send_ue_context_modification_failure_and_await_pdu_session_setup_response()
  {
    // Inject UE Context Modification Failure and wait for PDU Session Resource Setup Response
    get_du(du_idx).push_ul_pdu(
        generate_ue_context_modification_failure(ue_ctx->cu_ue_id.value(), ue_ctx->du_ue_id.value()));
    bool result = this->wait_for_ngap_tx_pdu(ngap_pdu);
    report_fatal_error_if_not(result, "Failed to receive PDU Session Resource Setup Response");
    report_fatal_error_if_not(test_helpers::is_valid_pdu_session_resource_setup_response(ngap_pdu),
                              "Invalid PDU Session Resource Setup Response");
    report_fatal_error_if_not(test_helpers::is_expected_pdu_session_resource_setup_response(ngap_pdu, {}, {psi}),
                              "Unsuccessful PDU Session Resource Setup Response");
  }

  void send_ue_context_modification_response_and_await_bearer_context_modification_request()
  {
    ASSERT_TRUE(
        cu_cp_test_environment::send_ue_context_modification_response_and_await_bearer_context_modification_request(
            du_idx, cu_up_idx, du_ue_id, crnti));
  }

  void send_bearer_context_modification_failure_and_await_pdu_session_setup_response()
  {
    // Inject Bearer Context Modification Failure and wait for PDU Session Resource Setup Response
    get_cu_up(cu_up_idx).push_tx_pdu(
        generate_bearer_context_modification_failure(ue_ctx->cu_cp_e1ap_id.value(), ue_ctx->cu_up_e1ap_id.value()));
    bool result = this->wait_for_ngap_tx_pdu(ngap_pdu);
    report_fatal_error_if_not(result, "Failed to receive PDU Session Resource Setup Response");
    report_fatal_error_if_not(test_helpers::is_valid_pdu_session_resource_setup_response(ngap_pdu),
                              "Invalid PDU Session Resource Setup Response");
    report_fatal_error_if_not(test_helpers::is_expected_pdu_session_resource_setup_response(ngap_pdu, {}, {psi}),
                              "Unsuccessful PDU Session Resource Setup Response");
  }

  void send_bearer_context_modification_response_and_await_rrc_reconfiguration(
      const std::map<pdu_session_id_t, drb_test_params>& pdu_sessions_to_add = {},
      const std::map<pdu_session_id_t, drb_id_t>& pdu_sessions_to_modify   = {{pdu_session_id_t::min, drb_id_t::drb1}},
      const std::optional<std::vector<srb_id_t>>& expected_srbs_to_add_mod = std::nullopt,
      const std::optional<std::vector<drb_id_t>>& expected_drbs_to_add_mod = std::nullopt)
  {
    ASSERT_TRUE(cu_cp_test_environment::send_bearer_context_modification_response_and_await_rrc_reconfiguration(
        du_idx,
        cu_up_idx,
        du_ue_id,
        pdu_sessions_to_add,
        pdu_sessions_to_modify,
        expected_srbs_to_add_mod,
        expected_drbs_to_add_mod));
  }

  void timeout_rrc_reconfiguration_and_await_pdu_session_setup_response()
  {
    // Fail RRC Reconfiguration (UE doesn't respond) and wait for PDU Session Resource Setup Response
    ASSERT_FALSE(tick_until(std::chrono::milliseconds(this->get_cu_cp_cfg().rrc.rrc_procedure_timeout_ms),
                            [&]() { return false; }));
    bool result = this->wait_for_ngap_tx_pdu(ngap_pdu);
    report_fatal_error_if_not(result, "Failed to receive PDU Session Resource Setup Response");
    report_fatal_error_if_not(test_helpers::is_valid_pdu_session_resource_setup_response(ngap_pdu),
                              "Invalid PDU Session Resource Setup Response");
    report_fatal_error_if_not(test_helpers::is_expected_pdu_session_resource_setup_response(ngap_pdu, {}, {psi}),
                              "Unsuccessful PDU Session Resource Setup Response");
  }

  void send_rrc_reconfiguration_complete_and_await_pdu_session_setup_response(
      byte_buffer                          rrc_container,
      const std::vector<pdu_session_id_t>& expected_pdu_sessions_to_setup,
      const std::vector<pdu_session_id_t>& expected_pdu_sessions_failed_to_setup)
  {
    ASSERT_TRUE(cu_cp_test_environment::send_rrc_reconfiguration_complete_and_await_pdu_session_setup_response(
        du_idx,
        du_ue_id,
        std::move(rrc_container),
        expected_pdu_sessions_to_setup,
        expected_pdu_sessions_failed_to_setup));
  }

  unsigned du_idx    = 0;
  unsigned cu_up_idx = 0;

  gnb_du_ue_f1ap_id_t    du_ue_id      = gnb_du_ue_f1ap_id_t::min;
  rnti_t                 crnti         = to_rnti(0x4601);
  amf_ue_id_t            amf_ue_id     = amf_ue_id_t::min;
  gnb_cu_up_ue_e1ap_id_t cu_up_e1ap_id = gnb_cu_up_ue_e1ap_id_t::min;

  const ue_context* ue_ctx = nullptr;

  pdu_session_id_t psi  = uint_to_pdu_session_id(1);
  pdu_session_id_t psi2 = uint_to_pdu_session_id(2);
  qos_flow_id_t    qfi  = uint_to_qos_flow_id(1);
  qos_flow_id_t    qfi2 = uint_to_qos_flow_id(2);

  ngap_message ngap_pdu;
  f1ap_message f1ap_pdu;
  e1ap_message e1ap_pdu;
};

TEST_F(cu_cp_pdu_session_resource_setup_test,
       when_pdu_session_setup_request_with_unconfigured_fiveqi_received_setup_fails)
{
  // Inject NGAP PDU Session Resource Setup Request and await PDU Session Setup Response
  send_pdu_session_resource_setup_request_and_await_pdu_session_setup_response(
      generate_pdu_session_resource_setup_request_with_unconfigured_fiveqi());
}

TEST_F(cu_cp_pdu_session_resource_setup_test, when_bearer_context_setup_failure_received_then_setup_fails)
{
  // Inject NGAP PDU Session Resource Setup Request and await Bearer Context Setup Request
  send_pdu_session_resource_setup_request_and_await_bearer_context_setup_request(
      generate_valid_pdu_session_resource_setup_request_message(
          ue_ctx->amf_ue_id.value(), ue_ctx->ran_ue_id.value(), {{psi, {{qfi, 9}}}}));

  // Inject Bearer Context Setup Failure and await PDU Session Resource Setup Response
  send_bearer_context_setup_failure_and_await_pdu_session_setup_response();
}

TEST_F(cu_cp_pdu_session_resource_setup_test, when_ue_context_modification_failure_received_then_setup_fails)
{
  // Inject NGAP PDU Session Resource Setup Request and await Bearer Context Setup Request
  send_pdu_session_resource_setup_request_and_await_bearer_context_setup_request(
      generate_valid_pdu_session_resource_setup_request_message(
          ue_ctx->amf_ue_id.value(), ue_ctx->ran_ue_id.value(), {{psi, {{qfi, 9}}}}));

  // Inject Bearer Context Setup Response and await UE Context Modification Request
  send_bearer_context_setup_response_and_await_ue_context_modification_request();

  // Inject UE Context Modification Failure and await PDU Session Resource Setup Response
  send_ue_context_modification_failure_and_await_pdu_session_setup_response();
}

TEST_F(cu_cp_pdu_session_resource_setup_test, when_bearer_context_modification_failure_received_then_setup_fails)
{
  // Inject NGAP PDU Session Resource Setup Request and await Bearer Context Setup Request
  send_pdu_session_resource_setup_request_and_await_bearer_context_setup_request(
      generate_valid_pdu_session_resource_setup_request_message(
          ue_ctx->amf_ue_id.value(), ue_ctx->ran_ue_id.value(), {{psi, {{qfi, 9}}}}));

  // Inject Bearer Context Setup Response and await UE Context Modification Request
  send_bearer_context_setup_response_and_await_ue_context_modification_request();

  // Inject UE Context Modification Response and await Bearer Context Modification Request
  send_ue_context_modification_response_and_await_bearer_context_modification_request();

  // Inject Bearer Context Modification Failure and await PDU Session Resource Setup Response
  send_bearer_context_modification_failure_and_await_pdu_session_setup_response();
}

TEST_F(cu_cp_pdu_session_resource_setup_test, when_rrc_reconfiguration_fails_then_setup_fails)
{
  // Inject NGAP PDU Session Resource Setup Request and await Bearer Context Setup Request
  send_pdu_session_resource_setup_request_and_await_bearer_context_setup_request(
      generate_valid_pdu_session_resource_setup_request_message(
          ue_ctx->amf_ue_id.value(), ue_ctx->ran_ue_id.value(), {{psi, {{qfi, 9}}}}));

  // Inject Bearer Context Setup Response and await UE Context Modification Request
  send_bearer_context_setup_response_and_await_ue_context_modification_request();

  // Inject UE Context Modification Response and await Bearer Context Modification Request
  send_ue_context_modification_response_and_await_bearer_context_modification_request();

  // Inject Bearer Context Modification Response and await DL RRC Message Transfer containing RRC Reconfiguration
  send_bearer_context_modification_response_and_await_rrc_reconfiguration(
      {}, {{psi, drb_id_t::drb1}}, std::vector<srb_id_t>{srb_id_t::srb2}, std::vector<drb_id_t>{drb_id_t::drb1});

  // Let the RRC Reconfiguration timeout and await PDU Session Resource Setup Response
  timeout_rrc_reconfiguration_and_await_pdu_session_setup_response();
}

TEST_F(cu_cp_pdu_session_resource_setup_test, when_rrc_reconfiguration_succeeds_then_setup_succeeds)
{
  // Inject NGAP PDU Session Resource Setup Request and await Bearer Context Setup Request
  send_pdu_session_resource_setup_request_and_await_bearer_context_setup_request(
      generate_valid_pdu_session_resource_setup_request_message(
          ue_ctx->amf_ue_id.value(), ue_ctx->ran_ue_id.value(), {{psi, {{qfi, 9}}}}));

  // Inject Bearer Context Setup Response and await UE Context Modification Request
  send_bearer_context_setup_response_and_await_ue_context_modification_request();

  // Inject UE Context Modification Response and await Bearer Context Modification Request
  send_ue_context_modification_response_and_await_bearer_context_modification_request();

  // Inject Bearer Context Modification Response and await DL RRC Message Transfer containing RRC Reconfiguration
  send_bearer_context_modification_response_and_await_rrc_reconfiguration(
      {}, {{psi, drb_id_t::drb1}}, std::vector<srb_id_t>{srb_id_t::srb2}, std::vector<drb_id_t>{drb_id_t::drb1});

  // Inject RRC Reconfiguration Complete and await successful PDU Session Resource Setup Response
  send_rrc_reconfiguration_complete_and_await_pdu_session_setup_response(
      make_byte_buffer("00070e00cc6fcda5").value(), {psi}, {});
}

TEST_F(cu_cp_pdu_session_resource_setup_test, when_pdu_session_setup_for_existing_session_arrives_then_setup_fails)
{
  // Setup first PDU session
  setup_pdu_session(psi, drb_id_t::drb1, qfi);

  // Inject NGAP PDU Session Resource Setup Request and await PDU Session Setup Response
  send_pdu_session_resource_setup_request_and_await_pdu_session_setup_response(
      generate_valid_pdu_session_resource_setup_request_message(
          ue_ctx->amf_ue_id.value(), ue_ctx->ran_ue_id.value(), {{psi, {{qfi, 9}}}}));
}

TEST_F(cu_cp_pdu_session_resource_setup_test, when_setup_for_pdu_sessions_with_two_qos_flows_received_setup_succeeds)
{
  // Inject NGAP PDU Session Resource Setup Request and await Bearer Context Setup Request
  send_pdu_session_resource_setup_request_and_await_bearer_context_setup_request(
      generate_valid_pdu_session_resource_setup_request_message(
          ue_ctx->amf_ue_id.value(), ue_ctx->ran_ue_id.value(), {{psi, {{qfi, 9}, {qfi2, 9}}}}));

  // Inject Bearer Context Setup Response and await UE Context Modification Request
  send_bearer_context_setup_response_and_await_ue_context_modification_request();

  // Inject UE Context Modification Response and await Bearer Context Modification Request
  send_ue_context_modification_response_and_await_bearer_context_modification_request();

  // Inject Bearer Context Modification Response and await DL RRC Message Transfer containing RRC Reconfiguration
  send_bearer_context_modification_response_and_await_rrc_reconfiguration(
      {},
      {{psi, drb_id_t::drb1}, {psi2, drb_id_t::drb2}},
      std::vector<srb_id_t>{srb_id_t::srb2},
      std::vector<drb_id_t>{drb_id_t::drb1, drb_id_t::drb2});

  // Inject RRC Reconfiguration Complete and await successful PDU Session Resource Setup Response
  send_rrc_reconfiguration_complete_and_await_pdu_session_setup_response(
      make_byte_buffer("00070e00cc6fcda5").value(), {psi}, {});
}

TEST_F(
    cu_cp_pdu_session_resource_setup_test,
    when_setup_for_two_pdu_sessions_is_requested_but_only_first_could_be_setup_at_cu_up_setup_succeeds_with_fail_list)
{
  // Inject NGAP PDU Session Resource Setup Request and await Bearer Context Setup Request
  send_pdu_session_resource_setup_request_and_await_bearer_context_setup_request(
      generate_valid_pdu_session_resource_setup_request_message(
          ue_ctx->amf_ue_id.value(),
          ue_ctx->ran_ue_id.value(),
          {{psi, {qos_flow_test_params{qfi, 9}}}, {psi2, {qos_flow_test_params{qfi2, 7}}}}));

  // Inject Bearer Context Setup Response and await UE Context Modification Request
  get_cu_up(cu_up_idx).push_tx_pdu(generate_bearer_context_setup_response(
      ue_ctx->cu_cp_e1ap_id.value(), cu_up_e1ap_id, {{psi, drb_test_params{drb_id_t::drb1, qfi}}}, {psi2}));
  bool result = this->wait_for_f1ap_tx_pdu(du_idx, f1ap_pdu);
  report_fatal_error_if_not(result, "Failed to receive UE Context Modification Request");
  report_fatal_error_if_not(test_helpers::is_valid_ue_context_modification_request(f1ap_pdu),
                            "Invalid UE Context Modification Request");

  // Inject UE Context Modification Response and await Bearer Context Modification Request
  send_ue_context_modification_response_and_await_bearer_context_modification_request();

  // Inject Bearer Context Modification Response and await DL RRC Message Transfer containing RRC Reconfiguration
  get_cu_up(cu_up_idx).push_tx_pdu(
      generate_bearer_context_modification_response(ue_ctx->cu_cp_e1ap_id.value(), cu_up_e1ap_id));
  result = this->wait_for_f1ap_tx_pdu(du_idx, f1ap_pdu);
  report_fatal_error_if_not(result, "Failed to receive F1AP DL RRC Message (containing RRC Reconfiguration)");
  report_fatal_error_if_not(test_helpers::is_valid_dl_rrc_message_transfer(f1ap_pdu),
                            "Invalid DL RRC Message Transfer");
  {
    const byte_buffer& rrc_container = test_helpers::get_rrc_container(f1ap_pdu);
    report_fatal_error_if_not(
        test_helpers::is_valid_rrc_reconfiguration(test_helpers::extract_dl_dcch_msg(rrc_container),
                                                   true,
                                                   std::vector<srb_id_t>{srb_id_t::srb2},
                                                   std::vector<drb_id_t>{drb_id_t::drb1}),
        "Invalid RRC Reconfiguration");
  }

  // Inject RRC Reconfiguration Complete and await successful PDU Session Resource Setup Response
  get_du(du_idx).push_ul_pdu(test_helpers::create_ul_rrc_message_transfer(
      du_ue_id, ue_ctx->cu_ue_id.value(), srb_id_t::srb1, make_byte_buffer("00070e00cc6fcda5").value()));
  result = this->wait_for_ngap_tx_pdu(ngap_pdu);
  report_fatal_error_if_not(result, "Failed to receive PDU Session Resource Setup Response");
  report_fatal_error_if_not(test_helpers::is_valid_pdu_session_resource_setup_response(ngap_pdu),
                            "Invalid PDU Session Resource Setup Response");
  report_fatal_error_if_not(test_helpers::is_expected_pdu_session_resource_setup_response(ngap_pdu, {psi}, {psi2}),
                            "Unsuccessful PDU Session Resource Setup Response");
}

TEST_F(cu_cp_pdu_session_resource_setup_test,
       when_setup_for_two_pdu_sessions_is_requested_and_both_succeed_setup_succeeds)
{
  // Inject NGAP PDU Session Resource Setup Request and await Bearer Context Setup Request
  send_pdu_session_resource_setup_request_and_await_bearer_context_setup_request(
      generate_valid_pdu_session_resource_setup_request_message(
          ue_ctx->amf_ue_id.value(), ue_ctx->ran_ue_id.value(), {{psi, {{qfi, 9}}}, {psi2, {{qfi2, 9}}}}));

  // Inject Bearer Context Setup Response and await UE Context Modification Request
  send_bearer_context_setup_response_and_await_ue_context_modification_request();

  // Inject UE Context Modification Response and await Bearer Context Modification Request
  send_ue_context_modification_response_and_await_bearer_context_modification_request();

  // Inject Bearer Context Modification Response and await DL RRC Message Transfer containing RRC Reconfiguration
  send_bearer_context_modification_response_and_await_rrc_reconfiguration(
      {},
      {{psi, drb_id_t::drb1}, {psi2, drb_id_t::drb2}},
      std::vector<srb_id_t>{srb_id_t::srb2},
      std::vector<drb_id_t>{drb_id_t::drb1, drb_id_t::drb2});

  // Inject RRC Reconfiguration Complete and await successful PDU Session Resource Setup Response
  send_rrc_reconfiguration_complete_and_await_pdu_session_setup_response(
      make_byte_buffer("00070e00cc6fcda5").value(), {psi}, {});
}

TEST_F(cu_cp_pdu_session_resource_setup_test, when_two_consecutive_setups_arrive_bearer_setup_and_modification_succeed)
{
  // Setup first PDU session
  setup_pdu_session(psi, drb_id_t::drb1, qfi);

  // Setup second PDU session
  setup_pdu_session(psi2, drb_id_t::drb2, qfi2, make_byte_buffer("00080800e6847bbd").value(), false);
}