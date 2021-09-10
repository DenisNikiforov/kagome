/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/wasmedge/module_factory_impl.hpp"

#include "host_api/host_api_factory.hpp"
#include "runtime/wasmedge/memory_provider.hpp"
#include "runtime/wasmedge/core_api_factory_impl.hpp"
#include "runtime/wasmedge/instance_environment_factory.hpp"
#include "runtime/wasmedge/module_impl.hpp"
#include "runtime/common/trie_storage_provider_impl.hpp"

namespace kagome::runtime::wasmedge {

  ModuleFactoryImpl::ModuleFactoryImpl(
      std::shared_ptr<InstanceEnvironmentFactory> env_factory,
      std::shared_ptr<storage::trie::TrieStorage> storage)
      : env_factory_{std::move(env_factory)}, storage_{std::move(storage)} {
    BOOST_ASSERT(env_factory_ != nullptr);
    BOOST_ASSERT(storage_ != nullptr);
  }

  outcome::result<std::unique_ptr<Module>> ModuleFactoryImpl::make(
      storage::trie::RootHash const &state,
      gsl::span<const uint8_t> code) const {
    std::vector<uint8_t> code_vec{code.begin(), code.end()};
    auto res = ModuleImpl::createFromCode(code_vec, env_factory_);
    if (res.has_value()) {
      return std::unique_ptr<Module>(std::move(res.value()));
    }
    return res.error();
  }

}  // namespace kagome::runtime::wasmedge
