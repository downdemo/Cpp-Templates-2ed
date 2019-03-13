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
