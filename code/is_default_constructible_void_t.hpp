template <class T, T v>
struct integral_constant {
  static constexpr T value = v;
  using value_type = T;
  using type = integral_constant<T, v>;
  constexpr operator value_type() const noexcept { return value; }
  constexpr value_type operator()() const noexcept { return value; }
};

template <bool B>
using bool_constant = integral_constant<bool, B>;

using true_type = bool_constant<true>;
using false_type = bool_constant<false>;

template <typename...>
using void_t = void;

template <typename, typename = void_t<>>
struct is_default_constructible : false_type {};

template <typename T>
struct is_default_constructible<T, void_t<decltype(T())>> : true_type {};

template <typename T>
constexpr bool is_default_constructible_v = is_default_constructible<T>::value;