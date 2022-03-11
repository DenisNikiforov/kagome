/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_CONSENSUS_GRANDPA_GRANDPA
#define KAGOME_CORE_CONSENSUS_GRANDPA_GRANDPA

#include "consensus/grandpa/grandpa_observer.hpp"

namespace kagome::consensus::grandpa {

  /**
   * Interface for launching new grandpa rounds
   *
   */
  class Grandpa {
   public:
    virtual ~Grandpa() = default;

    /**
     * Tries to execute the next round
     * @param round_number new round number
     */
    virtual void executeNextRound(RoundNumber round_number) = 0;

    /**
     * Force update for the round. Next round to the provided one will be
     * checked and updated to the new prevote ghost (if any), round estimate (if
     * any), finalized block (if any) and completability
     * @param round_number the round number the following to which is updated
     */
    virtual void updateNextRound(RoundNumber round_number) = 0;
  };

}  // namespace kagome::consensus::grandpa

#endif  // KAGOME_CORE_CONSENSUS_GRANDPA_GRANDPA
