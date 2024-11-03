#pragma once

#include <optional>
#include "InheritanceSortedTypeList.hpp"


template <class From, auto target>
struct Mapping {
using Class = From;
  static constexpr auto Value = target;
};

namespace detail {

template <class Base, class Target, type_lists::TypeList TL>
struct PolymorphicMapper {
	static std::optional<Target> map(const Base& object) {
		return dynamic_cast<const typename TL::Head::Class*> (&object) ?
      		TL::Head::Value :
			PolymorphicMapper<Base, Target, typename TL::Tail>::map(object);
	}
};

template <class Base, class Target, type_lists::Empty TL>
struct PolymorphicMapper<Base, Target, TL> {
	static std::optional<Target> map(const Base&) {
		return std::nullopt;
	}
};

} //namespace detail

template <class Base, class Target, class Mapping>
concept ValidMapping = std::is_base_of_v<Base, typename Mapping::Class>
                      && std::is_same_v<const Target, decltype(Mapping::Value)>;

template <class Base, class Target, class... Mappings>
	requires (ValidMapping<Base, Target, Mappings> && ...)
struct PolymorphicMapper {
	static std::optional<Target> map(const Base& object) {
		return detail::PolymorphicMapper<Base, Target,
    type_lists::SortedByInheritance<type_lists::PPackToTypeList<Mappings...>>>::map(object);
	}
};
