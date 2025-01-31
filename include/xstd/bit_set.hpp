#ifndef XSTD_BIT_SET_HPP
#define XSTD_BIT_SET_HPP

//          Copyright Rein Halbersma 2014-2022.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <algorithm>            // lexicographical_compare_three_way, max
#include <bit>                  // countl_zero, countr_zero, has_single_bit, popcount
#include <cassert>              // assert
#include <compare>              // strong_ordering
#include <concepts>             // constructible_from, innput_iteratorl, unsigned_integral
#include <cstddef>              // ptrdiff_t, size_t
#include <functional>           // identity, less
#include <initializer_list>     // initializer_list
#include <iterator>             // begin, bidirectional_iterator_tag, end, next, prev, rbegin, rend, reverse_iterator
#include <limits>               // digits
#include <numeric>              // accumulate
#include <ranges>               // all_of, copy_backward, copy_n, equal, fill_n, none_of, range, subrange, swap_ranges, views::drop, views::take
#include <tuple>                // tie
#include <type_traits>          // common_type_t, conditional_t, is_class_v, make_signed_t
#include <utility>              // forward, pair, swap

namespace xstd {

template<std::size_t N, std::unsigned_integral Block = std::size_t>
class bit_set
{
        static_assert(N <= std::numeric_limits<int>::max());

        static constexpr auto M = static_cast<int>(N);  // keep size_t from spilling all over the code base
        static constexpr auto block_size = std::numeric_limits<Block>::digits;
        static constexpr auto num_logical_blocks = (M - 1 + block_size) / block_size;
        static constexpr auto num_storage_blocks = std::max(num_logical_blocks, 1);
        static constexpr auto num_bits = num_logical_blocks * block_size;
        static constexpr auto has_unused_bits = num_bits > M;
        static constexpr auto num_unused_bits = num_bits - M;
        static_assert(0 <= num_unused_bits && num_unused_bits < block_size);

        template<bool> class proxy_reference;
        template<bool> class proxy_iterator;

        using const_proxy_reference = proxy_reference<true>;
        using const_proxy_iterator = proxy_iterator<true>;

        Block m_data[num_storage_blocks]{};     // zero-initialization
public:
        using key_type               = int;
        using key_compare            = std::less<key_type>;
        using value_type             = int;
        using value_compare          = std::less<value_type>;
        using pointer                = const_proxy_iterator;
        using const_pointer          = const_proxy_iterator;
        using reference              = const_proxy_reference;
        using const_reference        = const_proxy_reference;
        using size_type              = std::size_t;
        using difference_type        = std::ptrdiff_t;
        using iterator               = const_proxy_iterator;
        using const_iterator         = const_proxy_iterator;
        using reverse_iterator       = std::reverse_iterator<iterator>;
        using const_reverse_iterator = std::reverse_iterator<const_iterator>;
        using block_type             = Block;

        bit_set() = default;                    // zero-initialization

        template<class InputIterator>
        [[nodiscard]] constexpr bit_set(InputIterator first, InputIterator last) noexcept
        {
                insert(first, last);
        }

        [[nodiscard]] constexpr bit_set(std::initializer_list<value_type> ilist) noexcept
        :
                bit_set(ilist.begin(), ilist.end())
        {}

        constexpr auto& operator=(std::initializer_list<value_type> ilist) noexcept
        {
                clear();
                insert(ilist.begin(), ilist.end());
                return *this;
        }

        bool operator==(bit_set const&) const = default;

        [[nodiscard]] constexpr auto operator<=>(bit_set const& other [[maybe_unused]]) const noexcept
                -> std::strong_ordering
        {
                if constexpr (num_logical_blocks == 1) {
                        return other.m_data[0] <=> this->m_data[0];
                } else if constexpr (num_logical_blocks == 2) {
                        constexpr auto tied = [](auto const& bs) {
                                return std::tie(bs.m_data[1], bs.m_data[0]);
                        };
                        return tied(other) <=> tied(*this);
                } else {
                        return std::lexicographical_compare_three_way(
                                std::rbegin(other.m_data), std::rend(other.m_data),
                                std::rbegin(this->m_data), std::rend(this->m_data)
                        );
                }
        }

        [[nodiscard]] constexpr auto begin()         noexcept { return       iterator(this, find_first()); }
        [[nodiscard]] constexpr auto begin()   const noexcept { return const_iterator(this, find_first()); }
        [[nodiscard]] constexpr auto end()           noexcept { return       iterator(this, M); }
        [[nodiscard]] constexpr auto end()     const noexcept { return const_iterator(this, M); }

        [[nodiscard]] constexpr auto rbegin()        noexcept { return       reverse_iterator(end()); }
        [[nodiscard]] constexpr auto rbegin()  const noexcept { return const_reverse_iterator(end()); }
        [[nodiscard]] constexpr auto rend()          noexcept { return       reverse_iterator(begin()); }
        [[nodiscard]] constexpr auto rend()    const noexcept { return const_reverse_iterator(begin()); }

        [[nodiscard]] constexpr auto cbegin()  const noexcept { return const_iterator(begin()); }
        [[nodiscard]] constexpr auto cend()    const noexcept { return const_iterator(end());   }
        [[nodiscard]] constexpr auto crbegin() const noexcept { return const_reverse_iterator(rbegin()); }
        [[nodiscard]] constexpr auto crend()   const noexcept { return const_reverse_iterator(rend());   }

        [[nodiscard]] constexpr auto front() const noexcept
                -> const_reference
        {
                assert(!empty());
                return { *this, find_front() };
        }

        [[nodiscard]] constexpr auto back() const noexcept
                -> const_reference
        {
                assert(!empty());
                return { *this, find_back() };
        }

        [[nodiscard]] constexpr auto empty() const noexcept
        {
                if constexpr (num_logical_blocks == 1) {
                        return !m_data[0];
                } else if constexpr (num_logical_blocks == 2) {
                        return !(m_data[0] || m_data[1]);
                } else {
                        return std::ranges::none_of(m_data, std::identity{});
                }
        }

        [[nodiscard]] constexpr auto full() const noexcept
        {
                if constexpr (has_unused_bits) {
                        return m_data[0] == used_bits && std::ranges::all_of(m_data | std::views::drop(1), [](auto block) {
                                return block == ones;
                        });
                } else {
                        return std::ranges::all_of(m_data | std::views::take(num_logical_blocks), [](auto block) {
                                return block == ones;
                        });
                }
        }

        [[nodiscard]] constexpr auto ssize() const noexcept
        {
                if constexpr (num_logical_blocks == 1) {
                        return std::popcount(m_data[0]);
                } else if constexpr (num_logical_blocks == 2) {
                        return std::popcount(m_data[0]) + std::popcount(m_data[1]);
                } else {
                        // C++23: http://open-std.org/JTC1/SC22/WG21/docs/papers/2019/p1813r0.pdf
                        return std::accumulate(std::begin(m_data), std::end(m_data), 0, [](auto sum, auto block) {
                                return sum + std::popcount(block);
                        });
                }
        }

        [[nodiscard]] constexpr auto size() const noexcept
        {
                return static_cast<size_type>(ssize());
        }

        [[nodiscard]] static constexpr auto max_size() noexcept
        {
                return N;
        }

        [[nodiscard]] static constexpr auto capacity() noexcept
                -> size_type
        {
                return num_bits;
        }

        template<class... Args>
        constexpr auto emplace(Args&&... args) noexcept
                requires (sizeof...(Args) == 1)
        {
                return insert(value_type(std::forward<Args>(args)...));
        }

        template<class... Args>
        constexpr auto emplace_hint(const_iterator hint, Args&&... args) noexcept
                requires (sizeof...(Args) == 1)
        {
                return insert(hint, value_type(std::forward<Args>(args)...));
        }

        constexpr auto add(value_type x) noexcept
        {
                assert(is_valid(x));
                auto&& [ block, mask ] = block_mask(x);
                block |= mask;
                assert(contains(x));
        }

private:
        constexpr auto do_insert(value_type x) noexcept
                -> std::pair<iterator, bool>
        {
                assert(is_valid(x));
                auto&& [ block, mask ] = block_mask(x);
                auto const inserted = !(block & mask);
                block |= mask;
                assert(contains(x));
                return { { this, x },  inserted };
        }

public:
        constexpr auto insert(value_type const& x) noexcept
        {
                return do_insert(x);
        }

        constexpr auto insert(value_type&& x) noexcept
        {
                return do_insert(std::move(x));
        }

private:
        constexpr auto do_insert(const_iterator /* hint */, value_type x) noexcept
                -> iterator
        {
                add(x);
                return { this, x };
        }

public:
        constexpr auto insert(const_iterator hint, value_type const& x) noexcept
        {
                return do_insert(hint, x);
        }

        constexpr auto insert(const_iterator hint, value_type&& x) noexcept
        {
                return do_insert(hint, std::move(x));
        }

        template<class InputIterator>
        constexpr auto insert(InputIterator first, InputIterator last) noexcept
                requires std::input_iterator<InputIterator> && std::constructible_from<value_type, decltype(*first)>
        {
                for (auto x : std::ranges::subrange(first, last)) {
                        add(x);
                };
        }

        template<class Range>
        constexpr auto insert_range(Range&& rg) noexcept
                requires std::ranges::range<Range> && std::constructible_from<value_type, decltype(*rg.begin())>
        {
                for (auto x : rg) {
                        add(x);
                };
        }

        constexpr auto insert(std::initializer_list<value_type> ilist) noexcept
        {
                insert(ilist.begin(), ilist.end());
        }

        constexpr auto fill() noexcept
        {
                if constexpr (has_unused_bits) {
                        m_data[0] = used_bits;
                        std::ranges::fill_n(std::next(std::begin(m_data)), num_logical_blocks - 1, ones);
                } else {
                        std::ranges::fill_n(std::begin(m_data), num_logical_blocks, ones);
                }
                assert(full());
        }

        constexpr auto pop(key_type x) noexcept
        {
                assert(is_valid(x));
                auto&& [ block, mask ] = block_mask(x);
                block &= static_cast<block_type>(~mask);
                assert(!contains(x));
        }

        constexpr auto erase(key_type const& x) noexcept
        {
                assert(is_valid(x));
                auto&& [ block, mask ] = block_mask(x);
                auto const erased = static_cast<size_type>(static_cast<bool>(block & mask));
                block &= static_cast<block_type>(~mask);
                assert(!contains(x));
                return erased;
        }

        constexpr auto erase(const_iterator pos) noexcept
        {
                assert(pos != end());
                pop(*pos++);
                return pos;
        }

        constexpr auto erase(const_iterator first, const_iterator last) noexcept
        {
                for (auto x : std::ranges::subrange(first, last)) {
                        pop(x);
                }
                return last;
        }

        constexpr auto swap(bit_set& other [[maybe_unused]]) noexcept
        {
                std::ranges::swap_ranges(this->m_data, other.m_data);
        }

        constexpr auto clear() noexcept
        {
                std::ranges::fill_n(std::begin(m_data), num_logical_blocks, zero);
                assert(empty());
        }

        constexpr auto replace(value_type x [[maybe_unused]]) noexcept
        {
                assert(is_valid(x));
                auto&& [ block, mask ] = block_mask(x);
                block ^= mask;
        }

        [[nodiscard]] constexpr auto find(key_type const& x) noexcept
        {
                assert(is_valid(x));
                return contains(x) ? iterator(this, x) : end();
        }

        [[nodiscard]] constexpr auto find(key_type const& x) const noexcept
        {
                assert(is_valid(x));
                return contains(x) ? const_iterator(this, x) : cend();
        }

        [[nodiscard]] constexpr auto count(key_type const& x) const noexcept
                -> size_type
        {
                assert(is_valid(x));
                return contains(x);
        }

        [[nodiscard]] constexpr auto contains(key_type const& x) const noexcept
                -> bool
        {
                assert(is_valid(x));
                auto&& [ block, mask ] = block_mask(x);
                return block & mask;
        }

        [[nodiscard]] constexpr auto lower_bound(key_type const& x) noexcept
                -> iterator
        {
                assert(is_valid(x));
                return { this, find_next(x) };
        }

        [[nodiscard]] constexpr auto lower_bound(key_type const& x) const noexcept
                -> const_iterator
        {
                assert(is_valid(x));
                return { this, find_next(x) };
        }

        [[nodiscard]] constexpr auto upper_bound(key_type const& x) noexcept
                -> iterator
        {
                assert(is_valid(x));
                return { this, find_next(x + 1) };
        }

        [[nodiscard]] constexpr auto upper_bound(key_type const& x) const noexcept
                -> const_iterator
        {
                assert(is_valid(x));
                return { this, find_next(x + 1) };
        }

        [[nodiscard]] constexpr auto equal_range(key_type const& x) noexcept
                -> std::pair<iterator, iterator>
        {
                assert(is_valid(x));
                return { lower_bound(x), upper_bound(x) };
        }

        [[nodiscard]] constexpr auto equal_range(key_type const& x) const noexcept
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
                } else {
                        for (auto& block : m_data | std::views::take(num_logical_blocks)) {
                                block = static_cast<block_type>(~block);
                        };
                }
                clear_unused_bits();
                return *this;
        }

        constexpr auto& operator&=(bit_set const& other [[maybe_unused]]) noexcept
        {
                if constexpr (num_logical_blocks == 1) {
                        this->m_data[0] &= other.m_data[0];
                } else if constexpr (num_logical_blocks == 2) {
                        this->m_data[0] &= other.m_data[0];
                        this->m_data[1] &= other.m_data[1];
                } else {
                        // C++23 (currently available in range-v3)
                        // for (auto&& [lhs, rhs] : ranges::views::zip(this->m_data, other.m_data)) {
                        //         lhs &= rhs;
                        // }
                        for (auto i = 0; i < num_logical_blocks; ++i) {
                                this->m_data[i] &= other.m_data[i];
                        }
                }
                return *this;
        }

        constexpr auto& operator|=(bit_set const& other [[maybe_unused]]) noexcept
        {
                if constexpr (num_logical_blocks == 1) {
                        this->m_data[0] |= other.m_data[0];
                } else if constexpr (num_logical_blocks == 2) {
                        this->m_data[0] |= other.m_data[0];
                        this->m_data[1] |= other.m_data[1];
                } else {
                        // C++23 (currently available in range-v3)
                        // for (auto&& [lhs, rhs] : ranges::views::zip(this->m_data, other.m_data)) {
                        //         lhs |= rhs;
                        // }
                        for (auto i = 0; i < num_logical_blocks; ++i) {
                                this->m_data[i] |= other.m_data[i];
                        }
                }
                return *this;
        }

        constexpr auto& operator^=(bit_set const& other [[maybe_unused]]) noexcept
        {
                if constexpr (num_logical_blocks == 1) {
                        this->m_data[0] ^= other.m_data[0];
                } else if constexpr (num_logical_blocks == 2) {
                        this->m_data[0] ^= other.m_data[0];
                        this->m_data[1] ^= other.m_data[1];
                } else {
                        // C++23 (currently available in range-v3)
                        // for (auto&& [lhs, rhs] : ranges::views::zip(this->m_data, other.m_data)) {
                        //         lhs ^= rhs;
                        // }
                        for (auto i = 0; i < num_logical_blocks; ++i) {
                                this->m_data[i] ^= other.m_data[i];
                        }
                }
                return *this;
        }

        constexpr auto& operator-=(bit_set const& other [[maybe_unused]]) noexcept
        {
                if constexpr (num_logical_blocks == 1) {
                        this->m_data[0] &= static_cast<block_type>(~other.m_data[0]);
                } else if constexpr (num_logical_blocks == 2) {
                        this->m_data[0] &= static_cast<block_type>(~other.m_data[0]);
                        this->m_data[1] &= static_cast<block_type>(~other.m_data[1]);
                } else {
                        // C++23 (currently available in range-v3)
                        // for (auto&& [lhs, rhs] : ranges::views::zip(this->m_data, other.m_data)) {
                        //         lhs &= static_cast<block_type>(~rhs);
                        // }
                        for (auto i = 0; i < num_logical_blocks; ++i) {
                                this->m_data[i] &= static_cast<block_type>(~other.m_data[i]);
                        }
                }
                return *this;
        }

        constexpr auto& operator<<=(value_type n [[maybe_unused]]) noexcept
        {
                assert(is_valid(n));
                if constexpr (num_logical_blocks == 1) {
                        m_data[0] >>= n;
                } else {
                        if (n == 0) {
                                return *this;
                        }
                        auto const [ n_block, R_shift ] = div(n, block_size);
                        if (R_shift == 0) {
                                std::ranges::copy_n(std::next(std::begin(m_data), n_block), num_logical_blocks - n_block, std::begin(m_data));
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
                        std::ranges::fill_n(std::prev(std::end(m_data), n_block), n_block, zero);
                }
                clear_unused_bits();
                return *this;
        }

        constexpr auto& operator>>=(value_type n [[maybe_unused]]) noexcept
        {
                assert(is_valid(n));
                if constexpr (num_logical_blocks == 1) {
                        m_data[0] <<= n;
                } else {
                        if (n == 0) {
                                return *this;
                        }
                        auto const [ n_block, L_shift ] = div(n, block_size);
                        if (L_shift == 0) {
                                std::ranges::copy_backward(m_data | std::views::take(num_logical_blocks - n_block), std::end(m_data));
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
                        std::ranges::fill_n(std::begin(m_data), n_block, zero);
                }
                return *this;
        }

        [[nodiscard]] constexpr auto is_subset_of(bit_set const& other [[maybe_unused]]) const noexcept
        {
                // C++23 (currently available in range-v3)
                // return std::ranges::none_of(ranges::views::zip(this->m_data, other.m_data),
                //         [](auto const& t) { auto const& [lhs, rhs] = t;
                //                 return lhs & ~rhs;
                //         }
                // );
                return std::ranges::equal(this->m_data, other.m_data, [](auto lhs, auto rhs) {
                        return !(lhs & ~rhs);
                });
        }

        [[nodiscard]] constexpr auto is_proper_subset_of(bit_set const& other [[maybe_unused]]) const noexcept
        {
                auto i = 0;
                for (/* init-statement before loop */; i < num_logical_blocks; ++i) {
                        if (this->m_data[i] & ~other.m_data[i]) {
                                return false;
                        }
                        if (other.m_data[i] & ~this->m_data[i]) {
                                break;
                        }
                }
                // C++23 (currently available in range-v3)
                // return (i == num_logical_blocks) ? false : std::ranges::none_of(
                //         ranges::views::zip(this->m_data, other.m_data) | ranges::views::drop(i),
                //         [](auto const& t) { auto const& [lhs, rhs] = t;
                //                 return lhs & ~rhs;
                //         }
                // );
                return (i == num_logical_blocks) ? false : std::ranges::equal(
                        this->m_data | std::views::drop(i), other.m_data | std::views::drop(i),
                        [](auto lhs, auto rhs) {
                                return !(lhs & ~rhs);
                        }
                );
        }

        [[nodiscard]] constexpr auto intersects(bit_set const& other [[maybe_unused]]) const noexcept
                -> bool
        {
                // C++23 (currently available in range-v3)
                // return std::ranges::any_of(
                //         ranges::views::zip(this->m_data, other.m_data),
                //         [](auto const& t) { auto const& [lhs, rhs] = t;
                //                 return lhs & rhs;
                //         }
                // );
                return !std::ranges::equal(this->m_data, other.m_data, [](auto lhs, auto rhs) {
                        return !(lhs & rhs);
                });
        }

private:
        static constexpr auto zero = static_cast<block_type>( 0);
        static constexpr auto ones = static_cast<block_type>(-1);
        static constexpr auto used_bits = static_cast<block_type>(ones << num_unused_bits);
        static constexpr auto last_block = num_logical_blocks - 1;
        static constexpr auto last_bit = static_cast<block_type>(static_cast<block_type>(1) << (block_size - 1));
        static_assert(std::has_single_bit(last_bit));

        [[nodiscard]] static constexpr auto is_valid(value_type n) noexcept
        {
                return 0 <= n && n < M;
        }

        [[nodiscard]] static constexpr auto in_range(value_type n) noexcept
        {
                return 0 <= n && n <= M;
        }

        [[nodiscard]] static constexpr auto div(value_type numer, value_type denom) noexcept
                -> std::pair<value_type, value_type>
        {
                return { numer / denom, numer % denom };
        }

        [[nodiscard]] static constexpr auto index_offset(value_type n) noexcept
                -> std::pair<value_type, value_type>
        {
                assert(is_valid(n));
                if constexpr (num_logical_blocks == 1) {
                        return { 0, n };
                } else {
                        return div(n, block_size);
                }
        }

        [[nodiscard]] constexpr auto block_mask(value_type n) noexcept
                -> std::pair<block_type&, block_type>
        {
                assert(is_valid(n));
                auto const [ index, offset ] = index_offset(n);
                return { m_data[last_block - index], static_cast<block_type>(last_bit >> offset) };
        }

        [[nodiscard]] constexpr auto block_mask(value_type n) const noexcept
                -> std::pair<block_type const&, block_type>
        {
                assert(is_valid(n));
                auto const [ index, offset ] = index_offset(n);
                return { m_data[last_block - index], static_cast<block_type>(last_bit >> offset) };
        }

        constexpr auto clear_unused_bits() noexcept
        {
                if constexpr (has_unused_bits) {
                        m_data[0] &= used_bits;
                }
        }

        [[nodiscard]] constexpr auto find_front() const noexcept
        {
                assert(!empty());
                if constexpr (num_logical_blocks == 1) {
                        return std::countl_zero(m_data[0]);
                } else if constexpr (num_logical_blocks == 2) {
                        return m_data[1] ? std::countl_zero(m_data[1]) : std::countl_zero(m_data[0]) + block_size;
                } else {
                        auto n = 0;
                        for (auto i = num_storage_blocks - 1; i > 0; --i, n += block_size) {
                                if (auto const block = m_data[i]; block) {
                                        return n + std::countl_zero(block);
                                }
                        }
                        return n + std::countl_zero(m_data[0]);
                }
        }

        [[nodiscard]] constexpr auto find_back() const noexcept
        {
                assert(!empty());
                if constexpr (num_logical_blocks == 1) {
                        return num_bits - 1 - std::countr_zero(m_data[0]);
                } else if constexpr (num_logical_blocks == 2) {
                        return m_data[0] ? num_bits - 1 - std::countr_zero(m_data[0]) : block_size - 1 - std::countr_zero(m_data[1]);
                } else {
                        auto n = num_bits - 1;
                        for (auto i = 0; i < num_storage_blocks - 1; ++i, n -= block_size) {
                                if (auto const block = m_data[i]; block) {
                                        return n - std::countr_zero(block);
                                }
                        }
                        return n - std::countr_zero(m_data[num_storage_blocks - 1]);
                }
        }

        [[nodiscard]] constexpr auto find_first() const noexcept
        {
                if constexpr (num_logical_blocks == 1) {
                        if (m_data[0]) {
                                return std::countl_zero(m_data[0]);
                        }
                } else {
                        for (auto i = num_logical_blocks - 1, n = 0; i >= 0; --i, n += block_size) {
                                if (auto const block = m_data[i]; block) {
                                        return n + std::countl_zero(block);
                                }
                        }
                }
                return M;
        }

        [[nodiscard]] constexpr auto find_next(value_type n) const noexcept
        {
                assert(in_range(n));
                if (n == M) {
                        return M;
                }
                if constexpr (num_logical_blocks == 1) {
                        if (auto const block = static_cast<block_type>(m_data[0] << n); block) {
                                return n + std::countl_zero(block);
                        }
                } else if constexpr (num_logical_blocks >= 2) {
                        auto const [ index, offset ] = index_offset(n);
                        auto i = last_block - index;
                        if (offset) {
                                if (auto const block = static_cast<block_type>(m_data[i] << offset); block) {
                                        return n + std::countl_zero(block);
                                }
                                --i;
                                n += block_size - offset;
                        }
                        for (/* init-statement before loop */; i >= 0; --i, n += block_size) {
                                if (auto const block = m_data[i]; block) {
                                        return n + std::countl_zero(block);
                                }
                        }
                }
                return M;
        }

        [[nodiscard]] constexpr auto find_prev(value_type n) const noexcept
        {
                assert(is_valid(n));
                if constexpr (num_logical_blocks == 1) {
                        return n - std::countr_zero(static_cast<block_type>(m_data[0] >> (block_size - 1 - n)));
                } else {
                        if constexpr (num_logical_blocks >= 2) {
                                auto const [ index, offset ] = index_offset(n);
                                auto i = last_block - index;
                                if (auto const reverse_offset = block_size - 1 - offset; reverse_offset) {
                                        if (auto const block = static_cast<block_type>(m_data[i] >> reverse_offset); block) {
                                                return n - std::countr_zero(block);
                                        }
                                        ++i;
                                        n -= block_size - reverse_offset;
                                }
                                for (/* init-statement before loop */; i < num_storage_blocks - 1; ++i, n -= block_size) {
                                        if (auto const block = m_data[i]; block) {
                                                return n - std::countr_zero(block);
                                        }
                                }
                        }
                        return n - std::countr_zero(m_data[num_storage_blocks - 1]);
                }
        }

        template<bool IsConst>
        class proxy_reference
        {
                using rimpl_type = std::conditional_t<IsConst, bit_set const&, bit_set&>;
                using value_type = bit_set::value_type;
                rimpl_type m_ref;
                value_type m_val;
        public:
                ~proxy_reference() = default;
                proxy_reference(proxy_reference const&) = default;
                proxy_reference(proxy_reference&&) = default;
                proxy_reference& operator=(proxy_reference const&) = delete;
                proxy_reference& operator=(proxy_reference&&) = delete;

                proxy_reference() = delete;

                [[nodiscard]] constexpr proxy_reference(rimpl_type r, value_type v) noexcept
                :
                        m_ref(r),
                        m_val(v)
                {
                        assert(is_valid(m_val));
                }

                [[nodiscard]] constexpr auto operator==(proxy_reference const& other) const noexcept
                {
                        return this->m_val == other.m_val;
                }

                [[nodiscard]] constexpr auto operator&() const noexcept
                        -> proxy_iterator<IsConst>
                {
                        return { &m_ref, m_val };
                }

                [[nodiscard]] explicit(false) constexpr operator value_type() const noexcept
                {
                        return m_val;
                }

                template<class T>
                [[nodiscard]] explicit(false) constexpr operator T() const noexcept(noexcept(T(m_val)))
                        requires std::is_class_v<T> && std::constructible_from<T, value_type>
                {
                        return m_val;
                }
        };

        template<bool IsConst>
        class proxy_iterator
        {
        public:
                using iterator_category = std::bidirectional_iterator_tag;
                using difference_type   = bit_set::difference_type;
                using value_type        = bit_set::value_type;
                using pointer           = proxy_iterator<IsConst>;
                using reference         = proxy_reference<IsConst>;

        private:
                using pimpl_type = std::conditional_t<IsConst, bit_set const*, bit_set*>;
                pimpl_type m_ptr;
                value_type m_val;

        public:
                proxy_iterator() = default;

                [[nodiscard]] constexpr proxy_iterator(pimpl_type p, value_type v) noexcept
                :
                        m_ptr(p),
                        m_val(v)
                {
                        assert(in_range(m_val));
                }

                [[nodiscard]] constexpr auto operator==(proxy_iterator const& other) const noexcept
                {
                        assert(this->m_ptr == other.m_ptr);
                        return this->m_val == other.m_val;
                }

                [[nodiscard]] constexpr auto operator*() const noexcept
                        -> proxy_reference<IsConst>
                {
                        assert(is_valid(m_val));
                        return { *m_ptr, m_val };
                }

                constexpr auto& operator++() noexcept
                {
                        assert(is_valid(m_val));
                        m_val = m_ptr->find_next(m_val + 1);
                        assert(is_valid(m_val - 1));
                        return *this;
                }

                constexpr auto operator++(int) noexcept
                {
                        auto nrv = *this; ++*this; return nrv;
                }

                constexpr auto& operator--() noexcept
                {
                        assert(is_valid(m_val - 1));
                        m_val = m_ptr->find_prev(m_val - 1);
                        assert(is_valid(m_val));
                        return *this;
                }

                constexpr auto operator--(int) noexcept
                {
                        auto nrv = *this; --*this; return nrv;
                }
        };
};

template<std::size_t N, std::unsigned_integral Block>
[[nodiscard]] constexpr auto operator~(bit_set<N, Block> const& lhs) noexcept
{
        auto nrv = lhs; nrv.complement(); return nrv;
}

template<std::size_t N, std::unsigned_integral Block>
[[nodiscard]] constexpr auto operator&(bit_set<N, Block> const& lhs, bit_set<N, Block> const& rhs) noexcept
{
        auto nrv = lhs; nrv &= rhs; return nrv;
}

template<std::size_t N, std::unsigned_integral Block>
[[nodiscard]] constexpr auto operator|(bit_set<N, Block> const& lhs, bit_set<N, Block> const& rhs) noexcept
{
        auto nrv = lhs; nrv |= rhs; return nrv;
}

template<std::size_t N, std::unsigned_integral Block>
[[nodiscard]] constexpr auto operator^(bit_set<N, Block> const& lhs, bit_set<N, Block> const& rhs) noexcept
{
        auto nrv = lhs; nrv ^= rhs; return nrv;
}

template<std::size_t N, std::unsigned_integral Block>
[[nodiscard]] constexpr auto operator-(bit_set<N, Block> const& lhs, bit_set<N, Block> const& rhs) noexcept
{
        auto nrv = lhs; nrv -= rhs; return nrv;
}

template<std::size_t N, std::unsigned_integral Block>
[[nodiscard]] constexpr auto operator<<(bit_set<N, Block> const& lhs, int n) noexcept
{
        auto nrv = lhs; nrv <<= n; return nrv;
}

template<std::size_t N, std::unsigned_integral Block>
[[nodiscard]] constexpr auto operator>>(bit_set<N, Block> const& lhs, int n) noexcept
{
        auto nrv = lhs; nrv >>= n; return nrv;
}

template<std::size_t N, std::unsigned_integral Block>
constexpr auto swap(bit_set<N, Block>& lhs, bit_set<N, Block>& rhs) noexcept
{
        lhs.swap(rhs);
}

template<std::size_t N, std::unsigned_integral Block>
[[nodiscard]] constexpr auto begin(bit_set<N, Block>& bs) noexcept
{
        return bs.begin();
}

template<std::size_t N, std::unsigned_integral Block>
[[nodiscard]] constexpr auto begin(bit_set<N, Block> const& bs) noexcept
{
        return bs.begin();
}

template<std::size_t N, std::unsigned_integral Block>
[[nodiscard]] constexpr auto end(bit_set<N, Block>& bs) noexcept
{
        return bs.end();
}

template<std::size_t N, std::unsigned_integral Block>
[[nodiscard]] constexpr auto end(bit_set<N, Block> const& bs) noexcept
{
        return bs.end();
}

template<std::size_t N, std::unsigned_integral Block>
[[nodiscard]] constexpr auto rbegin(bit_set<N, Block>& bs) noexcept
{
        return bs.rbegin();
}

template<std::size_t N, std::unsigned_integral Block>
[[nodiscard]] constexpr auto rbegin(bit_set<N, Block> const& bs) noexcept
{
        return bs.rbegin();
}

template<std::size_t N, std::unsigned_integral Block>
[[nodiscard]] constexpr auto rend(bit_set<N, Block>& bs) noexcept
{
        return bs.rend();
}

template<std::size_t N, std::unsigned_integral Block>
[[nodiscard]] constexpr auto rend(bit_set<N, Block> const& bs) noexcept
{
        return bs.rend();
}

template<std::size_t N, std::unsigned_integral Block>
[[nodiscard]] constexpr auto cbegin(bit_set<N, Block> const& bs) noexcept
{
        return xstd::begin(bs);
}

template<std::size_t N, std::unsigned_integral Block>
[[nodiscard]] constexpr auto cend(bit_set<N, Block> const& bs) noexcept
{
        return xstd::end(bs);
}

template<std::size_t N, std::unsigned_integral Block>
[[nodiscard]] constexpr auto crbegin(bit_set<N, Block> const& bs) noexcept
{
        return xstd::rbegin(bs);
}

template<std::size_t N, std::unsigned_integral Block>
[[nodiscard]] constexpr auto crend(bit_set<N, Block> const& bs) noexcept
{
        return xstd::rend(bs);
}

template<std::size_t N, std::unsigned_integral Block>
[[nodiscard]] constexpr auto size(bit_set<N, Block> const& bs) noexcept
{
        return bs.size();
}

template<std::size_t N, std::unsigned_integral Block>
[[nodiscard]] constexpr auto ssize(bit_set<N, Block> const& bs) noexcept
{
        using R = std::common_type_t<std::ptrdiff_t, std::make_signed_t<decltype(bs.size())>>;
        return static_cast<R>(bs.size());
}

template<std::size_t N, std::unsigned_integral Block>
[[nodiscard]] constexpr auto empty(bit_set<N, Block> const& bs) noexcept
{
        return bs.empty();
}

}       // namespace xstd

#endif  // include guard
