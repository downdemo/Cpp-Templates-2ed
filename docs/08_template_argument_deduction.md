## Deduced Context

* 复杂的类型声明的匹配过程从最顶层构造开始，然后不断递归子构造，即各种组成元素，这些构造被称为 deduced context，non-deduced context 不会参与推断，而是使用其他处推断的结果，受限类型名称如 `A<T>::type` 不能用来推断 T，非类型表达式如 `A<N + 1>` 不能用来推断 N

```cpp
namespace jc {

template <int N>
struct A {
  using T = int;

  void f(int) {}
};

template <int N>  // A<N>::T 是 non-deduced context，X<N>::*p 是 deduced context
void f(void (A<N>::*p)(typename A<N>::T)) {}

}  // namespace jc

int main() {
  using namespace jc;
  f(&A<0>::f);  // 由 A<N>::*p 推断 N 为 0，A<N>::T 则使用 N 变为 A<0>::T
}
```

* 默认实参不能用于推断

```cpp
namespace jc {

template <typename T>
void f(T x = 42) {}

}  // namespace jc

int main() {
  jc::f<int>();  // T = int
  jc::f();       // 错误：无法推断 T
}
```

## 特殊的推断情况

* 成员函数的推断

```cpp
namespace jc {

struct A {
  void f(int*) const noexcept {}
};

template <typename RT, typename T, typename... Args>
void f(RT (T::*)(Args...) const) {}

}  // namespace jc

int main() {
  jc::f(&jc::A::f);  // RT = void，T = A，Args = int*
}
```

* 取函数模板地址和调用转型运算符模板的推断

```cpp
namespace jc {

template <typename T>
void f(T) {}

struct A {
  template <typename T>
  operator T&() {
    static T x;
    return x;
  }
};

void g(int (&)[3]) {}

}  // namespace jc

int main() {
  void (*pf)(int) = &jc::f;  // 推断为 f<int>(int)

  jc::A a;
  jc::g(a);  // a 要转为 int(&)[3]，T 推断为 int[3]
}
```

* 初始化列表作为实参没有具体类型，不能直接推断为初始化列表
 
```cpp
#include <initializer_list>

namespace jc {

template <typename T>
void f(T) {}

template <typename T>
void g(std::initializer_list<T>) {}

}  // namespace jc

int main() {
  // jc::f({1, 2, 3});  // 错误：不能推断出 T 为 initializer_list
  jc::g({1, 2, 3});  // OK：T 为 int
}
```

* 参数包的推断

```cpp
namespace jc {

template <typename T, typename U>
struct A {};

template <typename T, typename... Args>
void f(const A<T, Args>&...);

template <typename... T, typename... U>
void g(const A<T, U>&...);

}  // namespace jc

int main() {
  using namespace jc;
  f(A<int, bool>{}, A<int, char>{});   // T = int, Args = [bool,char]
  g(A<int, bool>{}, A<int, char>{});   // T = [int, int], U = [bool, char]
  g(A<int, bool>{}, A<char, char>{});  // T = [int, char], U = [bool, char]
  // f(A<int, bool>{}, A<char, char>{});  // 错误，T 分别推断为 int 和 char
}
```

* 完美转发处理空指针常量时，整型值会被当作常量值 0

```cpp
#include <utility>

namespace jc {

constexpr int g(...) { return 1; }
constexpr int g(int*) { return 2; }

template <typename T>
constexpr int f(T&& t) {
  return g(std::forward<T>(t));
}

}  // namespace jc

static_assert(jc::f(0) == 1);
static_assert(jc::g(0) == 2);
static_assert(jc::f(nullptr) == 2);
static_assert(jc::g(nullptr) == 2);

int main() {}
```

## [SFINAE（Substitution Failure Is Not An Error）](https://en.cppreference.com/w/cpp/language/sfinae)

* SFINAE 用于禁止不相关函数模板在重载解析时造成错误，当替换返回类型无意义时，会忽略（SFINAE out）匹配而选择另一个更差的匹配

```cpp
#include <vector>

namespace jc {

template <typename T, std::size_t N>
T* begin(T (&a)[N]) {
  return a;
}

template <typename Container>
typename Container::iterator begin(Container& c) {
  return c.begin();
}

}  // namespace jc

int main() {
  std::vector<int> v;
  int a[10] = {};

  jc::begin(v);  // OK：只匹配第二个，SFINAE out 第一个
  jc::begin(a);  // OK：只匹配第一个，SFINAE out 第二个
}
```

* SFINAE 只发生于函数模板替换的即时上下文中，对于模板定义中不合法的表达式，不会使用 SFINAE 机制

```cpp
namespace jc {

template <typename T, typename U>
auto f(T t, U u) -> decltype(t + u) {
  return t + u;
}

void f(...) {}

template <typename T, typename U>
auto g(T t, U u) -> decltype(auto) {  // 必须实例化 t 和 u 来确定返回类型
  return t + u;  // 不是即时上下文，不会使用 SFINAE
}

void g(...) {}

struct X {};

using A = decltype(f(X{}, X{}));  // OK：A 为 void
using B = decltype(g(X{}, X{}));  // 错误：g<X, X> 的实例化非法

}  // namespace jc

int main() {}
```

* 一个简单的 SFINAE 技巧是使用尾置返回类型，用 devltype 和逗号运算符定义返回类型，在 decltype 中定义必须有效的表达式

```cpp
#include <cassert>
#include <string>

namespace jc {

template <typename T>
auto size(const T& t) -> decltype(t.size(), T::size_type()) {
  return t.size();
}

}  // namespace jc

int main() {
  std::string s;
  assert(jc::size(s) == 0);
}
```

* 如果替换时使用了类成员，则会实例化类模板，此期间发生的错误不在即时上下文中，即使另一个函数模板匹配无误也不会使用 SFINAE

```cpp
namespace jc {

template <typename T>
class Array {
 public:
  using iterator = T*;
};

template <typename T>
void f(typename Array<T>::iterator) {}

template <typename T>
void f(T*) {}

}  // namespace jc

int main() {
  jc::f<int&>(0);  // 错误：第一个模板实例化 Array<int&>，创建引用的指针是非法的
}
```

* SFINAE 最出名的应用是 [std::enable_if](https://en.cppreference.com/w/cpp/types/enable_if)

```cpp
#include <cassert>
#include <iostream>
#include <sstream>
#include <string>
#include <type_traits>

namespace jc {

template <
    typename K, typename V,
    std::enable_if_t<std::is_same_v<std::decay_t<V>, bool>, void*> = nullptr>
void append(std::ostringstream& os, const K& k, const V& v) {
  os << R"(")" << k << R"(":)" << std::boolalpha << v;
}

template <typename K, typename V,
          std::enable_if_t<!std::is_same_v<std::decay_t<V>, bool> &&
                               std::is_arithmetic_v<std::decay_t<V>>,
                           void*> = nullptr>
void append(std::ostringstream& os, const K& k, const V& v) {
  os << R"(")" << k << R"(":)" << v;
}

template <
    typename K, typename V,
    std::enable_if_t<std::is_constructible_v<std::string, std::decay_t<V>>,
                     void*> = nullptr>
void append(std::ostringstream& os, const K& k, const V& v) {
  os << R"(")" << k << R"(":")" << v << R"(")";
}

void kv_string_impl(std::ostringstream& os) {}

template <typename V, typename... Args>
std::void_t<decltype(std::cout << std::declval<std::decay_t<V>>())>
kv_string_impl(std::ostringstream& os, const std::string& k, const V& v,
               const Args&... args) {
  append(os, k, v);
  if constexpr (sizeof...(args) >= 2) {
    os << ",";
  }
  kv_string_impl(os, args...);
}

template <typename... Args>
std::string kv_string(const std::string& field, const Args&... args) {
  std::ostringstream os;
  os << field << ":{";
  kv_string_impl(os, args...);
  os << "}";
  return os.str();
}

}  // namespace jc

int main() {
  std::string a{R"(data:{})"};
  std::string b{R"(data:{"name":"jc","ID":1})"};
  std::string c{R"(data:{"name":"jc","ID":1,"active":true})"};
  assert(a == jc::kv_string("data"));
  assert(b == jc::kv_string("data", "name", "jc", "ID", 1));
  assert(c == jc::kv_string("data", "name", "jc", "ID", 1, "active", true));
}
```

## Deduction Guides

* 字符串字面值传引用时推断为字符数组

```cpp
#include <vector>

namespace jc {

template <typename T>
class A {
 public:
  A(const T& val) : container_({val}) {}

 private:
  std::vector<T> container_;
};

}  // namespace jc

int main() {
  jc::A a = "downdemo";  // 错误：T 为 char[9]，构造 std::vector<char[9]> 出错
}
```

* 改为传值，字符串字面值会推断为 `const char*`

```cpp
#include <type_traits>
#include <vector>

namespace jc {

template <typename T>
class A {
 public:
  A(T val) : container_({std::move(val)}) {}

 private:
  std::vector<T> container_;
};

}  // namespace jc

int main() {
  jc::A a = "downdemo";
  static_assert(std::is_same_v<decltype(a), jc::A<const char*>>);
}
```

* C++17 可以定义 deduction guides 对特定类型的实参指定其推断类型

```cpp
#include <string>
#include <type_traits>
#include <vector>

namespace jc {

template <typename T>
class A {
 public:
  A(const T& val) : container_({val}) {}

 private:
  std::vector<T> container_;
};

A(const char*)->A<std::string>;

}  // namespace jc

int main() {
  jc::A a{"downdemo"};  // 等号初始化会出错，const char[9] 不能转为 std::string
  static_assert(std::is_same_v<decltype(a), jc::A<std::string>>);
}
```

* 为聚合类模板定义 deduction guides

```cpp
#include <cassert>
#include <string>
#include <type_traits>

namespace jc {

template <typename T>
struct A {
  T x;
  std::string s;
};

A(const char*, const char*)->A<std::string>;

}  // namespace jc

int main() {
  jc::A a = {"down", "demo"};
  assert(a.x == "down");
  static_assert(std::is_same_v<decltype(a.x), std::string>);
}
```

* 使用花括号赋值可以解决没有初始化列表的问题，圆括号则不行

```cpp
namespace jc {

template <typename T>
struct A {
  T x;
};

template <typename T>
A(T) -> A<T>;

}  // namespace jc

int main() {
  jc::A a1{0};     // OK
  jc::A a2 = {0};  // OK
  jc::A a3(0);   // 错误：没有初始化列表，int 不能转为 jc::A<int>
  jc::A a4 = 0;  // 错误：没有初始化列表，int 不能转为 jc::A<int>
}
```

* explicit 声明的 deduction guides 只用于直接初始化

```cpp
namespace jc {

template <typename T, typename U>
struct A {
  A(const T&) {}
  A(T&&) {}
};

template <typename T>
A(const T&) -> A<T, T&>;

template <typename T>
explicit A(T&&) -> A<T, T>;  // 只能用于直接初始化

}  // namespace jc

int main() {
  jc::A a = 1;  // A<int, int&> a = 1;
  jc::A b{2};   // A<int, int> b{2};
}
```

* [std::array](https://en.cppreference.com/w/cpp/container/array) 是一个聚合类模板，[C++17 为其定义了一个 deduction guides](https://en.cppreference.com/w/cpp/container/array/deduction_guides) 来推断模板参数

```cpp
#include <array>
#include <type_traits>

// template <typename T, typename... U>
// array(T, U...)
//     -> array<
// 		enable_if_t<(is_same_v<T, U> && ...), T>,
// 		1 + sizeof...(U)
// 	>;

int main() {
  std::array a{1, 2, 3, 4};
  static_assert(std::is_same_v<decltype(a), std::array<int, 4>>);
}
```

* C++17 允许[类模板实参推断](https://en.cppreference.com/w/cpp/language/class_template_argument_deduction)，但类模板的所有参数要么通过显式指定指出，要么通过实参推断推出，不能一部分使用显式指定一部分使用推断

```cpp
#include <string>

namespace jc {

template <typename T, typename U, typename Y = U>
struct A {
  A(T x = T{}, U y = U{}, Y z = Y{}) {}
};

}  // namespace jc

int main() {
  jc::A{1, 3.14, "hello"};  // T = int，U = double，T3 = const char*
  jc::A{1, 3.14};           // T = int，U = Y = double
  jc::A{"hi", "downdemo"};  // T = U = Y = const char*
  jc::A<std::string>{"hi", "downdemo", 42};  // 错误：只指定了 T，U 未推断
  jc::A<>{1, 3.14, 42};                      // 错误：T 和 U 都未指定
}
```

* 类模板实参推断的本质是为每个构造函数和构造函数模板隐式添加一个 deduction guides

```cpp
#include <type_traits>
#include <vector>

namespace jc {

template <typename T>
class A {
 public:
  A(const T& val) : container_({val}) {}

 private:
  std::vector<T> container_;
};

// template <typename T>
// A(const T&) -> A<T>;  // 隐式 deduction guides

}  // namespace jc

int main() {
  jc::A a1 = 0;
  jc::A a2{0};
  jc::A a3(0);
  auto a4 = jc::A{0};
  static_assert(std::is_same_v<decltype(a1), jc::A<int>>);
  static_assert(std::is_same_v<decltype(a2), jc::A<int>>);
  static_assert(std::is_same_v<decltype(a3), jc::A<int>>);
  static_assert(std::is_same_v<decltype(a4), jc::A<int>>);
}
```

## Deduction Guides 的问题

* 用类模板实例作为实参时，Deduction guides 对实参推断的类型有歧义，标准委员会对于该情况有争议地规定，推断时不会将实参推断为类模板的实例

```cpp
#include <type_traits>

namespace jc {

template <typename T>
struct A {
  A(T x) {}
};

template <typename T>
A(T) -> A<T>;

}  // namespace jc

int main() {
  jc::A a1{0};
  jc::A a2{a1};  // A<int> 还是 A<A<int>>？标准委员会规定为 A<int>
  jc::A a3(a1);  // A<int> 还是 A<A<int>>？标准委员会规定为 A<int>
  static_assert(std::is_same_v<decltype(a1), jc::A<int>>);
  static_assert(std::is_same_v<decltype(a2), jc::A<int>>);
  static_assert(std::is_same_v<decltype(a3), jc::A<int>>);
}
```

* 这个争议造成的问题如下

```cpp
#include <type_traits>
#include <vector>

namespace jc {

template <typename T, typename... Args>
auto f(const T& x, const Args&... args) {  // 如果 T 为 std::vector
  return std::vector{x, args...};  // 参数包是否为空将决定不同的返回类型
}

}  // namespace jc

int main() {
  using std::vector;
  vector v1{1, 2, 3};
  vector v2{v1};
  vector v3{v1, v1};
  static_assert(std::is_same_v<decltype(v1), vector<int>>);
  static_assert(std::is_same_v<decltype(v2), vector<int>>);
  static_assert(std::is_same_v<decltype(v3), vector<vector<int>>>);
  static_assert(std::is_same_v<decltype(jc::f(v1)), vector<int>>);
  static_assert(std::is_same_v<decltype(jc::f(v1, v1)), vector<vector<int>>>);
}
```

* 添加隐式 deduction guides 是有争议的，主要反对观点是这个特性自动将接口添加到已存在的库中，并且对于有限定名称的情况，deduction guides 会失效

```cpp
namespace jc {

template <typename T>
struct type_identity {
  using type = T;
};

template <typename T>
class A {
 public:
  using ArgType = typename type_identity<T>::type;
  A(ArgType) {}
};

template <typename T>
A(typename type_identity<T>::type) -> A<T>;
// 该 deduction guides 无效，因为有限定名称符 type_identity<T>::

}  // namespace jc

int main() {
  jc::A a{0};  // 错误
}
```

* 为了保持向后兼容性，如果模板名称是[注入类名](https://en.cppreference.com/w/cpp/language/injected-class-name)，则禁用类模板实参推断

```cpp
#include <type_traits>

namespace jc {

template <typename T>
struct A {
  template <typename U>
  A(U x) {}

  template <typename U>
  auto f(U x) {
    return A(x);  // 根据注入类名规则 A 是 A<T>，根据类模板实参推断 A 是 A<U>
  }
};

}  // namespace jc

int main() {
  jc::A<int> a{0};
  auto res = a.f<double>(3.14);
  static_assert(std::is_same_v<decltype(res), jc::A<int>>);
}
```

* 使用转发引用的 deduction guides 可能推断出引用类型，导致实例化错误或产生空悬引用，因此标准委员会决定使用隐式 deduction guides 的推断时，禁用 T&& 这个特殊的推断规则

```cpp
#include <string>
#include <type_traits>

namespace jc {

template <typename T>
struct A {
  A(const T&) {}
  A(T&&) {}
};

// template <typename T>
// A(const T&) -> A<T>;  // 隐式生成

// template <typename T>
// A(T&&) -> A<T>;  // 不会隐式生成该 deduction guides

}  // namespace jc

int main() {
  std::string s;
  jc::A a = s;  // T 推断为 std::string
  static_assert(std::is_same_v<decltype(a), jc::A<std::string>>);
  // 若指定 T&& 的 deduction guides，则 T 推断为 std::string&
}
```

* Deduction guides 只用于推断而非调用，实参的传递方式不必完全对应构造函数

```cpp
#include <iostream>
#include <type_traits>
#include <utility>

namespace jc {

template <typename T>
struct A {};

template <typename T>
struct B {
  B(const A<T>&) { std::cout << 1 << std::endl; }
  B(A<T>&&) { std::cout << 2 << std::endl; }
};

template <typename T>
B(A<T>) -> B<T>;  // 不需要完全对应构造函数

}  // namespace jc

int main() {
  jc::A<int> a;
  jc::B{a};             // 1
  jc::B{std::move(a)};  // 2
}
```
