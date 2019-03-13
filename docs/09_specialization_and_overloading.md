## [函数模板重载](https://en.cppreference.com/w/cpp/language/function_template#Function_template_overloading)

* 对于实参推断能匹配多个模板的情况，标准规定了偏序（partial ordering）规则，最终将调用最特殊（能接受更少类型）的模板

```cpp
#include <cassert>

namespace jc {

template <typename T>
int f(T) {
  return 1;
}

template <typename T>
int f(T*) {
  return 2;
}

}  // namespace jc

int main() {
  int* p = nullptr;
  assert(jc::f<int*>(p) == 1);
  assert(jc::f<int>(p) == 2);
  assert(jc::f(p) == 2);  // 两个模板均匹配，第二个模板更特殊
  assert(jc::f(0) == 1);  // 0 推断为 int，匹配第一个模板
  assert(jc::f(nullptr) == 1);  // nullptr 推断为 std::nullptr_t，匹配第一个模板
}
```

* 对于两个模板，用实参替代第一个模板的参数，替代后的结果作为实参去推断第二个模板，如果推断成功，反过来用第二个模板推断第一个模板，若推断失败，则第一个模板更特殊，如果均推断失败或推断成功，则两个模板没有偏序关系

```cpp
#include <cassert>

namespace jc {

template <typename T>
int f(T) {  // 1
  return 1;
}

template <typename T>
int f(T*) {  // 2
  return 2;
}

template <typename T>
int f(const T*) {  // 3
  return 3;
}

}  // namespace jc

int main() {
  const int* p = nullptr;
  assert(jc::f(p) == 3);
  // 推断结果：
  // 1: f(T)        [T = const int*]
  // 2: f(T*)       [T = const int]
  // 3: f(const T*) [T = int]
  // 偏序处理：
  // 用 2 推断 1，T = U*，推断成功
  // 用 1 推断 2，T* = U，无法推断 T
  // 2 比 1 特殊
  // 用 3 推断 1，T = const U*，推断成功
  // 用 1 推断 3，const T* = U，无法推断 T
  // 3 比 1 特殊
  // 用 3 推断 2，T = const U，推断成功
  // 用 2 推断 3，const T = U，无法推断 T
  // 3 比 2 特殊
  // 3 最特殊，因此调用 3
}
```

* 函数模板可以和非模板函数重载

```cpp
#include <iostream>

namespace jc {

struct A {
  A() = default;
  A(const A&) { std::cout << 1; }
  A(A&&) { std::cout << 2; }

  template <typename T>
  A(T&&) {
    std::cout << 3;
  }
};

}  // namespace jc

int main() {
  jc::A a1;
  jc::A a2{a1};  // 3，对 non-const 对象，成员模板优于拷贝构造函数
  jc::A a3{std::move(a1)};  // 2，移动构造函数
  const jc::A b1;
  jc::A b2{b1};             // 1，拷贝构造函数
  jc::A b3{std::move(b1)};  // 3，const A&&，更匹配成员模板
}
```

* 变参模板的重载

```cpp
namespace jc {

template <typename... Ts>
struct A {};

template <typename T>
constexpr int f(A<T*>) {
  return 1;
}

template <typename... Ts>
constexpr int f(A<Ts...>) {
  return 2;
}

template <typename... Ts>
constexpr int f(A<Ts*...>) {
  return 3;
}

static_assert(f(A<int*>{}) == 1);
static_assert(f(A<int, double>{}) == 2);
static_assert(f(A<int*, double*>{}) == 3);

}  // namespace jc

int main() {}
```

## [特化（Specialization）](https://en.cppreference.com/w/cpp/language/template_specialization)

* 函数模板特化引入了重载和实参推断，如果能推断特化版本，就可以不显式声明模板实参

```cpp
#include <cassert>

namespace jc {

template <typename T>
int f(T) {  // 1
  return 1;
}

template <typename T>
int f(T*) {  // 2
  return 2;
}

template <>
int f(int) {  // OK：1 的特化
  return 3;
}

template <>
int f(int*) {  // OK：2 的特化
  return 4;
}

}  // namespace jc

int main() {
  int* p = nullptr;
  assert(jc::f(p) == 4);
  assert(jc::f(0) == 3);
  assert(jc::f(nullptr) == 1);
}
```

* 函数模板的特化不能有默认实参，但会使用要被特化的模板的默认实参

```cpp
namespace jc {

template <typename T>
constexpr int f(T x = 1) {  // T 不会由默认实参推断
  return x;
}

template <>
constexpr int f(int x) {  // 不能指定默认实参
  return x + 1;
}

static_assert(f<int>() == 2);

}  // namespace jc

int main() {}
```

* 类模板特化的实参列表必须对应模板参数，如果有默认实参可以不指定对应参数。可以特化整个类模板，也可以特化部分成员。如果对某种类型特化类模板成员，就不能再特化整个类模板，其他未特化的成员会被保留

```cpp
#include <cassert>

namespace jc {

template <typename T, typename U = int>
struct A;

template <>
struct A<void> {
  constexpr int f();
};

constexpr int A<void>::f() { return 1; }

template <>
struct A<int, int> {
  int i = 0;
};

template <>
struct A<char, char> {
  template <typename T>
  struct B {
    int f() { return i; }
    static int i;
  };
};

template <typename T>
int A<char, char>::B<T>::i = 1;

template <>
int A<char, char>::B<double>::i = 2;

template <>
int A<char, char>::B<char>::f() {
  return 0;
};

// template <>
// struct A<char, char> {};  // 错误，不能对已经特化过成员的类型做特化

template <>
struct A<char, char>::B<bool> {
  int j = 3;
};

}  // namespace jc

int main() {
  static_assert(jc::A<void>{}.f() == 1);
  static_assert(jc::A<void, int>{}.f() == 1);
  // jc::A<void, double>{};  // 错误：未定义类型
  assert((jc::A<int, int>{}.i == 0));
  assert((jc::A<char, char>::B<int>{}.i == 1));
  assert((jc::A<char, char>::B<int>{}.f() == 1));
  assert((jc::A<char, char>::B<double>{}.i == 2));
  assert((jc::A<char, char>::B<double>{}.f() == 2));
  assert((jc::A<char, char>::B<char>{}.i == 1));
  assert((jc::A<char, char>::B<char>{}.f() == 0));
  assert((jc::A<char, char>::B<bool>{}.j == 3));
}
```

* 类模板特化必须在实例化之前，对已实例化的类型不能再进行特化

```cpp
namespace jc {

template <typename T>
struct A {};

A<int> a;

template <>
struct A<double> {};  // OK

template <>
struct A<int> {};  // 错误：不能特化已实例化的 A<int>

}  // namespace jc

int main() {}
```

## [偏特化（Partial Specialization）](https://en.cppreference.com/w/cpp/language/partial_specialization)

* 类模板偏特化限定一些类型，而非某个具体类型

```cpp
namespace jc {

template <typename T>
struct A;  // primary template

template <typename T>
struct A<const T> {};

template <typename T>
struct A<T*> {
  static constexpr int size = 0;
};

template <typename T, int N>
struct A<T[N]> {
  static constexpr int size = N;
};

template <typename Class>
struct A<int * Class::*> {
  static constexpr int i = 1;
};

template <typename T, typename Class>
struct A<T * Class::*> {
  static constexpr int i = 2;
};

template <typename Class>
struct A<void (Class::*)()> {
  static constexpr int i = 3;
};

template <typename RT, typename Class>
struct A<RT (Class::*)() const> {
  static constexpr int i = 4;
};

template <typename RT, typename Class, typename... Args>
struct A<RT (Class::*)(Args...)> {
  static constexpr int i = 5;
};

template <typename RT, typename Class, typename... Args>
struct A<RT (Class::*)(Args...) const noexcept> {
  static constexpr int i = 6;
};

struct B {
  int* i = nullptr;
  double* j = nullptr;
  void f1() {}
  constexpr int f2() const { return 0; }
  void f3(int&, double) {}
  void f4(int&, double) const noexcept {}
};

static_assert(A<decltype(&B::i)>::i == 1);
static_assert(A<decltype(&B::j)>::i == 2);
static_assert(A<decltype(&B::f1)>::i == 3);
static_assert(A<decltype(&B::f2)>::i == 4);
static_assert(A<decltype(&B::f3)>::i == 5);
static_assert(A<decltype(&B::f4)>::i == 6);

}  // namespace jc

int main() {
  int a[] = {1, 2, 3};
  static_assert(jc::A<decltype(&a)>::size == 0);
  static_assert(jc::A<decltype(a)>::size == 3);
  // jc::A<const int[3]>{};  // 错误：匹配多个版本
}
```

* [变量模板（variable template）](https://en.cppreference.com/w/cpp/language/variable_template) 的特化和偏特化

```cpp
#include <cassert>
#include <list>
#include <type_traits>
#include <vector>

namespace jc {

template <typename T>
constexpr int i = sizeof(T);

template <>
constexpr int i<void> = 0;

template <typename T>
constexpr int i<T&> = sizeof(void*);

static_assert(i<int> == sizeof(int));
static_assert(i<double> == sizeof(double));
static_assert(i<void> == 0);
static_assert(i<int&> == sizeof(void*));

// 变量模板特化的类型可以不匹配 primary template
template <typename T>
typename T::iterator null_iterator;

template <>
int* null_iterator<std::vector<int>> = nullptr;

template <typename T, std::size_t N>
T* null_iterator<T[N]> = nullptr;

}  // namespace jc

int main() {
  auto it1 = jc::null_iterator<std::vector<int>>;
  auto it2 = jc::null_iterator<std::list<int>>;
  auto it3 = jc::null_iterator<double[3]>;
  static_assert(std::is_same_v<decltype(it1), int*>);
  assert(!it1);
  static_assert(std::is_same_v<decltype(it2), std::list<int>::iterator>);
  static_assert(std::is_same_v<decltype(it3), double*>);
  assert(!it3);
}
```
