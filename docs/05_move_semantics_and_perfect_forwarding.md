## 移动语义（Move Semantics）

* C++11 的[值类别](https://en.cppreference.com/w/cpp/language/value_category)包括左值（lvalue）、纯右值（prvalue）、亡值（xvalue），左值和亡值组成了泛左值（glvalue），纯右值和亡值组成了右值（rvalue）。为了让编译器识别接受右值作为参数的构造函数，则需要引入右值引用符号（&&），以区分移动构造函数和拷贝构造函数

```cpp
#include <cassert>
#include <string>
#include <utility>
#include <vector>

namespace jc {

struct A {
  A() : data(new std::string) {}
  A(const A& rhs) : data(new std::string{*rhs.data}) {}
  A(A&& rhs) noexcept : data(rhs.data) { rhs.data = nullptr; }
  ~A() { delete data; }

  std::string* data = nullptr;
};

}  // namespace jc

int main() {
  std::vector<jc::A> v;
  v.emplace_back(jc::A{});  // 调用默认构造函数、移动构造函数、析构函数
  jc::A a;
  v.emplace_back(a);  // 调用拷贝构造函数
  assert(a.data);
  v.emplace_back(std::move(a));  // 调用移动构造函数
  assert(!a.data);
}
```

* 右值引用即只能绑定到右值的引用，字面值（纯右值）和临时变量（亡值）就是常见的右值。如果把左值传递给右值引动参数，则需要强制类型转换，[std::move](https://en.cppreference.com/w/cpp/utility/move) 就是不需要显式指定类型的到右值引用的强制类型转换

```cpp
#include <cassert>
#include <string>
#include <type_traits>
#include <utility>

namespace jc {

template <typename T>
constexpr std::remove_reference_t<T>&& move(T&& x) noexcept {
  return static_cast<std::remove_reference_t<T>&&>(x);
}

constexpr int f(const std::string&) { return 1; }
constexpr int f(std::string&&) { return 2; }

}  // namespace jc

int main() {
  std::string s;
  static_assert(jc::f(s) == 1);
  assert(jc::f(std::string{}) == 2);
  static_assert(jc::f(static_cast<std::string&&>(s)) == 2);
  static_assert(jc::f(jc::move(s)) == 2);
  static_assert(jc::f(std::move(s)) == 2);
}
```

## 完美转发（Perfect Forwarding）

* 右值引用是能接受右值的引用，引用可以取址，是左值，因此右值引用是左值。如果一个函数接受右值引用参数，把参数传递给其他函数时，会按左值传递，这样就丢失了原有的值类别

```cpp
#include <cassert>
#include <string>
#include <utility>

namespace jc {

constexpr int f(const std::string&) { return 1; }
constexpr int f(std::string&&) { return 2; }
constexpr int g(std::string&& s) { return f(s); }

void test() {
  std::string s;
  assert(f(std::string{}) == 2);
  assert(g(std::string{}) == 1);
  static_assert(f(std::move(s)) == 2);
  static_assert(g(std::move(s)) == 1);
}

}  // namespace jc

int main() { jc::test(); }
```

* 为了转发时保持值类别不丢失，需要手写多个重载版本

```cpp
#include <cassert>
#include <string>
#include <utility>

namespace jc {

constexpr int f(std::string&) { return 1; }
constexpr int f(const std::string&) { return 2; }
constexpr int f(std::string&&) { return 3; }
constexpr int g(std::string& s) { return f(s); }
constexpr int g(const std::string& s) { return f(s); }
constexpr int g(std::string&& s) { return f(std::move(s)); }

void test() {
  std::string s;
  const std::string& s2 = s;
  static_assert(g(s) == 1);
  assert(g(s2) == 2);
  static_assert(g(std::move(s)) == 3);
  assert(g(std::string{}) == 3);
}

}  // namespace jc

int main() { jc::test(); }
```

* 模板参数中右值引用符号表示的是万能引用（universal reference），因为模板参数本身可以推断为引用，它可以匹配几乎任何类型（少部分特殊类型无法匹配，如位域），传入左值时推断为左值引用类型，传入右值时推断为右值引用类型。对万能引用参数使用 [std::forward](https://en.cppreference.com/w/cpp/utility/forward) 则可以保持值类别不丢失，这种保留值类别的转发手法就叫完美转发，因此万能引用也叫转发引用（forwarding reference）

```cpp
#include <cassert>
#include <string>
#include <type_traits>

namespace jc {

template <typename T>
constexpr T&& forward(std::remove_reference_t<T>& t) noexcept {
  return static_cast<T&&>(t);
}

constexpr int f(std::string&) { return 1; }
constexpr int f(const std::string&) { return 2; }
constexpr int f(std::string&&) { return 3; }

template <typename T>
constexpr int g(T&& s) {
  return f(jc::forward<T>(s));  // 等价于 std::forward
}

void test() {
  std::string s;
  const std::string& s2 = s;
  static_assert(g(s) == 1);             // T = T&& = std::string&
  assert(g(s2) == 2);                   // T = T&& = const std::string&
  static_assert(g(std::move(s)) == 3);  // T = std::string, T&& = std::string&&
  assert(g(std::string{}) == 3);        // T = T&& = std::string&
  assert(g("downdemo") == 3);           // T = T&& = const char (&)[9]
}

}  // namespace jc

int main() { jc::test(); }
```

* 结合变参模板完美转发转发任意数量的实参

```cpp
#include <iostream>
#include <string>
#include <type_traits>
#include <utility>

namespace jc {

template <typename F, typename... Args>
constexpr void constexpr_for(F&& f, Args&&... args) {
  (std::invoke(std::forward<F>(f), std::forward<Args>(args)), ...);
}

template <typename... Args>
void print(Args&&... args) {
  constexpr_for([](const auto& x) { std::cout << x << std::endl; },
                std::forward<Args>(args)...);
}

}  // namespace jc

int main() { jc::print(3.14, 42, std::string{"hello"}, "world"); }
```

* [Lambda](https://en.cppreference.com/w/cpp/language/lambda) 中使用完美转发需要借助 decltype 推断类型

```cpp
#include <iostream>
#include <string>
#include <type_traits>
#include <utility>

namespace jc {

constexpr auto constexpr_for = [](auto&& f, auto&&... args) {
  (std::invoke(std::forward<decltype(f)>(f),
               std::forward<decltype(args)>(args)),
   ...);
};

auto print = [](auto&&... args) {
  constexpr_for([](const auto& x) { std::cout << x << std::endl; },
                std::forward<decltype(args)>(args)...);
};

}  // namespace jc

int main() { jc::print(3.14, 42, std::string{"hello"}, "world"); }
```

* C++20 可以为 lambda 指定模板参数

```cpp
#include <iostream>
#include <string>
#include <type_traits>
#include <utility>

namespace jc {

constexpr auto constexpr_for = []<typename F, typename... Args>(
                                   F&& f, Args&&... args) {
  (std::invoke(std::forward<F>(f), std::forward<Args>(args)), ...);
};

auto print = []<typename... Args>(Args&&... args) {
  constexpr_for([](const auto& x) { std::cout << x << std::endl; },
                std::forward<Args>(args)...);
};

}  // namespace jc

int main() { jc::print(3.14, 42, std::string{"hello"}, "world"); }
```

* C++20 的 lambda 可以捕获参数包

```cpp
#include <iostream>
#include <string>
#include <type_traits>
#include <utility>

namespace jc {

template <typename... Args>
void print(Args&&... args) {
  [... args = std::forward<Args>(args)]<typename F>(F&& f) {
    (std::invoke(std::forward<F>(f), args), ...);
  }([](const auto& x) { std::cout << x << std::endl; });
}

}  // namespace jc

int main() { jc::print(3.14, 42, std::string{"hello"}, "world"); }
```

## 构造函数模板

* 模板也能用于构造函数，但它不是真正的构造函数，从函数模板实例化而来的函数不和普通函数等价，由成员函数模板实例化的函数不会重写虚函数，由构造函数模板实例化的构造函数不是拷贝或移动构造函数，但对一个 non-const 对象调用构造函数时，万能引用是更优先的匹配

```cpp
#include <string>
#include <utility>

namespace jc {

struct A {
  template <typename T>
  explicit A(T&& t) : s(std::forward<T>(t)) {}

  A(const A& rhs) : s(rhs.s) {}
  A(A&& rhs) noexcept : s(std::move(rhs.s)) {}

  std::string s;
};

}  // namespace jc

int main() {
  const jc::A a{"downdemo"};
  jc::A b{a};  // OK，匹配拷贝构造函数
  //   jc::A c{b};  // 错误，匹配模板构造函数
}
```

* 为此可以用 [std::enable_if](https://en.cppreference.com/w/cpp/types/enable_if) 约束模板参数，在条件满足的情况下才会匹配模板

```cpp
#include <string>
#include <type_traits>
#include <utility>

namespace jc {

struct A {
  template <typename T,  // 要求 T 能转为 std::string
            typename = std::enable_if_t<std::is_convertible_v<T, std::string>>>
  explicit A(T&& t) : s(std::forward<T>(t)) {}

  A(const A& rhs) : s(rhs.s) {}
  A(A&& rhs) noexcept : s(std::move(rhs.s)) {}

  std::string s;
};

}  // namespace jc

int main() {
  const jc::A a{"downdemo"};
  jc::A b{a};  // OK，匹配拷贝构造函数
  jc::A c{b};  // OK，匹配拷贝构造函数
}
```

* C++20 可以用 [concepts](https://en.cppreference.com/w/cpp/concepts) 约束模板参数


```cpp
#include <concepts>
#include <string>
#include <utility>

namespace jc {

struct A {
  template <typename T>
  requires std::convertible_to<T, std::string>
  explicit A(T&& t) : s(std::forward<T>(t)) {}

  A(const A& rhs) : s(rhs.s) {}
  A(A&& rhs) noexcept : s(std::move(rhs.s)) {}

  std::string s;
};

}  // namespace jc

int main() {
  const jc::A a{"downdemo"};
  jc::A b{a};  // OK，匹配拷贝构造函数
  jc::A c{b};  // OK，匹配拷贝构造函数
}
```
