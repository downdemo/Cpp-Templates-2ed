## [函数模板](https://en.cppreference.com/w/cpp/language/function_template)

* 编写时不指定具体类型，直到使用时才能确定，这个概念就是泛型。模板，顾名思义，编写一次即可适用于任意类型。模板定义以关键词 template 开始，后跟一个模板参数列表，类型参数前必须使用关键字 typename 或 class，在模板参数列表中这两个关键字含义相同，可以互换使用。函数模板通常不用声明为 inline，唯一例外的是特定类型的全特化，因为编译器可能忽略 inline，函数模板是否内联取决于编译器的优化策略

```cpp
#include <cassert>
#include <string>

namespace jc {

template <typename T>
T max(const T& a, const T& b) {
  return a < b ? b : a;
}

}  // namespace jc

int main() {
  assert(jc::max<int>(1, 3) == 3);
  assert(jc::max<double>(1.0, 3.14) == 3.14);
  std::string s1 = "down";
  std::string s2 = "demo";
  assert(jc::max<std::string>(s1, s2) == "down");
}
```

## 两阶段编译（Two-Phase Translation）

* 模板编译分为实例化前检查和实例化两个阶段。实例化前检查模板代码本身，包括
  * 检查语法是否正确，如是否遗漏分号
  * 检查是否使用不依赖于模板参数的未知名称，如未声明的类型名、函数名
  * 检查不依赖于模板参数的静态断言

```cpp
template <typename T>
void f(T x) {
  undeclared();  // 一阶段编译错误，未声明的函数
  static_assert(sizeof(int) > 10);  // 一阶段，sizeof(int) <= 10，总会编译失败
}

int main() {}
```

* 实例化期间保证代码有效，比如对不能解引用的类型进行解引用就会实例化出错，此外会再次检查依赖于模板参数的部分

```cpp
template <typename T>
void f(T x) {
  undeclared(x);  // 调用 undeclared(T) 才会出现函数未声明的实例化错误
  static_assert(sizeof(T) > 10);  // 如果 sizeof(T) <= 10 则实例化错误
}

int main() {
  f(42);  // 调用函数才会进行实例化，不调用则不会有实例化错误
}
```

## [模板实参推断（Template Argument Deduction）](https://en.cppreference.com/w/cpp/language/template_argument_deduction)

* 调用模板时，如果不显式指定模板参数类型，则编译器会根据传入的实参推断模板参数类型

```cpp
#include <cassert>
#include <string>

namespace jc {

template <typename T>
T max(const T& a, const T& b) {
  return a < b ? b : a;
}

}  // namespace jc

int main() {
  assert(jc::max(1, 3) == 3);          // T 推断为 int
  assert(jc::max(1.0, 3.14) == 3.14);  // T 推断为 double
  std::string s1 = "down";
  std::string s2 = "demo";
  assert(jc::max(s1, s2) == "down");  // T 推断为 std::string
}
```

* 实参的推断要求一致，其本身不会为了编译通过自动做类型转换

```cpp
#include <cassert>

namespace jc {

template <typename T>
T max(const T& a, const T& b) {
  return a < b ? b : a;
}

}  // namespace jc

int main() {
  jc::max(1, 3.14);  // 错误，T 分别推断出 int 和 double，类型不明确
}
```

* 字符串字面值传引用会推断为字符数组（传值则推断为 `const char*`，数组和函数会 decay 为指针）

```cpp
#include <cassert>
#include <string>

namespace jc {

template <typename T, typename U>
T max(const T& a, const U& b) {
  return a < b ? b : a;
}

}  // namespace jc

int main() {
  std::string s = "down";
  jc::max("down", s);  // 错误，T 推断为 char[5] 和 std::string
}
```

* 对于推断不一致的情况，可以显式指定类型而不使用推断机制，或者强制转换实参为希望的类型使得推断结果一致

```cpp
#include <cassert>
#include <string>

namespace jc {

template <typename T, typename U>
T max(const T& a, const U& b) {
  return a < b ? b : a;
}

}  // namespace jc

int main() {
  std::string s = "demo";
  assert(jc::max<std::string>("down", "demo") == "down");
  assert(jc::max(std::string{"down"}, s) == "down");
}
```

* 也可以增加一个模板参数，这样每个实参的推断都是独立的，不会出现矛盾

```cpp
#include <cassert>

namespace jc {

template <typename T, typename U>
T max(const T& a, const U& b) {
  return a < b ? b : a;
}

}  // namespace jc

int main() {
  assert(jc::max(1, 3.14) == 3);  // T 推断为 int，返回值截断为 int
  assert(jc::max<double>(1, 3.14) == 3.14);
}
```

* 模板实参不能推断返回类型，必须显式指定

```cpp
#include <cassert>

namespace jc {

template <typename RT, typename T, typename U>
RT max(const T& a, const U& b) {
  return a < b ? b : a;
}

}  // namespace jc

int main() {
  assert(jc::max<double>(1, 3.14) == 3.14);
  assert((jc::max<double, int, int>(1, 3.14) == 3));
}
```

* C++14 允许 auto 作为返回类型，它通过 return 语句推断返回类型，C++11 则需要额外指定尾置返回类型，对于三目运算符，其结果类型为两个操作数类型中更公用的类型，比如 int 和 double 的公用类型是 double

```cpp
#include <cassert>

namespace jc {

template <typename T, typename U>
auto max(const T& a, const U& b) -> decltype(true ? a : b) {
  return a < b ? b : a;
}

}  // namespace jc

int main() { assert(jc::max(1, 3.14) == 3.14); }
```

* 用 constexpr 函数可以生成编译期值

```cpp
namespace jc {

template <typename T, typename U>
constexpr auto max(const T& a, const U& b) {
  return a < b ? b : a;
}

}  // namespace jc

int main() { static_assert(jc::max(1, 3.14) == 3.14); }
```

## [type traits](https://en.cppreference.com/w/cpp/header/type_traits)

* 对于类型进行计算的模板称为 type traits，也可以称为元函数，比如用 [std::common_type](https://en.cppreference.com/w/cpp/types/common_type) 来计算不同类型中最通用的类型

```cpp
#include <cassert>
#include <type_traits>

namespace jc {

template <typename T, typename U, typename RT = std::common_type_t<T, U>>
RT max(const T& a, const U& b) {
  return a < b ? b : a;
}

}  // namespace jc

int main() { assert(jc::max(1, 3.14) == 3.14); }
```

## 重载

* 当类型同时匹配普通函数和模板时，优先匹配普通函数

```cpp
#include <cassert>

namespace jc {

int f(int a, int b) { return 1; }

template <typename T, typename U>
int f(const T&, const U&) {
  return 2;
}

}  // namespace jc

int main() {
  assert(jc::f(1, 3) == 1);
  assert(jc::f<double>(1, 3) == 2);
  assert(jc::f<>(1, 3) == 2);
  assert(jc::f(1, 3.14) == 2);
  assert(jc::f(3.14, 1) == 2);
}
```

* 模板参数不同就会构成重载，如果对于给定的实参能同时匹配两个模板，重载解析会优先匹配更特殊的模板，如果同样特殊则产生二义性错误

```cpp
#include <cassert>

namespace jc {

template <typename T, typename U>
int f(const T&, const U&) {
  return 1;
}

template <typename RT, typename T, typename U>
int f(const T& a, const U& b) {
  return 2;
}

}  // namespace jc

int main() {
  assert(jc::f(1, 3.14) == 1);
  assert(jc::f<double>(1, 3.14) == 2);
  //   jc::f<int>(1, 3.14);  // 二义性错误
}
```

* C-style 字符串的重载

```cpp
#include <cassert>
#include <cstring>
#include <string>

namespace jc {

template <typename T>
T max(T a, T b) {
  return a < b ? b : a;
}

template <typename T>
T* max(T* a, T* b) {
  return *a < *b ? b : a;
}

const char* max(const char* a, const char* b) {
  return std::strcmp(a, b) < 0 ? b : a;
}

}  // namespace jc

int main() {
  int a = 1;
  int b = 3;
  assert(jc::max(a, b) == b);
  assert(jc::max(&a, &b) == &b);

  std::string s1 = "down";
  std::string s2 = "demo";
  assert(jc::max(s1, s2) == "down");
  assert(std::strcmp(jc::max("down", "demo"), "down") == 0);
}
```

* 注意不能返回 C-style 字符串的引用

```cpp
namespace jc {

template <typename T>
const T& f(const char* s) {
  return s;
}

}  // namespace jc

int main() {
  const char* s = "downdemo";
  jc::f<const char*>(s);  // 错误：返回临时对象的引用
}
```

* 这种错误可能在添加代码的过程中引入

```cpp
#include <cstring>

namespace jc {

template <typename T>
const T& max(const T& a, const T& b) {
  return b < a ? a : b;
}

// 新增函数来支持 C-style 参数
const char* max(const char* a, const char* b) {
  return std::strcmp(a, b) < 0 ? b : a;
}

template <typename T>
const T& max(const T& a, const T& b, const T& c) {
  return max(max(a, b), c);  // max("down", "de") 返回临时对象的引用
}

}  // namespace jc

int main() {
  const char* a = "down";
  const char* b = "de";
  const char* c = "mo";
  jc::max<const char*>(a, b, c);  // 错误：返回临时对象的引用
}
```

* 只有在函数调用前声明的重载才会被匹配，即使后续有更优先的匹配，由于不可见也会被忽略

```cpp
#include <cassert>

namespace jc {

template <typename T>
int f(T) {
  return 1;
}

template <typename T>
int g(T a) {
  return f(a);
}

int f(int) { return 2; }

}  // namespace jc

int main() { assert(jc::g(0) == 1); }
```

## 用于原始数组与字符串字面值（string literal）的模板

* 字符串字面值传引用会推断为字符数组，为此需要为原始数组和字符串字面值提供特定处理的模板

```cpp
#include <cassert>
#include <cstddef>

namespace jc {

template <typename T, typename U>
constexpr bool less(const T& a, const U& b) {
  return a < b;
}

template <typename T, std::size_t M, std::size_t N>
constexpr bool less(T (&a)[M], T (&b)[N]) {
  for (std::size_t i = 0; i < M && i < N; ++i) {
    if (a[i] < b[i]) {
      return true;
    }
    if (b[i] < a[i]) {
      return false;
    }
  }
  return M < N;
}

}  // namespace jc

static_assert(jc::less(0, 42));
static_assert(!jc::less("down", "demo"));
static_assert(jc::less("demo", "down"));

int main() {}
```

* 各种类型的数组参数对应的偏特化

```cpp
#include <cstddef>

namespace jc {

template <typename T>
struct A;

template <typename T, std::size_t N>
struct A<T[N]> {
  static constexpr int value = 1;
};

template <typename T, std::size_t N>
struct A<T (&)[N]> {
  static constexpr int value = 2;
};

template <typename T>
struct A<T[]> {
  static constexpr int value = 3;
};

template <typename T>
struct A<T (&)[]> {
  static constexpr int value = 4;
};

template <typename T>
struct A<T*> {
  static constexpr int value = 5;
};

template <typename T1, typename T2, typename T3>
constexpr void test(int a1[7], int a2[], int (&a3)[42], int (&x0)[], T1 x1,
                    T2& x2, T3&& x3) {
  static_assert(A<decltype(a1)>::value == 5);  // A<T*>
  static_assert(A<decltype(a2)>::value == 5);  // A<T*>
  static_assert(A<decltype(a3)>::value == 2);  // A<T(&)[N]>
  static_assert(A<decltype(x0)>::value == 4);  // A<T(&)[]>
  static_assert(A<decltype(x1)>::value == 5);  // A<T*>
  static_assert(A<decltype(x2)>::value == 4);  // A<T(&)[]>
  static_assert(A<decltype(x3)>::value == 4);  // A<T(&)[]>
}

}  // namespace jc

int main() {
  int a[42];
  static_assert(jc::A<decltype(a)>::value == 1);
  extern int x[];  // 传引用时将变为 int(&)[]
  static_assert(jc::A<decltype(x)>::value == 3);  // A<T[]>
  jc::test(a, a, a, x, x, x, x);
}

int x[] = {1, 2, 3};  // 定义前置声明的数组
```

## 零初始化（Zero Initialization）

* 使用模板时常希望模板类型的变量已经用默认值初始化，但内置类型无法满足要求。解决方法是显式调用内置类型的默认构造函数

```cpp
namespace jc {

template <typename T>
constexpr T default_value() {
  T x{};
  return x;
}

template <typename T>
struct DefaultValue {
  constexpr DefaultValue() : value() {}
  T value;
};

template <typename T>
struct DefaultValue2 {
  T value{};
};

static_assert(default_value<bool>() == false);
static_assert(default_value<char>() == 0);
static_assert(default_value<int>() == 0);
static_assert(default_value<double>() == 0);

static_assert(DefaultValue<bool>{}.value == false);
static_assert(DefaultValue<char>{}.value == 0);
static_assert(DefaultValue<int>{}.value == 0);
static_assert(DefaultValue<double>{}.value == 0);

static_assert(DefaultValue2<bool>{}.value == false);
static_assert(DefaultValue2<char>{}.value == 0);
static_assert(DefaultValue2<int>{}.value == 0);
static_assert(DefaultValue2<double>{}.value == 0);

}  // namespace jc

int main() {}
```
