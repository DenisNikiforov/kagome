/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_CONSENSUS_GRANDPA_IMPL_VOTING_ROUND_ERROR_HPP
#define KAGOME_CORE_CONSENSUS_GRANDPA_IMPL_VOTING_ROUND_ERROR_HPP

#include <outcome/outcome.hpp>

namespace kagome::consensus::grandpa {

  enum class VotingRoundError {
    INVALID_SIGNATURE = 1,
    REDUNDANT_EQUIVOCATION,
    NOT_ENOUGH_WEIGHT,
    JUSTIFICATION_FOR_ROUND_IN_PAST,
    JUSTIFICATION_FOR_BLOCK_IN_PAST,
    JUSTIFIED_BLOCK_IS_GREATER_THAN_ACTUALLY_FINALIZED,
    NO_KNOWN_AUTHORITIES_FOR_BLOCK,
    LAST_ESTIMATE_BETTER_THAN_PREVOTE,
    UNKNOWN_VOTER,
    ZERO_WEIGHT_VOTER,
    DUPLICATED_VOTE,
    EQUIVOCATED_VOTE,
    VOTE_OF_KNOWN_EQUIVOCATOR,
    NO_PREVOTE_CANDIDATE,
    ROUND_IS_NOT_FINALIZABLE
  };

}

OUTCOME_HPP_DECLARE_ERROR(kagome::consensus::grandpa, VotingRoundError);

#endif  // KAGOME_CORE_CONSENSUS_GRANDPA_IMPL_VOTING_ROUND_ERROR_HPP
