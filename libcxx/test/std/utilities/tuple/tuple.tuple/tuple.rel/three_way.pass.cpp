//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// <tuple>

// template <class... Types> class tuple;

// template<class... TTypes, class... UTypes>
//   auto
//   operator<=>(const tuple<TTypes...>& t, const tuple<UTypes...>& u);
// template<tuple-like UTuple>
//   friend constexpr auto operator<=>(const tuple& t, const UTuple& u); // since C++23

// UNSUPPORTED: c++03, c++11, c++14, c++17

#include "test_macros.h"

TEST_CLANG_DIAGNOSTIC_IGNORED("-Wsign-compare")
TEST_GCC_DIAGNOSTIC_IGNORED("-Wsign-compare")
TEST_MSVC_DIAGNOSTIC_IGNORED(4242 4244)

#include <array>
#include <cassert>
#include <compare>
#include <complex>
#include <limits>
#include <ranges>
#include <tuple>
#include <type_traits> // std::is_constant_evaluated
#include <utility>

// A custom three-way result type
struct CustomEquality {
  friend constexpr bool operator==(const CustomEquality&, int) noexcept { return true; }
  friend constexpr bool operator<(const CustomEquality&, int) noexcept { return false; }
  friend constexpr bool operator<(int, const CustomEquality&) noexcept { return false; }
};

constexpr bool test() {
  struct WeakSpaceship {
    constexpr bool operator==(const WeakSpaceship&) const { return true; }
    constexpr std::weak_ordering operator<=>(const WeakSpaceship&) const { return std::weak_ordering::equivalent; }
  };

  // Empty tuple
  {
    typedef std::tuple<> T0;
    // No member types yields strong ordering (all are equal).
    ASSERT_SAME_TYPE(decltype(T0() <=> T0()), std::strong_ordering);
    assert((T0() <=> T0()) == std::strong_ordering::equal);
  }
  // Mixed types with integers, which compare strongly ordered
  {
    typedef std::tuple<long> T1;
    typedef std::tuple<short> T2;
    ASSERT_SAME_TYPE(decltype(T1() <=> T2()), std::strong_ordering);
    assert((T1(1) <=> T2(1)) == std::strong_ordering::equal);
    assert((T1(1) <=> T2(0)) == std::strong_ordering::greater);
    assert((T1(1) <=> T2(2)) == std::strong_ordering::less);
  }
  {
    typedef std::tuple<long, unsigned int> T1;
    typedef std::tuple<short, unsigned long> T2;
    ASSERT_SAME_TYPE(decltype(T1() <=> T2()), std::strong_ordering);
    assert((T1(1, 2) <=> T2(1, 2)) == std::strong_ordering::equal);
    assert((T1(1, 2) <=> T2(0, 2)) == std::strong_ordering::greater);
    assert((T1(1, 2) <=> T2(2, 2)) == std::strong_ordering::less);
    assert((T1(1, 2) <=> T2(1, 1)) == std::strong_ordering::greater);
    assert((T1(1, 2) <=> T2(1, 3)) == std::strong_ordering::less);
  }
  {
    typedef std::tuple<long, int, unsigned short> T1;
    typedef std::tuple<short, long, unsigned int> T2;
    ASSERT_SAME_TYPE(decltype(T1() <=> T2()), std::strong_ordering);
    assert((T1(1, 2, 3) <=> T2(1, 2, 3)) == std::strong_ordering::equal);
    assert((T1(1, 2, 3) <=> T2(0, 2, 3)) == std::strong_ordering::greater);
    assert((T1(1, 2, 3) <=> T2(2, 2, 3)) == std::strong_ordering::less);
    assert((T1(1, 2, 3) <=> T2(1, 1, 3)) == std::strong_ordering::greater);
    assert((T1(1, 2, 3) <=> T2(1, 3, 3)) == std::strong_ordering::less);
    assert((T1(1, 2, 3) <=> T2(1, 2, 2)) == std::strong_ordering::greater);
    assert((T1(1, 2, 3) <=> T2(1, 2, 4)) == std::strong_ordering::less);
  }
  // Mixed types with floating point, which compare partially ordered
  {
    typedef std::tuple<long> T1;
    typedef std::tuple<double> T2;
    ASSERT_SAME_TYPE(decltype(T1() <=> T2()), std::partial_ordering);
    assert((T1(1) <=> T2(1)) == std::partial_ordering::equivalent);
    assert((T1(1) <=> T2(0.9)) == std::partial_ordering::greater);
    assert((T1(1) <=> T2(1.1)) == std::partial_ordering::less);
  }
  {
    typedef std::tuple<long, float> T1;
    typedef std::tuple<double, unsigned int> T2;
    ASSERT_SAME_TYPE(decltype(T1() <=> T2()), std::partial_ordering);
    assert((T1(1, 2) <=> T2(1, 2)) == std::partial_ordering::equivalent);
    assert((T1(1, 2) <=> T2(0.9, 2)) == std::partial_ordering::greater);
    assert((T1(1, 2) <=> T2(1.1, 2)) == std::partial_ordering::less);
    assert((T1(1, 2) <=> T2(1, 1)) == std::partial_ordering::greater);
    assert((T1(1, 2) <=> T2(1, 3)) == std::partial_ordering::less);
  }
  {
    typedef std::tuple<short, float, double> T1;
    typedef std::tuple<double, long, unsigned int> T2;
    ASSERT_SAME_TYPE(decltype(T1() <=> T2()), std::partial_ordering);
    assert((T1(1, 2, 3) <=> T2(1, 2, 3)) == std::partial_ordering::equivalent);
    assert((T1(1, 2, 3) <=> T2(0.9, 2, 3)) == std::partial_ordering::greater);
    assert((T1(1, 2, 3) <=> T2(1.1, 2, 3)) == std::partial_ordering::less);
    assert((T1(1, 2, 3) <=> T2(1, 1, 3)) == std::partial_ordering::greater);
    assert((T1(1, 2, 3) <=> T2(1, 3, 3)) == std::partial_ordering::less);
    assert((T1(1, 2, 3) <=> T2(1, 2, 2)) == std::partial_ordering::greater);
    assert((T1(1, 2, 3) <=> T2(1, 2, 4)) == std::partial_ordering::less);
  }
  {
    typedef std::tuple<float> T1;
    typedef std::tuple<double> T2;
    constexpr double nan = std::numeric_limits<double>::quiet_NaN();
    // Comparisons with NaN and non-NaN are non-constexpr in GCC, so both sides must be NaN
    assert((T1(nan) <=> T2(nan)) == std::partial_ordering::unordered);
  }
  {
    typedef std::tuple<double, double> T1;
    typedef std::tuple<float, float> T2;
    constexpr double nan = std::numeric_limits<double>::quiet_NaN();
    assert((T1(nan, 2) <=> T2(nan, 2)) == std::partial_ordering::unordered);
    assert((T1(1, nan) <=> T2(1, nan)) == std::partial_ordering::unordered);
  }
  {
    typedef std::tuple<double, float, float> T1;
    typedef std::tuple<double, double, float> T2;
    constexpr double nan = std::numeric_limits<double>::quiet_NaN();
    assert((T1(nan, 2, 3) <=> T2(nan, 2, 3)) == std::partial_ordering::unordered);
    assert((T1(1, nan, 3) <=> T2(1, nan, 3)) == std::partial_ordering::unordered);
    assert((T1(1, 2, nan) <=> T2(1, 2, nan)) == std::partial_ordering::unordered);
  }
  // Ordering classes and synthesized three way comparison
  {
    typedef std::tuple<long, int, unsigned int> T1;
    typedef std::tuple<int, long, unsigned short> T2;
    // All strongly ordered members yields strong ordering.
    ASSERT_SAME_TYPE(decltype(T1() <=> T2()), std::strong_ordering);
  }
  {
    typedef std::tuple<int, unsigned int, WeakSpaceship> T1;
    typedef std::tuple<int, unsigned long, WeakSpaceship> T2;
    // Strongly ordered members and a weakly ordered member yields weak ordering.
    ASSERT_SAME_TYPE(decltype(T1() <=> T2()), std::weak_ordering);
  }
  {
    typedef std::tuple<unsigned int, int, WeakSpaceship> T1;
    typedef std::tuple<double, long, WeakSpaceship> T2;
    // Doubles are partially ordered, so one partial, one strong, and one weak ordering
    // yields partial ordering.
    ASSERT_SAME_TYPE(decltype(T1() <=> T2()), std::partial_ordering);
  }
  {
    struct NoSpaceship {
      constexpr bool operator==(const NoSpaceship&) const { return true; }
      constexpr bool operator<(const NoSpaceship&) const { return false; }
    };
    typedef std::tuple<int, unsigned int, NoSpaceship> T1;
    typedef std::tuple<int, unsigned long, NoSpaceship> T2;
    // Strongly ordered members and a weakly ordered member (synthesized) yields weak ordering.
    ASSERT_SAME_TYPE(decltype(T1() <=> T2()), std::weak_ordering);
  }
  {
    struct SpaceshipNoEquals {
      constexpr std::strong_ordering operator<=>(const SpaceshipNoEquals&) const { return std::strong_ordering::equal; }
      constexpr bool operator<(const SpaceshipNoEquals&) const { return false; }
    };
    typedef std::tuple<int, unsigned int, SpaceshipNoEquals> T1;
    typedef std::tuple<int, unsigned long, SpaceshipNoEquals> T2;
    // Spaceship operator with no == operator falls back on the < operator and weak ordering.
    ASSERT_SAME_TYPE(decltype(T1() <=> T2()), std::weak_ordering);
  }
  {
    struct CustomSpaceship {
      constexpr CustomEquality operator<=>(const CustomSpaceship&) const { return CustomEquality(); }
    };
    typedef std::tuple<int, unsigned int, CustomSpaceship> T1;
    typedef std::tuple<short, unsigned long, CustomSpaceship> T2;
    typedef std::tuple<CustomSpaceship> T3;
    // Custom three way return types cannot be used in synthesized three way comparison,
    // but they can be used for (rewritten) operator< when synthesizing a weak ordering.
    ASSERT_SAME_TYPE(decltype(T1() <=> T2()), std::weak_ordering);
    ASSERT_SAME_TYPE(decltype(T3() <=> T3()), std::weak_ordering);
  }
  {
    typedef std::tuple<long, int> T1;
    typedef std::tuple<long, unsigned int> T2;
    // Even with the warning suppressed (-Wno-sign-compare) there should still be no <=> operator
    // between signed and unsigned types, so we should end up with a synthesized weak ordering.
    ASSERT_SAME_TYPE(decltype(T1() <=> T2()), std::weak_ordering);
  }

#ifdef TEST_COMPILER_GCC
  // GCC cannot evaluate NaN @ non-NaN constexpr, so test that runtime-only.
  if (!std::is_constant_evaluated())
#endif
  {
    {
      typedef std::tuple<double> T1;
      typedef std::tuple<int> T2;
      constexpr double nan = std::numeric_limits<double>::quiet_NaN();
      ASSERT_SAME_TYPE(decltype(T1() <=> T2()), std::partial_ordering);
      assert((T1(nan) <=> T2(1)) == std::partial_ordering::unordered);
    }
    {
      typedef std::tuple<double, double> T1;
      typedef std::tuple<int, int> T2;
      constexpr double nan = std::numeric_limits<double>::quiet_NaN();
      ASSERT_SAME_TYPE(decltype(T1() <=> T2()), std::partial_ordering);
      assert((T1(nan, 2) <=> T2(1, 2)) == std::partial_ordering::unordered);
      assert((T1(1, nan) <=> T2(1, 2)) == std::partial_ordering::unordered);
    }
    {
      typedef std::tuple<double, double, double> T1;
      typedef std::tuple<int, int, int> T2;
      constexpr double nan = std::numeric_limits<double>::quiet_NaN();
      ASSERT_SAME_TYPE(decltype(T1() <=> T2()), std::partial_ordering);
      assert((T1(nan, 2, 3) <=> T2(1, 2, 3)) == std::partial_ordering::unordered);
      assert((T1(1, nan, 3) <=> T2(1, 2, 3)) == std::partial_ordering::unordered);
      assert((T1(1, 2, nan) <=> T2(1, 2, 3)) == std::partial_ordering::unordered);
    }
  }

// Heterogeneous comparisons enabled by P2165R4.
#if TEST_STD_VER >= 23
  {
    using T1 = std::tuple<long, int>;
    using T2 = std::pair<int, long>;
    constexpr T1 t1{1, 2};
    constexpr T2 t2{1, 2};
    ASSERT_SAME_TYPE(decltype(t1 <=> t2), std::strong_ordering);
    assert((t1 <=> t2) == std::strong_ordering::equal);
  }
  {
    using T1 = std::tuple<long, int>;
    using T2 = std::pair<int, long>;
    constexpr T1 t1{1, 2};
    constexpr T2 t2{1, 0};
    ASSERT_SAME_TYPE(decltype(t1 <=> t2), std::strong_ordering);
    assert((t1 <=> t2) == std::strong_ordering::greater);
  }
  {
    using T1 = std::tuple<long, int>;
    using T2 = std::pair<double, long>;
    constexpr T1 t1{1, 2};
    constexpr T2 t2{1.1, 3};
    ASSERT_SAME_TYPE(decltype(t1 <=> t2), std::partial_ordering);
    assert((t1 <=> t2) == std::partial_ordering::less);
  }
  {
    using T1 = std::tuple<long, int>;
    using T2 = std::pair<double, long>;
    constexpr T1 t1{1, 2};
    constexpr T2 t2{1.0, 2};
    ASSERT_SAME_TYPE(decltype(t1 <=> t2), std::partial_ordering);
    assert((t1 <=> t2) == std::partial_ordering::equivalent);
  }
  {
    using T1 = std::tuple<long, int>;
    using T2 = std::pair<double, long>;
    constexpr T1 t1{1, 2};
    constexpr T2 t2{1.1, 3};
    ASSERT_SAME_TYPE(decltype(t1 <=> t2), std::partial_ordering);
    assert((t1 <=> t2) == std::partial_ordering::less);
  }
  {
    using T1 = std::tuple<long, int>;
    using T2 = std::array<double, 2>;
    constexpr T1 t1{1, 2};
    constexpr T2 t2{1.0, 2.0};
    ASSERT_SAME_TYPE(decltype(t1 <=> t2), std::partial_ordering);
    assert((t1 <=> t2) == std::partial_ordering::equivalent);
  }
  {
    using T1 = std::tuple<long, int>;
    using T2 = std::array<double, 2>;
    constexpr T1 t1{1, 2};
    constexpr T2 t2{1.1, 3.0};
    ASSERT_SAME_TYPE(decltype(t1 <=> t2), std::partial_ordering);
    assert((t1 <=> t2) == std::partial_ordering::less);
  }
  {
    using T1 = std::tuple<const int*, const int*>;
    using T2 = std::ranges::subrange<const int*>;

    int arr[1]{};
    T1 t1{arr, arr + 1};
    T2 t2{arr};
    ASSERT_SAME_TYPE(decltype(t1 <=> t2), std::strong_ordering);
    assert((t1 <=> t2) == std::strong_ordering::equal);
  }
  {
    using T1 = std::tuple<const int*, const int*>;
    using T2 = std::ranges::subrange<const int*>;

    int arr[1]{};
    T1 t1{arr + 1, arr + 1};
    T2 t2{arr};
    ASSERT_SAME_TYPE(decltype(t1 <=> t2), std::strong_ordering);
    assert((t1 <=> t2) == std::strong_ordering::greater);
  }
  {
    constexpr std::tuple<WeakSpaceship, WeakSpaceship> t1{};
    constexpr std::pair<WeakSpaceship, WeakSpaceship> t2{};
    ASSERT_SAME_TYPE(decltype(t1 <=> t2), std::weak_ordering);
    assert((t1 <=> t2) == std::weak_ordering::equivalent);
  }
  {
    constexpr std::tuple<WeakSpaceship, WeakSpaceship> t1{};
    constexpr std::array<WeakSpaceship, 2> t2{};
    ASSERT_SAME_TYPE(decltype(t1 <=> t2), std::weak_ordering);
    assert((t1 <=> t2) == std::weak_ordering::equivalent);
  }
  {
    constexpr std::tuple<> t1{};
    constexpr std::array<int*, 0> t2{};
    ASSERT_SAME_TYPE(decltype(t1 <=> t2), std::strong_ordering);
    assert((t1 <=> t2) == std::strong_ordering::equal);
  }
  {
    constexpr std::tuple<> t1{};
    constexpr std::array<double, 0> t2{};
    ASSERT_SAME_TYPE(decltype(t1 <=> t2), std::strong_ordering);
    assert((t1 <=> t2) == std::strong_ordering::equal);
  }
  {
    constexpr std::tuple<> t1{};
    constexpr std::array<WeakSpaceship, 0> t2{};
    ASSERT_SAME_TYPE(decltype(t1 <=> t2), std::strong_ordering);
    assert((t1 <=> t2) == std::strong_ordering::equal);
  }
#endif
#if TEST_STD_VER >= 26
  {
    using T1 = std::tuple<long, int>;
    using T2 = std::complex<double>;
    constexpr T1 t1{1, 2};
    constexpr T2 t2{1.0, 2.0};
    ASSERT_SAME_TYPE(decltype(t1 <=> t2), std::partial_ordering);
    assert((t1 <=> t2) == std::partial_ordering::equivalent);
  }
  {
    using T1 = std::tuple<long, int>;
    using T2 = std::complex<double>;
    constexpr T1 t1{1, 2};
    constexpr T2 t2{1.1, 3.0};
    ASSERT_SAME_TYPE(decltype(t1 <=> t2), std::partial_ordering);
    assert((t1 <=> t2) == std::partial_ordering::less);
  }
#endif

  return true;
}

int main(int, char**) {
  test();
  static_assert(test());

  return 0;
}
