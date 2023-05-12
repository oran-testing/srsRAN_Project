/*
 *
 * Copyright 2021-2023 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "lib/scheduler/support/dmrs_helpers.h"
#include "lib/scheduler/support/mcs_tbs_calculator.h"
#include "lib/scheduler/support/sch_pdu_builder.h"
#include "lib/scheduler/support/tbs_calculator.h"
#include "tests/unittests/scheduler/test_utils/config_generators.h"
#include "srsran/ran/pdsch/dlsch_info.h"
#include "srsran/ran/pusch/ulsch_info.h"
#include "srsran/ran/uci/uci_mapping.h"
#include "srsran/support/test_utils.h"
#include <gtest/gtest.h>

using namespace srsran;

///////////////       HELPER FUNCTIONS         ///////////////

// Contains the input test parameters.
struct mcs_test_entry {
  // Outputs.
  unsigned final_mcs;
  unsigned tbs_bytes;
  // Inputs.
  unsigned max_mcs;
  unsigned nof_prbs;
};

///////////////       DL TEST         ///////////////

class dl_mcs_tbs_calculator_test_bench : public ::testing::TestWithParam<mcs_test_entry>
{
public:
  dl_mcs_tbs_calculator_test_bench() :
    cell_cfg(test_helpers::make_default_sched_cell_configuration_request()),
    ue_cell_cfg(cell_cfg, config_helpers::create_default_initial_ue_serving_cell_config()),
    time_resource{0},
    pdsch_cfg(
        get_pdsch_config_f1_0_c_rnti(cell_cfg,
                                     cell_cfg.dl_cfg_common.init_dl_bwp.pdsch_common.pdsch_td_alloc_list[time_resource],
                                     ue_cell_cfg))
  {
  }

protected:
  const cell_configuration    cell_cfg;
  const ue_cell_configuration ue_cell_cfg;
  unsigned                    time_resource;
  pdsch_config_params         pdsch_cfg;
};

TEST_P(dl_mcs_tbs_calculator_test_bench, test_values)
{
  mcs_test_entry test_entry{.max_mcs = GetParam().max_mcs, .nof_prbs = GetParam().nof_prbs};

  // Run test function.
  optional<sch_mcs_tbs> test =
      compute_dl_mcs_tbs(pdsch_cfg, ue_cell_cfg, sch_mcs_index(test_entry.max_mcs), test_entry.nof_prbs);

  ASSERT_TRUE(test.has_value());
  ASSERT_EQ(GetParam().final_mcs, test.value().mcs);
  ASSERT_EQ(GetParam().tbs_bytes, test.value().tbs);
}

INSTANTIATE_TEST_SUITE_P(
    manual_test_for_max_mcs_and_prbs,
    dl_mcs_tbs_calculator_test_bench,
    testing::Values(mcs_test_entry{.final_mcs = 28, .tbs_bytes = 372, .max_mcs = 28, .nof_prbs = 5},
                    mcs_test_entry{.final_mcs = 28, .tbs_bytes = 1505, .max_mcs = 28, .nof_prbs = 20},
                    mcs_test_entry{.final_mcs = 26, .tbs_bytes = 7298, .max_mcs = 26, .nof_prbs = 106},
                    mcs_test_entry{.final_mcs = 23, .tbs_bytes = 177, .max_mcs = 23, .nof_prbs = 3},
                    mcs_test_entry{.final_mcs = 19, .tbs_bytes = 11272, .max_mcs = 19, .nof_prbs = 275},
                    mcs_test_entry{.final_mcs = 13, .tbs_bytes = 816, .max_mcs = 13, .nof_prbs = 32},
                    mcs_test_entry{.final_mcs = 11, .tbs_bytes = 816, .max_mcs = 11, .nof_prbs = 41},
                    mcs_test_entry{.final_mcs = 8, .tbs_bytes = 2817, .max_mcs = 8, .nof_prbs = 179},
                    mcs_test_entry{.final_mcs = 3, .tbs_bytes = 561, .max_mcs = 3, .nof_prbs = 84},
                    mcs_test_entry{.final_mcs = 0, .tbs_bytes = 177, .max_mcs = 0, .nof_prbs = 55},
                    mcs_test_entry{.final_mcs = 0, .tbs_bytes = 15, .max_mcs = 0, .nof_prbs = 5}));

///////////////       UL TEST         ///////////////

class ul_mcs_tbs_prbs_calculator_test_bench : public ::testing::TestWithParam<mcs_test_entry>
{
public:
  ul_mcs_tbs_prbs_calculator_test_bench() :
    cell_cfg(test_helpers::make_default_sched_cell_configuration_request()),
    ue_cell_cfg(cell_cfg, config_helpers::create_default_initial_ue_serving_cell_config()),
    time_resource{0},
    pusch_cfg(get_pusch_config_f0_0_tc_rnti(
        cell_cfg,
        cell_cfg.ul_cfg_common.init_ul_bwp.pusch_cfg_common.value().pusch_td_alloc_list[time_resource]))
  {
  }

protected:
  const cell_configuration    cell_cfg;
  const ue_cell_configuration ue_cell_cfg;
  unsigned                    time_resource;
  pusch_config_params         pusch_cfg;
};

TEST_P(ul_mcs_tbs_prbs_calculator_test_bench, test_values)
{
  mcs_test_entry test_entry{.max_mcs = GetParam().max_mcs, .nof_prbs = GetParam().nof_prbs};

  // Run test function.
  optional<sch_mcs_tbs> test =
      compute_ul_mcs_tbs(pusch_cfg, ue_cell_cfg, sch_mcs_index(test_entry.max_mcs), test_entry.nof_prbs);

  ASSERT_TRUE(test.has_value());
  ASSERT_EQ(GetParam().final_mcs, test.value().mcs);
  ASSERT_EQ(GetParam().tbs_bytes, test.value().tbs);
}

INSTANTIATE_TEST_SUITE_P(
    manual_test_for_max_mcs_and_prbs,
    ul_mcs_tbs_prbs_calculator_test_bench,
    testing::Values(mcs_test_entry{.final_mcs = 27, .tbs_bytes = 437, .max_mcs = 28, .nof_prbs = 5},
                    mcs_test_entry{.final_mcs = 28, .tbs_bytes = 1825, .max_mcs = 28, .nof_prbs = 20},
                    mcs_test_entry{.final_mcs = 26, .tbs_bytes = 8961, .max_mcs = 26, .nof_prbs = 106},
                    mcs_test_entry{.final_mcs = 23, .tbs_bytes = 209, .max_mcs = 23, .nof_prbs = 3},
                    mcs_test_entry{.final_mcs = 19, .tbs_bytes = 13829, .max_mcs = 19, .nof_prbs = 275},
                    mcs_test_entry{.final_mcs = 13, .tbs_bytes = 1008, .max_mcs = 13, .nof_prbs = 32},
                    mcs_test_entry{.final_mcs = 11, .tbs_bytes = 992, .max_mcs = 11, .nof_prbs = 41},
                    mcs_test_entry{.final_mcs = 8, .tbs_bytes = 3457, .max_mcs = 8, .nof_prbs = 179},
                    mcs_test_entry{.final_mcs = 3, .tbs_bytes = 673, .max_mcs = 3, .nof_prbs = 84},
                    mcs_test_entry{.final_mcs = 0, .tbs_bytes = 217, .max_mcs = 0, .nof_prbs = 55},
                    mcs_test_entry{.final_mcs = 0, .tbs_bytes = 19, .max_mcs = 0, .nof_prbs = 5}));