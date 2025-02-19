/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/runtime_api/impl/core.hpp"

#include "blockchain/block_header_repository.hpp"
#include "log/logger.hpp"
#include "runtime/executor.hpp"

namespace kagome::runtime {

  CoreImpl::CoreImpl(
      std::shared_ptr<Executor> executor,
      std::shared_ptr<storage::changes_trie::ChangesTracker> changes_tracker,
      std::shared_ptr<const blockchain::BlockHeaderRepository> header_repo)
      : executor_{std::move(executor)},
        changes_tracker_{std::move(changes_tracker)},
        header_repo_{std::move(header_repo)} {
    BOOST_ASSERT(executor_ != nullptr);
    BOOST_ASSERT(changes_tracker_ != nullptr);
    BOOST_ASSERT(header_repo_ != nullptr);
  }

  outcome::result<primitives::Version> CoreImpl::version(
      primitives::BlockHash const &block) {
    return executor_->callAt<primitives::Version>(block, "Core_version");
  }

  outcome::result<primitives::Version> CoreImpl::version() {
    return executor_->callAtGenesis<primitives::Version>("Core_version");
  }

  outcome::result<void> CoreImpl::execute_block(
      const primitives::Block &block) {
    OUTCOME_TRY(parent, header_repo_->getBlockHeader(block.header.parent_hash));
    BOOST_ASSERT(parent.number == block.header.number - 1);
    OUTCOME_TRY(changes_tracker_->onBlockExecutionStart(
        block.header.parent_hash, parent.number));
    OUTCOME_TRY(executor_->persistentCallAt<void>(
        block.header.parent_hash, "Core_execute_block", block));
    return outcome::success();
  }

  outcome::result<storage::trie::RootHash> CoreImpl::initialize_block(
      const primitives::BlockHeader &header) {
    OUTCOME_TRY(changes_tracker_->onBlockExecutionStart(
        header.parent_hash,
        header.number - 1));  // parent's number
    const auto res = executor_->persistentCallAt<void>(
        header.parent_hash, "Core_initialize_block", header);
    if (res.has_value()) {
      return res.value().new_storage_root;
    }
    return res.error();
  }

  outcome::result<std::vector<primitives::AuthorityId>> CoreImpl::authorities(
      const primitives::BlockHash &block_hash) {
    return executor_->callAt<std::vector<primitives::AuthorityId>>(
        block_hash, "Core_authorities", block_hash);
  }

}  // namespace kagome::runtime
