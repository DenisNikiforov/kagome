/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_PRIMITIVES_DIGEST
#define KAGOME_CORE_PRIMITIVES_DIGEST

#include <boost/variant.hpp>

#include "common/buffer.hpp"
#include "common/tagged.hpp"
#include "common/unused.hpp"
#include "primitives/scheduled_change.hpp"
#include "scale/scale.hpp"
#include "storage/changes_trie/changes_trie_config.hpp"

namespace kagome::primitives {
  /// Consensus engine unique ID.
  using ConsensusEngineId = common::Blob<4>;

  inline const auto kBabeEngineId =
      ConsensusEngineId::fromString("BABE").value();

  inline const auto kGrandpaEngineId =
      ConsensusEngineId::fromString("FRNK").value();

  inline const auto kUnsupportedEngineId_POL1 =
      ConsensusEngineId::fromString("POL1").value();

  inline const auto kUnsupportedEngineId_BEEF =
      ConsensusEngineId::fromString("BEEF").value();

  /// System digest item that contains the root of changes trie at given
  /// block. It is created for every block iff runtime supports changes
  /// trie creation.
  struct ChangesTrieRoot : public common::Hash256 {};

  // TODO (kamilsa): workaround unless we bump gtest version to 1.8.1+
  // after gtest update use `data` type directly
  struct ChangesTrieSignal {
    boost::variant<std::optional<storage::changes_trie::ChangesTrieConfig>>
        data;

    bool operator==(const ChangesTrieSignal &rhs) const {
      return data == rhs.data;
    }

    bool operator!=(const ChangesTrieSignal &rhs) const {
      return !operator==(rhs);
    }
  };

  template <class Stream,
            typename = std::enable_if_t<Stream::is_encoder_stream>>
  Stream &operator<<(Stream &s, const ChangesTrieSignal &sig) {
    return s << sig.data;
  }

  template <class Stream,
            typename = std::enable_if_t<Stream::is_decoder_stream>>
  Stream &operator>>(Stream &s, ChangesTrieSignal &sig) {
    return s >> sig.data;
  }

  struct Other : public common::Buffer {};

  namespace detail {
    struct DigestItemCommon {
      ConsensusEngineId consensus_engine_id;
      common::Buffer data;

      bool operator==(const DigestItemCommon &rhs) const {
        return consensus_engine_id == rhs.consensus_engine_id
               and data == rhs.data;
      }

      bool operator!=(const DigestItemCommon &rhs) const {
        return !operator==(rhs);
      }
    };
  }  // namespace detail

  /// A pre-runtime digest.
  ///
  /// These are messages from the consensus engine to the runtime, although
  /// the consensus engine can (and should) read them itself to avoid
  /// code and state duplication. It is erroneous for a runtime to produce
  /// these, but this is not (yet) checked.
  struct PreRuntime : public detail::DigestItemCommon {};

  /// https://github.com/paritytech/substrate/blob/polkadot-v0.9.8/primitives/consensus/babe/src/lib.rs#L130
  using BabeDigest =
      /// Note: order of types in variant matters
      boost::variant<Unused<0>,
                     NextEpochData,    // 1: (Auth C; R)
                     OnDisabled,       // 2: Auth ID
                     NextConfigData>;  // 3: c, S2nd

  /// https://github.com/paritytech/substrate/blob/polkadot-v0.9.8/primitives/finality-grandpa/src/lib.rs#L92
  using GrandpaDigest =
      /// Note: order of types in variant matters
      boost::variant<Unused<0>,
                     ScheduledChange,  // 1: (Auth C; N delay)
                     ForcedChange,     // 2: (Auth C; N delay)
                     OnDisabled,       // 3: Auth ID
                     Pause,            // 4: N delay
                     Resume>;          // 5: N delay

  using UnsupportedDigest_POL1 = Tagged<Empty, struct POL1>;
  using UnsupportedDigest_BEEF = Tagged<Empty, struct BEEF>;

  struct DecodedConsensusMessage {
    static outcome::result<DecodedConsensusMessage> create(
        ConsensusEngineId engine_id, common::Buffer const &data) {
      if (engine_id == primitives::kBabeEngineId) {
        OUTCOME_TRY(payload, scale::decode<BabeDigest>(data));
        return DecodedConsensusMessage{engine_id, std::move(payload)};
      } else if (engine_id == primitives::kGrandpaEngineId) {
        OUTCOME_TRY(payload, scale::decode<GrandpaDigest>(data));
        return DecodedConsensusMessage{engine_id, std::move(payload)};
      } else if (engine_id == primitives::kUnsupportedEngineId_POL1) {
        OUTCOME_TRY(payload, scale::decode<UnsupportedDigest_POL1>(data));
        return DecodedConsensusMessage{engine_id, std::move(payload)};
      } else if (engine_id == primitives::kUnsupportedEngineId_BEEF) {
        OUTCOME_TRY(payload, scale::decode<UnsupportedDigest_BEEF>(data));
        return DecodedConsensusMessage{engine_id, std::move(payload)};
      }
      BOOST_ASSERT_MSG(false, "Invalid consensus engine id");
      BOOST_UNREACHABLE_RETURN({})
    }

    const BabeDigest &asBabeDigest() const {
      BOOST_ASSERT(consensus_engine_id == primitives::kBabeEngineId);
      return boost::relaxed_get<BabeDigest>(digest);
    }

    template <typename T>
    bool isBabeDigestOf() const {
      return consensus_engine_id == primitives::kBabeEngineId
             && boost::get<T>(&asBabeDigest()) != nullptr;
    }

    const GrandpaDigest &asGrandpaDigest() const {
      BOOST_ASSERT(consensus_engine_id == primitives::kGrandpaEngineId);
      return boost::relaxed_get<GrandpaDigest>(digest);
    }

    template <typename T>
    bool isGrandpaDigestOf() const {
      return consensus_engine_id == primitives::kGrandpaEngineId
             && boost::get<T>(&asGrandpaDigest()) != nullptr;
    }

    ConsensusEngineId consensus_engine_id;
    boost::variant<BabeDigest,
                   GrandpaDigest,
                   UnsupportedDigest_POL1,
                   UnsupportedDigest_BEEF>
        digest{};

   private:
    DecodedConsensusMessage() = default;
  };

  /// A message from the runtime to the consensus engine. This should *never*
  /// be generated by the native code of any consensus engine, but this is not
  /// checked (yet).
  struct Consensus : public detail::DigestItemCommon {
    Consensus() = default;

    // Note: this ctor is needed only for tests
    template <class A>
    Consensus(const A &a) {
      // clang-format off
      if constexpr (std::is_same_v<A, NextEpochData>
                 or std::is_same_v<A, NextConfigData>) {
        consensus_engine_id = primitives::kBabeEngineId;
        data = common::Buffer(scale::encode(BabeDigest(a)).value());
      } else if constexpr (std::is_same_v<A, ScheduledChange>
                        or std::is_same_v<A, ForcedChange>
                        or std::is_same_v<A, OnDisabled>
                        or std::is_same_v<A, Pause>
                        or std::is_same_v<A, Resume>) {
        consensus_engine_id = primitives::kGrandpaEngineId;
        data = common::Buffer(scale::encode(GrandpaDigest(a)).value());
      } else {
        BOOST_UNREACHABLE_RETURN();
      }
      // clang-format on
    }

    outcome::result<DecodedConsensusMessage> decode() const {
      return DecodedConsensusMessage::create(consensus_engine_id, data);
    }
  };

  /// Put a Seal on it.
  /// This is only used by native code, and is never seen by runtimes.
  struct Seal : public detail::DigestItemCommon {};

  template <class Stream,
            typename = std::enable_if_t<Stream::is_encoder_stream>>
  Stream &operator<<(Stream &s, const detail::DigestItemCommon &dic) {
    return s << dic.consensus_engine_id << dic.data;
  }

  template <class Stream,
            typename = std::enable_if_t<Stream::is_decoder_stream>>
  Stream &operator>>(Stream &s, detail::DigestItemCommon &dic) {
    return s >> dic.consensus_engine_id >> dic.data;
  }

  /// Runtime code or heap pages updated.
  struct RuntimeEnvironmentUpdated : public Empty {};

  /// Digest item that is able to encode/decode 'system' digest items and
  /// provide opaque access to other items.
  /// Note: order of types in variant matters. Should match type ids from here:
  /// https://github.com/paritytech/substrate/blob/polkadot-v0.9.12/primitives/runtime/src/generic/digest.rs#L272
  using DigestItem = boost::variant<Other,                       // 0
                                    Unused<1>,                   // 1
                                    ChangesTrieRoot,             // 2
                                    Unused<3>,                   // 3
                                    Consensus,                   // 4
                                    Seal,                        // 5
                                    PreRuntime,                  // 6
                                    ChangesTrieSignal,           // 7
                                    RuntimeEnvironmentUpdated>;  // 8

  /**
   * Digest is an implementation- and usage-defined entity, for example,
   * information, needed to verify the block
   */
  using Digest = std::vector<DigestItem>;
}  // namespace kagome::primitives

#endif  // KAGOME_CORE_PRIMITIVES_DIGEST
