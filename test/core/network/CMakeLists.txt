#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_subdirectory(types)

addtest(rpc_libp2p_test
    rpc_libp2p_test.cpp
    )
target_link_libraries(rpc_libp2p_test
    scale_message_read_writer
    scale::scale
    outcome
    buffer
    blob
    testutil_read_writer_helper
    p2p::p2p_peer_id
    p2p::p2p_multiaddress
    logger_for_tests
    )

addtest(sync_protocol_observer_test
    sync_protocol_observer_test.cpp
    )
target_link_libraries(sync_protocol_observer_test
    sync_protocol_observer
    polkadot_trie
    logger_for_tests
    )

addtest(synchronizer_test
    synchronizer_test.cpp
    )
target_link_libraries(synchronizer_test
    synchronizer
    logger_for_tests
    sync_protocol
    block_tree_error
    p2p::p2p_basic_scheduler
    p2p::p2p_message_read_writer
    p2p::p2p_peer_id
    p2p::p2p_literals
    )
