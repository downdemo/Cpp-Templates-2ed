#include <tuple>
#include <utility>

template <typename T, T... Ns>
struct ValueList {};

template <size_t... Ns>
using IndexList = ValueList<size_t, Ns...>;

template <typename List, typename T, T N>
class PushFrontT;

template <typename T, T... Ns, T N>
struct PushFrontT<ValueList<T, Ns...>, T, N> {
  using Type = ValueList<T, N, Ns...>;
};

template <typename List, typename T, T N>
using PushFront = typename PushFrontT<List, T, N>::Type;

template <size_t N, typename T = ValueList<size_t>>
struct MakeIndexListT : MakeIndexListT<N - 1, PushFront<T, size_t, N - 1>> {};

template <typename T>
struct MakeIndexListT<0, T> {
  using Type = T;
};

template <size_t N>
using MakeIndexList = typename MakeIndexListT<N>::Type;

template <typename F, typename... Ts, size_t... I>
void applyImpl(F&& f, const std::tuple<Ts...>& t, ValueList<size_t, I...>) {
  std::forward<F>(f)(std::get<I>(t)...);
}

template <typename F, typename... Ts, size_t N = sizeof...(Ts)>
void apply(F&& f, const std::tuple<Ts...>& t) {
  applyImpl(std::forward<F>(f), t, MakeIndexList<N>{});
}