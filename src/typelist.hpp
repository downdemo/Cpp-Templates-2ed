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
