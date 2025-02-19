/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_BUFFER_HPP
#define KAGOME_BUFFER_HPP

#include <string_view>
#include <vector>

#include <fmt/format.h>
#include <boost/container_hash/hash.hpp>
#include <boost/operators.hpp>
#include <gsl/span>

#include "macro/endianness_utils.hpp"
#include "outcome/outcome.hpp"

namespace kagome::common {

  class BufferView;

  /**
   * @brief Class represents arbitrary (including empty) byte buffer.
   */
  class Buffer : public boost::equality_comparable<Buffer>,
                 public boost::equality_comparable<gsl::span<const uint8_t>>,
                 public boost::equality_comparable<std::vector<uint8_t>>,
                 public boost::less_than_comparable<Buffer>,
                 public boost::less_than_comparable<gsl::span<const uint8_t>> {
   public:
    static constexpr bool is_static_collection = false;
    using iterator = std::vector<uint8_t>::iterator;
    using const_iterator = std::vector<uint8_t>::const_iterator;
    using value_type = uint8_t;
    // with this gsl::span can be built from Buffer
    using pointer = typename std::vector<uint8_t>::pointer;
    using const_pointer = typename std::vector<uint8_t>::const_pointer;
    using size_type = typename std::vector<uint8_t>::size_type;

    /**
     * @brief allocates buffer of size={@param size}, filled with {@param byte}
     */
    Buffer(size_t size, uint8_t byte);

    ~Buffer() = default;

    /**
     * @brief lvalue construct buffer from a byte vector
     */
    explicit Buffer(std::vector<uint8_t> v);
    explicit Buffer(gsl::span<const uint8_t> s);

    Buffer(const uint8_t *begin, const uint8_t *end);

    Buffer() = default;
    Buffer(const Buffer &b) = default;
    Buffer(Buffer &&b) noexcept = default;
    Buffer(std::initializer_list<uint8_t> b);

    Buffer &reserve(size_t size);
    Buffer &resize(size_t size);

    Buffer &operator=(const Buffer &other) = default;
    Buffer &operator=(Buffer &&other) noexcept = default;

    Buffer &operator+=(const Buffer &other) noexcept;

    /**
     * @brief Accessor of byte elements given {@param index} in bytearray
     */
    uint8_t operator[](size_t index) const;

    /**
     * @brief Accessor of byte elements given {@param index} in bytearray
     */
    uint8_t &operator[](size_t index);

    /**
     * @brief Lexicographical comparison of two buffers
     */
    bool operator==(const Buffer &b) const noexcept;

    /**
     * @brief Lexicographical comparison of buffer and vector of bytes
     */
    bool operator==(const std::vector<uint8_t> &b) const noexcept;

    /**
     * @brief Lexicographical comparison of buffer and vector of bytes
     */
    bool operator==(gsl::span<const uint8_t> s) const noexcept;

    /**
     * @brief Lexicographical comparison of two buffers
     */
    bool operator<(const Buffer &b) const noexcept;

    bool operator<(gsl::span<const uint8_t> b) const noexcept;

    /**
     * @brief Iterator, which points to begin of this buffer.
     */
    iterator begin();

    /**
     * @brief Iterator, which points to the element next to the last in this
     * buffer.
     */
    iterator end();

    /**
     * @brief Iterator, which points to begin of this buffer.
     */
    const_iterator begin() const;

    /**
     * @brief Iterator, which points to the element next to the last in this
     * buffer.
     */
    const_iterator end() const;

    /**
     * @brief Getter for size of this buffer.
     */
    size_t size() const;

    /**
     * @brief Put a 8-bit {@param n} in this buffer.
     * @return this buffer, suitable for chaining.
     */
    Buffer &putUint8(uint8_t n);

    /**
     * @brief Put a 32-bit {@param n} number in this buffer. Will be serialized
     * as big-endian number.
     * @return this buffer, suitable for chaining.
     */
    Buffer &putUint32(uint32_t n);

    /**
     * @brief Put a 64-bit {@param n} number in this buffer. Will be serialized
     * as big-endian number.
     * @return this buffer, suitable for chaining.
     */
    Buffer &putUint64(uint64_t n);

    /**
     * @brief Put a string into byte buffer
     * @param s arbitrary string
     * @return this buffer, suitable for chaining.
     */
    Buffer &put(std::string_view str);

    /**
     * @brief Put a vector of bytes into byte buffer
     * @param s arbitrary vector of bytes
     * @return this buffer, suitable for chaining.
     */
    Buffer &put(const std::vector<uint8_t> &v);

    /**
     * @brief Put a sequence of bytes into byte buffer
     * @param s arbitrary span of bytes
     * @return this buffer, suitable for chaining.
     */
    Buffer &put(gsl::span<const uint8_t> s);

    /**
     * @brief Put a array of bytes bounded by pointers into byte buffer
     * @param begin pointer to the array start
     *        end pointer to the address after the last element
     * @return this buffer, suitable for chaining.
     */
    Buffer &putBytes(const uint8_t *begin, const uint8_t *end);

    /**
     * @brief Put another buffer content at the end of current one
     * @param buf another buffer
     * @return this buffer suitable for chaining.
     */
    Buffer &putBuffer(const Buffer &buf);

    /**
     * Clear the contents of the Buffer
     */
    void clear();

    /**
     * @brief getter for raw array of bytes
     */
    const uint8_t *data() const;
    uint8_t *data();

    /**
     * @brief getter for vector of bytes
     */
    const std::vector<uint8_t> &asVector() const;
    std::vector<uint8_t> &asVector();

    std::vector<uint8_t> toVector() & {
      return data_;
    }

    std::vector<uint8_t> toVector() && {
      return std::move(data_);
    }

    /**
     * Returns a copy of a part of the buffer
     * Works alike subspan() of gsl::span
     */
    Buffer subbuffer(size_t offset = 0, size_t length = -1) const;

    BufferView view(size_t offset = 0, size_t length = -1) const;

    /**
     * @brief encode bytearray as hex
     * @return hex-encoded string
     */
    std::string toHex() const;

    /**
     * Check if this buffer is empty
     * @return true, if buffer is empty, false otherwise
     */
    bool empty() const;

    /**
     * @brief Construct Buffer from hex string
     * @param hex hex-encoded string
     * @return result containing constructed buffer if input string is
     * hex-encoded string.
     */
    static outcome::result<Buffer> fromHex(std::string_view hex);

    /**
     * @brief return content of bytearray as string
     * @note Does not ensure correct encoding
     * @return string
     */
    std::string toString() const;

    /**
     * @brief return content of bytearray as a string copy data
     * @note Does not ensure correct encoding
     * @return string
     */
    std::string_view asString() const;

    /**
     * @brief stores content of a string to byte array
     */
    static Buffer fromString(const std::string_view &src);

   private:
    std::vector<uint8_t> data_;

    template <typename T>
    Buffer &putRange(const T &begin, const T &end);
  };

  static const Buffer kEmptyBuffer{};

  using BufferMutRef = std::reference_wrapper<Buffer>;
  using BufferConstRef = std::reference_wrapper<const Buffer>;

  class BufferView : public gsl::span<const uint8_t>,
                     public boost::equality_comparable<Buffer>,
                     public boost::less_than_comparable<Buffer> {
   public:
    BufferView() = default;

    BufferView(const Buffer &buf) noexcept
        : gsl::span<const uint8_t>{buf.data(),
                                   static_cast<index_type>(buf.size())} {}

    BufferView(gsl::span<const uint8_t> span) noexcept
        : gsl::span<const uint8_t>{span} {}

    std::string toHex() const;

    BufferView &operator=(gsl::span<const uint8_t> span) noexcept {
      *this = BufferView{span};
      return *this;
    }

    BufferView &operator=(Buffer &&buf) = delete;

    bool operator==(const common::Buffer &buf) const noexcept {
      return std::equal(begin(), end(), buf.begin(), buf.end());
    }

    bool operator<(const common::Buffer &buf) const noexcept {
      return std::lexicographical_compare(
          begin(), end(), buf.begin(), buf.end());
    }
  };

  std::ostream &operator<<(std::ostream &os, const Buffer &buffer);
  std::ostream &operator<<(std::ostream &os, BufferView view);

  namespace literals {
    /// creates a buffer filled with characters from the original string
    /// mind that it does not perform unhexing, there is ""_unhex for it
    inline Buffer operator""_buf(const char *c, size_t s) {
      std::vector<uint8_t> chars(c, c + s);
      return Buffer(std::move(chars));
    }

    constexpr bool is_hex(const char c) {
      return (c >= '0' && c <= '9') || (c >= 'A' && c <= 'F')
             || (c >= 'a' && c <= 'f');
    }

    template <char... cs>
    constexpr bool is_hex_str() {
      return (is_hex(cs) && ...);
    }

    inline Buffer operator""_hex2buf(const char *c, size_t s) {
      /// TODO(GaroRobe): After migrating to C++20 enable static_assert
      /// using literal operator template (see
      /// fe599c601d490b2d73c172a32c9ed1d6d58c8f78) static_assert(is_hex_str(c),
      /// "Expected hex string");
      return Buffer::fromHex({c, s}).value();
    }
  }  // namespace literals

}  // namespace kagome::common

template <>
struct std::hash<kagome::common::Buffer> {
  size_t operator()(const kagome::common::Buffer &x) const {
    return boost::hash_range(x.begin(), x.end());
  }
};

template <>
struct fmt::formatter<kagome::common::Buffer> {
  // Presentation format: 's' - short, 'l' - long.
  char presentation = 's';

  // Parses format specifications of the form ['s' | 'l'].
  constexpr auto parse(format_parse_context &ctx) -> decltype(ctx.begin()) {
    // Parse the presentation format and store it in the formatter:
    auto it = ctx.begin(), end = ctx.end();
    if (it != end && (*it == 's' || *it == 'l')) {
      presentation = *it++;
    }

    // Check if reached the end of the range:
    if (it != end && *it != '}') {
      throw format_error("invalid format");
    }

    // Return an iterator past the end of the parsed range:
    return it;
  }

  // Formats the Blob using the parsed format specification (presentation)
  // stored in this formatter.
  template <typename FormatContext>
  auto format(const kagome::common::Buffer &blob, FormatContext &ctx)
      -> decltype(ctx.out()) {
    // ctx.out() is an output iterator to write to.

    if (blob.empty()) {
      return format_to(ctx.out(), "empty");
    }

    if (presentation == 's' && blob.size() > 5) {
      return format_to(
          ctx.out(),
          "0x{:04x}…{:04x}",
          // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
          htobe16(*reinterpret_cast<const uint16_t *>(blob.data())),
          // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
          htobe16(*reinterpret_cast<const uint16_t *>(blob.data() + blob.size()
                                                      - sizeof(uint16_t))));
    }

    return format_to(ctx.out(), "0x{}", blob.toHex());
  }
};

#endif  // KAGOME_BUFFER_HPP
