## [非类型模板参数（Non-type Template Parameter）](https://en.cppreference.com/w/cpp/language/template_parameters#Non-type_template_parameter)

* 非类型模板参数表示在编译期或链接期可以确定的常量值

```cpp
#include <bitset>
#include <cassert>

namespace jc {

template <bool IsSet = true, std::size_t N>
std::size_t find_next(const std::bitset<N>& b, std::size_t cur) {
  for (std::size_t i = cur + 1; i < N; ++i) {
    if (!(b.test(i) ^ IsSet)) {
      return i;
    }
  }
  return N;
}

template <bool IsSet = true, std::size_t N>
std::size_t find_prev(const std::bitset<N>& b, std::size_t cur) {
  if (cur > N) {
    cur = N;
  }
  for (int i = static_cast<int>(cur) - 1; i >= 0; --i) {
    if (!(b.test(i) ^ IsSet)) {
      return i;
    }
  }
  return N;
}

template <bool IsSet = true, std::size_t N>
std::size_t circular_find_next(const std::bitset<N>& b, std::size_t cur) {
  if (cur > N) {
    cur = N;
  }
  std::size_t res = find_next<IsSet>(b, cur);
  if (res != N) {
    return res;
  }
  for (std::size_t i = 0; i < cur; ++i) {
    if (!(b.test(i) ^ IsSet)) {
      return i;
    }
  }
  return N;
}

template <bool IsSet = true, std::size_t N>
std::size_t circular_find_prev(const std::bitset<N>& b, std::size_t cur) {
  if constexpr (N == 0) {
    return N;
  }
  std::size_t res = find_prev<IsSet>(b, cur);
  if (res != N) {
    return res;
  }
  for (std::size_t i = N - 1; i > cur; --i) {
    if (!(b.test(i) ^ IsSet)) {
      return i;
    }
  }
  return N;
}

}  // namespace jc

void test_find_next() {
  std::bitset<8> b{"10010111"};
  static constexpr int _next_set[] = {1, 2, 4, 4, 7, 7, 7, 8, 8, 8};
  static constexpr int _next_unset[] = {3, 3, 3, 5, 5, 6, 8, 8, 8, 8};

  for (std::size_t i = 0; i < std::size(_next_set); ++i) {
    assert(jc::find_next<true>(b, i) == _next_set[i]);
    assert(jc::find_next<false>(b, i) == _next_unset[i]);
  }
}

void test_find_prev() {
  std::bitset<8> b{"10010110"};
  static constexpr int _prev_set[] = {8, 8, 1, 2, 2, 4, 4, 4, 7, 7};
  static constexpr int _prev_unset[] = {8, 0, 0, 0, 3, 3, 5, 6, 6, 6};

  for (std::size_t i = 0; i < std::size(_prev_set); ++i) {
    assert(jc::find_prev<true>(b, i) == _prev_set[i]);
    assert(jc::find_prev<false>(b, i) == _prev_unset[i]);
  }
}

void test_circular_find_next() {
  std::bitset<8> b{"01010111"};
  static constexpr int _next_set[] = {1, 2, 4, 4, 6, 6, 0, 0, 0, 0};
  static constexpr int _next_unset[] = {3, 3, 3, 5, 5, 7, 7, 3, 3, 3};

  for (std::size_t i = 0; i < std::size(_next_set); ++i) {
    assert(jc::circular_find_next<true>(b, i) == _next_set[i]);
    assert(jc::circular_find_next<false>(b, i) == _next_unset[i]);
  }
}

void test_circular_find_prev() {
  std::bitset<8> b{"10011001"};
  static constexpr int _prev_set[] = {7, 0, 0, 0, 3, 4, 4, 4, 7, 7};
  static constexpr int _prev_unset[] = {6, 6, 1, 2, 2, 2, 5, 6, 6, 6};

  for (std::size_t i = 0; i < std::size(_prev_set); ++i) {
    assert(jc::circular_find_prev<true>(b, i) == _prev_set[i]);
    assert(jc::circular_find_prev<false>(b, i) == _prev_unset[i]);
  }
}

int main() {
  test_find_next();
  test_find_prev();
  test_circular_find_next();
  test_circular_find_prev();
}
```

* 模板参数可以由之前的参数推断类型，C++17 允许将非类型模板参数定义为 auto 或 decltype(auto)

```cpp
#include <cassert>

namespace jc {

template <typename>
struct get_class;

template <typename ClassType, typename MemberType>
struct get_class<MemberType ClassType::*> {
  using type = ClassType;
};

template <typename T>
using get_class_t = typename get_class<T>::type;

template <auto ClassMember>
class Wrapper {
 public:
  Wrapper(get_class_t<decltype(ClassMember)>& obj) : obj_(obj) {}

  void increase() { ++(obj_.*ClassMember); }

 private:
  get_class_t<decltype(ClassMember)>& obj_;
};

struct A {
  int i = 0;
};

}  // namespace jc

int main() {
  jc::A a;
  jc::Wrapper<&jc::A::i>{a}.increase();
  assert(a.i == 1);
}
```

* C++14 允许 auto 作返回类型

```cpp
namespace jc {

template <typename T, typename U>
constexpr auto add(const T& a, const U& b) {
  return a + b;
}

}  // namespace jc

static_assert(jc::add('A', 2) == 'C');

int main() {}
```

## 限制

* C++20 之前，非类型模板参数不能是浮点数

```cpp
namespace jc {

template <auto N>
struct A {
  static constexpr auto f() { return N; }
};

}  // namespace jc

static_assert(jc::A<42>::f() == 42);
static_assert(jc::A<3.14>::f() == 3.14);  // C++20

int main() {}
```

* 不能用字符串字面值常量、临时对象、数据成员或其他子对象作模板实参。C++ 标准演进过程中逐渐放宽了对字符数组作为模板实参的限制，C++11 仅允许外链接（external linkage，不定义于单一的文件作用域，链接到全局符号表），C++14 允许外链接或内链接（internal linkage，只能在单个文件内部看到，不能被其他文件访问，不暴露给链接器），C++17 不要求链接

```cpp
namespace jc {

template <const char* s>
struct A {};

}  // namespace jc

constexpr const char* s1 = "hello";  // 内链接对象的指针
extern const char s2[] = "world";    // 外链接
const char s3[] = "down";            // 内链接

int main() {
  static const char s4[] = "demo";  // 无链接
  jc::A<"downdemo">{};              // 错误
  jc::A<s1>{};                      // 错误
  jc::A<s2>{};                      // C++11 允许
  jc::A<s3>{};                      // C++14 允许
  jc::A<s4>{};                      // C++17 允许
}
```

* 非类型模板参数可以是左值引用，此时实参必须是静态常量

```cpp
#include <cassert>

namespace jc {

template <int& N>
struct A {
  A() { ++N; }
  ~A() { --N; }
};

void test() {
  static int n = 0;
  {
    A<n> a;
    assert(n == 1);
  }
  assert(n == 0);
}

}  // namespace jc

int main() { jc::test(); }
```

* 函数和数组类型作为非类型模板参数会退化为指针类型

```cpp
namespace jc {

template <int buf[5]>
struct Lexer {};

// template <int* buf>
// struct Lexer {};  // 错误：重定义

template <int fun()>
struct FuncWrap {};

// template <int (*)()>
// struct FuncWrap {};  // 错误：重定义

}  // namespace jc

int main() {}
```

* 如果模板实参的表达式有大于号，必须用小括号包裹表达式，否则大于号会被编译器视为表示参数列表终止的右尖括号，导致编译错误

```cpp
namespace jc {

template <bool b>
struct A {
  inline static constexpr bool value = b;
};

}  // namespace jc

int main() { static_assert(jc::A<(sizeof(int) > 0)>::value); }
```

## [变量模板（Variable Template）](https://en.cppreference.com/w/cpp/language/variable_template)

* C++14 提供了变量模板

```cpp
namespace jc {

template <typename T = double>
constexpr T pi{static_cast<T>(3.1415926535897932385)};

static_assert(pi<bool> == true);
static_assert(pi<int> == 3);
static_assert(pi<double> == 3.1415926535897932385);
static_assert(pi<> == 3.1415926535897932385);

}  // namespace jc

int main() {}
```

* 变量模板可以由非类型参数参数化

```cpp
#include <array>
#include <cassert>

namespace jc {

template <int N>
std::array<int, N> arr{};

template <auto N>
constexpr decltype(N) x = N;

}  // namespace jc

static_assert(jc::x<'c'> == 'c');

int main() {
  jc::arr<10>[0] = 42;
  assert(jc::arr<10>[0] == 42);
}
```
