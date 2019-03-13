#include <cstddef>
#include <functional>
#include <iostream>
#include <tuple>
#include <type_traits>
#include <utility>

namespace jc {

template <std::size_t... Index>
struct index_sequence {
  using type = index_sequence<Index...>;
};

template <typename List1, typename List2>
struct concat;

template <std::size_t... List1, std::size_t... List2>
struct concat<index_sequence<List1...>, index_sequence<List2...>>
    : index_sequence<List1..., (sizeof...(List1) + List2)...> {};

template <typename List1, typename List2>
using concat_t = typename concat<List1, List2>::type;

template <std::size_t N>
struct make_index_sequence_impl
    : concat_t<typename make_index_sequence_impl<N / 2>::type,
               typename make_index_sequence_impl<N - N / 2>::type> {};

template <>
struct make_index_sequence_impl<0> : index_sequence<> {};

template <>
struct make_index_sequence_impl<1> : index_sequence<0> {};

template <std::size_t N>
using make_index_sequence = typename make_index_sequence_impl<N>::type;

template <typename... Types>
using index_sequence_for = make_index_sequence<sizeof...(Types)>;

static_assert(std::is_same_v<make_index_sequence<3>, index_sequence<0, 1, 2>>);

template <typename F, typename... List, std::size_t... Index>
void apply_impl(F&& f, const std::tuple<List...>& t, index_sequence<Index...>) {
  std::invoke(std::forward<F>(f), std::get<Index>(t)...);
}

template <typename F, typename... List>
void apply(F&& f, const std::tuple<List...>& t) {
  apply_impl(std::forward<F>(f), t, index_sequence_for<List...>{});
}

}  // namespace jc

struct Print {
  template <typename... Args>
  void operator()(const Args&... args) {
    auto no_used = {(std::cout << args << " ", 0)...};
  }
};

int main() {
  auto t = std::make_tuple(3.14, 42, "hello world");
  jc::apply(Print{}, t);  // 3.14 42 hello world
}
