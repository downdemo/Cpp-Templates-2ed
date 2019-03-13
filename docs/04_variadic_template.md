## 变参模板（Variadic Template）

* 如果函数要接受任意数量任意类型的参数，没有模板时可以通过 [std::va_list](https://en.cppreference.com/w/cpp/utility/variadic/va_list) 实现

```cpp
#include <cassert>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <string>

namespace jc {

void test(int n, ...) {
  std::va_list args;
  va_start(args, n);
  assert(va_arg(args, double) == 3.14);
  assert(va_arg(args, int) == 42);
  assert(std::strcmp(va_arg(args, const char*), "hello") == 0);
  assert(std::strcmp(va_arg(args, const char*), "world") == 0);
  va_end(args);
}

void test(const char* fmt, ...) {
  char buf[256];
  std::va_list args;
  va_start(args, fmt);
  std::vsnprintf(buf, 256, fmt, args);
  va_end(args);
  assert(std::strcmp(buf, "3.14 42 hello world") == 0);
}

}  // namespace jc

int main() {
  jc::test(4, 3.14, 42, std::string{"hello"}.c_str(), "world");
  jc::test("%.2f %d %s %s", 3.14, 42, std::string{"hello"}.c_str(), "world");
}
```

* C++11 引入了变参模板，用省略号表示一个[参数包](https://en.cppreference.com/w/cpp/language/parameter_pack)，类型名后接省略号表示任意数量给定类型的参数。在表达式后跟省略号，如果表达式中有参数包，就会把表达式应用到参数包中的每个参数。如果表达式中出现两次参数包，对整个表达式扩展，而不会做笛卡尔积计算

```cpp
#include <iostream>
#include <string>
#include <tuple>
#include <utility>

namespace jc {

void print() {}  // 参数包展开到零参数时调用

template <typename T, typename... Args>
void print(const T& t, Args&&... args) {
  std::cout << t << ",";
  print(std::forward<Args>(args)...);
}

template <int... Index>
struct A {};

template <typename... List, int... Index>
void test1(const std::tuple<List...>& t, A<Index...>) {
  print(std::get<Index>(t)...);  // print(std::get<2>(t), std::get<3>(t));
}

template <typename... List, int... Index>
void test2(const std::tuple<List...>& t, A<Index...>) {
  print((std::get<Index>(t) + std::get<Index>(t))...);
}

}  // namespace jc

int main() {
  auto t = std::make_tuple(3.14, 42, std::string{"hello"}, "world");
  jc::test1(t, jc::A<2, 3>{});     // hello,world
  jc::test2(t, jc::A<0, 1, 2>{});  // 6.28,84,hellohello,
}
```

* 注意参数包的省略号不能直接接在数值字面值后

```cpp
template <typename... Args>
void f(const Args&... args) {
  print(args + 1...);  // ERROR：1... 是带多个小数点的字面值，不合法
  print(args + 1 ...);   // OK
  print((args + 1)...);  // OK
}
```

* 可以直接用逗号运算符做参数包扩展，逗号左侧是对参数包每个元素做的操作，右侧是一个无关紧要的值，这样展开后对每个元素都做了操作，并形成了一个以无关值为元素的数组，这个数组无作用，只是为了满足扩展时省略号不能为表达式最后的 token 而引入

```cpp
#include <iostream>
#include <string>
#include <utility>

namespace jc {

template <typename... Args>
void print(Args&&... args) {
  auto a = {(std::cout << std::forward<Args>(args) << std::endl, 0)...};
}

}  // namespace jc

int main() { jc::print(3.14, 42, std::string{"hello"}, "world"); }
```

* C++11 引入了 [sizeof...](https://en.cppreference.com/w/cpp/language/sizeof...) 在编译期计算参数包中的元素数，C++17 引入了 if constexpr 判断编译期值，编译期结果为 true 才会实例化代码

```cpp
#include <iostream>
#include <string>
#include <utility>

namespace jc {

template <typename T, typename... Args>
void print(const T& t, Args&&... args) {
  std::cout << t << std::endl;
  if constexpr (sizeof...(args) > 0) {  // 不能用 if，因为零长包也会实例化代码
    print(std::forward<Args>(args)...);  // 当条件满足时才实例化
  }
}

}  // namespace jc

int main() { jc::print(3.14, 42, std::string{"hello"}, "world"); }
```

* 在 C++11 中可以利用偏特化来达到 if constexpr 的效果

```cpp
#include <iostream>
#include <string>
#include <utility>

namespace jc {

template <bool b>
struct A;

template <typename T, typename... Args>
void print(const T& t, Args&&... args) {
  std::cout << t << std::endl;
  A<(sizeof...(args) > 0)>::f(std::forward<Args>(args)...);
}

template <bool b>
struct A {
  template <typename... Args>
  static void f(Args&&... args) {
    print(std::forward<Args>(args)...);
  }
};

template <>
struct A<false> {
  static void f(...) {}
};

}  // namespace jc

int main() { jc::print(3.14, 42, std::string{"hello"}, "world"); }
```

## [折叠表达式（Fold Expression）](https://en.cppreference.com/w/cpp/language/fold)

* C++17 引入了折叠表达式，用于获取对所有参数包实参使用二元运算符的计算结果

```cpp
#include <iostream>
#include <tuple>
#include <utility>

namespace jc {

template <typename... Args>
auto sum(Args&&... args) {
  auto a = (... + std::forward<Args>(args));      // (((1 + 2) + 3) + 4)
  auto b = (std::forward<Args>(args) + ...);      // (1 + (2 + (3 + 4)))
  auto c = (5 + ... + std::forward<Args>(args));  // ((((5 + 1) + 2) + 3) + 4)
  auto d = (std::forward<Args>(args) + ... + 5);  // (1 + (2 + (3 + (4 + 5))))
  return std::make_tuple(a, b, c, d);
}

auto print1 = [](auto&&... args) {
  // operator<< 左折叠，std::cout 是初始值
  (std::cout << ... << std::forward<decltype(args)>(args));
};

auto print2 = [](auto&&... args) {
  // operator, 左折叠
  ((std::cout << std::forward<decltype(args)>(args) << ","), ...);
};

}  // namespace jc

int main() {
  auto [a, b, c, d] = jc::sum(1, 2, 3, 4);
  jc::print1(a, b, c, d);  // 10101515
  jc::print2(a, b, c, d);  // 10,10,15,15,
}
```

* 对于空扩展需要决定类型和值，空的一元折叠表达式通常会产生错误，除了三种例外情况
  * 一个 `&&` 的一元折叠的空扩展产生值 true
  * 一个 `||` 的一元折叠的空扩展产生值 false
  * 一个 `,` 的一元折叠空扩展产生一个 void 表达式

|折叠表达式|计算结果|
|:-:|:-:|
|(... op pack)|(((pack1 op pack2) op pack3) ... op PackN)|
|(pack op ...)|(pack1 op (... (packN-1 op packN)))|
|(init op ... op pack)|(((init op pack1) op pack2) ... op PackN)|
|(pack op ... op init)|(pack1 op (... (packN op init)))|

* 折叠表达式借鉴的是 [Haskell](https://www.haskell.org/) 的 fold

```hs
import Data.List (foldl')

foldlList :: [Char]
foldlList = foldl' (\x y -> concat ["(", x, "+", y, ")"]) "0" (map show [1 .. 4])

foldrList :: [Char]
foldrList = foldr ((\x y -> concat ["(", x, "+", y, ")"]) . show) "0" [1 .. 4]

main :: IO ()
main = do
  putStrLn foldlList -- ((((0+1)+2)+3)+4)
  putStrLn foldrList -- (1+(2+(3+(4+0))))
```

* 实现与 Haskell 类似的左折叠和右折叠

```cpp
#include <iostream>
#include <string>
#include <type_traits>

namespace jc {

template <typename F, typename T, typename... Args>
void foldlList(F&& f, T&& zero, Args&&... x) {
  ((std::invoke(std::forward<F>(f), (std::string(sizeof...(Args), '('))),
    std::invoke(std::forward<F>(f), (std::forward<T>(zero)))),
   ...,
   (std::invoke(std::forward<F>(f), ('+')),
    std::invoke(std::forward<F>(f), (std::forward<Args>(x))),
    std::invoke(std::forward<F>(f), (')'))));
}

template <typename F, typename T, typename... Args>
void foldrList(F&& f, T&& zero, Args&&... x) {
  ((std::invoke(std::forward<F>(f), ('(')),
    std::invoke(std::forward<F>(f), (std::forward<Args>(x))),
    std::invoke(std::forward<F>(f), ('+'))),
   ...,
   (std::invoke(std::forward<F>(f), (std::forward<T>(zero))),
    std::invoke(std::forward<F>(f), (std::string(sizeof...(Args), ')')))));
}

}  // namespace jc

int main() {
  auto print = [](const auto& x) { std::cout << x; };
  jc::foldlList(print, 0, 1, 2, 3, 4);  // ((((0+1)+2)+3)+4)
  jc::foldrList(print, 0, 1, 2, 3, 4);  // (1+(2+(3+(4+0))))
}
```

* 折叠表达式几乎可以使用所有二元运算符

```cpp
#include <cassert>

namespace jc {

struct Node {
  Node(int i) : val(i) {}

  int val = 0;
  Node* left = nullptr;
  Node* right = nullptr;
};

// 使用 operator->* 的折叠表达式，用于遍历指定的二叉树路径
template <typename T, typename... Args>
Node* traverse(T root, Args... paths) {
  return (root->*...->*paths);  // root ->* paths1 ->* paths2 ...
}

void test() {
  Node* root = new Node{0};
  root->left = new Node{1};
  root->left->right = new Node{2};
  root->left->right->left = new Node{3};

  auto left = &Node::left;
  auto right = &Node::right;
  Node* node1 = traverse(root, left);
  assert(node1->val == 1);
  Node* node2 = traverse(root, left, right);
  assert(node2->val == 2);
  Node* node3 = traverse(node2, left);
  assert(node3->val == 3);
}

}  // namespace jc

int main() { jc::test(); }
```

* 包扩展可以用于编译期表达式

```cpp
#include <type_traits>

namespace jc {

template <typename T, typename... Args>
constexpr bool is_homogeneous(T, Args...) {
  return (std::is_same_v<T, Args> && ...);  // operator&& 的折叠表达式
}

}  // namespace jc

static_assert(!jc::is_homogeneous(3.14, 42, "hello", "world"));
static_assert(jc::is_homogeneous("hello", "", "world"));

int main() {}
```

## 变参模板的应用

* 无需指定类型，自动获取 [std::variant](https://en.cppreference.com/w/cpp/utility/variant) 值

```cpp
#include <array>
#include <cassert>
#include <functional>
#include <string>
#include <type_traits>
#include <variant>

namespace jc {

template <typename F, std::size_t... N>
constexpr auto make_array_impl(F f, std::index_sequence<N...>)
    -> std::array<std::invoke_result_t<F, std::size_t>, sizeof...(N)> {
  return {std::invoke(f, std::integral_constant<decltype(N), N>{})...};
}

template <std::size_t N, typename F>
constexpr auto make_array(F f)
    -> std::array<std::invoke_result_t<F, std::size_t>, N> {
  return make_array_impl(f, std::make_index_sequence<N>{});
}

template <typename T, typename Dst, typename... List>
bool get_value_impl(const std::variant<List...>& v, Dst& dst) {
  if (std::holds_alternative<T>(v)) {
    if constexpr (std::is_convertible_v<T, Dst>) {
      dst = static_cast<Dst>(std::get<T>(v));
      return true;
    }
  }
  return false;
}

template <typename Dst, typename... List>
bool get_value(const std::variant<List...>& v, Dst& dst) {
  using Variant = std::variant<List...>;
  using F = std::function<bool(const Variant&, Dst&)>;
  static auto _list = make_array<sizeof...(List)>([](auto i) -> F {
    return &get_value_impl<std::variant_alternative_t<i, Variant>, Dst,
                           List...>;
  });
  return std::invoke(_list[v.index()], v, dst);
}

}  // namespace jc

int main() {
  std::variant<int, std::string> v = std::string{"test"};
  std::string s;
  assert(jc::get_value(v, s));
  assert(s == "test");
  v = 42;
  int i;
  assert(jc::get_value(v, i));
  assert(i == 42);
}
```

* 字节序转换

```cpp
// https://en.cppreference.com/w/cpp/language/fold

#include <cstdint>
#include <type_traits>
#include <utility>

namespace jc {

template <typename T, size_t... N>
constexpr T bswap_impl(T t, std::index_sequence<N...>) {
  return (((t >> N * 8 & 0xFF) << (sizeof(T) - 1 - N) * 8) | ...);
}

template <typename T, typename U = std::make_unsigned_t<T>>
constexpr U bswap(T t) {
  return bswap_impl<U>(t, std::make_index_sequence<sizeof(T)>{});
}

}  // namespace jc

static_assert(jc::bswap<std::uint32_t>(0x12345678u) == 0x78563412u);
static_assert((0x12345678u >> 0) == 0x12345678u);
static_assert((0x12345678u >> 8) == 0x00123456u);
static_assert((0x12345678u >> 16) == 0x00001234u);
static_assert((0x12345678u >> 24) == 0x00000012u);
static_assert(jc::bswap<std::uint16_t>(0x1234u) == 0x3412u);

int main() {}
```

* [自定义字面值（User-defined literals）](https://en.cppreference.com/w/cpp/language/user_literal)

```cpp
#include <algorithm>
#include <array>
#include <cassert>
#include <functional>
#include <iostream>
#include <sstream>
#include <string>

namespace jc {

template <char... args>
std::string operator"" _dbg() {
  std::array<char, sizeof...(args)> v{args...};
  std::stringstream os;
  for (const auto& x : v) {
    os << x;
  };
#ifndef NDEBUG
  std::cout << os.str() << std::endl;
#endif
  return os.str();
}

std::string operator"" _encrypt(const char* c, size_t) {
  std::string s{c};
  std::string p{R"(passwd: ")"};
  auto it = std::search(s.begin(), s.end(),
                        std::boyer_moore_horspool_searcher{p.begin(), p.end()});
  if (it != s.end()) {
    it += p.size();
    while (it != s.end() && *it != '\"') {
      *it++ = '*';
    }
  }
#if !defined(NDEBUG)
  std::cout << s << std::endl;
#endif
  return s;
}

}  // namespace jc

int main() {
  using namespace jc;

  assert(12.34_dbg == "12.34");

  std::string s = R"JSON({
  data_dir: "C:\Users\downdemo\.data\*.txt",
  user: "downdemo(accelerate rapidly)",
  passwd: "123456"
})JSON"_encrypt;

  std::string s2 = R"JSON({
  data_dir: "C:\Users\downdemo\.data\*.txt",
  user: "downdemo(accelerate rapidly)",
  passwd: "******"
})JSON";

  assert(s == s2);
}
```

* 变参基类

```cpp
#include <string>
#include <unordered_set>

namespace jc {

struct A {
  std::string s;
};

struct A_EQ {
  bool operator()(const A& lhs, const A& rhs) const { return lhs.s == rhs.s; }
};

struct A_Hash {
  std::size_t operator()(const A& a) const {
    return std::hash<std::string>{}(a.s);
  }
};

// 定义一个组合所有基类的 operator() 的派生类
template <typename... Bases>
struct Overloader : Bases... {
  using Bases::operator()...;  // C++17
};

using A_OP = Overloader<A_Hash, A_EQ>;

}  // namespace jc

int main() {
  // 将 A_EQ 和 A_Hash 组合到一个类型中

  /* unordered_set 的声明
  template<
  class Key,
      class Hash = std::hash<Key>,
      class KeyEqual = std::equal_to<Key>,
      class Allocator = std::allocator<Key>
  > class unordered_set;
  */

  std::unordered_set<jc::A, jc::A_Hash, jc::A_EQ> s1;
  std::unordered_set<jc::A, jc::A_OP, jc::A_OP> s2;
}
```

* C++14 使用 [std::integer_sequence](https://en.cppreference.com/w/cpp/utility/integer_sequence) 遍历 [std::tuple](https://en.cppreference.com/w/cpp/utility/tuple)

```cpp
#include <cstddef>
#include <functional>
#include <iostream>
#include <tuple>
#include <type_traits>
#include <utility>

namespace jc {

template <typename F, typename... List, std::size_t... Index>
void apply_impl(F&& f, const std::tuple<List...>& t,
                std::index_sequence<Index...>) {
  std::invoke(std::forward<F>(f), std::get<Index>(t)...);
}

template <typename F, typename... List>
void apply(F&& f, const std::tuple<List...>& t) {
  apply_impl(std::forward<F>(f), t, std::index_sequence_for<List...>{});
}

}  // namespace jc

struct Print {
  template <typename... Args>
  void operator()(const Args&... args) {
    auto no_used = {(std::cout << args << " ", 0)...};
  }
};

int main() {
  auto t = std::make_tuple(3.14, 42, "hello world");
  jc::apply(Print{}, t);  // 3.14 42 hello world
}
```

* C++11 未提供 [std::integer_sequence](https://en.cppreference.com/w/cpp/utility/integer_sequence)，手动实现一个即可

```cpp
#include <cstddef>
#include <functional>
#include <iostream>
#include <tuple>
#include <type_traits>
#include <utility>

namespace jc {

template <std::size_t... Index>
struct index_sequence {
  using type = index_sequence<Index...>;
};

template <typename List1, typename List2>
struct concat;

template <std::size_t... List1, std::size_t... List2>
struct concat<index_sequence<List1...>, index_sequence<List2...>>
    : index_sequence<List1..., (sizeof...(List1) + List2)...> {};

template <typename List1, typename List2>
using concat_t = typename concat<List1, List2>::type;

template <std::size_t N>
struct make_index_sequence_impl
    : concat_t<typename make_index_sequence_impl<N / 2>::type,
               typename make_index_sequence_impl<N - N / 2>::type> {};

template <>
struct make_index_sequence_impl<0> : index_sequence<> {};

template <>
struct make_index_sequence_impl<1> : index_sequence<0> {};

template <std::size_t N>
using make_index_sequence = typename make_index_sequence_impl<N>::type;

template <typename... Types>
using index_sequence_for = make_index_sequence<sizeof...(Types)>;

static_assert(std::is_same_v<make_index_sequence<3>, index_sequence<0, 1, 2>>);

template <typename F, typename... List, std::size_t... Index>
void apply_impl(F&& f, const std::tuple<List...>& t, index_sequence<Index...>) {
  std::invoke(std::forward<F>(f), std::get<Index>(t)...);
}

template <typename F, typename... List>
void apply(F&& f, const std::tuple<List...>& t) {
  apply_impl(std::forward<F>(f), t, index_sequence_for<List...>{});
}

}  // namespace jc

struct Print {
  template <typename... Args>
  void operator()(const Args&... args) {
    auto no_used = {(std::cout << args << " ", 0)...};
  }
};

int main() {
  auto t = std::make_tuple(3.14, 42, "hello world");
  jc::apply(Print{}, t);  // 3.14 42 hello world
}
```
