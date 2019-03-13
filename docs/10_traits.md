## Traits 的偏特化实现

* [std::is_same](https://en.cppreference.com/w/cpp/types/is_same)

```cpp
#include <type_traits>

namespace jc {

template <typename T, typename U>
struct is_same {
  static constexpr bool value = false;
};

template <typename T>
struct is_same<T, T> {
  static constexpr bool value = true;
};

template <typename T, typename U>
constexpr bool is_same_v = is_same<T, U>::value;

}  // namespace jc

static_assert(jc::is_same_v<int, int>);
static_assert(!jc::is_same_v<int, double>);
static_assert(!jc::is_same_v<int, int&>);

int main() {}
```

* 获取元素类型

```cpp
#include <type_traits>

namespace jc {

template <typename T>
struct get_element {
  using type = T;
};

template <typename T>
struct get_element<T[]> {
  using type = typename get_element<T>::type;
};

template <typename T, std::size_t N>
struct get_element<T[N]> {
  using type = typename get_element<T>::type;
};

template <typename T>
using get_element_t = typename get_element<T>::type;

}  // namespace jc

static_assert(std::is_same_v<jc::get_element_t<int>, int>);
static_assert(std::is_same_v<jc::get_element_t<int[]>, int>);
static_assert(std::is_same_v<jc::get_element_t<int[3][4][5]>, int>);

int main() {}
```

* [std::remove_reference](https://en.cppreference.com/w/cpp/types/remove_reference)

```cpp
#include <type_traits>

namespace jc {

template <typename T>
struct remove_reference {
  using type = T;
};

template <typename T>
struct remove_reference<T&> {
  using type = T;
};

template <typename T>
struct remove_reference<T&&> {
  using type = T;
};

template <typename T>
using remove_reference_t = typename remove_reference<T>::type;

}  // namespace jc

static_assert(std::is_same_v<jc::remove_reference_t<int>, int>);
static_assert(std::is_same_v<jc::remove_reference_t<int&>, int>);
static_assert(std::is_same_v<jc::remove_reference_t<int&&>, int>);

int main() {}
```

* [std::enable_if](https://en.cppreference.com/w/cpp/types/enable_if)

```cpp
#include <list>
#include <type_traits>
#include <utility>
#include <vector>

namespace jc {

template <bool, typename T = void>
struct enable_if {};

template <typename T>
struct enable_if<true, T> {
  using type = T;
};

template <bool B, typename T = void>
using enable_if_t = typename enable_if<B, T>::type;

}  // namespace jc

struct Base {};
struct Derived1 : Base {};
struct Derived2 : Base {};

template <typename T, template <typename...> class V>
void impl(const V<T>&) {
  static_assert(std::is_constructible_v<Base*, T*>);
}

template <typename T, template <typename...> class V, typename... Args,
          jc::enable_if_t<std::is_constructible_v<Base*, T*>, void*> = nullptr>
void f(const V<T>& t, Args&&... args) {
  impl(t);
  if constexpr (sizeof...(args) > 0) {
    f(std::forward<Args>(args)...);
  }
}

int main() { f(std::vector<Derived1>{}, std::list<Derived2>{}); }
```

## 元函数转发（Metafunction Forwarding）

* Traits 可以视为对类型做操作的函数，称为元函数，元函数一般包含一些相同的成员，将相同成员封装成一个基类作为基本元函数，继承这个基类即可使用成员，这种实现方式称为元函数转发，标准库中实现了 [std::integral_constant](https://en.cppreference.com/w/cpp/types/integral_constant) 作为基本元函数

```cpp
#include <cassert>
#include <type_traits>

namespace jc {

template <class T, T v>
struct integral_constant {
  static constexpr T value = v;
  using value_type = T;
  using type = integral_constant<T, v>;
  constexpr operator value_type() const noexcept { return value; }
  constexpr value_type operator()() const noexcept { return value; }
};

constexpr int to_int(char c) {
  // hexadecimal letters:
  if (c >= 'A' && c <= 'F') {
    return static_cast<int>(c) - static_cast<int>('A') + 10;
  }
  if (c >= 'a' && c <= 'f') {
    return static_cast<int>(c) - static_cast<int>('a') + 10;
  }
  assert(c >= '0' && c <= '9');
  return static_cast<int>(c) - static_cast<int>('0');
}

template <std::size_t N>
constexpr int parse_int(const char (&arr)[N]) {
  int base = 10;   // to handle base (default: decimal)
  int offset = 0;  // to skip prefixes like 0x
  if (N > 2 && arr[0] == '0') {
    switch (arr[1]) {
      case 'x':  // prefix 0x or 0X, so hexadecimal
      case 'X':
        base = 16;
        offset = 2;
        break;
      case 'b':  // prefix 0b or 0B (since C++14), so binary
      case 'B':
        base = 2;
        offset = 2;
        break;
      default:  // prefix 0, so octal
        base = 8;
        offset = 1;
        break;
    }
  }
  int res = 0;
  int multiplier = 1;
  for (std::size_t i = 0; i < N - offset; ++i) {
    if (arr[N - 1 - i] != '\'') {
      res += to_int(arr[N - 1 - i]) * multiplier;
      multiplier *= base;
    }
  }
  return res;
}

template <char... cs>
constexpr auto operator"" _c() {
  return integral_constant<int, parse_int<sizeof...(cs)>({cs...})>{};
}

static_assert(std::is_same_v<decltype(2_c), integral_constant<int, 2>>);
static_assert(std::is_same_v<decltype(0xFF_c), integral_constant<int, 255>>);
static_assert(
    std::is_same_v<decltype(0b1111'1111_c), integral_constant<int, 255>>);

}  // namespace jc

static_assert(jc::integral_constant<int, 42>::value == 42);
static_assert(std::is_same_v<int, jc::integral_constant<int, 0>::value_type>);
static_assert(jc::integral_constant<int, 42>{} == 42);

int main() {
  jc::integral_constant<int, 42> f;
  static_assert(f() == 42);
}
```

* 利用元函数转发实现 [std::is_same](https://en.cppreference.com/w/cpp/types/is_same)

```cpp
namespace jc {

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

template <typename T, typename U>
struct is_same : false_type {};

template <typename T>
struct is_same<T, T> : true_type {};

template <typename T, typename U>
constexpr bool is_same_v = is_same<T, U>::value;

}  // namespace jc

static_assert(jc::is_same_v<int, int>);
static_assert(!jc::is_same_v<int, double>);
static_assert(!jc::is_same_v<int, int&>);

int main() {}
```

## SFINAE-based traits

* [std::is_default_constructible](https://en.cppreference.com/w/cpp/types/is_default_constructible)

```cpp
#include <type_traits>

namespace jc {

template <typename T>
struct is_default_constructible {
 private:
  template <typename U, typename = decltype(U())>
  static std::true_type test(void*);

  template <typename>
  static std::false_type test(...);

 public:
  static constexpr bool value = decltype(test<T>(nullptr))::value;
};

template <typename T>
constexpr bool is_default_constructible_v = is_default_constructible<T>::value;

}  // namespace jc

struct A {
  A() = delete;
};

static_assert(!jc::is_default_constructible_v<A>);

int main() {}
```

* [std::void_t](https://en.cppreference.com/w/cpp/types/void_t)

```cpp
#include <type_traits>

namespace jc {

template <typename...>
using void_t = void;

template <typename, typename = void_t<>>
struct is_default_constructible : std::false_type {};

template <typename T>
struct is_default_constructible<T, void_t<decltype(T())>> : std::true_type {};

template <typename T>
constexpr bool is_default_constructible_v = is_default_constructible<T>::value;

}  // namespace jc

struct A {
  A() = delete;
};

static_assert(!jc::is_default_constructible_v<A>);

int main() {}
```

* [std::declval](https://en.cppreference.com/w/cpp/utility/declval)

```cpp
#include <type_traits>

namespace jc {

template <typename>
constexpr bool always_false = false;

template <typename T>
std::add_rvalue_reference_t<T> declval() noexcept {
  static_assert(always_false<T>, "declval not allowed in an evaluated context");
}

template <typename, typename = std::void_t<>>
struct has_less : std::false_type {};

template <typename T>
struct has_less<T, std::void_t<decltype(jc::declval<T>() < jc::declval<T>())>>
    : std::true_type {};

template <typename T>
constexpr bool has_less_v = has_less<T>::value;

}  // namespace jc

struct A {
  A() = delete;
  bool operator<(const A& rhs) const { return i < rhs.i; }
  int i;
};

static_assert(jc::has_less_v<A>);

int main() {}
```

* [std::is_nothrow_move_constructible](https://en.cppreference.com/w/cpp/types/is_move_constructible)

```cpp
#include <type_traits>

namespace jc {

template <typename T, typename = std::void_t<>>
struct is_nothrow_move_constructible : std::false_type {};

template <typename T>
struct is_nothrow_move_constructible<
    T, std::void_t<decltype(T(std::declval<T>()))>>
    : std::bool_constant<noexcept(T(std::declval<T>()))> {};

template <typename T>
constexpr bool is_nothrow_move_constructible_v =
    is_nothrow_move_constructible<T>::value;

}  // namespace jc

struct A {
  A(A&&) noexcept {}
};

struct B {
 private:
  B(B&&) noexcept {};
};

static_assert(jc::is_nothrow_move_constructible_v<A>);
static_assert(!jc::is_nothrow_move_constructible_v<B>);

int main() {}
```

* [std::is_convertible](https://en.cppreference.com/w/cpp/types/is_convertible)

```cpp
#include <type_traits>

namespace jc {

// 转为 void 类型需要单独处理，转为数组和函数类型总是 false
template <typename From, typename To,
          bool = std::is_void_v<To> || std::is_array_v<To> ||
                 std::is_function_v<To>>
struct is_convertible_impl {
  using type = std::bool_constant<std::is_void_v<To> && std::is_void_v<From>>;
};

template <typename From, typename To>
struct is_convertible_impl<From, To, false> {
 private:
  static void f(To);

  template <typename T, typename U,
            typename = decltype(f(std::declval<T>()))>  // 将 T 转为 To
  static std::true_type test(void*);

  template <typename, typename>
  static std::false_type test(...);

 public:
  using type = decltype(test<From, To>(nullptr));
};

template <typename From, typename To>
struct is_convertible : is_convertible_impl<From, To>::type {};

template <typename From, typename To>
constexpr bool is_convertible_v = is_convertible<From, To>::value;

}  // namespace jc

struct A {};
struct B : A {};

static_assert(jc::is_convertible_v<B, A>);
static_assert(jc::is_convertible_v<B*, A*>);
static_assert(!jc::is_convertible_v<A*, B*>);
static_assert(jc::is_convertible_v<void, void>);
static_assert(!jc::is_convertible_v<int*, int[]>);

int main() {}
```

* [std::is_class](https://en.cppreference.com/w/cpp/types/is_class)

```cpp
#include <string>
#include <type_traits>
#include <vector>

namespace jc {

template <typename T, typename = std::void_t<>>
struct is_class : std::false_type {};

template <typename T>
struct is_class<T, std::void_t<int T::*>> : std::true_type {};

template <class T>
constexpr bool is_class_v = is_class<T>::value;

}  // namespace jc

union A {};

static_assert(jc::is_class_v<std::string>);
static_assert(jc::is_class_v<std::vector<int>>);
static_assert(jc::is_class_v<A>);
static_assert(std::is_union_v<A>);   // 仅能由编译器开洞实现
static_assert(!std::is_class_v<A>);  // 排除了 union 类型

int main() {}
```

* [std::is_member_pointer](https://en.cppreference.com/w/cpp/types/is_member_pointer)

```cpp
#include <cassert>
#include <type_traits>

namespace jc {

template <class T>
struct is_member_pointer_helper : std::false_type {};

template <class T, class U>
struct is_member_pointer_helper<T U::*> : std::true_type {};

template <class T>
struct is_member_pointer : is_member_pointer_helper<std::remove_cv_t<T>> {};

template <class T>
constexpr bool is_member_pointer_v = is_member_pointer<T>::value;

}  // namespace jc

struct A {
  int f() const { return 1; }
  int i = 0;
};

static_assert(jc::is_member_pointer_v<decltype(&A::f)>);
static_assert(jc::is_member_pointer_v<int (A::*)() const>);
static_assert(jc::is_member_pointer_v<void (A::*)()>);
static_assert(jc::is_member_pointer_v<decltype(&A::i)>);
static_assert(jc::is_member_pointer_v<int A::*>);
static_assert(jc::is_member_pointer_v<double A::*>);

int main() {
  int (A::*pf)() const = &A::f;
  int A::*pi = &A::i;

  assert((A{}.*pf)() == 1);
  static_assert(jc::is_member_pointer_v<decltype(pf)>);

  assert(A{}.*pi == 0);
  static_assert(jc::is_member_pointer_v<decltype(pi)>);
}
```

* [std::is_member_function_pointer](https://en.cppreference.com/w/cpp/types/is_member_function_pointer)

```cpp
#include <type_traits>

namespace jc {

template <class T>
struct is_member_function_pointer_helper : std::false_type {};

template <class T, class U>
struct is_member_function_pointer_helper<T U::*> : std::is_function<T> {};

template <class T>
struct is_member_function_pointer
    : is_member_function_pointer_helper<std::remove_cv_t<T>> {};

template <class T>
constexpr bool is_member_function_pointer_v =
    is_member_function_pointer<T>::value;

}  // namespace jc

struct A {
  void f() {}
  static void g() {}
  int i = 0;
};

void f() {}

static_assert(jc::is_member_function_pointer_v<decltype(&A::f)>);
static_assert(!jc::is_member_function_pointer_v<decltype(&A::g)>);
static_assert(!jc::is_member_function_pointer_v<decltype(&f)>);
static_assert(!jc::is_member_function_pointer_v<decltype(&A::i)>);
static_assert(!jc::is_member_function_pointer_v<int A::*>);
static_assert(!jc::is_member_function_pointer_v<double A::*>);

int main() {}
```

* [std::is_member_object_pointer](https://en.cppreference.com/w/cpp/types/is_member_object_pointer)

```cpp
#include <type_traits>

namespace jc {

template <class T>
struct is_member_object_pointer
    : std::bool_constant<std::is_member_pointer_v<T> &&
                         !std::is_member_function_pointer_v<T>> {};

template <class T>
constexpr bool is_member_object_pointer_v = is_member_object_pointer<T>::value;

}  // namespace jc

struct A {
  int f() const { return 1; }
  int i = 0;
};

static_assert(!jc::is_member_object_pointer_v<decltype(&A::f)>);
static_assert(!jc::is_member_object_pointer_v<int (A::*)() const>);
static_assert(!jc::is_member_object_pointer_v<void (A::*)()>);
static_assert(jc::is_member_object_pointer_v<decltype(&A::i)>);
static_assert(jc::is_member_object_pointer_v<int A::*>);
static_assert(jc::is_member_object_pointer_v<double A::*>);

int main() {}
```

* 检查可访问的 non-static 成员

```cpp
#include <type_traits>
#include <utility>
#include <vector>

#define DEFINE_HAS_VAR(V)                                                  \
  template <typename, typename = std::void_t<>>                            \
  struct has_var_##V : std::false_type {};                                 \
  template <typename T>                                                    \
  struct has_var_##V<T, std::void_t<decltype(&T::V)>> : std::true_type {}; \
  template <typename T>                                                    \
  constexpr bool has_var_##V##_v = has_var_##V<T>::value;

#define DEFINE_HAS_METHOD(F)                                           \
  template <typename, typename = std::void_t<>>                        \
  struct has_func_##F : std::false_type {};                            \
  template <typename T>                                                \
  struct has_func_##F<T, std::void_t<decltype(std::declval<T>().F())>> \
      : std::true_type {};                                             \
  template <typename T>                                                \
  constexpr bool has_func_##F##_v = has_func_##F<T>::value;

namespace jc {

DEFINE_HAS_VAR(first);
DEFINE_HAS_METHOD(begin);

}  // namespace jc

static_assert(jc::has_var_first_v<std::pair<int, int>>);
static_assert(jc::has_func_begin_v<std::vector<int>>);

int main() {}
```

* [Detection idiom](https://en.cppreference.com/w/cpp/experimental/is_detected)

```cpp
#include <type_traits>
#include <utility>
#include <vector>

namespace jc {

template <typename, template <typename...> class Op, typename... Args>
struct detector : std::false_type {};

template <template <typename...> class Op, typename... Args>
struct detector<std::void_t<Op<Args...>>, Op, Args...> : std::true_type {};

template <template <typename...> class Op, typename... Args>
using is_detected = detector<void, Op, Args...>;

template <typename T>
using has_emplace_back = decltype(std::declval<T>().emplace_back(
    std::declval<typename T::value_type>()));

template <typename T>
constexpr bool has_emplace_back_v =
    is_detected<has_emplace_back, std::remove_reference_t<T>>::value;

}  // namespace jc

static_assert(jc::has_emplace_back_v<std::vector<int>>);
static_assert(jc::has_emplace_back_v<std::vector<int>&>);
static_assert(jc::has_emplace_back_v<std::vector<int>&&>);

int main() {}
```
