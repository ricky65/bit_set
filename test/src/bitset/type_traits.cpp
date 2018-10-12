//          Copyright Rein Halbersma 2014-2018.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <traits.hpp>
#include <xstd/int_set.hpp>                     // int_set
#include <boost/dynamic_bitset.hpp>             // dynamic_bitset
#include <boost/mpl/vector.hpp>                 // vector
#include <boost/test/test_case_template.hpp>    // BOOST_AUTO_TEST_CASE_TEMPLATE
#include <boost/test/unit_test.hpp>             // BOOST_AUTO_TEST_SUITE, BOOST_CHECK, BOOST_CHECK_EQUAL, BOOST_AUTO_TEST_SUITE_END
#include <bitset>                               // bitset
#include <type_traits>                          // is_nothrow_default_constructible_v, is_trivially_copyable, is_standard_layout

BOOST_AUTO_TEST_SUITE(TypeTraits)

using namespace xstd;

using bitset_types = boost::mpl::vector
<       std::bitset<  0>
,       std::bitset< 32>
,       std::bitset< 64>
,       std::bitset<128>
,       std::bitset<256>
,       int_set<  0>
,       int_set< 32>
,       int_set< 64>
,       int_set<128>
,       int_set<256>
>;

BOOST_AUTO_TEST_CASE_TEMPLATE(IsNothrowDefaultConstructible, T, bitset_types)
{
        static_assert(std::is_nothrow_default_constructible_v<T>);
}

BOOST_AUTO_TEST_CASE_TEMPLATE(IsTriviallyCopyable, T, bitset_types)
{
        static_assert(std::is_trivially_copyable_v<T>);
}

BOOST_AUTO_TEST_CASE_TEMPLATE(IsStandardLayout, T, bitset_types)
{
        static_assert(std::is_standard_layout_v<T>);
}

BOOST_AUTO_TEST_CASE_TEMPLATE(HasNoResize, T, bitset_types)
{
        static_assert(!tti::has_resize_v<T>);
}

BOOST_AUTO_TEST_CASE(DynamicBitsetHasResize)
{
        static_assert(tti::has_resize_v<boost::dynamic_bitset<>>);
}

BOOST_AUTO_TEST_SUITE_END()
