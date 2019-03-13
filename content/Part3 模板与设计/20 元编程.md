## 值元编程（Value Metaprogramming）

* 常量表达式最初只能用 enum 来声明

```cpp
template<int N>
struct Fac {
  enum { value = N * Fac<N - 1>::value };
};

template<>
struct Fac<0> {
  enum { value = 1 };
};

static_assert(Fac<5>::value == 120);
```

* C++98 允许在类内部初始化 static int 常量

```cpp
template<int N>
struct Fac {
  static const int value = N * Fac<N - 1>::value;
};

template<>
struct Fac<0> {
  static const int value = 1;
};

static_assert(Fac<5>::value == 120);
```

* Modern C++ 中可以使用 constexpr，且不再局限于int

```cpp
template<int N>
struct Fac {
  static constexpr auto value = N * Fac<N - 1>::value;
};

template<>
struct Fac<0> {
  static constexpr auto value = 1;
};

static_assert(Fac<5>::value == 120);
```

* 但类内初始化 static 成员存在一个问题，如果把结果按引用传递给函数

```cpp
void f(const int&) {}
f(Fac<5>::value);
```

* 由于函数参数是引用类型，编译器必须传递 `Fac<5>::value` 的地址，而调用 `f(Fac<5>::value)` 必须类外定义 static 数据成员（[ODR-use](https://en.cppreference.com/w/cpp/language/definition)）。enum 没有地址，不会有这个问题，因此早期一般偏向使用 enum
* C++17 引入的 [inline](https://en.cppreference.com/w/cpp/language/inline) 成员变量允许多于一次定义，解决了上述函数调用的地址问题。C++17中，constexpr static 成员默认 inline

```cpp
template<int N>
struct Fac {
  static inline constexpr auto value = N * Fac<N - 1>::value;
};

template<>
struct Fac<0> {
  static inline constexpr auto value = 1;
};

void f(const int&) {}
f(Fac<5>::value); // OK
```

* C++14 的 constexpr 函数简化了元编程的写法，用二分查找计算平方根的元编程实现是

```cpp
template <int N, int L = 1, int R = N>
struct Sqrt {
  static constexpr auto M = L + (R - L) / 2;
  static constexpr auto T = N / M;
  static constexpr auto value =
      (T < M) ? Sqrt<N, L, M>::value : Sqrt<N, M + 1, R>::value;
};

template <int N, int M>
struct Sqrt<N, M, M> {  // 终止条件为 L == R
  static constexpr auto value = M - 1;
};

static_assert(Sqrt<10000>::value == 100);
```

* C++14 的 constexpr 函数对相同功能的实现是

```cpp
constexpr int Sqrt(T x) {
  if (x <= 1) return x;
  int l = 1;
  int r = x;
  while (l < r) {
    int m = l + (r - l) / 2;
    int t = x / m;
    if (m == t) {
      return m;
    } else if (m > t) {
      r = m;
    } else {
      l = m + 1;
    }
  }
  return l - 1;
}

static_assert(Sqrt(10000) == 100);
```

## 递归实例化的开销

* 下面是 `Sqrt<10000>::value` 的计算过程

```cpp
Sqrt<10000, 1, 10000>::value => [M, T] = [5000,  2]
Sqrt<10000, 1,  5000>::value => [M, T] = [2500,  4]
Sqrt<10000, 1,  2500>::value => [M, T] = [1250,  8]
Sqrt<10000, 1,  1250>::value => [M, T] = [625,  16]
Sqrt<10000, 1,   625>::value => [M, T] = [313,  31]
Sqrt<10000, 1,   338>::value => [M, T] = [157,  63]
Sqrt<10000, 1,   157>::value => [M, T] = [79,  126]
Sqrt<10000, 80,  157>::value => [M, T] = [118,  84]
Sqrt<10000, 80,  118>::value => [M, T] = [99,  101]
Sqrt<10000, 100, 118>::value => [M, T] = [109,  91]
Sqrt<10000, 100, 109>::value => [M, T] = [104,  96]
Sqrt<10000, 100, 104>::value => [M, T] = [102,  98]
Sqrt<10000, 100, 102>::value => [M, T] = [101,  99]
Sqrt<10000, 100, 101>::value => [M, T] = [100, 100]
Sqrt<10000, 101, 101>::value => 100
```

* 但编译器计算下列表达式时，将实例化所有分支

```cpp
Sqrt<10000, 1, 10000>::value => [M, T] = [5000,  2]
(2 < 5000) ? Sqrt<10000, 1, 5000>::value : Sqrt<10000,5001,10000>::value
```

* 整个计算过程产生了大量实例化，数量几乎是 N 的两倍。使用 IfThenElse 可以避免此问题，实例化数量级将降低为 `log2(N)`

```cpp
template <bool COND, typename TrueType, typename FalseType>
struct IfThenElseT {
  using Type = TrueType;
};

template <typename TrueType, typename FalseType>
struct IfThenElseT<false, TrueType, FalseType> {
  using Type = FalseType;
};

template <bool COND, typename TrueType, typename FalseType>
using IfThenElse = typename IfThenElseT<COND, TrueType, FalseType>::Type;

template <int N, int L = 1, int R = N>
struct Sqrt {
  static constexpr auto M = L + (R - L) / 2;
  static constexpr auto T = N / M;
  static constexpr auto value =
      IfThenElse<(T < M), Sqrt<N, L, M>, Sqrt<N, M + 1, R>>::value;
};

template <int N, int M>
struct Sqrt<N, M, M> {
  static constexpr auto value = M - 1;
};

static_assert(Sqrt<10000>::value == 100);
```

* 实际上直接使用 constexpr 函数是最简单高效的做法

## 计算完整性（Computational Completeness）

* 上述例子说明一个模板元编程一般包含以下部分
  * 状态变量：即模板参数
  * 迭代构造：通过递归
  * 路径选择：通过条件表达式或特化
  * 整型算法
* 如果对递归实例化体和状态变量的数量没有限制，元编程就可以在编译期对任何可计算对象进行高效计算。但标准建议最多进行 1024 层递归实例化，因此实际开发中要有节制地使用模板元编程。某些情况下模板元编程是一个重要工具，尤其是实现一些对性能要求严格的算法

## 递归实例化与递归模板实参

```cpp
template <typename T, typename U>
struct Doublify {};

template <int N>
struct Trouble {
  using LongType = Doublify<typename Trouble<N - 1>::LongType,
                            typename Trouble<N - 1>::LongType>;
};

template <>
struct Trouble<0> {
  using LongType = double;
};

Trouble<10>::LongType ouch;
```

* 对于表达式 `Trouble<N>::LongType` 的类型描述，随着递归层次的增加，代码将越来越长，复杂度与 `2^N` 成正比

```cpp
// Trouble<0>::LongType
double
// Trouble<1>::LongType
Doublify<double, double>
// Trouble<2>::LongType
Doublify<Doublify<double, double>, Doublify<double, double>>
```

* 使用递归模板实参会强制编译器生成更多的实例化体，编译器要为每个类型保存一个 mangled name，早期编译器中 mangled name 的长度一般等于 template-id 的长度，于是 `Trouble<10>::LongType` 可能产生一个长度大于 10000 个字符的 managled name，虽然现在的编译器使用了智能压缩技术，可以将 `Trouble<10>::LongType` 压缩到几百个字符，但最好避免在模板实参中递归实例化

## 类型元编程（Type Metaprogramming）

* 以前讨论过的 traits 只能进行简单的类型计算，通过元编程可以进行更复杂的类型计算

```cpp
template <typename T>
struct sp_element {
  using type = T;
};

template <typename T>
struct sp_element<T[]> {
  using type = typename sp_element<T>::type;
};

template <typename T, std::size_t N>
struct sp_element<T[N]> {
  using type = typename sp_element<T>::type;
};

template<typename T>
using sp_element_t = typename sp_element<T>::type;

static_assert(std::is_same_v<sp_element_t<int[]>, int>);
static_assert(std::is_same_v<sp_element_t<int[5][10]>, int>);
static_assert(std::is_same_v<sp_element_t<int[][10]>, int>);
static_assert(std::is_same_v<sp_element_t<int(*)[5]>, int(*)[5]>);
```

* [std::integral_constant](https://en.cppreference.com/w/cpp/types/integral_constant) 是所有 type traits 的基类，它提供了基本的元函数

```cpp
template <class T, T v>
struct integral_constant {
  static constexpr T value = v;
  using value_type = T;
  using type = integral_constant;  // using injected-class-name
  constexpr operator value_type() const noexcept { return value; }
  constexpr value_type operator()() const noexcept { return value; }
};
```

* 可以直接继承 [std::integral_constant](https://en.cppreference.com/w/cpp/types/integral_constant) 来使用其中提供的元函数，这种方式称为元函数转发

```cpp
template <class T, T v>
struct A {
  static constexpr T value = v;
};

template <int N, int... Ns>
struct Max;

template <int N>
struct Max<N> : A<int, N> {};

template <int N1, int N2, int... Ns>
struct Max<N1, N2, Ns...> : A <int,
  N1 < N2 ? Max<N2, Ns...>::value : Max<N1, Ns...>::value> {};

template <int... Ns>
constexpr auto max_v = Max<Ns...>::value;

static_assert(max_v<3, 2, 1, 5, 4> == 5);
```

## 混合元编程（Hybrid Metaprogramming）

* 在编译期以编程的方式汇编小片带运行期效率的代码称为混合元编程。比如计算两个数组的点积

```cpp
template <typename T, std::size_t N>
auto dotProduct(const std::array<T, N>& a, const std::array<T, N>& b) {
  T res{};
  for (std::size_t i = 0; i < N; ++i) {
    res += a[i] * b[i];
  }
  return res;
}
```

* 在一些机器上，for 循环的汇编将产生分支指令，可能比直接写出循环造成更大的开销

```cpp
res += x[0]*y[0];
res += x[1]*y[1];
res += x[2]*y[2];
res += x[3]*y[3];
```

* 不过现代编译器将优化循环为目标平台最高效形式。使用元编程可以展开循环，虽然现在已经没有这个必要，但出于讨论还是给出实现

```cpp
template <typename T, std::size_t N>
struct DotProductT {
  static inline T value(const T* a, const T* b) {
    return *a * *b + DotProductT<T, N - 1>::value(a + 1, b + 1);
  }
};

template <typename T>
struct DotProductT<T, 0> {
  static inline T value(const T*, const T*) { return T{}; }
};

template <typename T, std::size_t N>
auto dotProduct(const std::array<T, N>& x, const std::array<T, N>& y) {
  return DotProductT<T, N>::value(&*begin(x), &*begin(y));
}
```

* [std::array](https://en.cppreference.com/w/cpp/container/array) 只是定长数组，实际上混合元编程中最强大的容器是 [std::tuple](https://en.cppreference.com/w/cpp/utility/tuple)，它可以包含任意数量任意类型的元素

```cpp
std::tuple<int, std::string, bool> t{42, "Answer", true};
```

## 对 [Unit Type](https://en.wikipedia.org/wiki/Unit_type) 的混合元编程

* 一些库用来计算不同 [unit type](https://en.wikipedia.org/wiki/Unit_type) 值结果，也体现了混合元编程的能力
* 以时间为例，主要的 unit 是秒，一毫秒就可以用 1/1000 来表示，一分钟则用 60/1 表示，为此先定义一个用于表示 unit 比值的类型 Ratio

```cpp
#include <iostream>

template <unsigned N, unsigned D = 1>
struct Ratio {
  static constexpr unsigned num = N;  // 分子
  static constexpr unsigned den = D;  // 分母
  using Type = Ratio<num, den>;
};

template <typename R1, typename R2>
struct RatioAddImpl {
 private:
  static constexpr unsigned den = R1::den * R2::den;
  static constexpr unsigned num = R1::num * R2::den + R2::num * R1::den;

 public:
  using Type = Ratio<num, den>;
};

template <typename R1, typename R2>
using RatioAdd = typename RatioAddImpl<R1, R2>::Type;

int main() {
  using R1 = Ratio<1, 1000>;
  using R2 = Ratio<2, 3>;
  using RS = RatioAdd<R1, R2>;                     // Ratio<2003,3000>
  std::cout << RS::num << '/' << RS::den << '\n';  // 2003/3000
  using RA = RatioAdd<Ratio<2, 3>, Ratio<5, 7>>;   // Ratio<29,21>
  std::cout << RA::num << '/' << RA::den << '\n';  // 29/21
}
```

* 基于此模板可以定义一个表示时间的类模板

```cpp
#include <iostream>

template <unsigned N, unsigned D = 1>
struct Ratio {
  static constexpr unsigned num = N;  // 分子
  static constexpr unsigned den = D;  // 分母
  using Type = Ratio<num, den>;
};

template <typename R1, typename R2>
struct RatioAddImpl {
 private:
  static constexpr unsigned den = R1::den * R2::den;
  static constexpr unsigned num = R1::num * R2::den + R2::num * R1::den;

 public:
  using Type = Ratio<num, den>;
};

template <typename R1, typename R2>
using RatioAdd = typename RatioAddImpl<R1, R2>::Type;

template <typename T, typename U = Ratio<1>>
class Duration {
 public:
  using ValueType = T;
  using UnitType = typename U::Type;  // 时间的基本单位

 private:
  ValueType val;

 public:
  constexpr Duration(ValueType v = 0) : val(v) {}
  constexpr ValueType value() const { return val; }
};

template <typename T1, typename U1, typename T2, typename U2>
constexpr auto operator+(const Duration<T1, U1>& a, const Duration<T2, U2>& b) {
  using VT = Ratio<1, RatioAdd<U1, U2>::den>;  // Ratio<1, U1::den * U2::den>
  auto res =
      (a.value() * U1::num / U1::den + b.value() * U2::num / U2::den) * VT::den;
  return Duration<decltype(res), VT>{res};
}

int main() {
  auto a = Duration<int, Ratio<1, 1000>>(10);   // 10 毫秒
  auto b = Duration<double, Ratio<1, 3>>(7.5);  // 2.5 秒
  auto c = a + b;  // c = 10 * 3 + 7.5 * 1000 = 7530 * 1/3000 秒
  using Type = decltype(c)::UnitType;
  std::cout << c.value() << "*" << Type::num << '/' << Type::den;
}
```

* 此外，`operator+` 是constexpr，如果值在编译期已知，就能直接在编译期计算，[std::chrono](https://en.cppreference.com/w/cpp/header/chrono) 使用的就是这个方法，并做了一些改进，比如使用预定义 unit（如 [std::chrono::milliseconds](https://en.cppreference.com/w/cpp/chrono/duration)）来支持[时间字面量](https://en.cppreference.com/mwiki/index.php?title=Special%3ASearch&search=std%3A%3Aliterals%3A%3Achrono_literals%3A%3Aoperator&button=)和溢出处理

```cpp
#include <iostream>
#include <chrono>

constexpr std::chrono::seconds operator ""s(unsigned long long s) {
  return std::chrono::seconds(s);
}

constexpr std::chrono::minutes operator ""min(unsigned long long m) {
  return std::chrono::minutes(m);
}

constexpr std::chrono::hours operator ""h(unsigned long long h) {
  return std::chrono::hours(h);
}

int main() {
  using namespace std::chrono_literals;
  std::cout << (1h + 1min + 30s).count(); // 3690
}
```

## 反射

* 模板元编程的方案选择必须从三个角度考虑：
  * Computation（计算）
  * Reflection（反射）：以编程方式检查程序特征的能力
  * Generation（生成）：为程序生成附加代码的能力
* 前面提到了递归实例化和 constexpr 函数两种方案。为了进行反射，type traits 中可以找到部分解决方案，但远不能覆盖反射所需的全部内容。比如给定一个类类型，许多应用程序都希望以编程方式探索该类的成员。现有的 traits 基于模板实例化，因此 C++ 可以提供附加的语言能力或内在库组件，在编译期生成包含反射信息的类模板实例。这是匹配基于递归模板实例化的方案，但类模板实例会消耗大量编译器空间，直到编译结束时才能释放。另一种与 constexpr 计算匹配的方案是引入一种新的标准类型来表示反射信息
* 在 C++ 中创建一个灵活的、通用的和程序员友好的代码生成机制仍是各方正在研究的挑战，但模板实例化也勉强算是一种代码生成机制，此外，编译器扩展内联函数的调用已经足够可靠，该机制也可以用作代码生成的工具。结合更强大的反射能力，现有技术已能实现卓越的元编程效果
