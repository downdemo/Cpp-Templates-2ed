#include <cassert>
#include <complex>
#include <cstring>
#include <functional>
#include <iostream>
#include <ostream>
#include <sstream>
#include <string>
#include <type_traits>
#include <utility>

#include "typelist.hpp"

namespace jc {

template <std::size_t N, typename T,
          bool = std::is_class_v<T> && !std::is_final_v<T>>
class tuple_elt;

template <std::size_t N, typename T>
class tuple_elt<N, T, false> {
 public:
  tuple_elt() = default;

  template <typename U>
  tuple_elt(U&& rhs) : value_(std::forward<U>(rhs)) {}

  T& get() { return value_; }

  const T& get() const { return value_; }

 private:
  T value_;
};

template <std::size_t N, typename T>
class tuple_elt<N, T, true> : private T {
 public:
  tuple_elt() = default;

  template <typename U>
  tuple_elt(U&& rhs) : T(std::forward<U>(rhs)) {}

  T& get() { return *this; }

  const T& get() const { return *this; }
};

template <typename... Types>
class tuple;

template <typename Head, typename... Tail>
class tuple<Head, Tail...> : private tuple_elt<sizeof...(Tail), Head>,
                             private tuple<Tail...> {
 public:
  tuple() = default;

  tuple(const Head& head, const tuple<Tail...>& tail)
      : HeadElt(head), TailElt(tail) {}

  template <
      typename VHead, typename... VTail,
      std::enable_if_t<sizeof...(VTail) == sizeof...(Tail), void*> = nullptr>
  tuple(VHead&& head, VTail&&... tail)
      : HeadElt(std::forward<VHead>(head)),
        TailElt(std::forward<VTail>(tail)...) {}

  template <
      typename VHead, typename... VTail,
      std::enable_if_t<sizeof...(VTail) == sizeof...(Tail), void*> = nullptr>
  tuple(const tuple<VHead, VTail...>& rhs)
      : HeadElt(rhs.get_head()), TailElt(rhs.get_tail()) {}

  // for push_back_tuple
  template <typename V, typename VHead, typename... VTail>
  tuple(const V& v, const tuple<VHead, VTail...>& rhs)
      : HeadElt(v), TailElt(rhs) {}

  Head& get_head() { return static_cast<HeadElt*>(this)->get(); }

  const Head& get_head() const {
    return static_cast<const HeadElt*>(this)->get();
  }

  tuple<Tail...>& get_tail() { return *this; }

  const tuple<Tail...>& get_tail() const { return *this; }

  template <typename T, T Index>
  auto& operator[](std::integral_constant<T, Index>);

 private:
  using HeadElt = tuple_elt<sizeof...(Tail), Head>;
  using TailElt = tuple<Tail...>;
};

template <>
class tuple<> {};

template <std::size_t N>
struct tuple_get {
  template <typename Head, typename... Tail>
  static auto& apply(const tuple<Head, Tail...>& t) {
    return tuple_get<N - 1>::apply(t.get_tail());
  }
};

template <>
struct tuple_get<0> {
  template <typename Head, typename... Tail>
  static const Head& apply(const tuple<Head, Tail...>& t) {
    return t.get_head();
  }
};

template <std::size_t N, typename... Types>
auto& get(const tuple<Types...>& t) {
  return tuple_get<N>::apply(t);
}

template <typename Head, typename... Tail>
template <typename T, T Index>
inline auto& tuple<Head, Tail...>::operator[](
    std::integral_constant<T, Index>) {
  return get<Index>(*this);
}

template <typename... Types>
auto make_tuple(Types&&... args) {
  return tuple<std::decay_t<Types>...>(std::forward<Types>(args)...);
}

bool operator==(const tuple<>&, const tuple<>&) { return true; }

template <
    typename Head1, typename... Tail1, typename Head2, typename... Tail2,
    std::enable_if_t<sizeof...(Tail1) == sizeof...(Tail2), void*> = nullptr>
bool operator==(const tuple<Head1, Tail1...>& lhs,
                const tuple<Head2, Tail2...>& rhs) {
  return lhs.get_head() == rhs.get_head() && lhs.get_tail() == rhs.get_tail();
}

void print_tuple(std::ostream& os, const tuple<>&, bool is_first = true) {
  os << (is_first ? '(' : ')');
}

template <typename Head, typename... Tail>
void print_tuple(std::ostream& os, const tuple<Head, Tail...>& t,
                 bool is_first = true) {
  os << (is_first ? "(" : ", ") << t.get_head();
  print_tuple(os, t.get_tail(), false);
}

template <typename... Types>
std::ostream& operator<<(std::ostream& os, const tuple<Types...>& t) {
  print_tuple(os, t);
  return os;
}

}  // namespace jc

namespace jc {  // typelist

template <>
struct is_empty<tuple<>> {
  static constexpr bool value = true;
};

template <typename Head, typename... Tail>
class front<tuple<Head, Tail...>> {
 public:
  using type = Head;
};

template <typename Head, typename... Tail>
class pop_front<tuple<Head, Tail...>> {
 public:
  using type = tuple<Tail...>;
};

template <typename... Types, typename Element>
class push_front<tuple<Types...>, Element> {
 public:
  using type = tuple<Element, Types...>;
};

template <typename... Types, typename Element>
class push_back<tuple<Types...>, Element> {
 public:
  using type = tuple<Types..., Element>;
};

template <typename... Types>
pop_front_t<tuple<Types...>> pop_front_tuple(const tuple<Types...>& t) {
  return t.get_tail();
}

template <typename... Types, typename V>
push_front_t<tuple<Types...>, V> push_front_tuple(const tuple<Types...>& t,
                                                  const V& v) {
  return push_front_t<tuple<Types...>, V>{v, t};
}

template <typename V>
tuple<V> push_back_tuple(const tuple<>&, const V& v) {
  return tuple<V>{v};
}

template <typename Head, typename... Tail, typename V>
tuple<Head, Tail..., V> push_back_tuple(const tuple<Head, Tail...>& t,
                                        const V& v) {
  return tuple<Head, Tail..., V>{t.get_head(),
                                 push_back_tuple(t.get_tail(), v)};
}

template <typename... Types, std::size_t... Indices>
auto select_tuple(const tuple<Types...>& t, std::index_sequence<Indices...>) {
  // find std::make_tuple using ADL so explicitly specify the namespace
  return jc::make_tuple(get<Indices>(t)...);
}

template <typename... Types>
auto reverse_tuple(const tuple<Types...>& t) {
  return select_tuple(t, reverse_t<std::index_sequence_for<Types...>>{});
}

// The following implementation copies elements repeatedly.
// tuple<> reverse_tuple(const tuple<>& t) { return t; }

// template <typename Head, typename... Tail>
// reverse_t<tuple<Head, Tail...>> reverse_tuple(const tuple<Head, Tail...>& t)
// {
//   return push_back_tuple(reverse_tuple(t.get_tail()), t.get_head());
// }

template <typename... Types>
pop_back_t<tuple<Types...>> pop_back_tuple(const tuple<Types...>& t) {
  return reverse_tuple(pop_front_tuple(reverse_tuple(t)));
}

template <std::size_t I, std::size_t N,
          typename IndexList = std::index_sequence<>>
struct replicated_index_list;

template <std::size_t I, std::size_t... Indices>
struct replicated_index_list<I, 0, std::index_sequence<Indices...>> {
  using type = std::index_sequence<Indices...>;
};

template <std::size_t I, std::size_t N, std::size_t... Indices>
struct replicated_index_list<I, N, std::index_sequence<Indices...>>
    : replicated_index_list<I, N - 1, std::index_sequence<Indices..., I>> {};

template <std::size_t I, std::size_t N>
using replicated_index_list_t = typename replicated_index_list<I, N>::type;

template <std::size_t I, std::size_t N, typename... Types>
auto splat_tuple(const tuple<Types...>& t) {
  return select_tuple(t, replicated_index_list_t<I, N>{});
}

template <typename List, template <typename T, typename U> class F>
struct metafun_of_nth_element {
  template <typename T, typename U>
  struct Apply;

  template <std::size_t N, std::size_t M>
  struct Apply<std::integral_constant<std::size_t, M>,
               std::integral_constant<std::size_t, N>>
      : F<nth_element_t<List, M>, nth_element_t<List, N>> {};
};

template <template <typename T, typename U> class Compare, typename... Types>
auto sort_tuple(const tuple<Types...>& t) {
  return select_tuple(
      t,
      insertion_sort_t<
          std::index_sequence_for<Types...>,
          metafun_of_nth_element<tuple<Types...>, Compare>::template Apply>());
}

template <typename F, typename... Types, std::size_t... Indices>
auto apply_impl(F&& f, const tuple<Types...>& t,
                std::index_sequence<Indices...>) {
  return std::invoke(std::forward<F>(f), get<Indices>(t)...);
}

template <typename F, typename... Types>
auto apply(F&& f, const tuple<Types...>& t) {
  return apply_impl(std::forward<F>(f), t, std::index_sequence_for<Types...>{});
}

}  // namespace jc

namespace jc {  // intefral_constant literal

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
  return std::integral_constant<int, parse_int<sizeof...(cs)>({cs...})>{};
}

}  // namespace jc

void test_make_tuple() {
  auto t = jc::make_tuple(42, 3.14, "downdemo");
  static_assert(std::is_same_v<decltype(jc::get<0>(t)), const int&>);
  static_assert(std::is_same_v<decltype(jc::get<1>(t)), const double&>);
  static_assert(std::is_same_v<decltype(jc::get<2>(t)), const char* const&>);
  assert(jc::get<0>(t) == 42);
  assert(jc::get<1>(t) == 3.14);
  assert(std::strcmp(jc::get<2>(t), "downdemo") == 0);

  using jc::operator"" _c;
  assert((t[0_c] == 42));
  assert((t[1_c] == 3.14));
  assert((std::strcmp(t[2_c], "downdemo") == 0));

  std::ostringstream os;
  os << t;
  assert(os.str() == "(42, 3.14, downdemo)");
}

void test_typelist() {
  jc::tuple<int, double, std::string> t{42, 3.14, "downdemo"};
  static_assert(std::is_same_v<jc::front_t<decltype(t)>, int>);
  static_assert(std::is_same_v<jc::pop_front_t<decltype(t)>,
                               jc::tuple<double, std::string>>);
  static_assert(std::is_same_v<jc::push_front_t<decltype(t), bool>,
                               jc::tuple<bool, int, double, std::string>>);
  static_assert(std::is_same_v<jc::push_back_t<decltype(t), bool>,
                               jc::tuple<int, double, std::string, bool>>);
  static_assert(std::is_same_v<jc::reverse_t<decltype(t)>,
                               jc::tuple<std::string, double, int>>);

  auto t2 = jc::pop_front_tuple(t);
  static_assert(std::is_same_v<decltype(t2), jc::tuple<double, std::string>>);
  assert(jc::get<0>(t2) == 3.14);
  assert(jc::get<1>(t2) == "downdemo");

  auto t3 = jc::push_front_tuple(t, true);
  static_assert(
      std::is_same_v<decltype(t3), jc::tuple<bool, int, double, std::string>>);
  assert(jc::get<0>(t3) == true);
  assert(jc::get<1>(t3) == 42);
  assert(jc::get<2>(t3) == 3.14);
  assert(jc::get<3>(t3) == "downdemo");

  auto t4 = jc::push_back_tuple(t, true);
  static_assert(
      std::is_same_v<decltype(t4), jc::tuple<int, double, std::string, bool>>);
  assert(jc::get<0>(t4) == 42);
  assert(jc::get<1>(t4) == 3.14);
  assert(jc::get<2>(t4) == "downdemo");
  assert(jc::get<3>(t4) == true);

  auto t5 = jc::reverse_tuple(t);
  static_assert(
      std::is_same_v<decltype(t5), jc::tuple<std::string, double, int>>);
  assert(jc::get<0>(t5) == "downdemo");
  assert(jc::get<1>(t5) == 3.14);
  assert(jc::get<2>(t5) == 42);

  auto t6 = jc::pop_back_tuple(t);
  static_assert(std::is_same_v<decltype(t6), jc::tuple<int, double>>);
  assert(jc::get<0>(t6) == 42);
  assert(jc::get<1>(t6) == 3.14);

  auto t7 = jc::splat_tuple<0, 3>(t);
  static_assert(std::is_same_v<decltype(t7), jc::tuple<int, int, int>>);
  assert(jc::get<0>(t7) == 42);
  assert(jc::get<1>(t7) == 42);
  assert(jc::get<2>(t7) == 42);
}

void test_sort_tuple() {
  auto t = jc::make_tuple(17LL, std::complex<double>(1, 2), 42, 3.14);
  auto t2 = jc::sort_tuple<jc::test::smaller>(t);
  static_assert(
      std::is_same_v<decltype(t),
                     jc::tuple<long long, std::complex<double>, int, double>>);
  static_assert(
      std::is_same_v<decltype(t2),
                     jc::tuple<int, double, long long, std::complex<double>>>);
  std::ostringstream os;
  os << t;
  assert(os.str() == "(17, (1,2), 42, 3.14)");
  os.str("");
  os << t2;
  assert(os.str() == "(42, 3.14, 17, (1,2))");
}

void test_apply() {
  std::ostringstream os;
  auto f = [&os](auto&&... args) { ((os << args << " "), ...); };
  auto t = jc::make_tuple(42, 3.14, "downdemo");
  jc::apply(f, t);
  assert(os.str() == "42 3.14 downdemo ");
}

void test_ebco() {
  struct A {};
  struct B {};
  // different results may be output on different platforms
  std::cout << sizeof(std::tuple<>) << std::endl;                        // 1
  std::cout << sizeof(std::tuple<A>) << std::endl;                       // 1
  std::cout << sizeof(std::tuple<A, B>) << std::endl;                    // 1
  std::cout << sizeof(std::tuple<A, A, B>) << std::endl;                 // 2
  std::cout << sizeof(std::tuple<A, B, A>) << std::endl;                 // 2
  std::cout << sizeof(std::tuple<A, A, B, B, char>) << std::endl;        // 3
  std::cout << sizeof(std::tuple<A, A, A, A>) << std::endl;              // 4
  std::cout << sizeof(std::tuple<A, B, A, A>) << std::endl;              // 3
  std::cout << sizeof(std::tuple<A, char, B, char, A, B>) << std::endl;  // 3
}

int main() {
  test_make_tuple();
  test_typelist();
  test_sort_tuple();
  test_apply();
  test_ebco();
}
