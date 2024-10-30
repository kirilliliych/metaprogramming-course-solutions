#pragma once

#include <concepts>

#include <type_tuples.hpp>


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

  template <typename T, TypeList TL>
  struct Cons {
    using Head = T;
    using Tail = TL;
  };


  namespace detail {
    template <type_tuples::TypeTuple>
    struct FromTupleDummy;

    template <>
    struct FromTupleDummy<type_tuples::TTuple<>> {
      using Type = Nil;
    };

    template <class Head, class... Tail>
    struct FromTupleDummy<type_tuples::TTuple<Head, Tail...>> {
      using Type = Cons<Head, typename FromTupleDummy<type_tuples::TTuple<Tail...>>::Type>;
    };


    template <TypeList TL, class... ExtractedFromTL>
    struct ToTupleDummy {
      using Type = typename ToTupleDummy<typename TL::Tail, ExtractedFromTL..., typename TL::Head>::Type;
    };

    template <Empty TL, class... ExtractedFromTL>
    struct ToTupleDummy<TL, ExtractedFromTL...> {
      using Type = type_tuples::TTuple<ExtractedFromTL...>;
    };


    template <std::size_t N, TypeList TL>
    struct TakeDummy {
      using Type = Cons<typename TL::Head, typename TakeDummy<N - 1, typename TL::Tail>::Type>;
    };

    template <std::size_t N, Empty TL>
    struct TakeDummy<N, TL> {
      using Type = Nil;
    };

    template <TypeList TL>
    struct TakeDummy<0, TL> {
      using Type = Nil;
    };


    template <std::size_t N, TypeList TL>
    struct DropDummy {
      using Type = typename DropDummy<N - 1, typename TL::Tail>::Type;
    };

    template <std::size_t N, Empty TL>
    struct DropDummy<N, TL> {
      using Type = TL;
    };

    template <TypeList TL>
    struct DropDummy<0, TL> {
      using Type = TL;
    };


    template <std::size_t N, class T>
    struct ReplicateDummy {
      using Type = Cons<T, typename ReplicateDummy<N - 1, T>::Type>;
    };

    template <class T>
    struct ReplicateDummy<0, T> {
      using Type = Nil;
    };


    template <template <class> class F, TypeList TL>
    struct MapDummy {
      using Head = F<typename TL::Head>;
      using Tail = MapDummy<F, typename TL::Tail>;
    };

    template <template <class> class F, Empty TL>
    struct MapDummy<F, TL> : Nil {};


    template <template <class> class P, TypeList TL>
    struct FilterDummy : FilterDummy<P, typename TL::Tail> {};

    template <template <class> class P, TypeList TL>
    requires (P<typename TL::Head>::Value)
    struct FilterDummy<P, TL> {
      using Head = TL::Head;
      using Tail = FilterDummy<P, typename TL::Tail>;
    };

    template <template <class> class P, Empty TL>
    struct FilterDummy<P, TL> : Nil{};


    template <TypeList TL, class... ExtractedFromTL>
    struct CycleDummy {
      using Head = typename TL::Head;
      using Tail = CycleDummy<typename TL::Tail, ExtractedFromTL..., typename TL::Head>;
    };

    template <Empty TL, class T, class... ExtractedFromTL>
    struct CycleDummy<TL, T, ExtractedFromTL...> {
      using Head = T;
      using Tail = CycleDummy<TL, ExtractedFromTL..., T>;
    };

    template <Empty TL>
    struct CycleDummy<TL> : Nil {};


    template <template <class, class> class OP, class T, TypeList TL>
    struct ScanlDummy {
      using Head = OP<T, typename TL::Head>;
      using Tail = ScanlDummy<OP, Head, typename TL::Tail>;
    };

    template <template <class, class> class OP, class T, Empty TL>
    struct ScanlDummy<OP, T, TL> : Nil {};
    

    template <template <class, class> class OP, class T, TypeList TL>
    struct FoldlDummy {
      using Type = typename FoldlDummy<OP, OP<T, typename TL::Head>, typename TL::Tail>::Type;
    };

    template <template <class, class> class OP, class T, Empty TL>
    struct FoldlDummy<OP, T, TL> : Nil {
      using Type = T;
    };
    

    template <template <class, class> class OP, class T, TypeList TL>
    struct FoldrDummy {
      using Type = OP<typename TL::Head, typename FoldrDummy<OP, T, typename TL::Tail>::Type>;
    };

    template <template <class, class> class OP, class T, Empty TL>
    struct FoldrDummy<OP, T, TL> : Nil {
      using Type = T;
    };


    template <TypeList L, TypeList R>
    struct Zip2Dummy {
      using Head = type_tuples::TTuple<typename L::Head, typename R::Head>;
      using Tail = Zip2Dummy<typename L::Tail, typename R::Tail>;
    };

    template<TypeList L, TypeList R>
    requires Empty<L> || Empty<R>
    struct Zip2Dummy<L, R> : Nil {};


    template <TypeList... TL>
    struct ZipDummy {
      using Head = type_tuples::TTuple<typename TL::Head...>;
      using Tail = ZipDummy<typename TL::Tail...>;
    };


    template <template <class, class> class Eq, class Cur, TypeList TL, bool flag   >
    struct MakeGroup {
      using Next = MakeGroup<Eq, typename TL::Head, typename TL::Tail,
                             Eq<Cur, typename TL::Head>::Value>;
      using Result = Cons<Cur, typename Next::Result>;
      using Head = typename Next::Head;
      using Tail = typename Next::Tail;
    };

    template <template <class, class> class Eq, class Cur, TypeList TL>
    struct MakeGroup<Eq, Cur, TL, false> {
      using Result = Nil;
      using Head = Cur;
      using Tail = TL;
    };

    template <template <class, class> class Eq, class Cur, Empty TL>
    struct MakeGroup<Eq, Cur, TL, false> {
      using Result = Nil;
      using Head = Cur;
      using Tail = Nil;
    };

    template <template <class, class> class Eq, class Cur, Empty TL>
    struct MakeGroup<Eq, Cur, TL, true> {
      using Result = Cons<Cur, Nil>;
      using Head = Nil;
      using Tail = Nil;
    };

    template <template <class, class> class Eq, class Cur, TypeList TL>
    struct GroupByInt {
      using Group = MakeGroup<Eq, Cur, TL, true>;
      using Head = typename Group::Result;
      using Tail = GroupByInt<Eq, typename Group::Head, typename Group::Tail>;
    };

    template <template <class, class> class Eq, class Cur, Empty TL>
    struct GroupByInt<Eq, Cur, TL> {
      using Head = Cons<Cur, Nil>;
      using Tail = Nil;
    };

    template <template <class, class> class Eq, Empty TL>
    struct GroupByInt<Eq, Nil, TL> : Nil {};

    template <template <class, class> class Eq, TypeList TL>
    struct GroupBy {
      using Groupped = GroupByInt<Eq, typename TL::Head, typename TL::Tail>;
      using Head = typename Groupped::Head;
      using Tail = typename Groupped::Tail;
    };

    template <template <class, class> class Eq, Empty TL>
    struct GroupBy<Eq, TL> : Nil {};
  }; // namespace detail


template <class TT>
using FromTuple = typename detail::FromTupleDummy<TT>::Type;

template <TypeList TL>
using ToTuple = typename detail::ToTupleDummy<TL>::Type;

template <class T>
struct Repeat {
    using Head = T;
    using Tail = Repeat<T>;
};

template <std::size_t N, TypeList TL>
using Take = typename detail::TakeDummy<N, TL>::Type;

template <std::size_t N, TypeList TL>
using Drop = typename detail::DropDummy<N, TL>::Type;

template <std::size_t N, class T>
using Replicate = typename detail::ReplicateDummy<N, T>::Type;

template <template <class> class F, TypeList TL>
using Map = detail::MapDummy<F, TL>;

template <template <class> class P, TypeList TL>
using Filter = detail::FilterDummy<P, TL>;

template <template <class> class F, class T>
struct Iterate {
    using Head = T;
    using Tail = Iterate<F, F<T>>;
};

template <TypeList TL>
using Cycle = detail::CycleDummy<TL>;

template <TypeList TL, class... Types>
struct InitsDummy {
    using Head = FromTuple<type_tuples::TTuple<Types...>>;
    using Tail = InitsDummy<typename TL::Tail, Types..., typename TL::Head>;
};

template <Empty TL, class... Types>
struct InitsDummy<TL, Types...> {
    using Head = FromTuple<type_tuples::TTuple<Types...>>;
    using Tail = Nil;
};

template <TypeList TL>
using Inits = InitsDummy<TL>;


template<TypeList TL>
struct TailsDummy {
    using Head = TL;
    using Tail = TailsDummy<typename TL::Tail>;
};

template<Empty TL>
struct TailsDummy<TL> {
    using Head = Nil;
    using Tail = Nil;
};

template<TypeList TL>
using Tails = TailsDummy<TL>;

template <template <class, class> class OP, class T, TypeList TL>
using Scanl = Cons<T, detail::ScanlDummy<OP, T, TL>>;

template <template <class, class> class OP, class T, TypeList TL>
using Foldl = typename detail::FoldlDummy<OP, T, TL>::Type;

template <template <class, class> class OP, class T, TypeList TL>
using Foldr = typename detail::FoldrDummy<OP, T, TL>::Type;

template <TypeList L, TypeList R>
using Zip2 = detail::Zip2Dummy<L, R>;

template <TypeList... TL>
using Zip = detail::ZipDummy<TL...>;

template <template <class, class> class Eq, TypeList TL>
using GroupBy = detail::GroupBy<Eq, TL>;

} // namespace type_lists
