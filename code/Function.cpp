#include <cassert>
#include <iostream>
#include <type_traits>
#include <vector>

template <typename T>
class IsEqualityComparable {
 private:
  static void* conv(bool);
  template <typename U>
  static std::true_type test(
      decltype(conv(std::declval<const U&>() == std::declval<const U&>())),
      decltype(conv(!(std::declval<const U&>() == std::declval<const U&>()))));

  template <typename U>
  static std::false_type test(...);

 public:
  static constexpr bool value = decltype(test<T>(nullptr, nullptr))::value;
};

template <typename T, bool EqComparable = IsEqualityComparable<T>::value>
struct TryEquals {
  static bool equals(const T& x1, const T& x2) { return x1 == x2; }
};

class NotEqualityComparable : public std::exception {};

template <typename T>
struct TryEquals<T, false> {
  static bool equals(const T& x1, const T& x2) {
    throw NotEqualityComparable();
  }
};

template <typename R, typename... Args>
class B {
 public:
  virtual ~B() {}
  virtual B* clone() const = 0;
  virtual R invoke(Args... args) const = 0;
  virtual bool equals(const B* fb) const = 0;
};

template <typename F, typename R, typename... Args>
class X : public B<R, Args...> {
 private:
  F f;

 public:
  template <typename T>
  X(T&& f) : f(std::forward<T>(f)) {}

  virtual X* clone() const override { return new X(f); }

  virtual R invoke(Args... args) const override {
    return f(std::forward<Args>(args)...);
  }

  virtual bool equals(const B<R, Args...>* fb) const override {
    if (auto specFb = dynamic_cast<const X*>(fb)) {
      return TryEquals<F>::equals(f, specFb->f);
    }
    return false;
  }
};

template <typename Signature>
class A;

template <typename R, typename... Args>
class A<R(Args...)> {
 private:
  B<R, Args...>* bridge;  // 该指针负责管理函数对象
 public:
  A() : bridge(nullptr) {}

  A(const A& other) : bridge(nullptr) {
    if (other.bridge) {
      bridge = other.bridge->clone();
    }
  }

  A(A& other) : A(static_cast<const A&>(other)) {}

  A(A&& other) noexcept : bridge(other.bridge) { other.bridge = nullptr; }

  template <typename F>
  A(F&& f)
      : bridge(nullptr)  // 从任意函数对象构造
  {
    using Functor = std::decay_t<F>;
    using Bridge = X<Functor, R, Args...>;  // X的实例化存储一个函数对象副本
    bridge =
        new Bridge(std::forward<F>(f));  // 派生类到基类的转换，F丢失，类型擦除
  }

  A& operator=(const A& other) {
    A tmp(other);
    swap(*this, tmp);
    return *this;
  }

  A& operator=(A&& other) noexcept {
    delete bridge;
    bridge = other.bridge;
    other.bridge = nullptr;
    return *this;
  }

  template <typename F>
  A& operator=(F&& f) {
    A tmp(std::forward<F>(f));
    swap(*this, tmp);
    return *this;
  }

  ~A() { delete bridge; }

  friend void swap(A& fp1, A& fp2) noexcept {
    std::swap(fp1.bridge, fp2.bridge);
  }

  explicit operator bool() const { return bridge == nullptr; }

  R operator()(Args... args) const {
    return bridge->invoke(std::forward<Args>(args)...);
  }

  friend bool operator==(const A& f1, const A& f2) {
    if (!f1 || !f2) {
      return !f1 && !f2;
    }
    return f1.bridge->equals(f2.bridge);  // equals要求operator==
  }

  friend bool operator!=(const A& f1, const A& f2) { return !(f1 == f2); }
};

void forUpTo(int n, A<void(int)> f) {
  for (int i = 0; i < n; ++i) {
    f(i);
  }
}

void print(int i) { std::cout << i << ' '; }

int main() {
  forUpTo(5, print);  // 0 1 2 3 4
  std::vector<int> v;
  A<void(int)> a = [&v](int i) { v.emplace_back(i); };
  A<void(int)> b = [](int) {};
  assert(a == b);
  forUpTo(5, a);
  for (auto& x : v) {
    std::cout << x;  // 01234
  }
}
