## 元编程（Metaprogramming）

* 元编程将计算在编译期完成，避免了运行期计算的开销

```cpp
#include <type_traits>

namespace jc {

template <int N, int... Ns>
struct max;

template <int N>
struct max<N> : std::integral_constant<int, N> {};

template <int N1, int N2, int... Ns>
struct max<N1, N2, Ns...>
    : std::integral_constant<int, (N1 < N2) ? max<N2, Ns...>::value
                                            : max<N1, Ns...>::value> {};

template <int... Ns>
inline constexpr auto max_v = max<Ns...>::value;

}  // namespace jc

static_assert(jc::max_v<3, 2, 1, 5, 4> == 5);

int main() {}
```

* 模板元编程通常使用偏特化和递归实现，由于编译期需要实例化代码，如果递归层次过深，会带来代码体积膨胀的问题

```cpp
#include <type_traits>

namespace jc {

template <int N, int L = 1, int R = N>
struct sqrt {
  static constexpr auto M = L + (R - L) / 2;
  static constexpr auto T = N / M;
  static constexpr auto value =  // 避免递归实例化所有分支
      std::conditional_t<(T < M), sqrt<N, L, M>, sqrt<N, M + 1, R>>::value;
};

template <int N, int M>
struct sqrt<N, M, M> {
  static constexpr auto value = M - 1;
};

template <int N>
inline constexpr auto sqrt_v = sqrt<N, 1, N>::value;

}  // namespace jc

static_assert(jc::sqrt_v<10000> == 100);

int main() {}
```

* C++14 支持 constexpr 函数，简化了实现并且没有递归实例化的代码膨胀问题

```cpp
namespace jc {

template <int N>
constexpr int sqrt() {
  if constexpr (N <= 1) {
    return N;
  }
  int l = 1;
  int r = N;
  while (l < r) {
    int m = l + (r - l) / 2;
    int t = N / m;
    if (m == t) {
      return m;
    } else if (m > t) {
      r = m;
    } else {
      l = m + 1;
    }
  }
  return l - 1;
}

}  // namespace jc

static_assert(jc::sqrt<10000>() == 100);

int main() {}
```

## 循环展开（Loop Unrolling）

* 在一些机器上，for 循环的汇编将产生分支指令

```cpp
#include <array>
#include <cassert>

namespace jc {

template <typename T, std::size_t N>
auto dot_product(const std::array<T, N>& lhs, const std::array<T, N>& rhs) {
  T res{};
  for (std::size_t i = 0; i < N; ++i) {
    res += lhs[i] * rhs[i];
  }
  return res;
}

}  // namespace jc

int main() {
  std::array<int, 3> a{1, 2, 3};
  std::array<int, 3> b{4, 5, 6};
  assert(jc::dot_product(a, b) == 32);
}
```

* 循环展开是一种牺牲体积加快程序执行速度的方法，现代编译器会优化循环为目标平台最高效形式。使用元编程可以展开循环，虽然已经没有必要，但还是给出实现

```cpp
#include <array>
#include <cassert>

namespace jc {

template <typename T, std::size_t N>
struct dot_product_impl {
  static T value(const T* lhs, const T* rhs) {
    return *lhs * *rhs + dot_product_impl<T, N - 1>::value(lhs + 1, rhs + 1);
  }
};

template <typename T>
struct dot_product_impl<T, 0> {
  static T value(const T*, const T*) { return T{}; }
};

template <typename T, std::size_t N>
auto dot_product(const std::array<T, N>& lhs, const std::array<T, N>& rhs) {
  return dot_product_impl<T, N>::value(&*std::begin(lhs), &*std::begin(rhs));
}

}  // namespace jc

int main() {
  std::array<int, 3> a{1, 2, 3};
  std::array<int, 3> b{4, 5, 6};
  assert(jc::dot_product(a, b) == 32);
}
```

## [Unit Type](https://en.wikipedia.org/wiki/Unit_type)

* [std::ratio](https://en.cppreference.com/w/cpp/numeric/ratio/ratio)

```cpp
#include <cassert>
#include <cmath>
#include <type_traits>

namespace jc {

template <int N, int D = 1>
struct ratio {
  static constexpr int num = N;
  static constexpr int den = D;
  using type = ratio<num, den>;
};

template <typename R1, typename R2>
struct ratio_add_impl {
 private:
  static constexpr int den = R1::den * R2::den;
  static constexpr int num = R1::num * R2::den + R2::num * R1::den;

 public:
  using type = ratio<num, den>;
};

template <typename R1, typename R2>
using ratio_add = typename ratio_add_impl<R1, R2>::type;

template <typename T, typename U = ratio<1>>
class duration {
 public:
  using rep = T;
  using period = typename U::type;

 public:
  constexpr duration(rep r = 0) : r_(r) {}
  constexpr rep count() const { return r_; }

 private:
  rep r_;
};

template <typename T1, typename U1, typename T2, typename U2>
constexpr auto operator+(const duration<T1, U1>& lhs,
                         const duration<T2, U2>& rhs) {
  using CommonType = ratio<1, ratio_add<U1, U2>::den>;
  auto res =
      (lhs.count() * U1::num / U1::den + rhs.count() * U2::num / U2::den) *
      CommonType::den;
  return duration<decltype(res), CommonType>{res};
}

}  // namespace jc

int main() {
  constexpr auto a = jc::duration<double, jc::ratio<1, 1000>>(10);  // 10 ms
  constexpr auto b = jc::duration<double, jc::ratio<1, 3>>(7.5);    // 2.5 s
  constexpr auto c = a + b;  // 10 * 3 + 7.5 * 1000 = 7530 * 1/3000 s
  assert(std::abs(c.count() - 7530) < 1e-6);
  static_assert(std::is_same_v<std::decay_t<decltype(c)>,
                               jc::duration<double, jc::ratio<1, 3000>>>);
  static_assert(decltype(c)::period::num == 1);
  static_assert(decltype(c)::period::den == 3000);
}
```

## Typelist

```cpp
// typelist.hpp

#pragma once

#include <type_traits>

namespace jc {

template <typename...>
struct typelist {};

template <typename List>
struct front;

template <typename Head, typename... Tail>
struct front<typelist<Head, Tail...>> {
  using type = Head;
};

template <typename List>
using front_t = typename front<List>::type;

// pop_front_t
template <typename List>
struct pop_front;

template <typename Head, typename... Tail>
struct pop_front<typelist<Head, Tail...>> {
  using type = typelist<Tail...>;
};

template <typename List>
using pop_front_t = typename pop_front<List>::type;

// push_front_t
template <typename List, typename NewElement>
struct push_front;

template <typename... Elements, typename NewElement>
struct push_front<typelist<Elements...>, NewElement> {
  using type = typelist<NewElement, Elements...>;
};

template <typename List, typename NewElement>
using push_front_t = typename push_front<List, NewElement>::type;

// nth_element_t
template <typename List, std::size_t N>
struct nth_element : nth_element<pop_front_t<List>, N - 1> {};

template <typename List>
struct nth_element<List, 0> : front<List> {};

template <typename List, std::size_t N>
using nth_element_t = typename nth_element<List, N>::type;

// is_empty
template <typename T>
struct is_empty {
  static constexpr bool value = false;
};

template <>
struct is_empty<typelist<>> {
  static constexpr bool value = true;
};

template <typename T>
inline constexpr bool is_empty_v = is_empty<T>::value;

// find_index_of_t
template <typename List, typename T, std::size_t N = 0,
          bool Empty = is_empty_v<List>>
struct find_index_of;

template <typename List, typename T, std::size_t N>
struct find_index_of<List, T, N, false>
    : std::conditional_t<std::is_same_v<front_t<List>, T>,
                         std::integral_constant<std::size_t, N>,
                         find_index_of<pop_front_t<List>, T, N + 1>> {};

template <typename List, typename T, std::size_t N>
struct find_index_of<List, T, N, true> {};

template <typename List, typename T>
using find_index_of_t = typename find_index_of<List, T>::type;

// push_back_t
template <typename List, typename NewElement, bool = is_empty_v<List>>
struct push_back_impl;

template <typename List, typename NewElement>
struct push_back_impl<List, NewElement, false> {
 private:
  using head = front_t<List>;
  using tail = pop_front_t<List>;
  using new_tail = typename push_back_impl<tail, NewElement>::type;

 public:
  using type = push_front_t<new_tail, head>;
};

template <typename List, typename NewElement>
struct push_back_impl<List, NewElement, true> {
  using type = push_front_t<List, NewElement>;
};

template <typename List, typename NewElement>
struct push_back : push_back_impl<List, NewElement> {};

/*
 * template <typename List, typename NewElement>
 * struct push_back;
 *
 * template <typename... Elements, typename NewElement>
 * struct push_back<typelist<Elements...>, NewElement> {
 * using type = typelist<Elements..., NewElement>;
 * };
 */

template <typename List, typename NewElement>
using push_back_t = typename push_back<List, NewElement>::type;

// reverse_t
template <typename List, bool Empty = is_empty_v<List>>
struct reverse;

template <typename List>
using reverse_t = typename reverse<List>::type;

template <typename List>
struct reverse<List, false>
    : push_back<reverse_t<pop_front_t<List>>, front_t<List>> {};

template <typename List>
struct reverse<List, true> {
  using type = List;
};

// pop_back_t
template <typename List>
struct pop_back {
  using type = reverse_t<pop_front_t<reverse_t<List>>>;
};

template <typename List>
using pop_back_t = typename pop_back<List>::type;

// largest_type_t
template <typename List, bool = is_empty_v<List>>
struct largest_type;

template <typename List>
struct largest_type<List, false> {
 private:
  using contender = front_t<List>;
  using best = typename largest_type<pop_front_t<List>>::type;

 public:
  using type =
      std::conditional_t<(sizeof(contender) >= sizeof(best)), contender, best>;
};

template <typename List>
struct largest_type<List, true> {
  using type = char;
};

template <typename List>
using largest_type_t = typename largest_type<List>::type;

// transform_t
template <typename List, template <typename T> class MetaFun,
          bool = is_empty_v<List>>
struct transform;

/*
 * template <typename List, template <typename T> class MetaFun>
 * struct transform<List, MetaFun, false>
 *     : push_front<typename transform<pop_front_t<List>, MetaFun>::type,
 *                  typename MetaFun<front_t<List>>::type> {};
 */

template <typename... Elements, template <typename T> class MetaFun>
struct transform<typelist<Elements...>, MetaFun, false> {
  using type = typelist<typename MetaFun<Elements>::type...>;
};

template <typename List, template <typename T> class MetaFun>
struct transform<List, MetaFun, true> {
  using type = List;
};

template <typename List, template <typename T> class MetaFun>
using transform_t = typename transform<List, MetaFun>::type;

// accumulate_t
template <typename List, template <typename T, typename U> class F,
          typename Init, bool = is_empty_v<List>>
struct accumulate;

template <typename List, template <typename T, typename U> class MetaFun,
          typename Init>
struct accumulate<List, MetaFun, Init, false>
    : accumulate<pop_front_t<List>, MetaFun,
                 typename MetaFun<Init, front_t<List>>::type> {};

template <typename List, template <typename T, typename U> class MetaFun,
          typename Init>
struct accumulate<List, MetaFun, Init, true> {
  using type = Init;
};

template <typename List, template <typename T, typename U> class MetaFun,
          typename Init>
using accumulate_t = typename accumulate<List, MetaFun, Init>::type;

// insert_sorted_t
template <typename T>
struct type_identity {
  using type = T;
};

template <typename List, typename Element,
          template <typename T, typename U> class Compare,
          bool = is_empty_v<List>>
struct insert_sorted;

template <typename List, typename Element,
          template <typename T, typename U> class Compare>
struct insert_sorted<List, Element, Compare, false> {
 private:
  // compute the tail of the resulting list:
  using new_tail = typename std::conditional_t<
      Compare<Element, front_t<List>>::value, type_identity<List>,
      insert_sorted<pop_front_t<List>, Element, Compare>>::type;

  // compute the head of the resulting list:
  using new_head = std::conditional_t<Compare<Element, front_t<List>>::value,
                                      Element, front_t<List>>;

 public:
  using type = push_front_t<new_tail, new_head>;
};

template <typename List, typename Element,
          template <typename T, typename U> class Compare>
struct insert_sorted<List, Element, Compare, true> : push_front<List, Element> {
};

template <typename List, typename Element,
          template <typename T, typename U> class Compare>
using insert_sorted_t = typename insert_sorted<List, Element, Compare>::type;

// insertion_sort_t
template <typename List, template <typename T, typename U> class Compare,
          bool = is_empty_v<List>>
struct insertion_sort;

template <typename List, template <typename T, typename U> class Compare>
using insertion_sort_t = typename insertion_sort<List, Compare>::type;

template <typename List, template <typename T, typename U> class Compare>
struct insertion_sort<List, Compare, false>
    : insert_sorted<insertion_sort_t<pop_front_t<List>, Compare>, front_t<List>,
                    Compare> {};

template <typename List, template <typename T, typename U> class Compare>
struct insertion_sort<List, Compare, true> {
  using type = List;
};

// multiply_t
template <typename T, typename U>
struct multiply;

template <typename T, T Value1, T Value2>
struct multiply<std::integral_constant<T, Value1>,
                std::integral_constant<T, Value2>> {
  using type = std::integral_constant<T, Value1 * Value2>;
};

template <typename T, typename U>
using multiply_t = typename multiply<T, U>::type;

// for std::index_sequence
template <std::size_t... Values>
struct is_empty<std::index_sequence<Values...>> {
  static constexpr bool value = sizeof...(Values) == 0;
};

template <std::size_t Head, std::size_t... Tail>
struct front<std::index_sequence<Head, Tail...>> {
  using type = std::integral_constant<std::size_t, Head>;
  static constexpr std::size_t value = Head;
};

template <std::size_t Head, std::size_t... Tail>
struct pop_front<std::index_sequence<Head, Tail...>> {
  using type = std::index_sequence<Tail...>;
};

template <std::size_t... Values, std::size_t New>
struct push_front<std::index_sequence<Values...>,
                  std::integral_constant<std::size_t, New>> {
  using type = std::index_sequence<New, Values...>;
};

template <std::size_t... Values, std::size_t New>
struct push_back<std::index_sequence<Values...>,
                 std::integral_constant<std::size_t, New>> {
  using type = std::index_sequence<Values..., New>;
};

// select_t
template <typename Types, typename Indices>
struct select;

template <typename Types, std::size_t... Indices>
struct select<Types, std::index_sequence<Indices...>> {
  using type = typelist<nth_element_t<Types, Indices>...>;
};

template <typename Types, typename Indices>
using select_t = typename select<Types, Indices>::type;

// Cons
struct Nil {};

template <typename Head, typename Tail = Nil>
struct Cons {
  using head = Head;
  using tail = Tail;
};

template <typename List>
struct front {
  using type = typename List::head;
};

template <typename List, typename Element>
struct push_front {
  using type = Cons<Element, List>;
};

template <typename List>
struct pop_front {
  using type = typename List::tail;
};

template <>
struct is_empty<Nil> {
  static constexpr bool value = true;
};

}  // namespace jc

namespace jc::test {

template <typename T, typename U>
struct smaller {
  static constexpr bool value = sizeof(T) < sizeof(U);
};

template <typename T, typename U>
struct less;

template <typename T, T M, T N>
struct less<std::integral_constant<T, M>, std::integral_constant<T, N>> {
  static constexpr bool value = M < N;
};

template <typename T, T... Values>
using integral_constant_typelist =
    typelist<std::integral_constant<T, Values>...>;

static_assert(std::is_same_v<integral_constant_typelist<std::size_t, 2, 3, 5>,
                             typelist<std::integral_constant<std::size_t, 2>,
                                      std::integral_constant<std::size_t, 3>,
                                      std::integral_constant<std::size_t, 5>>>);
static_assert(is_empty_v<typelist<>>);
using T1 = push_front_t<typelist<>, char>;
static_assert(std::is_same_v<T1, typelist<char>>);
static_assert(std::is_same_v<front_t<T1>, char>);
using T2 = push_front_t<T1, double>;
static_assert(std::is_same_v<T2, typelist<double, char>>);
static_assert(std::is_same_v<front_t<T2>, double>);
static_assert(std::is_same_v<pop_front_t<T2>, typelist<char>>);
using T3 = push_back_t<T2, int*>;
static_assert(std::is_same_v<T3, typelist<double, char, int*>>);
static_assert(std::is_same_v<nth_element_t<T3, 0>, double>);
static_assert(std::is_same_v<nth_element_t<T3, 1>, char>);
static_assert(std::is_same_v<nth_element_t<T3, 2>, int*>);
static_assert(std::is_same_v<find_index_of_t<T3, double>,
                             std::integral_constant<std::size_t, 0>>);
static_assert(std::is_same_v<find_index_of_t<T3, char>,
                             std::integral_constant<std::size_t, 1>>);
static_assert(std::is_same_v<find_index_of_t<T3, int*>,
                             std::integral_constant<std::size_t, 2>>);
static_assert(std::is_same_v<reverse_t<T3>, typelist<int*, char, double>>);
static_assert(std::is_same_v<pop_back_t<T3>, typelist<double, char>>);
static_assert(std::is_same_v<largest_type_t<T3>, double>);
static_assert(std::is_same_v<transform_t<T3, std::add_const>,
                             typelist<const double, const char, int* const>>);
static_assert(std::is_same_v<accumulate_t<T3, push_front, typelist<>>,
                             typelist<int*, char, double>>);
static_assert(std::is_same_v<insertion_sort_t<T3, smaller>,
                             typelist<char, int*, double>>);
static_assert(accumulate_t<integral_constant_typelist<int, 2, 3, 5>, multiply,
                           std::integral_constant<int, 1>>::value == 30);

static_assert(
    std::is_same_v<insertion_sort_t<std::index_sequence<2, 3, 0, 1>, less>,
                   std::index_sequence<0, 1, 2, 3>>);
static_assert(is_empty_v<std::index_sequence<>>);
static_assert(std::is_same_v<std::make_index_sequence<4>,
                             std::index_sequence<0, 1, 2, 3>>);
static_assert(front<std::make_index_sequence<4>>::value == 0);
static_assert(std::is_same_v<front_t<std::make_index_sequence<4>>,
                             std::integral_constant<std::size_t, 0>>);
static_assert(std::is_same_v<pop_front_t<std::make_index_sequence<4>>,
                             std::index_sequence<1, 2, 3>>);
static_assert(
    std::is_same_v<push_front_t<std::make_index_sequence<4>,
                                std::integral_constant<std::size_t, 4>>,
                   std::index_sequence<4, 0, 1, 2, 3>>);
static_assert(
    std::is_same_v<push_back_t<std::make_index_sequence<4>,
                               std::integral_constant<std::size_t, 4>>,
                   std::index_sequence<0, 1, 2, 3, 4>>);
static_assert(std::is_same_v<select_t<typelist<bool, char, int, double>,
                                      std::index_sequence<2, 3, 0, 1>>,
                             typelist<int, double, bool, char>>);

using ConsList = Cons<int, Cons<char, Cons<short, Cons<double>>>>;
static_assert(is_empty_v<Nil>);
static_assert(std::is_same_v<
              push_front_t<ConsList, bool>,
              Cons<bool, Cons<int, Cons<char, Cons<short, Cons<double>>>>>>);
static_assert(std::is_same_v<pop_front_t<ConsList>,
                             Cons<char, Cons<short, Cons<double>>>>);
static_assert(std::is_same_v<front_t<ConsList>, int>);
static_assert(std::is_same_v<insertion_sort_t<ConsList, smaller>,
                             Cons<char, Cons<short, Cons<int, Cons<double>>>>>);

}  // namespace jc::test
```

## [std::tuple](https://en.cppreference.com/w/cpp/utility/tuple)

```cpp
#include <cassert>
#include <complex>
#include <cstring>
#include <functional>
#include <ostream>
#include <sstream>
#include <string>
#include <type_traits>
#include <utility>

#include "typelist.hpp"

namespace jc {

template <typename... Types>
class tuple;

template <typename Head, typename... Tail>
class tuple<Head, Tail...> {
 public:
  tuple() = default;

  tuple(const Head& head, const tuple<Tail...>& tail)
      : head_(head), tail_(tail) {}

  template <
      typename VHead, typename... VTail,
      std::enable_if_t<sizeof...(VTail) == sizeof...(Tail), void*> = nullptr>
  tuple(VHead&& head, VTail&&... tail)
      : head_(std::forward<VHead>(head)), tail_(std::forward<VTail>(tail)...) {}

  template <
      typename VHead, typename... VTail,
      std::enable_if_t<sizeof...(VTail) == sizeof...(Tail), void*> = nullptr>
  tuple(const tuple<VHead, VTail...>& rhs)
      : head_(rhs.get_head()), tail_(rhs.get_tail()) {}

  // for push_back_tuple
  template <typename V, typename VHead, typename... VTail>
  tuple(const V& v, const tuple<VHead, VTail...>& rhs) : head_(v), tail_(rhs) {}

  Head& get_head() { return head_; }

  const Head& get_head() const { return head_; }

  tuple<Tail...>& get_tail() { return tail_; }

  const tuple<Tail...>& get_tail() const { return tail_; }

  template <typename T, T Index>
  auto& operator[](std::integral_constant<T, Index>);

 private:
  Head head_;
  tuple<Tail...> tail_;
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

namespace jc {  // integral_constant literal

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

int main() {
  test_make_tuple();
  test_typelist();
  test_sort_tuple();
  test_apply();
}
```

* 上述 tuple 的存储实现比实际需要使用了更多的空间，一个问题在于 tail 成员最终将是一个空 tuple，为此可使用 EBCO，派生自尾 tuple，而不是让它作为一个成员，这样可以消除一个字节的大小，[std::tuple](https://en.cppreference.com/w/cpp/utility/tuple) 采用了这种做法

```cpp
namespace jc {

template <typename... Types>
class tuple;

template <typename Head, typename... Tail>
class tuple<Head, Tail...> : private tuple<Tail...> {
 public:
  tuple() = default;

  tuple(const Head& head, const tuple<Tail...>& tail)
      : head_(head), TailElt(tail) {}

  template <
      typename VHead, typename... VTail,
      std::enable_if_t<sizeof...(VTail) == sizeof...(Tail), void*> = nullptr>
  tuple(VHead&& head, VTail&&... tail)
      : head_(std::forward<VHead>(head)),
        TailElt(std::forward<VTail>(tail)...) {}

  template <
      typename VHead, typename... VTail,
      std::enable_if_t<sizeof...(VTail) == sizeof...(Tail), void*> = nullptr>
  tuple(const tuple<VHead, VTail...>& rhs)
      : head_(rhs.get_head()), TailElt(rhs.get_tail()) {}

  // for push_back_tuple
  template <typename V, typename VHead, typename... VTail>
  tuple(const V& v, const tuple<VHead, VTail...>& rhs)
      : head_(v), TailElt(rhs) {}

  Head& get_head() { return head_; }

  const Head& get_head() const { return head_; }

  tuple<Tail...>& get_tail() { return *this; }

  const tuple<Tail...>& get_tail() const { return *this; }

  template <typename T, T Index>
  auto& operator[](std::integral_constant<T, Index>);

 private:
  Head head_;
  using TailElt = tuple<Tail...>;
};

template <>
class tuple<> {};

}  // namespace jc
```

* 由于 tail 在基类中，会先于 head 成员初始化，为此引入一个包裹 head 成员的类模板 tuple_elt，将其作为基类并置于 tail 之前。由于不能继承相同的类型，为此需要给 tuple_elt 一个额外的模板参数用于区分类型，以允许 tuple 有相同类型的元素

```cpp
namespace jc {

template <std::size_t N, typename T>
class tuple_elt {
 public:
  tuple_elt() = default;

  template <typename U>
  tuple_elt(U&& rhs) : value_(std::forward<U>(rhs)) {}

  T& get() { return value_; }
  const T& get() const { return value_; }

 private:
  T value_;
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

}  // namespace jc
```

* 让 tuple_elt 继承元素类型以进一步使用 EBCO

```cpp
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

}  // namespace jc
```

## [std::variant](https://en.cppreference.com/w/cpp/utility/variant)

```cpp
#include <cassert>
#include <exception>
#include <new>  // for std::launder()
#include <sstream>
#include <string>
#include <type_traits>
#include <utility>

#include "typelist.hpp"

namespace jc {

class computed_result_type;

template <typename Visitor, typename T>
using visit_element_result =
    decltype(std::declval<Visitor>()(std::declval<T>()));

template <typename R, typename Visitor, typename... Types>
struct visit_result {
  using type = R;
};

template <typename Visitor, typename... Types>
struct visit_result<computed_result_type, Visitor, Types...> {
  using type = std::common_type_t<visit_element_result<Visitor, Types>...>;
};

template <typename R, typename Visitor, typename... Types>
using visit_result_t = typename visit_result<R, Visitor, Types...>::type;

struct empty_variant : std::exception {};

template <typename R, typename V, typename Visitor, typename Head,
          typename... Tail>
R variant_visit_impl(V&& variant, Visitor&& vis, typelist<Head, Tail...>) {
  if (variant.template is<Head>()) {
    return static_cast<R>(std::forward<Visitor>(vis)(
        std::forward<V>(variant).template get<Head>()));
  } else if constexpr (sizeof...(Tail) > 0) {
    return variant_visit_impl<R>(std::forward<V>(variant),
                                 std::forward<Visitor>(vis),
                                 typelist<Tail...>{});
  } else {
    throw empty_variant();
  }
}

template <typename... Types>
class variant_storage {
 public:
  unsigned char get_discriminator() const { return discriminator_; }

  void set_discriminator(unsigned char d) { discriminator_ = d; }

  void* get_raw_buffer() { return buffer_; }

  const void* get_raw_buffer() const { return buffer_; }

  template <typename T>
  T* get_buffer_as() {
    return std::launder(reinterpret_cast<T*>(buffer_));
  }

  template <typename T>
  const T* get_buffer_as() const {
    return std::launder(reinterpret_cast<const T*>(buffer_));
  }

 private:
  using largest_t = largest_type_t<typelist<Types...>>;
  alignas(Types...) unsigned char buffer_[sizeof(largest_t)];
  unsigned char discriminator_ = 0;
};

template <typename... Types>
class variant;

template <typename T, typename... Types>
class variant_choice {
  using Derived = variant<Types...>;

  Derived& get_derived() { return *static_cast<Derived*>(this); }

  const Derived& get_derived() const {
    return *static_cast<const Derived*>(this);
  }

 protected:
  static constexpr unsigned Discriminator =
      find_index_of_t<typelist<Types...>, T>::value + 1;

 public:
  variant_choice() = default;

  variant_choice(const T& value) {
    new (get_derived().get_raw_buffer()) T(value);  // CRTP
    get_derived().set_discriminator(Discriminator);
  }

  variant_choice(T&& value) {
    new (get_derived().get_raw_buffer()) T(std::move(value));
    get_derived().set_discriminator(Discriminator);
  }

  bool destroy() {
    if (get_derived().get_discriminator() == Discriminator) {
      get_derived().template get_buffer_as<T>()->~T();
      return true;
    }
    return false;
  }

  Derived& operator=(const T& value) {
    if (get_derived().get_discriminator() == Discriminator) {
      *get_derived().template get_buffer_as<T>() = value;
    } else {
      get_derived().destroy();
      new (get_derived().get_raw_buffer()) T(value);
      get_derived().set_discriminator(Discriminator);
    }
    return get_derived();
  }

  Derived& operator=(T&& value) {
    if (get_derived().get_discriminator() == Discriminator) {
      *get_derived().template get_buffer_as<T>() = std::move(value);
    } else {
      get_derived().destroy();
      new (get_derived().get_raw_buffer()) T(std::move(value));
      get_derived().set_discriminator(Discriminator);
    }
    return get_derived();
  }
};

/*
 * class variant<int, double, std::string>
 *     : private variant_storage<int, double, std::string>,
 *       private variant_choice<int, int, double, std::string>,
 *       private variant_choice<double, int, double, std::string>,
 *       private variant_choice<std::string, int, double, std::string> {};
 *
 * variant_choice<int, int, double, std::string>::discriminator_ == 1;
 * variant_choice<double, int, double, std::string>::discriminator_ == 2;
 * variant_choice<std::string, int, double, std::string>::discriminator_ == 3;
 */
template <typename... Types>
class variant : private variant_storage<Types...>,
                private variant_choice<Types, Types...>... {
  template <typename T, typename... OtherTypes>
  friend class variant_choice;  // enable CRTP

 public:
  /*
   * ctor of variant<int, double, string>:
   * variant(const int&);
   * variant(int&&);
   * variant(const double&);
   * variant(double&&);
   * variant(const string&);
   * variant(string&&);
   */
  using variant_choice<Types, Types...>::variant_choice...;
  using variant_choice<Types, Types...>::operator=...;

  variant() { *this = front_t<typelist<Types...>>(); }

  variant(const variant& rhs) {
    if (!rhs.empty()) {
      rhs.visit([&](const auto& value) { *this = value; });
    }
  }

  variant(variant&& rhs) {
    if (!rhs.empty()) {
      std::move(rhs).visit([&](auto&& value) { *this = std::move(value); });
    }
  }

  template <typename... SourceTypes>
  variant(const variant<SourceTypes...>& rhs) {
    if (!rhs.empty()) {
      rhs.visit([&](const auto& value) { *this = value; });
    }
  }

  template <typename... SourceTypes>
  variant(variant<SourceTypes...>&& rhs) {
    if (!rhs.empty()) {
      std::move(rhs).visit([&](auto&& value) { *this = std::move(value); });
    }
  }

  variant& operator=(const variant& rhs) {
    if (!rhs.empty()) {
      rhs.visit([&](const auto& value) { *this = value; });
    } else {
      destroy();
    }
    return *this;
  }

  variant& operator=(variant&& rhs) {
    if (!rhs.empty()) {
      std::move(rhs).visit([&](auto&& value) { *this = std::move(value); });
    } else {
      destroy();
    }
    return *this;
  }

  template <typename... SourceTypes>
  variant& operator=(const variant<SourceTypes...>& rhs) {
    if (!rhs.empty()) {
      rhs.visit([&](const auto& value) { *this = value; });
    } else {
      destroy();
    }
    return *this;
  }

  template <typename... SourceTypes>
  variant& operator=(variant<SourceTypes...>&& rhs) {
    if (!rhs.empty()) {
      std::move(rhs).visit([&](auto&& value) { *this = std::move(value); });
    } else {
      destroy();
    }
    return *this;
  }

  bool empty() const { return this->get_discriminator() == 0; }

  ~variant() { destroy(); }

  void destroy() {
    (variant_choice<Types, Types...>::destroy(), ...);
    this->set_discriminator(0);
  }

  template <typename T>
  bool is() const {
    return this->get_discriminator() ==
           variant_choice<T, Types...>::Discriminator;
  }

  template <typename T>
  T& get() & {
    if (empty()) {
      throw empty_variant();
    }
    assert(is<T>());
    return *this->template get_buffer_as<T>();
  }

  template <typename T>
  const T& get() const& {
    if (empty()) {
      throw empty_variant();
    }
    assert(is<T>());
    return *this->template get_buffer_as<T>();
  }

  template <typename T>
  T&& get() && {
    if (empty()) {
      throw empty_variant();
    }
    assert(is<T>());
    return std::move(*this->template get_buffer_as<T>());
  }

  template <typename R = computed_result_type, typename Visitor>
  visit_result_t<R, Visitor, Types&...> visit(Visitor&& vis) & {
    using Result = visit_result_t<R, Visitor, Types&...>;
    return variant_visit_impl<Result>(*this, std::forward<Visitor>(vis),
                                      typelist<Types...>{});
  }

  template <typename R = computed_result_type, typename Visitor>
  visit_result_t<R, Visitor, const Types&...> visit(Visitor&& vis) const& {
    using Result = visit_result_t<R, Visitor, const Types&...>;
    return variant_visit_impl<Result>(*this, std::forward<Visitor>(vis),
                                      typelist<Types...>{});
  }

  template <typename R = computed_result_type, typename Visitor>
  visit_result_t<R, Visitor, Types&&...> visit(Visitor&& vis) && {
    using Result = visit_result_t<R, Visitor, Types&&...>;
    return variant_visit_impl<Result>(
        std::move(*this), std::forward<Visitor>(vis), typelist<Types...>{});
  }
};

}  // namespace jc

namespace jc::test {

struct copied_noncopyable : std::exception {};

struct noncopyable {
  noncopyable() = default;

  noncopyable(const noncopyable&) { throw copied_noncopyable(); }

  noncopyable(noncopyable&&) = default;

  noncopyable& operator=(const noncopyable&) { throw copied_noncopyable(); }

  noncopyable& operator=(noncopyable&&) = default;
};

template <typename V, typename Head, typename... Tail>
void print_impl(std::ostream& os, const V& v) {
  if (v.template is<Head>()) {
    os << v.template get<Head>();
  } else if constexpr (sizeof...(Tail) > 0) {
    print_impl<V, Tail...>(os, v);
  }
}

template <typename... Types>
void print(std::ostream& os, const variant<Types...>& v) {
  print_impl<variant<Types...>, Types...>(os, v);
}

}  // namespace jc::test

void test_variant() {
  jc::variant<int, double, std::string> v{42};
  assert(!v.empty());
  assert(v.is<int>());
  assert(v.get<int>() == 42);
  v = 3.14;
  assert(v.is<double>());
  assert(v.get<double>() == 3.14);
  v = "hello";
  assert(v.is<std::string>());
  assert(v.get<std::string>() == "hello");

  std::stringstream os;
  v.visit([&os](const auto& value) { os << value; });
  assert(os.str() == "hello");

  os.str("");
  jc::test::print(os, v);
  assert(os.str() == "hello");

  jc::variant<int, double, std::string> v2;
  assert(!v2.empty());
  assert(v2.is<int>());
  v2 = std::move(v);
  assert(v.is<std::string>());
  assert(v.get<std::string>().empty());
  assert(v2.is<std::string>());
  assert(v2.get<std::string>() == "hello");
  v2.destroy();
  assert(v2.empty());
}

void test_noncopyable() {
  jc::variant<int, jc::test::noncopyable> v(42);
  try {
    jc::test::noncopyable nc;
    v = nc;
  } catch (jc::test::copied_noncopyable) {
    assert(!v.is<int>() && !v.is<jc::test::noncopyable>());
  }
}

int main() {
  test_variant();
  test_noncopyable();
}
```
