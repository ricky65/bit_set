#pragma once

//          Copyright Rein Halbersma 2014-2020.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <algorithm>            // all_of, copy_backward, copy_n, equal, fill_n, for_each, lexicographical_compare, max, none_of, swap_ranges
#include <cassert>              // assert
#include <compare>              // strong_ordering
#include <concepts>             // constructible_from, unsigned_integral
#include <cstddef>              // ptrdiff_t, size_t
#include <cstdint>              // uint64_t
#include <functional>           // less
#include <initializer_list>     // initializer_list
#include <iosfwd>               // basic_ostream
#include <iterator>             // bidirectional_iterator_tag, begin, end, next, prev, rbegin, rend, reverse_iterator
#include <limits>               // digits
#include <numeric>              // accumulate
#include <tuple>                // tie
#include <type_traits>          // common_type_t, is_class_v, make_signed_t
#include <utility>              // forward, pair, swap

// http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2017/p0202r3.html
#define XSTD_PP_CONSTEXPR_ALGORITHM     /* constexpr */

// http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2018/p0879r0.html
#define XSTD_PP_CONSTEXPR_SWAP          /* constexpr */

namespace xstd {
namespace detail {

#if __cpp_lib_three_way_comparison >= 201711L

template< class InputIt1, class InputIt2>
constexpr auto lexicographical_compare_three_way(InputIt1 first1, InputIt1 last1, InputIt2 first2, InputIt2 last2)
{
        return std::lexicographical_compare_three_way(first1, last1, first2, last2);
}

#else

template< class InputIt1, class InputIt2>
constexpr auto lexicographical_compare_three_way(InputIt1 first1, InputIt1 last1, InputIt2 first2, InputIt2 last2)
{
        for (/* no initalization */; first1 != last1 && first2 != last2; void(++first1), void(++first2) ) {
                if (auto cmp = *first1 <=> *first2; cmp != 0) {
                        return cmp;
                }
        }
        return
                first1 != last1 ? std::strong_ordering::greater :
                first2 != last2 ? std::strong_ordering::less    :
                                  std::strong_ordering::equal
        ;
}

#endif

}       // namespace detail

#if defined(__GNUG__)

#define DO_PRAGMA(X) _Pragma(#X)
#define PRAGMA_GCC_DIAGNOSTIC_PUSH_IGNORED(X)   \
        _Pragma("GCC diagnostic push")          \
        DO_PRAGMA(GCC diagnostic ignored X)

#define PRAGMA_GCC_DIAGNOSTIC_POP               \
        _Pragma("GCC diagnostic pop")

#else

#define PRAGMA_GCC_DIAGNOSTIC_PUSH_IGNORED(X)
#define PRAGMA_GCC_DIAGNOSTIC_POP

#endif

#if defined(_MSC_VER)

#define PRAGMA_VC_WARNING_PUSH_DISABLE(X)       \
        __pragma(warning(push))                 \
        __pragma(warning(disable: X))

#define PRAGMA_VC_WARNING_POP                   \
        __pragma(warning(pop))

#else

#define PRAGMA_VC_WARNING_PUSH_DISABLE(X)
#define PRAGMA_VC_WARNING_POP

#endif

#if defined(__GNUG__)

#define XSTD_PP_CONSTEXPR_INTRINSIC     true

#elif defined(_MSC_VER)

#define XSTD_PP_CONSTEXPR_INTRINSIC     false

#endif

#if XSTD_PP_CONSTEXPR_INTRINSIC

#define XSTD_PP_CONSTEXPR_INTRINSIC_FUN constexpr
#define XSTD_PP_CONSTEXPR_INTRINSIC_MEM constexpr
#define XSTD_PP_CONSTEXPR_INTRINSIC_VAR constexpr

#else

#define XSTD_PP_CONSTEXPR_INTRINSIC_FUN /* constexpr */
#define XSTD_PP_CONSTEXPR_INTRINSIC_MEM inline const
#define XSTD_PP_CONSTEXPR_INTRINSIC_VAR const

#endif

namespace builtin {

#if defined(__GNUG__)

template<std::size_t N>
constexpr auto get(__uint128_t x) noexcept
{
        static_assert(0 <= N && N < 2);
        return static_cast<uint64_t>(x >> (64 * N));
}

template<std::unsigned_integral T>
constexpr auto ctznz(T x) // Throws: Nothing.
{
        assert(x != 0);
        if constexpr (sizeof(T) < sizeof(unsigned)) {
                return __builtin_ctz(static_cast<unsigned>(x));
        } else if constexpr (sizeof(T) == sizeof(unsigned)) {
                return __builtin_ctz(x);
        } else if constexpr (sizeof(T) == sizeof(unsigned long long)) {
                return __builtin_ctzll(x);
        } else if constexpr (sizeof(T) == 2 * sizeof(unsigned long long)) {
                return get<0>(x) != 0 ? __builtin_ctzll(get<0>(x)) : __builtin_ctzll(get<1>(x)) + 64;
        }
}

template<std::unsigned_integral T>
constexpr auto bsfnz(T x) // Throws: Nothing.
{
        assert(x != 0);
        return ctznz(x);
}

template<std::unsigned_integral T>
constexpr auto clznz(T x) // Throws: Nothing.
{
        assert(x != 0);
        if constexpr (sizeof(T) < sizeof(unsigned)) {
                constexpr auto padded_zeros = std::numeric_limits<unsigned>::digits - std::numeric_limits<T>::digits;
                return __builtin_clz(static_cast<unsigned>(x)) - padded_zeros;
        } else if constexpr (sizeof(T) == sizeof(unsigned)) {
                return __builtin_clz(x);
        } else if constexpr (sizeof(T) == sizeof(unsigned long long)) {
                return __builtin_clzll(x);
        } else if constexpr (sizeof(T) == 2 * sizeof(unsigned long long)) {
                return get<1>(x) != 0 ? __builtin_clzll(get<1>(x)) : __builtin_clzll(get<0>(x)) + 64;
        }
}

template<std::unsigned_integral T>
constexpr auto bsrnz(T x) // Throws: Nothing.
{
        assert(x != 0);
        return std::numeric_limits<T>::digits - 1 - clznz(x);
}

template<std::unsigned_integral T>
constexpr auto popcount(T x) noexcept
{
        if constexpr (sizeof(T) < sizeof(unsigned)) {
                return __builtin_popcount(static_cast<unsigned>(x));
        } else if constexpr (sizeof(T) == sizeof(unsigned)) {
                return __builtin_popcount(x);
        } else if constexpr (sizeof(T) == sizeof(unsigned long long)) {
                return __builtin_popcountll(x);
        } else if constexpr (sizeof(T) == 2 * sizeof(unsigned long long)) {
                return __builtin_popcountll(get<0>(x)) + __builtin_popcountll(get<1>(x));
        }
}

#elif defined(_MSC_VER)

#include <intrin.h>

#pragma intrinsic(_BitScanForward)
#pragma intrinsic(_BitScanReverse)
#pragma intrinsic(__popcnt)

#if defined(_WIN64)

#pragma intrinsic(_BitScanForward64)
#pragma intrinsic(_BitScanReverse64)
#pragma intrinsic(__popcnt64)

#endif

template<std::unsigned_integral T>
auto bsfnz(T x) // Throws: Nothing.
{
        assert(x != 0);
        unsigned long index;
        if constexpr (sizeof(T) < sizeof(unsigned long)) {
                _BitScanForward(&index, static_cast<unsigned long>(x));
        } else if constexpr (sizeof(T) == sizeof(unsigned long)) {
                _BitScanForward(&index, x);
        } else if constexpr (sizeof(T) == sizeof(uint64_t)) {
                _BitScanForward64(&index, x);
        }
        return static_cast<int>(index);
}

template<std::unsigned_integral T>
auto ctznz(T x) // Throws: Nothing.
{
        return bsfnz(x);
}

template<std::unsigned_integral T>
auto bsrnz(T x) // Throws: Nothing.
{
        assert(x != 0);
        unsigned long index;
        if constexpr (sizeof(T) < sizeof(unsigned long)) {
                _BitScanReverse(&index, static_cast<unsigned long>(x));
        } else if constexpr (sizeof(T) == sizeof(unsigned long)) {
                _BitScanReverse(&index, x);
        } else if constexpr (sizeof(T) == sizeof(uint64_t)) {
                _BitScanReverse64(&index, x);
        }
        return static_cast<int>(index);
}

template<std::unsigned_integral T>
auto clznz(T x) // Throws: Nothing.
{
        assert(x != 0);
        return std::numeric_limits<T>::digits - 1 - bsrnz(x);
}

template<std::unsigned_integral T>
auto popcount(T x) noexcept
{
        if constexpr (sizeof(T) < sizeof(unsigned short)) {
                return static_cast<int>(__popcnt16(static_cast<unsigned short>(x)));
        } else if constexpr (sizeof(T) == sizeof(unsigned short)) {
                return static_cast<int>(__popcnt16(x));
        } else if constexpr (sizeof(T) == sizeof(unsigned)) {
                return static_cast<int>(__popcnt(x));
        } else if constexpr (sizeof(T) == sizeof(uint64_t)) {
                return static_cast<int>(__popcnt64(x));
        }
}

#endif

template<std::unsigned_integral T>
XSTD_PP_CONSTEXPR_INTRINSIC_FUN auto ctz(T x) noexcept
{
        return x ? ctznz(x) : std::numeric_limits<T>::digits;
}

template<std::unsigned_integral T>
XSTD_PP_CONSTEXPR_INTRINSIC_FUN auto bsf(T x) noexcept
{
        return x ? bsfnz(x) : std::numeric_limits<T>::digits;
}

template<std::unsigned_integral T>
XSTD_PP_CONSTEXPR_INTRINSIC_FUN auto clz(T x) noexcept
{
        return x ? clznz(x) : std::numeric_limits<T>::digits;
}

template<std::unsigned_integral T>
XSTD_PP_CONSTEXPR_INTRINSIC_FUN auto bsr(T x) noexcept
{
        return x ? bsrnz(x) : -1;
}

}       // namespace builtin

template<std::size_t N, std::unsigned_integral Block = std::size_t>
class bit_set
{
        static_assert(0 <= N && N <= std::numeric_limits<int>::max());

        static constexpr auto M = static_cast<int>(N);  // keep size_t from spilling all over the code base
        static constexpr auto block_size = std::numeric_limits<Block>::digits;
        static constexpr auto num_logical_blocks = (M - 1 + block_size) / block_size;
        static constexpr auto num_storage_blocks = std::max(num_logical_blocks, 1);
        static constexpr auto num_bits = num_logical_blocks * block_size;
        static constexpr auto num_excess_bits = num_bits - M;
        static_assert(0 <= num_excess_bits && num_excess_bits < block_size);

        class proxy_reference;
        class proxy_iterator;

        Block m_data[num_storage_blocks]{};             // zero-initialization
public:
        using key_type               = int;
        using key_compare            = std::less<key_type>;
        using value_type             = int;
        using value_compare          = std::less<value_type>;
        using pointer                = proxy_iterator;
        using const_pointer          = proxy_iterator;
        using reference              = proxy_reference;
        using const_reference        = proxy_reference;
        using size_type              = std::size_t;
        using difference_type        = std::ptrdiff_t;
        using iterator               = proxy_iterator;
        using const_iterator         = proxy_iterator;
        using reverse_iterator       = std::reverse_iterator<iterator>;
        using const_reverse_iterator = std::reverse_iterator<const_iterator>;
        using block_type             = Block;

        bit_set() = default;                            // zero-initialization

        template<class InputIterator>
        constexpr bit_set(InputIterator first, InputIterator last) // Throws: Nothing.
        {
                insert(first, last);
        }

        constexpr bit_set(std::initializer_list<value_type> ilist) // Throws: Nothing.
        :
                bit_set(ilist.begin(), ilist.end())
        {}

        XSTD_PP_CONSTEXPR_ALGORITHM auto& operator=(std::initializer_list<value_type> ilist) // Throws: Nothing.
        {
                clear();
                insert(ilist.begin(), ilist.end());
                return *this;
        }

        XSTD_PP_CONSTEXPR_INTRINSIC_FUN auto begin()         noexcept { return       iterator{this, find_first()}; }
        XSTD_PP_CONSTEXPR_INTRINSIC_FUN auto begin()   const noexcept { return const_iterator{this, find_first()}; }
        constexpr                       auto end()           noexcept { return       iterator{this, M}; }
        constexpr                       auto end()     const noexcept { return const_iterator{this, M}; }

        constexpr                       auto rbegin()        noexcept { return       reverse_iterator{end()}; }
        constexpr                       auto rbegin()  const noexcept { return const_reverse_iterator{end()}; }
        XSTD_PP_CONSTEXPR_INTRINSIC_FUN auto rend()          noexcept { return       reverse_iterator{begin()}; }
        XSTD_PP_CONSTEXPR_INTRINSIC_FUN auto rend()    const noexcept { return const_reverse_iterator{begin()}; }

        XSTD_PP_CONSTEXPR_INTRINSIC_FUN auto cbegin()  const noexcept { return const_iterator{begin()}; }
        constexpr                       auto cend()    const noexcept { return const_iterator{end()};   }
        constexpr                       auto crbegin() const noexcept { return const_reverse_iterator{rbegin()}; }
        XSTD_PP_CONSTEXPR_INTRINSIC_FUN auto crend()   const noexcept { return const_reverse_iterator{rend()};   }

        XSTD_PP_CONSTEXPR_INTRINSIC_FUN auto front() const // Throws: Nothing.
                -> const_reference
        {
                assert(!empty());
                return { *this, find_front() };
        }

        XSTD_PP_CONSTEXPR_INTRINSIC_FUN auto back() const // Throws: Nothing.
                -> const_reference
        {
                assert(!empty());
                return { *this, find_back() };
        }

        [[nodiscard]] XSTD_PP_CONSTEXPR_ALGORITHM auto empty() const noexcept
                -> bool
        {
                if constexpr (num_logical_blocks == 0) {
                        return true;
                } else if constexpr (num_logical_blocks == 1) {
                        return !m_data[0];
                } else if constexpr (num_logical_blocks == 2) {
                        return !(
                                m_data[0] ||
                                m_data[1]
                        );
                } else if constexpr (num_logical_blocks >= 3) {
                        return std::none_of(std::begin(m_data), std::end(m_data), [](auto block) -> bool {
                                return block;
                        });
                }
        }

        [[nodiscard]] XSTD_PP_CONSTEXPR_ALGORITHM auto full() const noexcept
        {
                if constexpr (num_excess_bits == 0) {
                        if constexpr (num_logical_blocks == 0) {
                                return true;
                        } else if constexpr (num_logical_blocks == 1) {
                                return m_data[0] == ones;
                        } else if constexpr (num_logical_blocks == 2) {
                                return
                                        m_data[0] == ones &&
                                        m_data[1] == ones
                                ;
                        } else if constexpr (num_logical_blocks >= 3) {
                                return std::all_of(std::begin(m_data), std::end(m_data), [](auto block) {
                                        return block == ones;
                                });
                        }
                } else {
                        static_assert(num_logical_blocks >= 1);
                        if constexpr (num_logical_blocks == 1) {
                                return m_data[0] == no_excess_bits;
                        } else if constexpr (num_logical_blocks == 2) {
                                return
                                        m_data[0] == no_excess_bits &&
                                        m_data[1] == ones
                                ;
                        } else if constexpr (num_logical_blocks >= 3) {
                                return
                                        m_data[0] == no_excess_bits &&
                                        std::all_of(std::next(std::begin(m_data)), std::end(m_data), [](auto block) {
                                                return block == ones;
                                        })
                                ;
                        }
                }
        }

        XSTD_PP_CONSTEXPR_INTRINSIC_FUN auto ssize() const noexcept
        {
                if constexpr (num_logical_blocks == 0) {
                        return 0;
                } else if constexpr (num_logical_blocks == 1) {
                        return builtin::popcount(m_data[0]);
                } else if constexpr (num_logical_blocks == 2) {
                        return
                                builtin::popcount(m_data[0]) +
                                builtin::popcount(m_data[1])
                        ;
                } else if constexpr (num_logical_blocks >= 3) {
                        return std::accumulate(std::begin(m_data), std::end(m_data), 0, [](auto sum, auto block) {
                                return sum + builtin::popcount(block);
                        });
                }
        }

        XSTD_PP_CONSTEXPR_INTRINSIC_FUN auto size() const noexcept
        {
                return static_cast<size_type>(ssize());
        }

        static constexpr auto max_size() noexcept
        {
                return N;
        }

        static constexpr auto capacity() noexcept
                -> size_type
        {
                return num_bits;
        }

        template<class... Args>
        constexpr auto emplace(Args&&... args) // Throws: Nothing.
        {
                static_assert(sizeof...(Args) == 1);
                return insert(value_type(std::forward<Args>(args)...));
        }

        template<class... Args>
        constexpr auto emplace_hint(const_iterator hint, Args&&... args) // Throws: Nothing.
        {
                static_assert(sizeof...(Args) == 1);
                return insert(hint, value_type(std::forward<Args>(args)...));
        }

        constexpr auto insert(value_type const& x) // Throws: Nothing.
                -> std::pair<iterator, bool>
        {
                assert(is_valid(x));
                if constexpr (num_logical_blocks >= 1) {
                        PRAGMA_GCC_DIAGNOSTIC_PUSH_IGNORED("-Wconversion")
                        m_data[which(x)] |= single_bit_mask(where(x));
                        PRAGMA_GCC_DIAGNOSTIC_POP
                }
                assert(contains(x));
                return { { this, x }, true };
        }

        constexpr auto insert(const_iterator /* hint */, value_type const& x) // Throws: Nothing.
                -> iterator
        {
                assert(is_valid(x));
                insert(x);
                return { this, x };
        }

        template<class InputIterator>
        XSTD_PP_CONSTEXPR_ALGORITHM auto insert(InputIterator first, InputIterator last) // Throws: Nothing.
        {
                std::for_each(first, last, [&](auto const& x) {
                        insert(x);
                });
        }

        XSTD_PP_CONSTEXPR_ALGORITHM auto insert(std::initializer_list<value_type> ilist) // Throws: Nothing.
        {
                insert(ilist.begin(), ilist.end());
        }

        XSTD_PP_CONSTEXPR_ALGORITHM auto& fill() noexcept
        {
                if constexpr (num_excess_bits == 0) {
                        if constexpr (num_logical_blocks == 1) {
                                m_data[0] = ones;
                        } else if constexpr (num_logical_blocks == 2) {
                                m_data[0] = ones;
                                m_data[1] = ones;
                        } else if constexpr (num_logical_blocks >= 3) {
                                std::fill_n(std::begin(m_data), num_logical_blocks, ones);
                        }
                } else {
                        if constexpr (num_logical_blocks == 1) {
                                m_data[0] = no_excess_bits;
                        } else if constexpr (num_logical_blocks == 2) {
                                m_data[0] = no_excess_bits;
                                m_data[1] = ones;
                        } else if constexpr (num_logical_blocks >= 3) {
                                m_data[0] = no_excess_bits;
                                std::fill_n(std::next(std::begin(m_data)), num_logical_blocks - 1, ones);
                        }
                }
                assert(full());
                return *this;
        }

        constexpr auto erase(key_type const& x) // Throws: Nothing.
                -> size_type
        {
                assert(is_valid(x));
                if constexpr (num_logical_blocks >= 1) {
                        PRAGMA_GCC_DIAGNOSTIC_PUSH_IGNORED("-Wconversion")
                        m_data[which(x)] &= ~single_bit_mask(where(x));
                        PRAGMA_GCC_DIAGNOSTIC_POP
                }
                assert(!contains(x));
                return 1;
        }

        constexpr auto erase(const_iterator pos) // Throws: Nothing.
        {
                assert(pos != end());
                erase(*pos++);
                return pos;
        }

        XSTD_PP_CONSTEXPR_ALGORITHM auto erase(const_iterator first, const_iterator last) // Throws: Nothing.
        {
                std::for_each(first, last, [&](auto const& x) {
                        erase(x);
                });
                return last;
        }

        XSTD_PP_CONSTEXPR_SWAP auto swap(bit_set& other [[maybe_unused]]) noexcept
        {
                if constexpr (num_logical_blocks == 1) {
                        std::swap(m_data[0], other.m_data[0]);
                } else if constexpr (num_logical_blocks == 2) {
                        std::swap(m_data[0], other.m_data[0]);
                        std::swap(m_data[1], other.m_data[1]);
                } else if constexpr (num_logical_blocks >= 3) {
                        std::swap_ranges(std::begin(m_data), std::end(m_data), std::begin(other.m_data));
                }
        }

        XSTD_PP_CONSTEXPR_ALGORITHM auto& clear() noexcept
        {
                if constexpr (num_logical_blocks == 1) {
                        m_data[0] = zero;
                } else if constexpr (num_logical_blocks == 2) {
                        m_data[0] = zero;
                        m_data[1] = zero;
                } else if constexpr (num_logical_blocks >= 3) {
                        std::fill_n(std::begin(m_data), num_logical_blocks, zero);
                }
                assert(empty());
                return *this;
        }

        constexpr auto& replace(value_type const& x [[maybe_unused]]) // Throws: Nothing.
        {
                assert(is_valid(x));
                if constexpr (num_logical_blocks >= 1) {
                        PRAGMA_GCC_DIAGNOSTIC_PUSH_IGNORED("-Wconversion")
                        m_data[which(x)] ^= single_bit_mask(where(x));
                        PRAGMA_GCC_DIAGNOSTIC_POP
                }
                return *this;
        }

        constexpr auto find(key_type const& x) // Throws: Nothing.
        {
                assert(is_valid(x));
                return contains(x) ? iterator{this, x} : end();
        }

        constexpr auto find(key_type const& x) const // Throws: Nothing.
        {
                assert(is_valid(x));
                return contains(x) ? const_iterator{this, x} : cend();
        }

        constexpr auto count(key_type const& x) const // Throws: Nothing.
                -> size_type
        {
                assert(is_valid(x));
                return contains(x);
        }

        [[nodiscard]] constexpr auto contains(key_type const& x) const // Throws: Nothing.
        {
                assert(is_valid(x));
                if constexpr (num_logical_blocks >= 1) {
                        if (m_data[which(x)] & single_bit_mask(where(x))) {
                                return true;
                        }
                }
                return false;
        }

        XSTD_PP_CONSTEXPR_INTRINSIC_FUN auto lower_bound(key_type const& x) // Throws: Nothing.
                -> iterator
        {
                assert(is_valid(x));
                return { this, find_next(x) };
        }

        XSTD_PP_CONSTEXPR_INTRINSIC_FUN auto lower_bound(key_type const& x) const // Throws: Nothing.
                -> const_iterator
        {
                assert(is_valid(x));
                return { this, find_next(x) };
        }

        XSTD_PP_CONSTEXPR_INTRINSIC_FUN auto upper_bound(key_type const& x) // Throws: Nothing.
                -> iterator
        {
                assert(is_valid(x));
                return { this, find_next(x + 1) };
        }

        XSTD_PP_CONSTEXPR_INTRINSIC_FUN auto upper_bound(key_type const& x) const // Throws: Nothing.
                -> const_iterator
        {
                assert(is_valid(x));
                return { this, find_next(x + 1) };
        }

        XSTD_PP_CONSTEXPR_INTRINSIC_FUN auto equal_range(key_type const& x) // Throws: Nothing.
                -> std::pair<iterator, iterator>
        {
                assert(is_valid(x));
                return { lower_bound(x), upper_bound(x) };
        }

        XSTD_PP_CONSTEXPR_INTRINSIC_FUN auto equal_range(key_type const& x) const // Throws: Nothing.
                -> std::pair<const_iterator, const_iterator>
        {
                assert(is_valid(x));
                return { lower_bound(x), upper_bound(x) };
        }

        constexpr auto& complement() noexcept
        {
                if constexpr (num_logical_blocks == 1) {
                        m_data[0] = static_cast<block_type>(~m_data[0]);
                } else if constexpr (num_logical_blocks == 2) {
                        m_data[0] = static_cast<block_type>(~m_data[0]);
                        m_data[1] = static_cast<block_type>(~m_data[1]);
                } else if constexpr (num_logical_blocks >= 3) {
                        for (auto& block : m_data) {
                                block = static_cast<block_type>(~block);
                        }
                }
                clear_excess_bits();
                return *this;
        }

        constexpr auto& operator&=(bit_set const& other [[maybe_unused]]) noexcept
        {
                if constexpr (num_logical_blocks == 1) {
                        m_data[0] &= other.m_data[0];
                } else if constexpr (num_logical_blocks == 2) {
                        m_data[0] &= other.m_data[0];
                        m_data[1] &= other.m_data[1];
                } else if constexpr (num_logical_blocks >= 3) {
                        for (auto i = 0; i < num_logical_blocks; ++i) {
                                m_data[i] &= other.m_data[i];
                        }
                }
                return *this;
        }

        constexpr auto& operator|=(bit_set const& other [[maybe_unused]]) noexcept
        {
                if constexpr (num_logical_blocks == 1) {
                        m_data[0] |= other.m_data[0];
                } else if constexpr (num_logical_blocks == 2) {
                        m_data[0] |= other.m_data[0];
                        m_data[1] |= other.m_data[1];
                } else if constexpr (num_logical_blocks >= 3) {
                        for (auto i = 0; i < num_logical_blocks; ++i) {
                                m_data[i] |= other.m_data[i];
                        }
                }
                return *this;
        }

        constexpr auto& operator^=(bit_set const& other [[maybe_unused]]) noexcept
        {
                if constexpr (num_logical_blocks == 1) {
                        m_data[0] ^= other.m_data[0];
                } else if constexpr (num_logical_blocks == 2) {
                        m_data[0] ^= other.m_data[0];
                        m_data[1] ^= other.m_data[1];
                } else if constexpr (num_logical_blocks >= 3) {
                        for (auto i = 0; i < num_logical_blocks; ++i) {
                                m_data[i] ^= other.m_data[i];
                        }
                }
                return *this;
        }

        constexpr auto& operator-=(bit_set const& other [[maybe_unused]]) noexcept
        {
                if constexpr (num_logical_blocks == 1) {
                        PRAGMA_GCC_DIAGNOSTIC_PUSH_IGNORED("-Wconversion")
                        m_data[0] &= ~other.m_data[0];
                        PRAGMA_GCC_DIAGNOSTIC_POP
                } else if constexpr (num_logical_blocks == 2) {
                        PRAGMA_GCC_DIAGNOSTIC_PUSH_IGNORED("-Wconversion")
                        m_data[0] &= ~other.m_data[0];
                        m_data[1] &= ~other.m_data[1];
                        PRAGMA_GCC_DIAGNOSTIC_POP
                } else if constexpr (num_logical_blocks >= 3) {
                        for (auto i = 0; i < num_logical_blocks; ++i) {
                                PRAGMA_GCC_DIAGNOSTIC_PUSH_IGNORED("-Wconversion")
                                m_data[i] &= ~other.m_data[i];
                                PRAGMA_GCC_DIAGNOSTIC_POP
                        }
                }
                return *this;
        }

        XSTD_PP_CONSTEXPR_ALGORITHM auto& operator<<=(value_type n [[maybe_unused]]) // Throws: Nothing.
        {
                assert(is_valid(n));
                if constexpr (num_logical_blocks == 1) {
                        PRAGMA_GCC_DIAGNOSTIC_PUSH_IGNORED("-Wconversion")
                        m_data[0] >>= n;
                        PRAGMA_GCC_DIAGNOSTIC_POP
                } else if constexpr (num_logical_blocks >= 2) {
                        if (n == 0) {
                                return *this;
                        }

                        auto const n_block = n / block_size;
                        auto const R_shift = n % block_size;

                        if (R_shift == 0) {
                                std::copy_n(std::next(std::begin(m_data), n_block), num_logical_blocks - n_block, std::begin(m_data));
                        } else {
                                auto const L_shift = block_size - R_shift;

                                for (auto i = 0; i < num_logical_blocks - 1 - n_block; ++i) {
                                        m_data[i] =
                                                static_cast<block_type>(m_data[i + n_block    ] >> R_shift) |
                                                static_cast<block_type>(m_data[i + n_block + 1] << L_shift)
                                        ;
                                }
                                m_data[num_logical_blocks - 1 - n_block] = static_cast<block_type>(m_data[num_logical_blocks - 1] >> R_shift);
                        }
                        std::fill_n(std::prev(std::end(m_data), n_block), n_block, zero);
                }
                clear_excess_bits();
                return *this;
        }

        XSTD_PP_CONSTEXPR_ALGORITHM auto& operator>>=(value_type n [[maybe_unused]]) // Throws: Nothing.
        {
                assert(is_valid(n));
                if constexpr (num_logical_blocks == 1) {
                        PRAGMA_GCC_DIAGNOSTIC_PUSH_IGNORED("-Wconversion")
                        m_data[0] <<= n;
                        PRAGMA_GCC_DIAGNOSTIC_POP
                } else if constexpr (num_logical_blocks >= 2) {
                        if (n == 0) {
                                return *this;
                        }

                        auto const n_block = n / block_size;
                        auto const L_shift = n % block_size;

                        if (L_shift == 0) {
                                std::copy_backward(std::begin(m_data), std::prev(std::end(m_data), n_block), std::end(m_data));
                        } else {
                                auto const R_shift = block_size - L_shift;

                                for (auto i = num_logical_blocks - 1; i > n_block; --i) {
                                        m_data[i] =
                                                static_cast<block_type>(m_data[i - n_block    ] << L_shift) |
                                                static_cast<block_type>(m_data[i - n_block - 1] >> R_shift)
                                        ;
                                }
                                m_data[n_block] = static_cast<block_type>(m_data[0] << L_shift);
                        }
                        std::fill_n(std::begin(m_data), n_block, zero);
                }
                return *this;
        }

        XSTD_PP_CONSTEXPR_ALGORITHM auto operator==(bit_set const& other [[maybe_unused]]) const noexcept
                -> bool
        {
                if constexpr (num_logical_blocks == 0) {
                        return true;
                } else if constexpr (num_logical_blocks <= 1) {
                        return m_data[0] == other.m_data[0];
                } else if constexpr (num_logical_blocks == 2) {
                        constexpr auto tied = [](auto const& bs) { return std::tie(bs.m_data[0], bs.m_data[1]); };
                        return tied(*this) == tied(other);
                } else if constexpr (num_logical_blocks >= 3) {
                        return std::equal(
                                std::begin(m_data), std::end(m_data),
                                std::begin(other.m_data), std::end(other.m_data)
                        );
                }
        }

        XSTD_PP_CONSTEXPR_ALGORITHM auto operator<=>(bit_set const& other [[maybe_unused]]) const noexcept
                -> std::strong_ordering
        {
                if constexpr (num_logical_blocks == 0) {
                        return std::strong_ordering::equal;
                } else if constexpr (num_logical_blocks <= 1) {
                        return other.m_data[0] <=> m_data[0];
                } else if constexpr (num_logical_blocks == 2) {
                        constexpr auto tied = [](auto const& bs) { return std::tie(bs.m_data[1], bs.m_data[0]); };
                        return tied(other) <=> tied(*this);
                } else if constexpr (num_logical_blocks >= 3) {
                        return detail::lexicographical_compare_three_way(
                                std::rbegin(other.m_data), std::rend(other.m_data),
                                std::rbegin(m_data), std::rend(m_data)
                        );
                }
        }

        friend XSTD_PP_CONSTEXPR_ALGORITHM auto is_subset_of(bit_set const& lhs [[maybe_unused]], bit_set const& rhs [[maybe_unused]]) noexcept
                -> bool
        {
                if constexpr (num_logical_blocks == 0) {
                        return true;
                } else if constexpr (num_logical_blocks == 1) {
                        return !(lhs.m_data[0] & ~rhs.m_data[0]);
                } else if constexpr (num_logical_blocks == 2) {
                        return
                                !(lhs.m_data[0] & ~rhs.m_data[0]) &&
                                !(lhs.m_data[1] & ~rhs.m_data[1])
                        ;
                } else if constexpr (num_logical_blocks >= 3) {
                        return std::equal(
                                std::begin(lhs.m_data), std::end(lhs.m_data),
                                std::begin(rhs.m_data), std::end(rhs.m_data),
                                [](auto wL, auto wR) -> bool { return !(wL & ~wR); }
                        );
                }
        }

        friend XSTD_PP_CONSTEXPR_ALGORITHM auto intersects(bit_set const& lhs [[maybe_unused]], bit_set const& rhs [[maybe_unused]]) noexcept
                -> bool
        {
                if constexpr (num_logical_blocks == 0) {
                        return false;
                } else if constexpr (num_logical_blocks == 1) {
                        return lhs.m_data[0] & rhs.m_data[0];
                } else if constexpr (num_logical_blocks == 2) {
                        return
                                (lhs.m_data[0] & rhs.m_data[0]) ||
                                (lhs.m_data[1] & rhs.m_data[1])
                        ;
                } else if constexpr (num_logical_blocks >= 3) {
                        return !std::equal(
                                std::begin(lhs.m_data), std::end(lhs.m_data),
                                std::begin(rhs.m_data), std::end(rhs.m_data),
                                [](auto wL, auto wR) -> bool { return !(wL & wR); }
                        );
                }
        }

private:
        static constexpr auto zero = static_cast<block_type>( 0);
        static constexpr auto ones = static_cast<block_type>(-1);

        PRAGMA_VC_WARNING_PUSH_DISABLE(4309)
        static constexpr auto no_excess_bits = static_cast<block_type>(ones << num_excess_bits);
        PRAGMA_VC_WARNING_POP

        static_assert(num_excess_bits ^ (ones == no_excess_bits));
        static constexpr auto unit = static_cast<block_type>(static_cast<block_type>(1) << (block_size - 1));

        static constexpr auto single_bit_mask(value_type n) // Throws: Nothing.
                -> block_type
        {
                static_assert(num_logical_blocks >= 1);
                assert(0 <= n && n < block_size);
                return static_cast<block_type>(unit >> n);
        }

        static constexpr auto is_valid(value_type n) noexcept
                -> bool
        {
                return 0 <= n && n < M;
        }

        static constexpr auto is_range(value_type n) noexcept
                -> bool
        {
                return 0 <= n && n <= M;
        }

        static constexpr auto which(value_type n [[maybe_unused]]) // Throws: Nothing.
                -> value_type
        {
                static_assert(num_logical_blocks >= 1);
                assert(is_valid(n));
                if constexpr (num_logical_blocks == 1) {
                        return 0;
                } else {
                        return num_logical_blocks - 1 - n / block_size;
                }
        }

        static constexpr auto where(value_type n) // Throws: Nothing.
                -> value_type
        {
                static_assert(num_logical_blocks >= 1);
                assert(is_valid(n));
                if constexpr (num_logical_blocks == 1) {
                        return n;
                } else {
                        return n % block_size;
                }
        }

        constexpr auto clear_excess_bits() noexcept
        {
                if constexpr (num_excess_bits != 0) {
                        static_assert(num_logical_blocks >= 1);
                        m_data[0] &= no_excess_bits;
                }
        }

        XSTD_PP_CONSTEXPR_INTRINSIC_FUN auto find_front() const // Throws: Nothing.
        {
                assert(!empty());
                if constexpr (num_logical_blocks == 1) {
                        return builtin::clznz(m_data[0]);
                } else if constexpr (num_logical_blocks == 2) {
                        return
                                m_data[1] ?
                                builtin::clznz(m_data[1]) :
                                builtin::clznz(m_data[0]) + block_size
                        ;
                } else {
                        auto n = 0;
                        for (auto i = num_storage_blocks - 1; i > 0; --i, n += block_size) {
                                if (auto const block = m_data[i]; block) {
                                        return n + builtin::clznz(block);
                                }
                        }
                        return n + builtin::clznz(m_data[0]);
                }
        }

        XSTD_PP_CONSTEXPR_INTRINSIC_FUN auto find_back() const // Throws: Nothing.
        {
                assert(!empty());
                if constexpr (num_logical_blocks == 1) {
                        return num_bits - 1 - builtin::ctznz(m_data[0]);
                } else if constexpr (num_logical_blocks == 2) {
                        return
                                m_data[0] ?
                                num_bits - 1 - builtin::ctznz(m_data[0]) :
                                block_size - 1 - builtin::ctznz(m_data[1])
                        ;
                } else {
                        auto n = num_bits - 1;
                        for (auto i = 0; i < num_storage_blocks - 1; ++i, n -= block_size) {
                                if (auto const block = m_data[i]; block) {
                                        return n - builtin::ctznz(block);
                                }
                        }
                        return n - builtin::ctznz(m_data[num_storage_blocks - 1]);
                }
        }

        XSTD_PP_CONSTEXPR_INTRINSIC_FUN auto find_first() const noexcept
        {
                if constexpr (num_logical_blocks == 1) {
                        if (m_data[0]) {
                                return builtin::clznz(m_data[0]);
                        }
                } else if constexpr (num_logical_blocks >= 2) {
                        auto n = 0;
                        for (auto i = num_logical_blocks - 1; i >= 0; --i, n += block_size) {
                                if (auto const block = m_data[i]; block) {
                                        return n + builtin::clznz(block);
                                }
                        }
                }
                return M;
        }

        XSTD_PP_CONSTEXPR_INTRINSIC_FUN auto find_next(int n) const // Throws: Nothing.
        {
                assert(is_range(n));
                if (n == M) {
                        return M;
                }
                if constexpr (num_logical_blocks == 1) {
                        if (auto const block = static_cast<block_type>(m_data[0] << n); block) {
                                return n + builtin::clznz(block);
                        }
                } else if constexpr (num_logical_blocks >= 2) {
                        auto i = which(n);
                        if (auto const offset = where(n); offset) {
                                if (auto const block = static_cast<block_type>(m_data[i] << offset); block) {
                                        return n + builtin::clznz(block);
                                }
                                --i;
                                n += block_size - offset;
                        }
                        for (/* init-statement before loop */; i >= 0; --i, n += block_size) {
                                if (auto const block = m_data[i]; block) {
                                        return n + builtin::clznz(block);
                                }
                        }
                }
                return M;
        }

        XSTD_PP_CONSTEXPR_INTRINSIC_FUN auto find_prev(int n) const // Throws: Nothing.
        {
                assert(is_valid(n));
                if constexpr (num_logical_blocks == 1) {
                        return n - builtin::ctznz(static_cast<block_type>(m_data[0] >> (block_size - 1 - n)));
                } else {
                        if constexpr (num_logical_blocks >= 2) {
                                auto i = which(n);
                                if (auto const offset = block_size - 1 - where(n); offset) {
                                        if (auto const block = static_cast<block_type>(m_data[i] >> offset); block) {
                                                return n - builtin::ctznz(block);
                                        }
                                        ++i;
                                        n -= block_size - offset;
                                }
                                for (/* init-statement before loop */; i < num_storage_blocks - 1; ++i, n -= block_size) {
                                        if (auto const block = m_data[i]; block) {
                                                return n - builtin::ctznz(block);
                                        }
                                }
                        }
                        return n - builtin::ctznz(m_data[num_storage_blocks - 1]);
                }
        }

        class proxy_reference
        {
                bit_set const& m_bs;
                value_type const m_value;
        public:
                ~proxy_reference() = default;
                proxy_reference(proxy_reference const&) = default;
                proxy_reference(proxy_reference&&) = default;
                proxy_reference& operator=(proxy_reference const&) = delete;
                proxy_reference& operator=(proxy_reference&&) = delete;

                proxy_reference() = delete;

                constexpr proxy_reference(bit_set const& bs, value_type const& v) noexcept
                :
                        m_bs{bs},
                        m_value{v}
                {
                        assert(is_valid(m_value));
                }

                constexpr auto operator&() const noexcept
                        -> proxy_iterator
                {
                        return { &m_bs, m_value };
                }

                explicit(false) constexpr operator value_type() const noexcept
                {
                        return m_value;
                }

                template<class T>
                        requires std::is_class_v<T> && std::constructible_from<T, value_type>
                explicit(false) constexpr operator T() const noexcept(noexcept(T(m_value)))
                {
                        return m_value;
                }
        };

        class proxy_iterator
        {
        public:
                using difference_type   = typename bit_set::difference_type;
                using value_type        = typename bit_set::value_type;
                using pointer           = proxy_iterator;
                using reference         = proxy_reference;
                using iterator_category = std::bidirectional_iterator_tag;

        private:
                bit_set const* m_bs;
                value_type m_value;

        public:
                proxy_iterator() = default;

                constexpr proxy_iterator(bit_set const* bs, value_type const& v) // Throws: Nothing.
                :
                        m_bs{bs},
                        m_value{v}
                {
                        assert(is_range(m_value));
                }

                constexpr auto operator*() const // Throws: Nothing.
                        -> proxy_reference
                {
                        assert(is_valid(m_value));
                        return { *m_bs, m_value };
                }

                XSTD_PP_CONSTEXPR_INTRINSIC_FUN auto& operator++() // Throws: Nothing.
                {
                        assert(is_valid(m_value));
                        m_value = m_bs->find_next(m_value + 1);
                        assert(is_valid(m_value - 1));
                        return *this;
                }

                XSTD_PP_CONSTEXPR_INTRINSIC_FUN auto operator++(int) // Throws: Nothing.
                {
                        auto nrv = *this; ++*this; return nrv;
                }

                XSTD_PP_CONSTEXPR_INTRINSIC_FUN auto& operator--() // Throws: Nothing.
                {
                        assert(is_valid(m_value - 1));
                        m_value = m_bs->find_prev(m_value - 1);
                        assert(is_valid(m_value));
                        return *this;
                }

                XSTD_PP_CONSTEXPR_INTRINSIC_FUN auto operator--(int) // Throws: Nothing.
                {
                        auto nrv = *this; --*this; return nrv;
                }

                constexpr auto operator==(proxy_iterator const& other) const noexcept
                        -> bool
                {
                        assert(m_bs == other.m_bs);
                        return m_value == other.m_value;
                }
        };
};

template<std::size_t N, std::unsigned_integral Block>
constexpr auto operator~(bit_set<N, Block> const& lhs) noexcept
{
        auto nrv{lhs}; nrv.complement(); return nrv;
}

template<std::size_t N, std::unsigned_integral Block>
constexpr auto operator&(bit_set<N, Block> const& lhs, bit_set<N, Block> const& rhs) noexcept
{
        auto nrv{lhs}; nrv &= rhs; return nrv;
}

template<std::size_t N, std::unsigned_integral Block>
constexpr auto operator|(bit_set<N, Block> const& lhs, bit_set<N, Block> const& rhs) noexcept
{
        auto nrv{lhs}; nrv |= rhs; return nrv;
}

template<std::size_t N, std::unsigned_integral Block>
constexpr auto operator^(bit_set<N, Block> const& lhs, bit_set<N, Block> const& rhs) noexcept
{
        auto nrv{lhs}; nrv ^= rhs; return nrv;
}

template<std::size_t N, std::unsigned_integral Block>
constexpr auto operator-(bit_set<N, Block> const& lhs, bit_set<N, Block> const& rhs) noexcept
{
        auto nrv{lhs}; nrv -= rhs; return nrv;
}

template<std::size_t N, std::unsigned_integral Block>
XSTD_PP_CONSTEXPR_ALGORITHM auto operator<<(bit_set<N, Block> const& lhs, int n) // Throws: Nothing.
{
        assert(0 <= n && n < static_cast<int>(N));
        auto nrv{lhs}; nrv <<= n; return nrv;
}

template<std::size_t N, std::unsigned_integral Block>
XSTD_PP_CONSTEXPR_ALGORITHM auto operator>>(bit_set<N, Block> const& lhs, int n) // Throws: Nothing.
{
        assert(0 <= n && n < static_cast<int>(N));
        auto nrv{lhs}; nrv >>= n; return nrv;
}

template<std::size_t N, std::unsigned_integral Block>
XSTD_PP_CONSTEXPR_ALGORITHM auto is_superset_of(bit_set<N, Block> const& lhs, bit_set<N, Block> const& rhs) noexcept
{
        return is_subset_of(rhs, lhs);
}

template<std::size_t N, std::unsigned_integral Block>
XSTD_PP_CONSTEXPR_ALGORITHM auto is_proper_subset_of(bit_set<N, Block> const& lhs, bit_set<N, Block> const& rhs) noexcept
{
        return is_subset_of(lhs, rhs) && !is_subset_of(rhs, lhs);
}

template<std::size_t N, std::unsigned_integral Block>
XSTD_PP_CONSTEXPR_ALGORITHM auto is_proper_superset_of(bit_set<N, Block> const& lhs, bit_set<N, Block> const& rhs) noexcept
{
        return is_superset_of(lhs, rhs) && !is_superset_of(rhs, lhs);
}

template<std::size_t N, std::unsigned_integral Block>
XSTD_PP_CONSTEXPR_ALGORITHM auto disjoint(bit_set<N, Block> const& lhs, bit_set<N, Block> const& rhs) noexcept
{
        return !intersects(lhs, rhs);
}

template<std::size_t N, std::unsigned_integral Block>
XSTD_PP_CONSTEXPR_SWAP auto swap(bit_set<N, Block>& lhs, bit_set<N, Block>& rhs) noexcept
{
        lhs.swap(rhs);
}

template<std::size_t N, std::unsigned_integral Block>
XSTD_PP_CONSTEXPR_INTRINSIC_FUN auto begin(bit_set<N, Block>& bs) noexcept
{
        return bs.begin();
}

template<std::size_t N, std::unsigned_integral Block>
XSTD_PP_CONSTEXPR_INTRINSIC_FUN auto begin(bit_set<N, Block> const& bs) noexcept
{
        return bs.begin();
}

template<std::size_t N, std::unsigned_integral Block>
constexpr auto end(bit_set<N, Block>& bs) noexcept
{
        return bs.end();
}

template<std::size_t N, std::unsigned_integral Block>
constexpr auto end(bit_set<N, Block> const& bs) noexcept
{
        return bs.end();
}

template<std::size_t N, std::unsigned_integral Block>
constexpr auto rbegin(bit_set<N, Block>& bs) noexcept
{
        return bs.rbegin();
}

template<std::size_t N, std::unsigned_integral Block>
constexpr auto rbegin(bit_set<N, Block> const& bs) noexcept
{
        return bs.rbegin();
}

template<std::size_t N, std::unsigned_integral Block>
XSTD_PP_CONSTEXPR_INTRINSIC_FUN auto rend(bit_set<N, Block>& bs) noexcept
{
        return bs.rend();
}

template<std::size_t N, std::unsigned_integral Block>
XSTD_PP_CONSTEXPR_INTRINSIC_FUN auto rend(bit_set<N, Block> const& bs) noexcept
{
        return bs.rend();
}

template<std::size_t N, std::unsigned_integral Block>
XSTD_PP_CONSTEXPR_INTRINSIC_FUN auto cbegin(bit_set<N, Block> const& bs) noexcept
{
        return xstd::begin(bs);
}

template<std::size_t N, std::unsigned_integral Block>
constexpr auto cend(bit_set<N, Block> const& bs) noexcept
{
        return xstd::end(bs);
}

template<std::size_t N, std::unsigned_integral Block>
constexpr auto crbegin(bit_set<N, Block> const& bs) noexcept
{
        return xstd::rbegin(bs);
}

template<std::size_t N, std::unsigned_integral Block>
XSTD_PP_CONSTEXPR_INTRINSIC_FUN auto crend(bit_set<N, Block> const& bs) noexcept
{
        return xstd::rend(bs);
}

template<std::size_t N, std::unsigned_integral Block>
XSTD_PP_CONSTEXPR_ALGORITHM auto size(bit_set<N, Block> const& bs) noexcept
{
        return bs.size();
}

template<std::size_t N, std::unsigned_integral Block>
XSTD_PP_CONSTEXPR_ALGORITHM auto ssize(bit_set<N, Block> const& bs) noexcept
{
        using R = std::common_type_t<std::ptrdiff_t, std::make_signed_t<decltype(bs.size())>>;
        return static_cast<R>(bs.size());
}

template<std::size_t N, std::unsigned_integral Block>
[[nodiscard]] XSTD_PP_CONSTEXPR_ALGORITHM auto empty(bit_set<N, Block> const& bs) noexcept
{
        return bs.empty();
}

template<class CharT, class Traits, std::size_t N, std::unsigned_integral Block>
auto& operator<<(std::basic_ostream<CharT, Traits>& ostr, bit_set<N, Block> const& bs)
{
        ostr << ostr.widen('[');
        auto first = true;
        for (auto const x : bs) {
                if (!first) {
                        ostr << ostr.widen(',');
                } else {
                        first = false;
                }
                ostr << x;
        }
        ostr << ostr.widen(']');
        return ostr;
}

template<class CharT, class Traits, std::size_t N, std::unsigned_integral Block>
auto& operator>>(std::basic_istream<CharT, Traits>& istr, bit_set<N, Block>& bs)
{
        typename bit_set<N, Block>::value_type x;
        CharT c;

        istr >> c;
        assert(c == istr.widen('['));
        istr >> c;
        for (auto first = true; c != istr.widen(']'); istr >> c) {
                assert(first == (c != istr.widen(',')));
                if (first) {
                        istr.putback(c);
                        first = false;
                }
                istr >> x;
                assert(0 <= x && x < static_cast<int>(bs.max_size()));
                bs.insert(x);
        }
        return istr;
}

}       // namespace xstd
