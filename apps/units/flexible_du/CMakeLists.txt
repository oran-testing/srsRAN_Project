#
# Copyright 2021-2024 Software Radio Systems Limited
#
# This file is part of srsRAN
#
# srsRAN is free software: you can redistribute it and/or modify
# it under the terms of the GNU Affero General Public License as
# published by the Free Software Foundation, either version 3 of
# the License, or (at your option) any later version.
#
# srsRAN is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU Affero General Public License for more details.
#
# A copy of the GNU Affero General Public License can be found in
# the LICENSE file in the top-level directory of this distribution
# and at http://www.gnu.org/licenses/.
#

add_subdirectory(du_high)
add_subdirectory(fapi)
add_subdirectory(support)
if (DU_SPLIT_6)
    add_subdirectory(split_6)
endif ()
if (DU_SPLIT_7_2)
    add_subdirectory(du_low)
    add_subdirectory(split_7_2)
endif ()
if (DU_SPLIT_8)
    add_subdirectory(du_low)
    add_subdirectory(split_8)
endif ()
if (DU_SPLIT_DYNAMIC)
    add_subdirectory(du_low)
    add_subdirectory(split_7_2)
    add_subdirectory(split_8)
    add_subdirectory(split_dynamic)
endif ()

