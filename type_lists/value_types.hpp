#pragma once

#include <type_tuples.hpp>

namespace value_types
{

template<auto V>
struct ValueTag{ static constexpr auto Value = V; };

using type_tuples::TTuple;

template<class T, T... ts>
using VTuple = TTuple<ValueTag<ts>...>;

}
