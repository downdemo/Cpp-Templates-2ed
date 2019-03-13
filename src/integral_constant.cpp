#include <cassert>
#include <type_traits>

namespace jc {

template <class T, T v>
struct integral_constant {
  static constexpr T value = v;
  using value_type = T;
  using type = integral_constant<T, v>;
  constexpr operator value_type() const noexcept { return value; }
  constexpr value_type operator()() const noexcept { return value; }
};

constexpr int to_int(char c) {
  // hexadecimal letters:
  if (c >= 'A' && c <= 'F') {
    return static_cast<int>(c) - static_cast<int>('A') + 10;
  }
  if (c >= 'a' && c <= 'f') {
    return static_cast<int>(c) - static_cast<int>('a') + 10;
  }
  assert(c >= '0' && c <= '9');
  return static_cast<int>(c) - static_cast<int>('0');
}

template <std::size_t N>
constexpr int parse_int(const char (&arr)[N]) {
  int base = 10;   // to handle base (default: decimal)
  int offset = 0;  // to skip prefixes like 0x
  if (N > 2 && arr[0] == '0') {
    switch (arr[1]) {
      case 'x':  // prefix 0x or 0X, so hexadecimal
      case 'X':
        base = 16;
        offset = 2;
        break;
      case 'b':  // prefix 0b or 0B (since C++14), so binary
      case 'B':
        base = 2;
        offset = 2;
        break;
      default:  // prefix 0, so octal
        base = 8;
        offset = 1;
        break;
    }
  }
  int res = 0;
  int multiplier = 1;
  for (std::size_t i = 0; i < N - offset; ++i) {
    if (arr[N - 1 - i] != '\'') {
      res += to_int(arr[N - 1 - i]) * multiplier;
      multiplier *= base;
    }
  }
  return res;
}

template <char... cs>
constexpr auto operator"" _c() {
  return integral_constant<int, parse_int<sizeof...(cs)>({cs...})>{};
}

static_assert(std::is_same_v<decltype(2_c), integral_constant<int, 2>>);
static_assert(std::is_same_v<decltype(0xFF_c), integral_constant<int, 255>>);
static_assert(
    std::is_same_v<decltype(0b1111'1111_c), integral_constant<int, 255>>);

}  // namespace jc

static_assert(jc::integral_constant<int, 42>::value == 42);
static_assert(std::is_same_v<int, jc::integral_constant<int, 0>::value_type>);
static_assert(jc::integral_constant<int, 42>{} == 42);

int main() {
  jc::integral_constant<int, 42> f;
  static_assert(f() == 42);
}
