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

#include "rlc_rx_entity.h"
#include "rlc_sdu_queue.h"
#include "rlc_um_pdu.h"
#include "srsgnb/rlc/rlc_metrics.h"
#include "srsgnb/support/timers.h"
#include <map>

namespace srsgnb {

///
/// \brief Rx state variables
/// Ref: 3GPP TS 38.322 version 16.2.0 Section 7.1
///
struct rlc_rx_um_state {
  ///
  /// \brief RX_Next_Reassembly – UM receive state variable
  /// The earliest SN that is still considered for reassembly
  ///
  uint32_t rx_next_reassembly = 0;

  ///
  /// \brief RX_Timer_Trigger - UM t-Reassembly state variable
  /// The SN following the SN which triggered t-Reassembly
  ///
  uint32_t rx_timer_trigger = 0;

  ///
  /// \brief RX_Next_Highest - UM receive state variable
  /// The SN following the SN of the UMD PDU with the highest SN among
  /// received UMD PDUs. It serves as the higher edge of the reassembly window.
  uint32_t rx_next_highest = 0;
};

class rlc_rx_um_entity : public rlc_rx_entity
{
private:
  // Config storage
  const rlc_rx_um_config cfg;

  // Rx state variables
  rlc_rx_um_state st;

  const uint32_t mod; // Rx counter modulus
  const uint32_t um_window_size;

  // Rx window
  struct rlc_umd_pdu_segments {
    std::map<uint32_t, rlc_um_pdu> segments; // Map of segments with SO as key
    byte_buffer                    sdu;
    uint32_t                       next_expected_so;
    uint32_t                       total_sdu_length;
  };
  std::map<uint32_t, rlc_umd_pdu_segments> rx_window;

  void update_total_sdu_length(rlc_umd_pdu_segments& pdu_segments, const rlc_um_pdu& rx_pdu);

  /// \brief t-Reassembly
  /// This timer is used by [...] the receiving side of an UM RLC entity in order to detect loss of RLC PDUs at lower
  /// layer (see sub clauses 5.2.2.2 and 5.2.3.2). If t-Reassembly is running, t-Reassembly shall not be started
  /// additionally, i.e.only one t-Reassembly per RLC entity is running at a given time.
  /// Ref: TS 38.322 Sec. 7.3
  unique_timer reassembly_timer; // to detect loss of RLC PDUs at lower layers

  bool sn_in_reassembly_window(const uint32_t sn);
  bool sn_invalid_for_rx_buffer(const uint32_t sn);
  void handle_rx_buffer_update(const uint32_t sn);
  bool has_missing_byte_segment(const uint32_t sn);

  constexpr uint32_t rx_mod_base(uint32_t x) { return (x - st.rx_next_highest - um_window_size) % mod; }

  void log_state(srslog::basic_levels level)
  {
    logger.log(level,
               "rx_next_reassembly={}, rx_timer_trigger={}, rx_next_highest={}, t_reassembly={}",
               st.rx_next_reassembly,
               st.rx_timer_trigger,
               st.rx_next_highest,
               reassembly_timer.is_running() ? "running" : "stopped");
  }

public:
  rlc_rx_um_entity(du_ue_index_t                     du_index,
                   lcid_t                            lcid,
                   const rlc_rx_um_config&           config,
                   rlc_rx_upper_layer_data_notifier& upper_dn,
                   timer_manager&                    timers);

  void timer_expired(uint32_t timeout_id);

  void handle_pdu(byte_buffer_slice buf) override;
};

} // namespace srsgnb
