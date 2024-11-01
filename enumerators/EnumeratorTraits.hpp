#pragma once

#include <array>
#include <cstdint>
#include <numeric>
#include <regex>
#include <string>
#include <type_traits>


namespace detail {
  constexpr std::string_view parseValue(std::string_view input) {
    constexpr std::string_view enumValueStr = "EnumValue = ";
    size_t enumStrPos = input.find(enumValueStr);
    if (enumStrPos != std::string_view::npos) {
      enumStrPos += enumValueStr.size();

      size_t endPos = input.find_first_of("],;", enumStrPos);
      if (endPos != std::string_view::npos) {
        input = input.substr(enumStrPos, endPos - enumStrPos);
        size_t lastColons = input.rfind("::");
        if (lastColons != std::string_view::npos &&
            lastColons < input.length() - 2) {
          return input.substr(lastColons + 2);
        }

        return input;
      }
    }

    return "";
  }

  template <auto EnumValue>
  static constexpr std::string_view getPrettyFunction() {
    return __PRETTY_FUNCTION__;
  }

  static constexpr bool valueIsOK(std::string_view input) {
    return input.size() > 58 && input[58] != '(';
  }
} // namespace detail

template <class Enum, std::size_t MAXN = 512>
        requires std::is_enum_v<Enum>
struct EnumeratorTraits {
  using EnumType = std::underlying_type<Enum>::type;
  
  template <size_t index>
  static constexpr std::string_view getDescription() {
    constexpr int diff = (int) index - (int) MAXN;
    if constexpr (std::numeric_limits<EnumType>::max() < diff ||
                  std::numeric_limits<EnumType>::min() > diff) {
      return "";
    } else {
      return detail::getPrettyFunction<static_cast<Enum>(diff)>();
    }
  }

  template <size_t... Indices>
  static constexpr std::array<std::string_view, MAXN * 2 + 1>
  getDescriptions(std::index_sequence<Indices...>) noexcept {
    return std::array<std::string_view, MAXN * 2 + 1>{
        getDescription<Indices>()...};
  }

  static constexpr std::array<std::string_view, 2 * MAXN + 1>
  descriptions = getDescriptions(std::make_index_sequence<MAXN * 2 + 1>{});

  static constexpr std::size_t size() noexcept {
    std::size_t result = 0;

    for (auto&& description : descriptions) {
      if (detail::valueIsOK(description)) {
        ++result;
      }
    }
    return result;
  }

  static constexpr Enum at(std::size_t i) noexcept {
    int index = -(int) MAXN;
    for (auto&& description : descriptions) {
      if (detail::valueIsOK(description)) {
        if (i == 0) {
          return Enum(index);
        }
        --i;
      }
      ++index;
    }
    return Enum(0);
  }

  static constexpr std::string_view nameAt(std::size_t i) noexcept {
    for (auto&& description : descriptions) {
      if (detail::valueIsOK(description)) {
        if (i == 0) {
          return detail::parseValue(description);
        }
        --i;
      }
    }
    return "";
  }
};
