#pragma once

#include <concepts>

namespace type_lists
{
  template<class TL>
  concept TypeSequence =
  	requires {
  		typename TL::Head;
  		typename TL::Tail;
  	};

  struct Nil {};

  template<class TL>
  concept Empty = std::derived_from<TL, Nil>;

  template<class TL>
  concept TypeList = Empty<TL> || TypeSequence<TL>;

  template<class... Ts>
  struct PPackToTypeList : Nil {};

  template<class T, class... Ts>
  struct PPackToTypeList<T, Ts...> {
    using Head = T;
    using Tail = PPackToTypeList<Ts...>;
  };

  template <TypeSequence TL>
  struct MostDerived {
    using Prev = MostDerived<typename TL::Tail>;
    constexpr static bool isLess = std::is_base_of_v<typename Prev::Value::Class, typename TL::Head::Class>;
    using Value = std::conditional_t<isLess, typename TL::Head, typename Prev::Value>;
    constexpr static std::size_t Index = isLess ? 0 : Prev::Index + 1;
  };

  template <TypeSequence TL>
  requires Empty<typename TL::Tail>
  struct MostDerived<TL> {
    using Value = typename TL::Head;
    constexpr static std::size_t Index = 0;
  };


  template <TypeSequence TL, std::size_t IndexOfMinimum>
  struct ExcludeMostDerived {
    using Head = typename TL::Head;
    using Tail = ExcludeMostDerived<typename TL::Tail, IndexOfMinimum - 1>;
  };

  template <TypeSequence TL>
  struct ExcludeMostDerived<TL, 0> : TL::Tail {};

  template <TypeList TL>
  struct SortedByInheritance {
    using LastInInheritance = MostDerived<TL>;
    using Head = typename LastInInheritance::Value;
    using Tail = SortedByInheritance<ExcludeMostDerived<TL, LastInInheritance::Index>>;
  };

  template <Empty TL>
  struct SortedByInheritance<TL> : Nil {};
} // namespace type_lists