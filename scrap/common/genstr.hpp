#pragma once

#include <algorithm>
#include <random>
#include <string_view>

namespace cmn {

template<class Char>
std::basic_string_view<Char> inline constexpr PRINTABLE_SYMBOLS{
  //
  "abcdefghijklmnopqrstuvwxyz{|}"
  "@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^"
  "0123456789:;<=>? !\"#$%&\'()*+,-./_`~"
};

template<>
std::basic_string_view<char8_t> inline constexpr PRINTABLE_SYMBOLS<char8_t>{
  u8"abcdefghijklmnopqrstuvwxyz{|}"
  "@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^"
  "0123456789:;<=>? !\"#$%&\'()*+,-./_`~"
};

template<>
std::basic_string_view<char16_t> inline constexpr PRINTABLE_SYMBOLS<char16_t>{
  u"abcdefghijklmnopqrstuvwxyz{|}"
  "@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^"
  "0123456789:;<=>? !\"#$%&\'()*+,-./_`~"
};

template<>
std::basic_string_view<char32_t> inline constexpr PRINTABLE_SYMBOLS<char32_t>{
  U"abcdefghijklmnopqrstuvwxyz{|}"
  "@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^"
  "0123456789:;<=>? !\"#$%&\'()*+,-./_`~"
};

template<class Char>
auto genstr(size_t len) -> std::basic_string<Char> {
  static std::mt19937                          gen{};
  static std::uniform_int_distribution<size_t> dist{
    0, std::size(PRINTABLE_SYMBOLS<Char>) - 1
  };

  std::basic_string<Char> str(len, ' ');
  std::generate(std::begin(str), std::end(str), [] {
    return PRINTABLE_SYMBOLS<Char>[dist(gen)];
  });
  return str;
}

template<class Char, size_t Len>
auto genstr() -> std::basic_string_view<Char> {
  thread_local static std::array<Char, Len + 1>             storage{};
  thread_local static std::mt19937                          gen{};
  thread_local static std::uniform_int_distribution<size_t> dist{
    0, std::size(PRINTABLE_SYMBOLS<Char>) - 1
  };

  std::basic_string_view<Char> str{ std::data(storage), Len };
  std::generate_n(std::begin(storage), Len, [] {
    return PRINTABLE_SYMBOLS<Char>[dist(gen)];
  });
  return str;
}


} // namespace cmn
