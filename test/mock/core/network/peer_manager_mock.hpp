/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_NETWORK_PEERMANAGERMOCK
#define KAGOME_NETWORK_PEERMANAGERMOCK

#include "network/peer_manager.hpp"

#include <gmock/gmock.h>

namespace kagome::network {

  class PeerManagerMock final : public PeerManager {
   public:
    MOCK_METHOD(void, connectToPeer, (const PeerInfo &), (override));

    MOCK_METHOD(void, reserveStreams, (const PeerId &), (const, override));

    MOCK_METHOD(void, keepAlive, (const PeerId &), (override));

    MOCK_METHOD(void, startPingingPeer, (const PeerId &), (override));

    MOCK_METHOD(void,
                updatePeerState,
                (const PeerId &, const Status &),
                (override));

    MOCK_METHOD(void,
                updatePeerState,
                (const PeerId &, const BlockAnnounce &),
                (override));

    MOCK_METHOD(void,
                updatePeerState,
                (const PeerId &, const GrandpaNeighborMessage &),
                (override));

    MOCK_METHOD(std::optional<PeerState>,
                getPeerState,
                (const PeerId &),
                (override));

    MOCK_METHOD(size_t, activePeersNumber, (), (const, override));

    MOCK_METHOD(void,
                forEachPeer,
                (std::function<void(const PeerId &)>),
                (const, override));

    MOCK_METHOD(void,
                forOnePeer,
                (const PeerId &, std::function<void(const PeerId &)>),
                (const, override));
  };

}  // namespace kagome::network

#endif  // KAGOME_NETWORK_PEERMANAGERMOCK
