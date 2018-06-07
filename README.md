[![Language](https://img.shields.io/badge/language-C++-blue.svg)](https://isocpp.org/) 
[![Standard](https://img.shields.io/badge/c%2B%2B-17-blue.svg)](https://en.wikipedia.org/wiki/C%2B%2B#Standardization) 
[![License](https://img.shields.io/badge/license-Boost-blue.svg)](https://opensource.org/licenses/BSL-1.0) 
[![](https://tokei.rs/b1/github/rhalbersma/int_set)](https://github.com/rhalbersma/int_set)

Rebooting the `std::bitset` franchise
=====================================

An `int_set<N>` is a modern reimagining of `std::bitset<N>`, keeping what time has proven to be effective, and throwing out what is not. `int_set`s are array-backed sets of integers that are compact and fast: they do less (i.e. they don't do bounds-checking, and they don't throw exceptions) and offer more (e.g. iterators to seamlessly interact with the rest of the Standard Library). This enables you to do your bit-twiddling with familiar syntax, typically leading to cleaner, more expressive code.

Hello World
===========

The code below demonstrates how an `int_set` is a drop-in replacement for `std::set<int>` for generating all primes less than a compile time number. With an `int_set`, the usual STL-style iterator code remains valid with the same semantics but with much better performance through higher data parallelism.

For power users, bit-twiddling syntax will make the code even more expressive and performant. The `int_set` iterators inside the range-for call platform dependent intrinsics to lookup the first or last 1-bit. Similar intrinsics are called for counting the number of elements in an `int_set`.

[![Try it online](https://img.shields.io/badge/try%20it-online-brightgreen.svg)](https://wandbox.org/permlink/fp9UbnAPCENV82UJ)

```cpp
#include "xstd/int_set.hpp"
#include <algorithm>
#include <iostream>
#include <experimental/iterator>
#include <set>

constexpr auto N = 100;

#define USE_INT_SET 1
#if USE_INT_SET
    using set_type = xstd::int_set<N>;  // storage: N / 8 bytes on the stack
#else
    using set_type = std::set<int>;     // storage: 48 bytes on the stack + 32 * N bytes on the heap
#endif

int main()
{
    // find all primes below N: sieve of Eratosthenes
    set_type primes;
    for (auto i = 2; i < N; ++i) {
        primes.insert(i);
    }
    for (auto p : primes) {
        if (p * p >= N) break;
        for (auto n = p * p; n < N; n += p) {
            primes.erase(n);
        }
    }

    // print solution
    std::copy(primes.begin(), primes.end(), std::experimental::make_ostream_joiner(std::cout, ','));
    std::cout << '\n';

    // find all twin primes below N: STL-style iterator twiddling
    for (auto it = primes.begin(); it != primes.end() && std::next(it) != primes.end(); ) {
        it = std::adjacent_find(it, primes.end(), [](auto p, auto q) {
            return q - p == 2;
        });
        if (it != primes.end()) {
            std::cout << *it << ',' << *std::next(it) << '\n';
            ++it;
        }
    }
    std::cout << '\n';

#if USE_INT_SET
    // find all twin primes below N: bit-twiddling power-up
    for (auto twin : primes & primes >> 2) {
        std::cout << twin << ',' << (twin + 2) << '\n';
    }
    std::cout << '\n';
#endif
}
```

Documentation
=============

The interface for the class template `xstd::int_set<N>` consist of three major pieces:
  1. the full interface of the class template [`std::set<int>`](http://en.cppreference.com/w/cpp/container/set);
  2. bitwise operators `&=`, `|=`, `^=`, `<<=`, `>>=`, `~`, `&`, `|`, `^`, `<<`, `>>` from the class template [`std::bitset<N>`](http://en.cppreference.com/w/cpp/utility/bitset);
  3. non-member functions `is_subset_of`, `is_superset_of`, `is_proper_subset_of`, `is_proper_superset_of`, `intersects`, `disjoint`, many of which can also be found in the class template [`boost::dynamic_bitset<Block, Allocator>`](https://www.boost.org/doc/libs/1_67_0/libs/dynamic_bitset/dynamic_bitset.html);

The main difference between `set<int>` and `int_set<N>` is that an `int_set<N>` has a statically (i.e. at compile-time) defined maximum size of `N`. Inserting values outside the interval `[0, N)` into an `int_set<N>` is **undefined behavior**.

Translating `bitset` expressions to `int_set`
---------------------------------------------

Most `bitset` expressions have a direct translation to equivalent `int_set` expressions, with only minor semantic differences.

| Expression for `bitset<N>` | Expression for `int_set<N>`            | Semantics for `int_set<N>`                          |
| :------------------------- | :------------------------------------- | :-------------------------------------------------- |
| `bs.set()`                 | `is.fill()`                            | |
| `bs.set(pos)`              | `is.insert(pos)`                       | does not do bounds-checking or throw `out_of_range` |
| `bs.set(pos, val)`         | `val ? is.insert(pos) : is.erase(pos)` | does not do bounds-checking or throw `out_of_range` |
| `bs.reset()`               | `is.clear()`                           | |
| `bs.reset(pos)`            | `is.erase(pos)`                        | does not do bounds-checking or throw `out_of_range` |
| `bs.flip()`                | `is.complement()`                      | |
| `bs.flip(pos)`             | `is.replace(pos)`                      | does not do bounds-checking or throw `out_of_range` |
| `bs.count()`               | `is.size()`                            | `size_type` is signed                               |
| `bs.size()`                | `is.max_size()`                        | `size_type` is signed                               |
| `bs.test(pos)`             | `is.contains(pos)`                     | does not do bounds-checking or throw `out_of_range` |
| `bs.all()`                 | `is.full()`                            | |
| `bs.any()`                 | `!is.empty()`                          | |
| `bs.none()`                | `is.empty()`                           | |
| `bs[pos]`                  | `is.contains(pos)`                     | |
| `bs[pos] = val`            | `val ? is.insert(pos) : is.erase(pos)` | |
| `std::hash(bs)`            | `xstd::uhash<H>(is)`                   | [N3980: Types don't know #](http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2014/n3980.html) |

The semantic differences are that `int_set` has a signed integral `size_type` and does not do bounds-checking for its members `insert`, `erase`, `replace` and `contains`. Instead of throwing an `out_of_range` exception for argument values outside the range `[0, N)`, this behavior is undefined.

Semantic differences with bitwise-shift operators
-------------------------------------------------

The bitwise operators in `int_set` have identical syntax and *almost* the same semantics as in `bitset`.

| Expression for `bitset<N>` | Expression for `int_set<N>` | Notes for `int_set<N>`                              |
| :------------------------- | :-------------------------- | :-------------------------------------------------- |
| `bs <<= pos`               | `is <<= pos`                | does not do bounds-checking or throw `out_of_range` |
| `bs >>= pos`               | `is >>= pos`                | does not do bounds-checking or throw `out_of_range` |
| `bs << pos`                | `is << pos`                 | does not do bounds-checking or throw `out_of_range` |
| `bs >> pos`                | `is >> pos`                 | does not do bounds-checking or throw `out_of_range` |

The semantic differences are that `int_set` does not do bounds-checking for its bitwise-shift operators. Instead of throwing an `out_of_range` exception for argument values outside the range `[0, N)`, this behavior is undefined.

Functionality from `bitset` that is not in `int_set`
----------------------------------------------------

  - **Constructors**: No construction from `unsigned long long`, `std::string` or `char const*`.
  - **Conversion**: No conversion to `unsigned long`, `unsigned long long` and `std::string`.
  - **I/O**: No overloaded I/O streaming through overloaded `operator>>` and `operator<<`.

Much of this functionality can be easily provided by user-defined code. For instance, the missing `bitset` constructors and input streaming can be replaced by the `int_set` constructors taking an iterator range or an `initalizer_list`. Similarly, the missing `bitset` conversion functions and output streaming can be replaced by the `copy` algorithm over the `int_set` iterators into a suitable `ostream_iterator`.

Composable data-parallel algorithms on sorted ranges
----------------------------------------------------

Many of the bitwise operators for `int_set` are equivalent to algorithms on sorted ranges from the C++ Standard Library header `<algorithm>`.

| Expression for `int_set<N>`       | Expression for `set<int>` |
| :-------------------------------- | :------------------------ |
| `is_subset_of(a, b)`              | `includes(begin(a), end(a), begin(b), end(b))` |
| `auto c = a & b;`                 | `set<int> c;` <br> `set_intersection(begin(a), end(a), begin(b), end(b), inserter(c, end(c)));` |
| <code>auto c = b &#124; b;</code> | `set<int> c;` <br> `set_union(begin(a), end(a), begin(b), end(b), inserter(c, end(c)));` |
| `auto c = a ^ b;`                 | `set<int> c;` <br> `set_symmetric_difference(begin(a), end(a), begin(b), end(b), inserter(c, end(c)));` |
| `auto c = a - b;`                 | `set<int> c;` <br> `set_difference(begin(a), end(a), begin(b), end(b), inserter(c, end(c)));` |
| `auto b = a << n;`                | `set<int> tmp, b;` <br> `transform(begin(a), end(a), inserter(tmp, end(tmp)), [=](int x){ return x + n; });` <br> `copy_if(begin(tmp), end(tmp), inserter(b, end(b)), [](int x){ return x < N; });` |
| `auto b = a >> n;`                | `set<int> tmp, b;` <br> `transform(begin(a), end(a), inserter(tmp, end(tmp)), [=](int x){ return x - n; });` <br> `copy_if(begin(tmp), end(tmp), inserter(b, end(b)), [](int x){ return 0 <= x; });` |

The difference with iterator-based algorithms on general sorted ranges is that the bitwise operators from `int_set` provide **composable** and **data-parallel** versions of these algorithms. For the upcoming [Range TS](http://en.cppreference.com/w/cpp/experimental/ranges), these algorithms can also be formulated in a composable way, but without the data-parallellism that `int_set` provides.

Frequently Asked Questions
==========================

**Q**: How can you iterate over individual bits? I thought a byte was the unit of addressing?   
**A**: Using proxy iterators, which hold a pointer and an offset.

**Q**: What happens if you dereference a proxy iterator?   
**A**: You get a proxy reference: `ref == *it`.

**Q**: What happens if you take the address of a proxy reference?   
**A**: You get a proxy iterator: `it == &ref`.

**Q**: How do you get any value out of a proxy reference?   
**A**: They implicitly convert to `int`.

**Q**: How can proxy references work if C++ does not allow overloading of `operator.`?   
**A**: Indeed, proxy references break the equivalence between functions calls like `ref.mem_fn()` and `it->mem_fn()`.

**Q**: How do you work around this?   
**A**: `int` is not a class-type and does not have member functions, so this situation never occurs.

**Q**: So iterating over an `int_set` is really fool-proof?   
**A**: Yes, `int_set` iterators are [easy to use correctly and hard to use incorrectly](http://www.aristeia.com/Papers/IEEE_Software_JulAug_2004_revised.htm).

**Q**: I'm no fool, but a C++ programmer, how do I break things?   
**A**: If you insist, it is possible to use `int_set` iterators incorrectly by relying on too many implicit conversions.

[![Try it online](https://img.shields.io/badge/try%20it-online-brightgreen.svg)](https://wandbox.org/permlink/1hD8lFDPveVbFdJx)

```cpp
#include "xstd/int_set.hpp"
#include <algorithm>
#include <iostream>
#include <experimental/iterator>
#include <set>

#define USE_INT_SET 1
#if USE_INT_SET
    constexpr auto N = 32;
    using set_type = xstd::int_set<N>;
#else
    using set_type = std::set<int>;
#endif

class Int
{
    int m_value;
public:
    /* implicit */ Int(int v) noexcept : m_value{v} {}
    /* implicit */ operator auto() const noexcept { return m_value; }
};

int main()
{
    auto primes = set_type { 2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31 };
    std::set<Int> s;

    // at most one user-defined conversion allowed
    // xstd::int_set::reference -> int -> Int is an error
    // std::set::reference == int& -> int -> Int is OK
    std::copy(primes.begin(), primes.end(), std::inserter(s, s.end()));

    std::copy(s.begin(), s.end(), std::experimental::make_ostream_joiner(std::cout, ','));
}
```

Requirements
============

This single-header library has no other dependencies than the C++ Standard Library and is continuously being tested with the following conforming [C++17](http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2017/n4659.pdf) compilers:

| Platform | Compiler | Versions | Build |
| :------- | :------- | :------- | :---- |
| Linux    | Clang <br> GCC | 6.0, 7-SVN<br> 7.3, 8.1 | [![codecov](https://codecov.io/gh/rhalbersma/int_set/branch/master/graph/badge.svg)](https://codecov.io/gh/rhalbersma/int_set) <br> [![Build Status](https://travis-ci.org/rhalbersma/int_set.svg)](https://travis-ci.org/rhalbersma/int_set) |
| Windows  | Visual Studio  |                    15.7 | [![Build status](https://ci.appveyor.com/api/projects/status/pn0u2i8mcfp4d9un?svg=true)](https://ci.appveyor.com/project/rhalbersma/int-set) |

License
=======

Copyright Rein Halbersma 2014-2018.   
Distributed under the [Boost Software License, Version 1.0](http://www.boost.org/users/license.html).   
(See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
