/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_HOST_API_CHILD_STORAGE_EXTENSION_HPP
#define KAGOME_HOST_API_CHILD_STORAGE_EXTENSION_HPP

#include <cstdint>

#include "log/logger.hpp"
#include "runtime/types.hpp"
#include "common/buffer.hpp"

namespace kagome::runtime {
  class MemoryProvider;
  class TrieStorageProvider;
  class Memory;
}  // namespace kagome::runtime

namespace kagome::host_api {
  /**
   * Implements HostApi functions related to storage
   */
  class ChildStorageExtension {
   public:
    ChildStorageExtension(
        std::shared_ptr<runtime::TrieStorageProvider> storage_provider,
        std::shared_ptr<const runtime::MemoryProvider> memory_provider);

    // ---------------------------- Child Storage ----------------------------

    /**
     * @see HostApi::ext_default_child_storage_set_version_1
     */
    void ext_default_child_storage_set_version_1(
        runtime::WasmSpan child_storage_key,
        runtime::WasmSpan key,
        runtime::WasmSpan value);

    /**
     * @see HostApi::ext_default_child_storage_get_version_1
     */
    runtime::WasmSpan ext_default_child_storage_get_version_1(
        runtime::WasmSpan child_storage_key, runtime::WasmSpan key) const;

    /**
     * @see HostApi::ext_default_child_storage_clear_version_1
     */
    void ext_default_child_storage_clear_version_1(
        runtime::WasmSpan child_storage_key, runtime::WasmSpan key);

    /**
     * @see HostApi::ext_default_child_storage_next_key_version_1
     */
    runtime::WasmSpan ext_default_child_storage_next_key_version_1(
        runtime::WasmSpan child_storage_key, runtime::WasmSpan key) const;

    /**
     * @see HostApi::ext_default_child_storage_root_version_1
     */
    runtime::WasmSpan ext_default_child_storage_root_version_1(
        runtime::WasmSpan child_storage_key) const;

   private:
    std::shared_ptr<runtime::TrieStorageProvider> storage_provider_;
    std::shared_ptr<const runtime::MemoryProvider> memory_provider_;
    log::Logger logger_;

    constexpr static auto kDefaultLoggerTag =
        "WASM Runtime [ChildStorageExtension]";

    template <typename R, typename F, typename... Args>
    outcome::result<R> executeOnChildStorage(
      const common::Buffer& child_storage_key, F func, Args&&... args) const;
    
    template <typename... Args>
    auto toBuffer(runtime::Memory& memory, Args&&... spans) const;
  };

}  // namespace kagome::host_api

#endif  // KAGOME_CHILD_STORAGE_HostApiS_HostApi_HPP
