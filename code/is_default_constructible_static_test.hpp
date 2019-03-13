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

template <typename T>
struct is_default_constructible {
 private:
  template <typename U, typename = decltype(U())>
  static true_type test(void*);

  template <typename>
  static false_type test(...);

 public:
  static constexpr bool value = decltype(test<T>(nullptr))::value;
};