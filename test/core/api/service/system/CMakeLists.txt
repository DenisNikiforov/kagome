#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

addtest(system_api_test
    system_api_test.cpp
    )
target_link_libraries(system_api_test
    system_api_service
    )