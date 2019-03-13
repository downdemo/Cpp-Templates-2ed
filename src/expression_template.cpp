#include <cassert>
#include <cstddef>
#include <type_traits>

namespace jc {

template <typename T>
class SArray {
 public:
  explicit SArray(std::size_t sz) : data_(new T[sz]), sz_(sz) { init(); }

  SArray(const SArray<T>& rhs) : data_(new T[rhs.sz_]), sz_(rhs.sz_) {
    copy(rhs);
  }

  SArray<T>& operator=(const SArray<T>& rhs) {
    if (&rhs != this) {
      copy(rhs);
    }
    return *this;
  }

  ~SArray() { delete[] data_; }

  std::size_t size() const { return sz_; }

  T& operator[](std::size_t i) { return data_[i]; }

  const T& operator[](std::size_t i) const { return data_[i]; }

  SArray<T>& operator+=(const SArray<T>& rhs) {
    assert(sz_ == rhs.sz_);
    for (std::size_t i = 0; i < sz_; ++i) {
      (*this)[i] += rhs[i];
    }
    return *this;
  }

  SArray<T>& operator*=(const SArray<T>& rhs) {
    assert(sz_ == rhs.sz_);
    for (std::size_t i = 0; i < sz_; ++i) {
      (*this)[i] *= rhs[i];
    }
    return *this;
  }

  SArray<T>& operator*=(const T& rhs) {
    for (std::size_t i = 0; i < sz_; ++i) {
      (*this)[i] *= rhs;
    }
    return *this;
  }

 protected:
  void init() {
    for (std::size_t i = 0; i < sz_; ++i) {
      data_[i] = T{};
    }
  }

  void copy(const SArray<T>& rhs) {
    assert(sz_ == rhs.sz_);
    for (std::size_t i = 0; i < sz_; ++i) {
      data_[i] = rhs.data_[i];
    }
  }

 private:
  T* data_;
  std::size_t sz_;
};

template <typename T>
SArray<T> operator+(const SArray<T>& lhs, const SArray<T>& rhs) {
  assert(lhs.size() == rhs.size());
  SArray<T> res{lhs.size()};
  for (std::size_t i = 0; i < lhs.size(); ++i) {
    res[i] = lhs[i] + rhs[i];
  }
  return res;
}

template <typename T>
SArray<T> operator*(const SArray<T>& lhs, const SArray<T>& rhs) {
  assert(lhs.size() == rhs.size());
  SArray<T> res{lhs.size()};
  for (std::size_t i = 0; i < lhs.size(); ++i) {
    res[i] = lhs[i] * rhs[i];
  }
  return res;
}

template <typename T>
SArray<T> operator*(const T& lhs, const SArray<T>& rhs) {
  SArray<T> res{rhs.size()};
  for (std::size_t i = 0; i < rhs.size(); ++i) {
    res[i] = lhs * rhs[i];
  }
  return res;
}

template <typename T>
class A_Scalar {
 public:
  constexpr A_Scalar(const T& v) : value_(v) {}

  constexpr const T& operator[](std::size_t) const { return value_; }

  constexpr std::size_t size() const { return 0; };

 private:
  const T& value_;
};

template <typename T>
struct A_Traits {
  using type = const T&;
};

template <typename T>
struct A_Traits<A_Scalar<T>> {
  using type = A_Scalar<T>;
};

template <typename T, typename OP1, typename OP2>
class A_Add {
 public:
  A_Add(const OP1& op1, const OP2& op2) : op1_(op1), op2_(op2) {}

  T operator[](std::size_t i) const { return op1_[i] + op2_[i]; }

  std::size_t size() const {
    assert(op1_.size() == 0 || op2_.size() == 0 || op1_.size() == op2_.size());
    return op1_.size() != 0 ? op1_.size() : op2_.size();
  }

 private:
  typename A_Traits<OP1>::type op1_;
  typename A_Traits<OP2>::type op2_;
};

template <typename T, typename OP1, typename OP2>
class A_Mult {
 public:
  A_Mult(const OP1& op1, const OP2& op2) : op1_(op1), op2_(op2) {}

  T operator[](std::size_t i) const { return op1_[i] * op2_[i]; }

  std::size_t size() const {
    assert(op1_.size() == 0 || op2_.size() == 0 || op1_.size() == op2_.size());
    return op1_.size() != 0 ? op1_.size() : op2_.size();
  }

 private:
  typename A_Traits<OP1>::type op1_;
  typename A_Traits<OP2>::type op2_;
};

template <typename T, typename A1, typename A2>
class A_Subscript {
 public:
  A_Subscript(const A1& a1, const A2& a2) : a1_(a1), a2_(a2) {}

  T& operator[](std::size_t i) {
    return const_cast<T&>(a1_[static_cast<std::size_t>(a2_[i])]);
  }

  decltype(auto) operator[](std::size_t i) const {
    return a1_[static_cast<std::size_t>(a2_[i])];
  }

  std::size_t size() const { return a2_.size(); }

 private:
  const A1& a1_;
  const A2& a2_;
};

}  // namespace jc

namespace jc::test {

template <typename T, typename Rep = SArray<T>>
class Array {
 public:
  explicit Array(std::size_t i) : r_(i) {}

  Array(const Rep& rhs) : r_(rhs) {}

  Array& operator=(const Array& rhs) {
    assert(size() == rhs.size());
    for (std::size_t i = 0; i < rhs.size(); ++i) {
      r_[i] = rhs[i];
    }
    return *this;
  }

  template <typename T2, typename Rep2>
  Array& operator=(const Array<T2, Rep2>& rhs) {
    assert(size() == rhs.size());
    for (std::size_t i = 0; i < rhs.size(); ++i) {
      r_[i] = rhs[i];
    }
    return *this;
  }

  std::size_t size() const { return r_.size(); }

  T& operator[](std::size_t i) {
    assert(i < size());
    return r_[i];
  }

  decltype(auto) operator[](std::size_t i) const {
    assert(i < size());
    return r_[i];
  }

  template <typename T2, typename Rep2>
  Array<T, A_Subscript<T, Rep, Rep2>> operator[](const Array<T2, Rep2>& rhs) {
    return Array<T, A_Subscript<T, Rep, Rep2>>{
        A_Subscript<T, Rep, Rep2>{this->rep(), rhs.rep()}};
  }

  template <typename T2, typename Rep2>
  decltype(auto) operator[](const Array<T2, Rep2>& rhs) const {
    return Array<T, A_Subscript<T, Rep, Rep2>>{
        A_Subscript<T, Rep, Rep2>{this->rep(), rhs.rep()}};
  }

  Rep& rep() { return r_; }

  const Rep& rep() const { return r_; }

 private:
  Rep r_;
};

template <typename T, typename R1, typename R2>
Array<T, A_Add<T, R1, R2>> operator+(const Array<T, R1>& lhs,
                                     const Array<T, R2>& rhs) {
  return Array<T, A_Add<T, R1, R2>>{A_Add<T, R1, R2>{lhs.rep(), rhs.rep()}};
}

template <typename T, typename R1, typename R2>
Array<T, A_Mult<T, R1, R2>> operator*(const Array<T, R1>& lhs,
                                      const Array<T, R2>& rhs) {
  return Array<T, A_Mult<T, R1, R2>>{A_Mult<T, R1, R2>{lhs.rep(), rhs.rep()}};
}

template <typename T, typename R2>
Array<T, A_Mult<T, A_Scalar<T>, R2>> operator*(const T& lhs,
                                               const Array<T, R2>& rhs) {
  return Array<T, A_Mult<T, A_Scalar<T>, R2>>{
      A_Mult<T, A_Scalar<T>, R2>{A_Scalar<T>(lhs), rhs.rep()}};
}

}  // namespace jc::test

int main() {
  constexpr std::size_t sz = 1000;
  constexpr double a = 10;
  constexpr double b = 2;
  jc::test::Array<double> x{sz};
  jc::test::Array<double> y{sz};
  assert(x.size() == sz);
  assert(y.size() == sz);
  for (std::size_t i = 0; i < sz; ++i) {
    x[i] = a;
    y[i] = b;
  }
  x = 1.2 * x + x * y;
  static_assert(std::is_same_v<
                decltype(1.2 * x),
                jc::test::Array<double, jc::A_Mult<double, jc::A_Scalar<double>,
                                                   jc::SArray<double>>>>);
  static_assert(std::is_same_v<
                decltype(x * y),
                jc::test::Array<double, jc::A_Mult<double, jc::SArray<double>,
                                                   jc::SArray<double>>>>);

  static_assert(
      std::is_same_v<
          decltype(1.2 * x + x * y),
          jc::test::Array<double,
                          jc::A_Add<double,
                                    jc::A_Mult<double, jc::A_Scalar<double>,
                                               jc::SArray<double>>,
                                    jc::A_Mult<double, jc::SArray<double>,
                                               jc::SArray<double>>>>>);

  for (std::size_t i = 0; i < sz; ++i) {
    assert(x[i] == 1.2 * a + a * b);
    y[i] = static_cast<double>(i);
  }

  /*
   * x[y] = 2.0 * x[y] equals to:
   * for (std::size_t i = 0; i < y.size(); ++i) {
   *   x[y[i]] = 2 * x[y[i]];
   * }
   */
  x[y] = 2.0 * x[y];
  for (std::size_t i = 0; i < sz; ++i) {
    assert(x[i] == 2.0 * (1.2 * a + a * b));
  }
}
