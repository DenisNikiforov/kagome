/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_RUNTIME_PTR_SIZE_HPP
#define KAGOME_CORE_RUNTIME_PTR_SIZE_HPP

#include "runtime/types.hpp"

#include <tuple>

namespace kagome::runtime {
  /**
   * Result of a call to a Runtime API wasm function is an i64 where first 32
   * bits are the address and next 32 bits are the size of the returned buffer.
   */
  struct PtrSize {
    constexpr PtrSize() : ptr{0}, size{0} {}

    explicit constexpr PtrSize(WasmSpan v) {
      std::tie(ptr, size) = splitSpan(v);
    }

    constexpr PtrSize(WasmPointer ptr, WasmSize size) : ptr{ptr}, size{size} {}

    /**
     * @brief makes combined pointer-size result from address
     * @return pointer-size uint64_t value
     */
    constexpr WasmSpan combine() const {
      return static_cast<WasmSpan>(ptr)
             | (static_cast<WasmSpan>(size) << 32ull);
    }

    bool operator==(const PtrSize &rhs) const {
      return ptr == rhs.ptr and size == rhs.size;
    }

    WasmPointer ptr = 0u;  ///< address of buffer
    WasmSize size = 0u;    ///< length of buffer
  };

}  // namespace kagome::runtime

#endif  // KAGOME_CORE_RUNTIME_PTR_SIZE_HPP
