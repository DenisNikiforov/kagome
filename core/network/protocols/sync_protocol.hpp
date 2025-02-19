/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_NETWORK_SYNCPROTOCOL
#define KAGOME_NETWORK_SYNCPROTOCOL

#include "network/protocol_base.hpp"

#include <memory>

#include <libp2p/connection/stream.hpp>
#include <libp2p/host/host.hpp>

#include "application/chain_spec.hpp"
#include "log/logger.hpp"
#include "network/sync_protocol_observer.hpp"

namespace kagome::network {

  using Stream = libp2p::connection::Stream;
  using Protocol = libp2p::peer::Protocol;
  using PeerId = libp2p::peer::PeerId;
  using PeerInfo = libp2p::peer::PeerInfo;

  class SyncProtocol : public virtual ProtocolBase {
   public:
    virtual void request(const PeerId &peer_id,
                         BlocksRequest block_request,
                         std::function<void(outcome::result<BlocksResponse>)>
                             &&response_handler) = 0;
  };

}  // namespace kagome::network

#endif  // KAGOME_NETWORK_SYNCPROTOCOL
