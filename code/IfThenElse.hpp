template <bool b, typename T, typename U>
struct IfThenElseT {
  using Type = T;
};

template <typename T, typename U>
struct IfThenElseT<false, T, U> {
  using Type = U;
};

template <bool b, typename T, typename U>
using IfThenElse = typename IfThenElseT<b, T, U>::Type;