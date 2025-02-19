/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "network/impl/protocols/sync_protocol_impl.hpp"

#include "common/visitor.hpp"
#include "network/adapters/protobuf_block_request.hpp"
#include "network/adapters/protobuf_block_response.hpp"
#include "network/common.hpp"
#include "network/helpers/peer_id_formatter.hpp"
#include "network/helpers/protobuf_message_read_writer.hpp"
#include "network/impl/protocols/protocol_error.hpp"
#include "network/rpc.hpp"
#include "network/types/blocks_request.hpp"
#include "network/types/blocks_response.hpp"

namespace kagome::network {

  namespace detail {
    BlocksResponseCache::BlocksResponseCache(
        std::size_t capacity, std::chrono::seconds expiration_time)
        : capacity_{capacity}, expiration_time_{expiration_time} {
      // resize to preallocate elements
      storage_.resize(capacity_);
      // preallocate internal storage of containers
      lookup_table_.reserve(capacity_);
      free_slots_.reserve(capacity_);
      for (std::size_t i = 0; i < capacity_; ++i) {
        free_slots_.emplace(i);
      }
    }

    bool BlocksResponseCache::isDuplicate(
        const PeerId &peer_id, BlocksRequest::Fingerprint request_fingerprint) {
      auto found = lookup_table_.find(peer_id);

      // the peer is not cached yet
      if (found == lookup_table_.end()) {
        cache(peer_id, request_fingerprint);
        return false;
      }

      // the peer was previously cached
      auto slot = found->second;
      auto &entry = storage_[slot];
      auto now = std::chrono::system_clock::now();
      if (not storage_[slot]          // dangling reference was found
          or now > entry->valid_till  // the record was expired
      ) {
        cache(peer_id, request_fingerprint, slot);
        return false;
      }

      // peer record in cache is valid and not expired
      entry->valid_till = now + expiration_time_;  // prolong expiry time
      auto &requests = entry->fingerprints;
      if (std::find(requests.begin(), requests.end(), request_fingerprint)
          != requests.end()) {
        return true;
      }
      requests.push_back(request_fingerprint);
      return false;
    }

    void BlocksResponseCache::cache(
        const PeerId &peer_id,
        BlocksRequest::Fingerprint request_fingerprint,
        std::optional<CacheRecordIndex> target_slot) {
      CacheRecordIndex slot;
      if (target_slot) {
        slot = *target_slot;
      } else {
        if (free_slots_.empty()) {
          purge();
        }
        if (free_slots_.empty()) {
          return;
        }
        auto free_slot = free_slots_.begin();
        slot = *free_slot;
        free_slots_.erase(free_slot);
      }

      boost::circular_buffer<BlocksRequest::Fingerprint> fingerprints(
          kMaxCacheEntriesPerPeer);
      if (target_slot and storage_[*target_slot]) {
        fingerprints = std::move(storage_[*target_slot]->fingerprints);
      }
      fingerprints.push_back(request_fingerprint);

      CacheRecord record{
          .peer_id = peer_id,
          .valid_till = std::chrono::system_clock::now() + expiration_time_,
          .fingerprints = std::move(fingerprints),
      };

      storage_[slot] = std::move(record);
      lookup_table_[peer_id] = slot;
    }

    void BlocksResponseCache::purge() {
      auto it = lookup_table_.begin();
      while (it != lookup_table_.end()) {
        auto slot = it->second;
        // remove dangling reference from lookup table
        if (not storage_[slot]) {
          free_slots_.insert(slot);
          it = lookup_table_.erase(it);  // points to the following element
          continue;
        }
        auto &record = storage_[slot].value();
        // remove expired entries
        if (std::chrono::system_clock::now() > record.valid_till) {
          storage_[slot] = std::nullopt;
          free_slots_.insert(slot);
          it = lookup_table_.erase(it);  // points to the following element
          continue;
        }
        it++;
      }
    }

  }  // namespace detail

  SyncProtocolImpl::SyncProtocolImpl(
      libp2p::Host &host,
      const application::ChainSpec &chain_spec,
      std::shared_ptr<SyncProtocolObserver> sync_observer,
      std::shared_ptr<PeerRatingRepository> rating_repository)
      : host_(host),
        sync_observer_(std::move(sync_observer)),
        rating_repository_(std::move(rating_repository)),
        response_cache_(kResponsesCacheCapacity,
                        kResponsesCacheExpirationTimeout) {
    BOOST_ASSERT(sync_observer_ != nullptr);
    BOOST_ASSERT(rating_repository_ != nullptr);
    const_cast<Protocol &>(protocol_) =
        fmt::format(kSyncProtocol.data(), chain_spec.protocolId());
  }

  bool SyncProtocolImpl::start() {
    host_.setProtocolHandler(protocol_, [wp = weak_from_this()](auto &&stream) {
      if (auto self = wp.lock()) {
        if (auto peer_id = stream->remotePeerId()) {
          SL_TRACE(self->log_,
                   "Handled {} protocol stream from {:l}",
                   self->protocol_,
                   peer_id.value());
          self->onIncomingStream(std::forward<decltype(stream)>(stream));
          return;
        }
        self->log_->warn("Handled {} protocol stream from unknown peer",
                         self->protocol_);
      }
    });
    return true;
  }

  bool SyncProtocolImpl::stop() {
    return true;
  }

  void SyncProtocolImpl::onIncomingStream(std::shared_ptr<Stream> stream) {
    BOOST_ASSERT(stream->remotePeerId().has_value());

    readRequest(stream);
  }

  void SyncProtocolImpl::newOutgoingStream(
      const PeerInfo &peer_info,
      std::function<void(outcome::result<std::shared_ptr<Stream>>)> &&cb) {
    SL_DEBUG(log_, "Connect for {} stream with {}", protocol_, peer_info.id);

    host_.newStream(
        peer_info.id,
        protocol_,
        [wp = weak_from_this(), peer_id = peer_info.id, cb = std::move(cb)](
            auto &&stream_res) mutable {
          auto self = wp.lock();
          if (not self) {
            cb(ProtocolError::GONE);
            return;
          }

          if (not stream_res.has_value()) {
            SL_VERBOSE(
                self->log_,
                "Error happened while connection over {} stream with {}: {}",
                self->protocol_,
                peer_id,
                stream_res.error().message());
            cb(stream_res.as_failure());
            return;
          }
          auto &stream = stream_res.value();

          SL_DEBUG(self->log_,
                   "Established connection over {} stream with {}",
                   self->protocol_,
                   peer_id);

          cb(std::move(stream));
        });
  }

  void SyncProtocolImpl::readRequest(std::shared_ptr<Stream> stream) {
    auto read_writer = std::make_shared<ProtobufMessageReadWriter>(stream);

    SL_DEBUG(log_,
             "Read request from incoming {} stream with {}",
             protocol_,
             stream->remotePeerId().value());

    read_writer->read<BlocksRequest>([stream, wp = weak_from_this()](
                                         auto &&block_request_res) mutable {
      auto self = wp.lock();
      if (not self) {
        stream->reset();
        return;
      }

      if (not block_request_res.has_value()) {
        SL_VERBOSE(self->log_,
                   "Error at read request from incoming {} stream with {}: {}",
                   self->protocol_,
                   stream->remotePeerId().value(),
                   block_request_res.error().message());

        stream->reset();
        return;
      }
      auto &block_request = block_request_res.value();

      if (self->log_->level() >= log::Level::VERBOSE) {
        std::string logmsg = fmt::format(
            "Block request is received from incoming {} stream with {}",
            self->protocol_,
            stream->remotePeerId().value());

        logmsg += ", fields=";
        if (block_request.fields & BlockAttribute::HEADER) logmsg += 'H';
        if (block_request.fields & BlockAttribute::BODY) logmsg += 'B';
        if (block_request.fields & BlockAttribute::RECEIPT) logmsg += 'R';
        if (block_request.fields & BlockAttribute::MESSAGE_QUEUE) logmsg += 'M';
        if (block_request.fields & BlockAttribute::JUSTIFICATION) logmsg += 'J';

        visit_in_place(block_request.from, [&](const auto &from) {
          logmsg += fmt::format(", from {}", from);
        });

        if (block_request.to.has_value()) {
          logmsg += fmt::format(" to {}", block_request.to.value());
        }

        logmsg +=
            block_request.direction == Direction::ASCENDING ? " anc" : " desc";

        if (block_request.max.has_value()) {
          logmsg += fmt::format(", max {}", block_request.max.value());
        }

        self->log_->verbose(std::move(logmsg));
      }

      auto block_response_res =
          self->sync_observer_->onBlocksRequest(block_request);

      if (not block_response_res) {
        SL_VERBOSE(
            self->log_,
            "Error at execute request from incoming {} stream with {}: {}",
            self->protocol_,
            stream->remotePeerId().value(),
            block_response_res.error().message());

        stream->reset();
        return;
      }
      auto &block_response = block_response_res.value();

      if ((not block_response.blocks.empty()) and stream->remotePeerId()
          and self->response_cache_.isDuplicate(stream->remotePeerId().value(),
                                                block_request.fingerprint())) {
        auto peer_id = stream->remotePeerId().value();
        SL_DEBUG(self->log_,
                 "Stream {} to {} reset due to repeating non-polite block "
                 "request with fingerprint {}",
                 self->protocol_,
                 peer_id,
                 block_request.fingerprint());
        self->rating_repository_->downvoteForATime(
            peer_id, kResponsesCacheExpirationTimeout);
        stream->reset();
        return;
      }

      self->writeResponse(std::move(stream), block_response);
    });
  }

  void SyncProtocolImpl::writeResponse(std::shared_ptr<Stream> stream,
                                       const BlocksResponse &block_response) {
    auto read_writer = std::make_shared<ProtobufMessageReadWriter>(stream);

    read_writer->write(
        block_response,
        [stream = std::move(stream),
         wp = weak_from_this()](auto &&write_res) mutable {
          auto self = wp.lock();
          if (not self) {
            stream->reset();
            return;
          }

          if (not write_res.has_value()) {
            SL_VERBOSE(
                self->log_,
                "Error at writing response to incoming {} stream with {}: {}",
                self->protocol_,
                stream->remotePeerId().value(),
                write_res.error().message());
            stream->reset();
            return;
          }

          stream->close([](auto &&...) {});
        });
  }

  void SyncProtocolImpl::writeRequest(
      std::shared_ptr<Stream> stream,
      BlocksRequest block_request,
      std::function<void(outcome::result<void>)> &&cb) {
    auto read_writer = std::make_shared<ProtobufMessageReadWriter>(stream);

    SL_DEBUG(log_,
             "Write request info outgoing {} stream with {}",
             protocol_,
             stream->remotePeerId().value());

    read_writer->write(
        block_request,
        [stream, wp = weak_from_this(), cb = std::move(cb)](
            auto &&write_res) mutable {
          auto self = wp.lock();
          if (not self) {
            stream->reset();
            cb(ProtocolError::GONE);
            return;
          }

          if (not write_res.has_value()) {
            SL_VERBOSE(
                self->log_,
                "Error at write request into outgoing {} stream with {}: {}",
                self->protocol_,
                stream->remotePeerId().value(),
                write_res.error().message());

            stream->reset();
            cb(write_res.as_failure());
            return;
          }

          SL_DEBUG(self->log_,
                   "Request written successful into outgoing {} stream with {}",
                   self->protocol_,
                   stream->remotePeerId().value());

          cb(outcome::success());
        });
  }

  void SyncProtocolImpl::readResponse(
      std::shared_ptr<Stream> stream,
      std::function<void(outcome::result<BlocksResponse>)> &&response_handler) {
    auto read_writer = std::make_shared<ProtobufMessageReadWriter>(stream);

    SL_DEBUG(log_,
             "Read response from outgoing {} stream with {}",
             protocol_,
             stream->remotePeerId().value());

    read_writer->read<BlocksResponse>([stream,
                                       wp = weak_from_this(),
                                       response_handler =
                                           std::move(response_handler)](
                                          auto &&block_response_res) mutable {
      auto self = wp.lock();
      if (not self) {
        stream->reset();
        response_handler(ProtocolError::GONE);
        return;
      }

      if (not block_response_res.has_value()) {
        SL_VERBOSE(self->log_,
                   "Error at read response from outgoing {} stream with {}: {}",
                   self->protocol_,
                   stream->remotePeerId().value(),
                   block_response_res.error().message());

        stream->reset();
        response_handler(block_response_res.as_failure());
        return;
      }
      auto &blocks_response = block_response_res.value();

      SL_DEBUG(self->log_,
               "Successful response read from outgoing {} stream with {}",
               self->protocol_,
               stream->remotePeerId().value());

      stream->reset();
      response_handler(std::move(blocks_response));
    });
  }

  void SyncProtocolImpl::request(
      const PeerId &peer_id,
      BlocksRequest block_request,
      std::function<void(outcome::result<BlocksResponse>)> &&response_handler) {
    auto addresses_res =
        host_.getPeerRepository().getAddressRepository().getAddresses(peer_id);
    if (not addresses_res.has_value()) {
      response_handler(addresses_res.as_failure());
      return;
    }

    if (log_->level() >= log::Level::DEBUG) {
      std::string logmsg = "Requesting blocks: fields=";

      if (block_request.fields & BlockAttribute::HEADER) logmsg += 'H';
      if (block_request.fields & BlockAttribute::BODY) logmsg += "B";
      if (block_request.fields & BlockAttribute::RECEIPT) logmsg += "R";
      if (block_request.fields & BlockAttribute::MESSAGE_QUEUE) logmsg += "M";
      if (block_request.fields & BlockAttribute::JUSTIFICATION) logmsg += "J";

      visit_in_place(block_request.from, [&](const auto &from) {
        logmsg += fmt::format(" from {}", from);
      });

      if (block_request.to.has_value()) {
        logmsg += fmt::format(" to {}", block_request.to.value());
      }

      logmsg +=
          block_request.direction == Direction::ASCENDING ? " anc" : " desc";

      if (block_request.max.has_value()) {
        logmsg += fmt::format(", max {}", block_request.max.value());
      }

      log_->debug(std::move(logmsg));
    }

    newOutgoingStream(
        {peer_id, addresses_res.value()},
        [wp = weak_from_this(),
         response_handler = std::move(response_handler),
         block_request = std::move(block_request)](auto &&stream_res) mutable {
          if (not stream_res.has_value()) {
            response_handler(stream_res.as_failure());
            return;
          }
          auto &stream = stream_res.value();

          auto self = wp.lock();
          if (not self) {
            stream->reset();
            response_handler(ProtocolError::GONE);
            return;
          }

          SL_DEBUG(self->log_,
                   "Established outgoing {} stream with {}",
                   self->protocol_,
                   stream->remotePeerId().value());

          self->writeRequest(stream,
                             std::move(block_request),
                             [stream,
                              wp = std::move(wp),
                              response_handler = std::move(response_handler)](
                                 auto &&write_res) mutable {
                               auto self = wp.lock();
                               if (not self) {
                                 stream->reset();
                                 response_handler(ProtocolError::GONE);
                                 return;
                               }

                               if (not write_res.has_value()) {
                                 stream->reset();
                                 response_handler(write_res.as_failure());
                                 return;
                               }

                               self->readResponse(std::move(stream),
                                                  std::move(response_handler));
                             });
        });
  }

}  // namespace kagome::network
