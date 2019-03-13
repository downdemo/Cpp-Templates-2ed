#include <any>
#include <cassert>
#include <exception>
#include <type_traits>

namespace jc {

template <typename T>
class is_equality_comparable {
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

template <typename T, bool = is_equality_comparable<T>::value>
struct try_equals {
  static bool equals(const T& lhs, const T& rhs) { return lhs == rhs; }
};

struct not_equality_comparable : std::exception {};

template <typename T>
struct try_equals<T, false> {
  static bool equals(const T& lhs, const T& rhs) {
    throw not_equality_comparable();
  }
};

template <typename R, typename... Args>
class functor_bridge {
 public:
  virtual ~functor_bridge() {}
  virtual functor_bridge* clone() const = 0;
  virtual R invoke(Args... args) const = 0;
  virtual bool equals(const functor_bridge*) const = 0;
};

template <typename F, typename R, typename... Args>
class functor_bridge_impl : public functor_bridge<R, Args...> {
 public:
  template <typename T>
  functor_bridge_impl(T&& f) : f_(std::forward<T>(f)) {}

  virtual functor_bridge_impl* clone() const override {
    return new functor_bridge_impl(f_);
  }

  virtual R invoke(Args... args) const override {
    return f_(std::forward<Args>(args)...);
  }

  virtual bool equals(const functor_bridge<R, Args...>* rhs) const override {
    if (auto p = dynamic_cast<const functor_bridge_impl*>(rhs)) {
      return try_equals<F>::equals(f_, p->f_);
    }
    return false;
  }

 private:
  F f_;
};

template <typename>
class function;

template <typename R, typename... Args>
class function<R(Args...)> {
  friend bool operator==(const function& lhs, const function& rhs) {
    if (!lhs || !rhs) {
      return !lhs && !rhs;
    }
    return lhs.bridge_->equals(rhs.bridge_);
  }

  friend bool operator!=(const function& lhs, const function& rhs) {
    return !(lhs == rhs);
  }

  friend void swap(function& lhs, function& rhs) noexcept {
    std::swap(lhs.bridge_, rhs.bridge_);
  }

 public:
  function() = default;

  function(const function& rhs) {
    if (rhs.bridge_) {
      bridge_ = rhs.bridge_->clone();
    }
  }

  function(function& rhs) : function(static_cast<const function&>(rhs)) {}

  function(function&& rhs) noexcept : bridge_(rhs.bridge_) {
    rhs.bridge_ = nullptr;
  }

  template <typename F>
  function(F&& f) {
    using Bridge = functor_bridge_impl<std::decay_t<F>, R, Args...>;
    bridge_ = new Bridge(std::forward<F>(f));  // type erasure
  }

  ~function() { delete bridge_; }

  function& operator=(const function& rhs) {
    function tmp(rhs);
    swap(*this, tmp);
    return *this;
  }

  function& operator=(function&& rhs) noexcept {
    delete bridge_;
    bridge_ = rhs.bridge_;
    rhs.bridge_ = nullptr;
    return *this;
  }

  template <typename F>
  function& operator=(F&& rhs) {
    function tmp(std::forward<F>(rhs));
    swap(*this, tmp);
    return *this;
  }

  explicit operator bool() const { return bridge_ == nullptr; }

  R operator()(Args... args) const {
    return bridge_->invoke(std::forward<Args>(args)...);
  }

 private:
  functor_bridge<R, Args...>* bridge_ = nullptr;
};

}  // namespace jc

int main() {
  jc::function<bool(int)> f = [](const std::any& a) -> int {
    return std::any_cast<int>(a);
  };
  assert(f(3.14) == 1);
}
