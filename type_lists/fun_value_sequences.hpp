#pragma once

#include <value_types.hpp>

using value_types::ValueTag;

namespace detail {
  template <class T>
  using Increment = value_types::ValueTag<T::Value + 1>;


  template <int N>
  struct FibDummy {
    using Type = ValueTag<FibDummy<N-1>::Type::Value + FibDummy<N-2>::Type::Value>;
  };

  template <>
  struct FibDummy<1> {
    using Type = ValueTag<1>;
  };

  template <>
  struct FibDummy<0> {
    using Type = ValueTag<0>;
  };

  template <class T>
  using FibByIndex = typename detail::FibDummy<T::Value>::Type;


  template <typename T>
  concept GreaterThanOne = requires (T) {
    requires(T::Value > 1 == true);
  };

  template <typename Dividend, typename Divisor>
  struct IsPrimeDummy {
    static constexpr bool Value = (Dividend::Value % Divisor::Value != 0) &&
        IsPrimeDummy<Dividend, ValueTag<Divisor::Value - 1>>::Value;
  };

  template <typename Dividend>
  struct IsPrimeDummy<Dividend, ValueTag<1>> {
    static constexpr bool Value = true;
  };

  template <typename T>
  struct IsPrime {
    static constexpr bool Value = IsPrimeDummy<T, ValueTag<T::Value - 1>>::Value;
  };

  template <>
  struct IsPrime<ValueTag<1>> {
    static constexpr bool Value = false;
  };

  template <>
  struct IsPrime<ValueTag<0>> {
    static constexpr bool Value = false;
  };
}; // namespace detail

using Nats = type_lists::Iterate<detail::Increment, value_types::ValueTag<0>>;
using Fib = type_lists::Map<detail::FibByIndex, Nats>;
using Primes = type_lists::Filter<detail::IsPrime, Nats>;
