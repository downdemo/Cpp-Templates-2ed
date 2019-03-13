# 01 一个实例：累加一个序列

## 1.1 Fixed Traits

```cpp
#include <iostream>

template <typename T>
T accum(const T* beg, const T* end) {
  T total{};
  while (beg != end) {
    total += *beg;
    ++beg;
  }
  return total;
}

int main() {
  char name[] = "templates";
  int length = sizeof(name) - 1;
  std::cout << accum(name, name + length) / length << '\n';  // -5
}
```

* 上述代码的问题是，对于 char 类型希望计算对应 ASCII 码的平均值，但结果却是 -5（预期结果是 108），原因在于模板基于 char 类型实例化，total 类型为 char 导致结果出现了越界。多引入一个模板参数 AccT 来指定 total 的类型即可解决此问题，但这样做的缺点是每次调用都要显式指定这个类型。另一个方法是使用 traits

```cpp
template <typename T>
struct Accumulationtraits;

template <>
struct Accumulationtraits<char> {
  using AccT = int;
};

template <typename T>
auto accum(const T* beg, const T* end) {
  using AccT = typename Accumulationtraits<T>::AccT;
  AccT total{};
  while (beg != end) {
    total += *beg;
    ++beg;
  }
  return total;
}
```

## 1.2 Value Traits

* accum 模板使用了默认构造函数的返回值初始化 total，但类型 AccT 不一定有一个默认构造函数。为了解决这个问题，需要添加一个 value traits

```cpp
template<typename T>
struct Accumulationtraits;

template<>
struct Accumulationtraits<char> {
  using AccT = int;
  static const AccT zero = 0;
};

template<typename T>
auto accum(const T* beg, const T* end)
{
  using AccT = typename Accumulationtraits<T>::AccT;
  AccT total = Accumulationtraits<T>::zero;
  while (beg != end)
  {
    total += *beg;
    ++beg;
  }
  return total;
}
```

* 但这种方法的缺点是，类内初始化的 static 成员变量只能是整型（int、long、unsigned）常量或枚举类型

```cpp
template <>
struct Accumulationtraits<float> {
  using Acct = float;
  static constexpr float zero = 0.0f;  // ERROR: not an integral type
};

class BigInt {
  BigInt(long long);
};

template <>
struct Accumulationtraits<BigInt> {
  using AccT = BigInt;
  static constexpr BigInt zero = BigInt{0};  // ERROR: not a literal type
};
```

* 直接的解决方法是在类外定义 value traits

```cpp
template <>
struct Accumulationtraits<BigInt> {
  using AccT = BigInt;
  static const BigInt zero;  // 仅声明
};
// 在源文件中初始化
const BigInt Accumulationtraits<BigInt>::zero = BigInt{0};
```

* 但这个方法仍有缺点，编译器不知道其他文件的定义，也就不知道 zero 的值为 0。C++17 可以使用 inline 变量解决此问题

```cpp
template <>
struct Accumulationtraits<BigInt> {
  using AccT = BigInt;
  inline static const BigInt zero = BigInt{0};  // OK since C++17
};
```

* C++17 的另一种优先做法是使用内联成员函数。如果返回一个 literal type，可以将函数声明为 constexpr

```cpp
template <typename T>
class Accumulationtraits;

template <>
struct Accumulationtraits<float> {
  using AccT = double;
  static constexpr AccT zero() { return 0; }
};

template <>
struct Accumulationtraits<BigInt> {
  using AccT = BigInt;
  static BigInt zero() { return BigInt{0}; }
};
```

* 使用的区别只是由访问静态数据成员改为使用函数调用语法

```cpp
AccT total = Accumulationtraits<T>::zero();
```

## 1.3 Parameterized Traits

* 添加一个模板参数来参数化traits

```cpp
template <typename T, typename Traits = Accumulationtraits<T>>
auto accum(const T* beg, const T* end) {
  typename Traits::AccT total = Traits::zero();
  while (beg != end) {
    total += *beg;
    ++beg;
  }
  return total;
};
```

# 02 Traits versus Policies and Policy Classes

* 除了求和还有其他形式的累积问题，如求积、连接字符串，或者找出序列中的最大值。这些问题只需要修改 `total += *beg` 即可，把这个算法作为一个 static 函数模板提取到一个 Policy 类中

```cpp
class MultPolicy {
 public:
  template <typename T, typename U>
  static void accumulate(T& total, const U& value) {
    total *= value;
  }
};

template <typename T, typename Policy = MultPolicy,
          typename Traits = AccumulationTraits<T>>
auto accum(const T* beg, const T* end) {
  using AccT = typename Traits::AccT;
  AccT total = Traits::zero();
  while (beg != end) {
    Policy::accumulate(total, *beg);
    ++beg;
  }
  return total;
}

int main() {
  int num[] = {1, 2, 3, 4, 5};
  std::cout << accum<int, MultPolicy>(num, num + 5);
}
```

* 但输出结果却是 0，原因是对于求积，0 是错误的初值，应当把 value traits 值改为 1

## 2.1 Member Templates versus Template Template Parameters

* 之前把 policy 实现为含成员模板的普通类，下面用类模板实现 policy 类，并将其用作模板的模板参数来修改 Accum 接口

```cpp
template <typename T, typename U>
class MultPolicy {
 public:
  static void accumulate(T& total, const U& value) { total *= value; }
};

template <typename T, template <typename, typename> class Policy = MultPolicy,
          typename traits = Accumulationtraits<T>>
auto accum(const T* beg, const T* end) {
  using AccT = typename traits::AccT;
  AccT total = traits::zero();
  while (beg != end) {
    Policy<AccT, T>::accumulate(total, *beg);
    ++beg;
  }
  return total;
}
```

* 用类模板实现 policy 的优点是，可以让 policy 类携带一些依赖于模板参数的状态信息（即 static 数据成员），缺点则是使用时需要定义模板参数的确切个数，使得 traits 的表达式更冗长

## 2.2 traits 和 policy 的区别

* traits 更注重于类型
  * traits 可以不需要通过额外的模板参数来传递（fixed traits）
  * traits 参数通常有十分自然的、很少或不能被改写的默认值
  * traits 参数紧密依赖于一个或多个主参数
  * traits 一般包含类型和常量
  * traits 通常由 traits 模板实现
* policy 更注重于行为
  * policy calss 需要额外的模板参数来传递
  * policy 参数不需要有默认值，通常都是显式指定（尽管许多泛型组件都配置了使用频率很高的缺省 policy）
  * policy 参数和模板的其他模板参数通常是正交的
  * policy 类一般包含成员函数
  * policy 既可以用普通类实现，也可以用类模板实现
* traits 和 policy 可以控制模板参数个数，但是多个 traits 和 policy 的排序问题就出现了。一种简单的策略是按使用频率升序排序，即 traits 参数位于 policy 参数右边，因为 policy 参数通常会被改写

## 2.3 运用普通的迭代器进行累积

* 下面是一个新版本的的 accum，它不仅支持指针，还支持普通的迭代器

```cpp
#include <iterator>

template <typename Iter>
auto accum(Iter beg, Iter end) {
  using VT = typename std::iterator_traits<Iter>::value_type;
  VT total{};
  while (beg != end) {
    total += *beg;
    ++beg;
  }
  return total;
}
```

* [std::iterator_traits](https://en.cppreference.com/w/cpp/iterator/iterator_traits) 封装了迭代器的相关属性

```cpp
namespace std {
template <typename T>
struct iterator_traits<T*> {
  using difference_type = ptrdiff_t;
  using value_type = T;
  using pointer = T*;
  using reference = T&;
  using iterator_category = random_access_iterator_tag;
};
}  // namespace std
```

# 03 类型函数（Type Function）

## 3.1 确定元素类型

* 下面用偏特化为给定容器指出元素类型

```cpp
#include <iostream>
#include <vector>

template <typename T>
struct ElementT;  // primary template

template <typename T>
struct ElementT<std::vector<T>> {  // partial specialization for std::vector
  using Type = T;
};

template <typename T, std::size_t N>
struct ElementT<T[N]> {  // partial specialization for arrays of known bounds
  using Type = T;
};

template <typename T>
struct ElementT<T[]> {  // partial specialization for arrays of unknown bounds
  using Type = T;
};

template <typename T>
using ElementType = typename ElementT<T>::Type;

template <typename T>
void printElementType(const T& c) {
  std::cout << typeid(ElementType<T>).name();
}

int main() {
  std::vector<bool> v;
  printElementType(v);  // bool
  int arr[42];
  printElementType(arr);  // int
}
```

* 大多数情况下，类型函数是和容器类型一起实现的，如果容器类型定义了一个成员类型 value_type，可以定义如下 traits

```cpp
template <typename C>
struct ElementT {
  using Type = typename C::value_type;
};
```

* 类型函数允许根据容器类型来参数化模板，而不需要指定元素类型的模板参数，比如

```cpp
template <typename T, typename C>
T f(const C& c);
```

* 可以更方便地写为

```cpp
template <typename C>
ElementType<C> f(const C& c);
```

## 3.2 Transformation traits

### 3.2.1 移除引用

```cpp
template <typename T>
struct RemoveReferenceT {
  using Type = T;
};

template <typename T>
struct RemoveReferenceT<T&> {
  using Type = T;
};

template <typename T>
struct RemoveReferenceT<T&&> {
  using Type = T;
};

template <typename T>
using RemoveReference = typename RemoveReferenceT<T>::Type;
```

* 标准库提供了对应的 [std::remove_reference](https://en.cppreference.com/w/cpp/types/remove_reference)

### 3.2.2 添加引用

```cpp
template <typename T>
struct AddLValueReferenceT {
  using Type = T&;
};

template <typename T>
using AddLValueReference = typename AddLValueReferenceT<T>::Type;

template <typename T>
struct AddRValueReferenceT {
  using Type = T&&;
};

template <typename T>
using AddRValueReference = typename AddRValueReferenceT<T>::Type;
```

* 引用折叠在这里会生效，如 `AddLValueReference<int&&>` 将折叠为 int&
* 如果不引入特化，可以将其简化为

```cpp
template <typename T>
using AddLValueReferenceT = T&;

template <typename T>
using AddRValueReferenceT = T&&;
```

* 这样可以不用实例化类模板，但对于 void 作为模板实参的情况，仍需要类模板特化（别名模板不能被特化）

```cpp
template <>
struct AddLValueReferenceT<void> {
  using Type = void;
};

template <>
struct AddLValueReferenceT<const void> {
  using Type = const void;
};

template <>
struct AddLValueReferenceT<volatile void> {
  using Type = volatile void;
};

template <>
struct AddLValueReferenceT<const volatile void> {
  using Type = const volatile void;
};
```

* 标准库提供了对应的 [std::add_lvalue_reference](https://en.cppreference.com/w/cpp/types/add_reference) 和 [std::add_rvalue_reference](https://en.cppreference.com/w/cpp/types/add_reference)，标准模板包含了 void 类型的特化

### 3.2.3 移除限定符

```cpp
template <typename T>
struct RemoveConstT {
  using Type = T;
};

template <typename T>
struct RemoveConstT<const T> {
  using Type = T;
};

template <typename T>
using RemoveConst = typename RemoveConstT<T>::Type;

template <typename T>
struct RemoveVolatileT {
  using Type = T;
};

template <typename T>
struct RemoveVolatileT<volatile T> {
  using Type = T;
};

template <typename T>
using RemoveVolatile = typename RemoveVolatileT<T>::Type;
```

* transformation traits 能被组合，比如创建一个 RemoveCVT traits，能同时移除 const 和 volatile

```cpp
template <typename T>
struct RemoveCVT : RemoveConstT<typename RemoveVolatileT<T>::Type> {};

template <typename T>
using RemoveCV = typename RemoveCVT<T>::Type;
```

* 如果不需要 RemoveCVT，RemoveCV 可以直接用别名模板简化如下

```cpp
template <typename T>
using RemoveCV = RemoveConst<RemoveVolatile<T>>;
```

* 标准库提供了对应的 [std::remove_volatile，std::remove_const 和 std::remove_cv](https://en.cppreference.com/w/cpp/types/remove_cv)

### 3.2.4 Decay

* 模仿传值时的类型转换，即把数组和函数转指针，并去除顶层 cv 或引用限定符

```cpp
#include <iostream>
#include <type_traits>

template <typename T>
void f(T) {}

template <typename A>
void printParameterType(void (*)(A)) {
  std::cout << "Parameter type: " << typeid(A).name() << '\n';
  std::cout << "- is int: " << std::is_same_v<A, int> << '\n';
  std::cout << "- is const: " << std::is_const_v<A> << '\n';
  std::cout << "- is pointer: " << std::is_pointer_v<A> << '\n';
}

int main() {
  printParameterType(f<int>);        // 未改变
  printParameterType(f<const int>);  // 退化为 int
  printParameterType(f<int[7]>);     // 退化为 int*
  printParameterType(f<int(int)>);   // 退化为 int(*)(int)
}
```

* 可以实现一个传值时产生相同的类型转换的 traits

```cpp
#include <iostream>
#include <type_traits>

template <typename T>
struct RemoveConstT {
  using Type = T;
};

template <typename T>
struct RemoveConstT<const T> {
  using Type = T;
};

template <typename T>
using RemoveConst = typename RemoveConstT<T>::Type;

template <typename T>
struct RemoveVolatileT {
  using Type = T;
};

template <typename T>
struct RemoveVolatileT<volatile T> {
  using Type = T;
};

template <typename T>
using RemoveVolatile = typename RemoveVolatileT<T>::Type;

template <typename T>
struct RemoveCVT : RemoveConstT<typename RemoveVolatileT<T>::Type> {};

// 首先定义 nonarray、nonfunction 的情况，移除任何 cv 限定符
template <typename T>
struct DecayT : RemoveCVT<T> {};

// 使用偏特化处理数组到指针的 decay，要求识别任何数组类型
template <typename T>
struct DecayT<T[]> {
  using Type = T*;
};

template <typename T, std::size_t N>
struct DecayT<T[N]> {
  using Type = T*;
};

// 函数到指针的 decay，必须匹配任何函数类型
template <typename R, typename... Args>
struct DecayT<R(Args...)> {
  using Type = R (*)(Args...);
};

// 匹配使用 C-style vararg 的函数类型
template <typename R, typename... Args>
struct DecayT<R(Args..., ...)> {
  using Type = R (*)(Args..., ...);
};

template <typename T>
using Decay = typename DecayT<T>::Type;

template <typename T>
void printDecayedType() {
  using A = Decay<T>;
  std::cout << "Parameter type: " << typeid(A).name() << '\n';
  std::cout << "- is int: " << std::is_same_v<A, int> << '\n';
  std::cout << "- is const: " << std::is_const_v<A> << '\n';
  std::cout << "- is pointer: " << std::is_pointer_v<A> << '\n';
}

int main() {
  printDecayedType<int>();
  printDecayedType<const int>();
  printDecayedType<int[7]>();
  printDecayedType<int(int)>();
}
```

* 标准库提供了对应的 [std::decay](https://en.cppreference.com/w/cpp/types/decay)

### 3.2.5 C++98 中用 typedef 处理

```cpp
template <typename T>
void apply(T& x, void (*f)(T)) {
  f(x);
}

void f1(int) {}
void f2(int&) {}

int main() {
  int x = 1;
  apply(x, f1);  // OK: apply<int>
  apply(x, f2);  // error: apply<int&>, int&& 不匹配 x
}
```

* 解决方法是创建一个类型函数，对不是引用的类型添加引用限定符，此外还可以提供其他需要的转换

```cpp
template <typename T>
class TypeOp {  // primary template
 public:
  typedef T ArgT;
  typedef T BareT;
  typedef const T ConstT;
  typedef T& RefT;
  typedef T& RefBareT;
  typedef const T& RefConstT;
};

template <typename T>
class TypeOp<const T> {  // partial specialization for const types
 public:
  typedef const T ArgT;
  typedef T BareT;
  typedef const T ConstT;
  typedef const T& RefT;
  typedef T& RefBareT;
  typedef const T& RefConstT;
};

template <typename T>
class TypeOp<T&> {  // partial specialization for references
 public:
  typedef T& ArgT;
  typedef typename TypeOp<T>::BareT BareT;
  typedef const T ConstT;
  typedef T& RefT;
  typedef typename TypeOp<T>::BareT& RefBareT;
  typedef const T& RefConstT;
};

// 不允许指向 void 的引用，可以将其看成普通的 void 类型
template <>
class TypeOp<void> {  // full specialization for void
 public:
  typedef void ArgT;
  typedef void BareT;
  typedef void const ConstT;
  typedef void RefT;
  typedef void RefBareT;
  typedef void RefConstT;
};

template <typename T>
void apply(typename TypeOp<T>::RefT x, void (*f)(T)) {
  f(x);
}
```

* 注意 T 位于受限名称中，不能被第一个实参推断出来，因此只能根据第二个实参推断 T，再根据结果生成第一个参数的实际类型

## 3.3 Predicate Traits

### 3.3.1 IsSameT

* 判断两个类型是否相等

```cpp
template <typename T, typename U>
struct IsSameT {
  static constexpr bool value = false;
};

template <typename T>
struct IsSameT<T, T> {
  static constexpr bool value = true;
};

template<typename T, typename U>
constexpr bool isSame = IsSameT<T, U>::value;
```

* 标准库提供了对应的 [std::is_same](https://en.cppreference.com/w/cpp/types/is_same)

### 3.3.2 true_type 和 false_type

* 声明一个类模板 BoolConstant，它可以实例化为 TrueType 和 FalseType 两种类型，让 IsSameT 继承 TrueType 或 FalseType，通过元函数转发获取基类的 value 成员

```cpp
template <bool b>
struct BoolConstant {
  using Type = BoolConstant<b>;
  static constexpr bool value = b;
};

using TrueType = BoolConstant<true>;
using FalseType = BoolConstant<false>;

template <typename T, typename U>
struct IsSameT : FalseType {};

template <typename T>
struct IsSameT<T, T> : TrueType {};
```

由此可以实现 tag dispatching

```cpp
#include <iostream>

template <typename T>
void fImpl(T, TrueType) {
  std::cout << 1;
}

template <typename T>
void fImpl(T, FalseType) {
  std::cout << 2;
}

template <typename T>
void f(T x) {
  fImpl(x, IsSameT<T, int>{});
}

int main() {
  f(42);    // 1
  f(3.14);  // 2
}
```

* [<type_traits>](https://en.cppreference.com/w/cpp/header/type_traits) 提供了 [std::true_type 和 std::false_type](https://en.cppreference.com/w/cpp/types/integral_constant)

```cpp
namespace std {
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
}  // namespace std
```

## 3.4 Result Type traits

* result type traits 是另一个处理多种类型的类型函数，常用于操作符模板

```cpp
template<typename T, typename U>
struct PlusResultT {
  using Type = decltype(T() + U());
};

template<typename T, typename U>
using PlusResult = typename PlusResultT<T, U>::Type;

template <typename T, typename U>
Array<PlusResult<T, U>> operator+(const Array<T>&, const Array<U>&);
```

* PlusResult 使用 decltype 推断类型，可能产生一个引用类型，而 Array 可能没有设计对引用类型的处理，另外 operator+ 还可能返回一个 const class 类型的值

```cpp
const A operator+(const A&, const A&);
```

* 因此需要移除限定符

```cpp
template <typename T, typename U>
Array<std::remove_cv_t<std::remove_reference_t<PlusResult<T, U>>>> operator+(
    const Array<T>&, const Array<U>&);
```

* 还有一个问题是，数组类本身可能不要求元素类型的值初始化，但表达式 `T() + U()` 会尝试值初始化，要求类型 T 和 U 有默认构造函数

### 3.4.1 declval

* 标准库提供了 [std::declval](https://en.cppreference.com/w/cpp/utility/declval) 来产生值但不要求构造函数

```cpp
namespace std {
template<typename T>
add_rvalue_reference_t<T> declval() noexcept;
}
```

* 这个函数模板是特意未定义的，因为它只针对于在 decltype、sizeof 或其他不需要定义的上下文中使用。declval 返回一个类型的右值引用，即使该类型没有默认构造函数或者不能创建对象，这使得 declval 甚至能处理不能从函数正常返回的类型，比如抽象类类型或数组类型。`declval<T>()`用作表达式时，从 T 到 T&& 的转换由于引用折叠对 `declval<T>()` 的行为没有影响。declval 本身不会抛出异常，在 noexcept 运算符的上下文中使用时很有用

```cpp
#include <utility>

struct A {
  A() = delete;
  int f() { return 0; }
};

int main() {
  decltype(std::declval<A>().f()) x = 1;  // OK：无需构造A对象
}
```

* 由此即可解决之前需要值初始化的问题

```cpp
#include <utility>

template <typename T, typename U>
struct PlusResultT {
  using Type = decltype(std::declval<T>() + std::declval<U>());
};

template <typename T, typename U>
using PlusResult = typename PlusResultT<T, U>::Type;
```

# 04 SFINAE-based traits

## 4.1 SFINAE Out 函数重载

* 使用 SFINAE 判断一个类型是否默认可构造

```cpp
#include <type_traits>

template <typename T>
struct IsDefaultConstructibleT {
 private:
  template <typename U, typename = decltype(U())>
  static char test(void*);

  template <typename>
  static long test(...);

 public:
  static constexpr bool value =
      std::is_same_v<decltype(test<T>(nullptr)), char>;
};

template <typename T>
constexpr bool IsDefaultConstructible = IsDefaultConstructibleT<T>::value;

struct A {
  A() = delete;
};

int main() { static_assert(!IsDefaultConstructible<A>); }
```

### 4.1.1 SFINAE-based traits 的其他实现策略

* SFINAE-based traits 从 C++98 开始就可以实现了，关键就是声明两个返回类型不同的重载函数模板

```cpp
template<...> static char test(void*);
template<...> static long test(...);
```

* 但最初的技术使用返回类型的大小来确定哪个重载被选择

```cpp
enum { value = sizeof(test<...>(0)) == 1 };
```

* 一些平台上可能发生 `sizeof(char) == sizeof(long)`，比如在 DSP 或老的 Cray 机器上所有的整型都有同样的大小，为了确保在所有平台上有不同的大小，可以定义如下

```cpp
using Size1T = char;
using Size2T = struct { char a[2]; };
// 或者
using Size1T = char(&)[1];
using Size2T = char(&)[2];

template<...> static Size1T test(void*);
template<...> static Size2T test(...);
```

### 4.1.2 Making SFINAE-based traits Predicate traits

* Predicate traits 返回一个派生自 std::true_type 或 std::false_type 的布尔值，这个方法也可以解决一些平台上 `sizeof(char) == sizeof(long)` 的问题

```cpp
#include <type_traits>

template <typename T>
struct IsDefaultConstructibleT {
 private:
  template <typename U, typename = decltype(U())>
  static std::true_type test(void*);

  template <typename>
  static std::false_type test(...);

 public:
  static constexpr bool value = decltype(test<T>(nullptr))::value;
};

template <typename T>
constexpr bool IsDefaultConstructible = IsDefaultConstructibleT<T>::value;

struct A {
  A() = delete;
};

int main() { static_assert(!IsDefaultConstructible<A>); }
```

## 4.2 SFINAE Out 偏特化

* 实现 SFINAE-based traits 的另一个方法是使用偏特化

```cpp
#include <type_traits>

template <typename...>
using void_t = void;

template <typename, typename = void_t<>>
struct IsDefaultConstructibleT : std::false_type {};

template <typename T>
struct IsDefaultConstructibleT<T, void_t<decltype(T())>> : std::true_type {};

template <typename T>
constexpr bool IsDefaultConstructible = IsDefaultConstructibleT<T>::value;

struct A {
  A() = delete;
};

int main() { static_assert(!IsDefaultConstructible<A>); }
```

* C++17 提供了 [std::void_t](https://en.cppreference.com/w/cpp/types/void_t)

## 4.3 为 SFINAE 使用泛型 lambda

* 无论使用哪种技术，总需要一些公式化的套路来定义 traits，C++17 可以在一个泛型 lambda 中指定检查条件，以简化公式化的代码

```cpp
#include <iostream>
#include <type_traits>
#include <utility>

template <typename F,
          typename... Args,  // 下面的默认实参相当于对 F 调用 Args...
          typename = decltype(std::declval<F>()(std::declval<Args &&>()...))>
std::true_type isValidImpl(void *);

template <typename F, typename... Args>
std::false_type isValidImpl(...);

// 下面的 lambda 参数 f 也是一个 lambda，用于检查对 args 调用 f 是否有效
inline constexpr auto isValid = [](auto f) {
  return [](auto &&... args) {
    return decltype(isValidImpl<decltype(f), decltype(args) &&...>(nullptr)){};
  };
};

template <typename T>
struct TypeT {};

template <typename T>
constexpr auto type = TypeT<T>{};

// decltype(valueT(x)) 可以得到 x 的原始类型
template <typename T>
T valueT(TypeT<T>);  // 不需要定义

struct A {
  A() = delete;
};

constexpr auto isDefaultConstructible =
    isValid([](auto x) -> decltype((void)decltype(valueT(x))()) {});
// 如果 x 的类型不能被默认构造，则 decltype(valueT(x))() 无效
// 由此导致第一个 isValidImpl() 声明中的默认实参失败，被 SFINAE out
// 于是匹配到 fallback，结果为 std::false_type

int main() {
  std::cout << std::boolalpha;
  std::cout << isDefaultConstructible(type<A>);    // false
  std::cout << isDefaultConstructible(type<int>);  // true
}
```

* isDefaultConstructible 比之前的 traits 更可读，并仍有办法使用之前的风格

```cpp
template <typename T>
using IsDefaultConstructibleT =
    decltype(isDefaultConstructible(std::declval<T>()));

static_assert(!IsDefaultConstructibleT<A>::value);
```

* 这个技术调用的表达式和使用风格十分复杂，一些编译器可能编译失败。但如果 isValid 可行，许多 traits 都能只用一个声明实现。比如检查是否存在一个名为 first 的成员

```cpp
constexpr auto hasFirst =
    isValid([](auto x) -> decltype((void)valueT(x).first) {});
```

## 4.4 SFINAE-Friendly traits

* 通常，一个 type traits 应该可以回应一个特殊的查询而不造成程序非法。SFINAE-based traits 绕开了隐藏的问题，把错误变成了否定的结果。然而，一些 traits 在面对错误时就不能正常表现了，比如之前的 PlusResultT

```cpp
#include <utility>

template<typename T, typename U>
struct PlusResultT {
  using Type = decltype(std::declval<T>() + std::declval<U>());
};

template<typename T, typename U>
using PlusResult = typename PlusResultT<T, U>::Type;
```

* 如果对没有合适的 operator+ 的类型使用 PlusResult 则会出错

```cpp
template <typename T, typename U>
Array<PlusResult<T, U>> operator+(const Array<T>&, const Array<U>&);

class A {};
class B {};

void addAB(const Array<A>& a, const Array<B>& b) {
  auto res = a + b;  // 错误
}
```

* 实际出现的问题不像这样明显

```cpp
template <typename T, typename U>
Array<PlusResult<T, U>> operator+(const Array<T>&, const Array<U>&);

Array<A> operator+(const Array<A>&, const Array<B>&);

void addAB(const Array<A>& a, const Array<B>& b) {
  auto res = a + b;  // 是否错误取决于是否实例化 PlusResult<A, B>
}
```

* 如果编译器能确定第二个`operator+`的声明是更好的匹配，则可以通过。但推断和替换候选函数模板时，在 PlusResult 中对 A 和 B 类型的元素调用 `operator+` 就会出错。为了解决这个问题，必须使 PlusResultT SFINAE-friendly
* 定义一个 HasPlusT 来检查给定的类型是否有合适的 `operator+`

```cpp
#include <type_traits>
#include <utility>

template <typename, typename, typename = std::void_t<>>
struct HasPlusT : std::false_type {};

template <typename T, typename U>
struct HasPlusT<T, U,
                std::void_t<decltype(std::declval<T>() + std::declval<U>())>>
    : std::true_type {};
```

* 这样如果 traits 用在 SFINAE 上下文（如上面的数组`operator+`模板的返回类型）中，如果没有成员 Type 将使模板实参推断失败

```cpp
template <typename T, typename U, bool = HasPlusT<T, U>::value>
struct PlusResultT {
  using Type = decltype(std::declval<T>() + std::declval<U>());
};

template <typename T, typename U>
struct PlusResultT<T, U, false> {};
```

* 再次考虑 `Array<A>` 和 `Array<B>` 的相加，在上面这个实现中，`PlusResult<A, B>` 没有 Type 成员，`operator+` 无效，SFINAE 将从考虑中除去函数模板，针对 `Array<A>` 和 `Array<B>` 重载的 `operator+` 将被选择
* 作为通用的设计原则，如果给出合理的模板实参作为输入，一个 traits 模板应该永远不会在实例化期间失败。通用的方法是执行两次对应的检查
  * 一次确定操作是否有效
  * 一次计算结果

# 05 IsConvertibleT

* 判断一个给定的类型能否转换为另一个给定的类型

```cpp
#include <iostream>
#include <string>
#include <type_traits>
#include <utility>

template <typename FROM, typename TO>
struct IsConvertibleHelper {
 private:
  static void aux(TO);  // 调用时参数将转换为 TO

  template <typename F, typename T,
            typename = decltype(aux(std::declval<F>()))>  // 调用 aux
  static std::true_type test(void*);  // F 能转换为 TO 则匹配，否则 SFINAE

  template <typename, typename>
  static std::false_type test(...);

 public:
  using Type = decltype(test<FROM, TO>(nullptr));
};

template <typename FROM, typename TO>
struct IsConvertibleT : IsConvertibleHelper<FROM, TO>::Type {};

template <typename FROM, typename TO>
using IsConvertible = typename IsConvertibleT<FROM, TO>::Type;

template <typename FROM, typename TO>
constexpr bool isConvertible = IsConvertibleT<FROM, TO>::value;

class A {};
class B : public A {};

int main() {
  static_assert(isConvertible<B, A>);
  static_assert(isConvertible<B*, A*>);
  static_assert(!isConvertible<A*, B*>);
  static_assert(isConvertible<const char*, std::string>);
}
```

* 有三种情况不能被正确处理
  * 转换为数组类型应该总是产生 false，但是这里 aux() 声明中的类型 TO 参数会退化为指针类型，导致对一些 FROM 类型产生 true 的结果
  * 转换为函数类型应该总是产生 false，但这里同数组的情况一样
  * 转换为（cv 限定符）void 类型应该总是产生 true，但这里甚至不会成功实例化 TO 为 void 类型的情况，因为参数类型不能有类型 void
* 对于这些情况需要额外的偏特化，但对每个可能的 cv 限定符组合添加特化很麻烦。可以如下给辅助类模板添加一个额外的模板参数

```cpp
template <typename FROM, typename TO,
          bool = std::is_void_v<TO> || std::is_array_v<TO> ||
                 std::is_function_v<TO>>
struct IsConvertibleHelper {
  using Type = std::bool_constant<std::is_void_v<TO> && std::is_void_v<FROM>>;
};

template <typename FROM, typename TO>
struct IsConvertibleHelper<FROM, TO, false> {
 private:
  static void aux(TO);

  template <typename F, typename T, typename = decltype(aux(std::declval<F>()))>
  static std::true_type test(void*);

  template <typename, typename>
  static std::false_type test(...);

 public:
  using Type = decltype(test<FROM, TO>(nullptr));
};
```

* 标准库提供了对应的 [std::is_convertible](https://en.cppreference.com/w/cpp/types/is_convertible)

# 06 检查成员

* SFINAE-based traits 还能确定给定的类型 T 是否有名为 X 的成员

## 6.1 检查类型成员

* 判断类型 T 是否有可访问的类型成员 size_type

```cpp
#include <iostream>
#include <type_traits>

template <typename, typename = std::void_t<>>
struct HasSizeTypeT : std::false_type {};

template <typename T>
struct HasSizeTypeT<T, std::void_t<typename T::size_type>> : std::true_type {};

struct A {
  using size_type = unsigned int;
};

struct B {
 private:
  using size_type = unsigned int;
};

int main() {
  static_assert(HasSizeTypeT<A>::value);
  static_assert(!HasSizeTypeT<B>::value);
}
```

### 6.1.1 处理引用类型

* 引用类型会引起一些问题

```cpp
static_assert(HasSizeTypeT<A>::value);
static_assert(!HasSizeTypeT<A&>::value);
```

* 为此可以移除引用限定符

```cpp
template <typename, typename = std::void_t<>>
struct HasSizeTypeT : std::false_type {};

template <typename T>
struct HasSizeTypeT<T,
                    std::void_t<typename std::remove_reference_t<T>::size_type>>
    : std::true_type {};
```

### 6.1.2 注入类名

* 注意 HasSizeTypeT 对注入类名也将产生 true

```cpp
struct size_type {};
struct A : size_type {};

static_assert(HasSizeTypeT<A>::value);
```

## 6.2 检查任意类型成员

* 要对任意指定的类型进行检查，之前用泛型 lambda 实现过此功能，这里通过宏实现

```cpp
#include <iostream>
#include <type_traits>
#include <vector>

#define DEFINE_HAS_TYPE(MemType)                                    \
  template <typename, typename = std::void_t<>>                     \
  struct HasTypeT_##MemType : std::false_type {};                   \
  template <typename T>                                             \
  struct HasTypeT_##MemType<                                        \
      T, std::void_t<typename std::remove_reference_t<T>::MemType>> \
      : std::true_type {}  // 注意这里不需要分号

DEFINE_HAS_TYPE(value_type);  // 定义HasTypeT_value_type
DEFINE_HAS_TYPE(char_type);   // 定义HasTypeT_char_type

int main() {
  static_assert(HasTypeT_value_type<std::vector<int>>::value);
  static_assert(HasTypeT_value_type<std::vector<int>&>::value);
  static_assert(!HasTypeT_value_type<std::iostream>::value);
  static_assert(HasTypeT_char_type<std::iostream>::value);
}
```

* 注意之前的注入类名的问题在这里仍会出现

```cpp
DEFINE_HAS_TYPE(size_type);

struct size_type {};
struct A : size_type {};

static_assert(HasTypeT_size_type<A>::value);
```

## 6.3 检查非类型成员

* 检查可访问的 non-static 成员

```cpp
#include <type_traits>
#include <vector>

#define DEFINE_HAS_MEMBER(Member)                                  \
  template <typename, typename = std::void_t<>>                    \
  struct HasMemberT_##Member : std::false_type {};                 \
  template <typename T>                                            \
  struct HasMemberT_##Member<T, std::void_t<decltype(&T::Member)>> \
      : std::true_type {}  // ; intentionally skipped

DEFINE_HAS_MEMBER(size);  // &T::size 无效时，SFINAE 丢弃上面的偏特化
DEFINE_HAS_MEMBER(first);  // 前缀 & 是为了要求成员必须是非类型、非枚举

int main() {
  static_assert(HasMemberT_size<std::vector<int>>::value);
  static_assert(HasMemberT_first<std::pair<int, int>>::value);
}
```

### 6.3.1 检查成员函数

* HasMember 只检查单个成员的名称，如果存在多个成员，比如重载的成员函数，traits 会失效

```cpp
DEFINE_HAS_MEMBER(begin);

static_assert(!HasMemberT_begin<std::vector<int>>::value);
```

* 为此可以简化 traits，只检查函数

```cpp
#include <type_trais>
#include <utility>

template <typename, typename = std::void_t<>>
struct HasBeginT : std::false_type {};

template <typename T>
struct HasBeginT<T, std::void_t<decltype(std::declval<T>().begin())>>
    : std::true_type {};

int main() { static_assert(HasBeginT<std::vector<int>>::value); }
```

### 6.3.2 检查其他表达式

* 上述技术也可以用于其他表达式，比如检查两个类型之间是否有比较大小的运算符

```cpp
#include <complex>
#include <iostream>
#include <type_traits>
#include <utility>

template <typename, typename, typename = std::void_t<>>
struct HasLessT : std::false_type {};

template <typename T, typename U>
struct HasLessT<T, U,
                std::void_t<decltype(std::declval<T>() < std::declval<U>())>>
    : std::true_type {};

int main() {
  static_assert(HasLessT<int, char>::value);
  static_assert(HasLessT<std::string, std::string>::value);
  static_assert(!HasLessT<std::string, int>::value);
  static_assert(HasLessT<std::string, char*>::value);
  static_assert(!HasLessT<std::complex<double>, std::complex<double>>::value);
}
```

* [std::void_t](https://en.cppreference.com/w/cpp/types/void_t) 可接受任意数量的模板参数，由此可以一次组合多个表达式

```cpp
#include <type_traits>
#include <utility>
#include <vector>

template <typename, typename = std::void_t<>>
struct HasVariousT : std::false_type {};

template <typename T>
struct HasVariousT<
    T, std::void_t<decltype(std::declval<T>().begin()),
                   typename T::difference_type, typename T::iterator>>
    : std::true_type {};

int main() { static_assert(HasVariousT<std::vector<int>>::value); }
```

## 6.4 使用泛型lambda检查成员

* 比起使用宏，泛型 lambda 能更紧凑地定义检查成员的 traits

```cpp
#include <string>
#include <utility>

template <typename F, typename... Args,
          typename = decltype(std::declval<F>()(std::declval<Args &&>()...))>
std::true_type isValidImpl(void *);

template <typename F, typename... Args>
std::false_type isValidImpl(...);

inline constexpr auto isValid = [](auto f) {
  return [](auto &&... args) {
    return decltype(isValidImpl<decltype(f), decltype(args) &&...>(nullptr)){};
  };
};

template <typename T>
struct TypeT {};

template <typename T>
constexpr auto type = TypeT<T>{};

template <typename T>
T valueT(TypeT<T>);

constexpr auto hasFirst =
    isValid([](auto x) -> decltype((void)valueT(x).first) {});

constexpr auto hasSizeType = isValid(
    [](auto x) -> typename std::decay_t<decltype(valueT(x))>::size_type {});

constexpr auto hasLess =
    isValid([](auto x, auto y) -> decltype(valueT(x) < valueT(y)) {});

struct A {
  using size_type = std::size_t;
};

int main() {
  static_assert(hasFirst(type<std::pair<int, int>>));
  static_assert(hasSizeType(type<A>));
  static_assert(hasSizeType(type<A &>));
  static_assert(!hasSizeType(type<int>));
  static_assert(hasLess(type<int>, type<char>));
  static_assert(hasLess(type<std::string>, type<std::string>));
  static_assert(!hasLess(type<std::string>, type<int>));
  static_assert(hasLess(type<std::string>, type<const char *>));
}
```

* 如果要支持常用的 traits 语法，可定义如下

```cpp
#include <string>
#include <type_traits>
#include <utility>

template <typename F, typename... Args,
          typename = decltype(std::declval<F>()(std::declval<Args&&>()...))>
std::true_type isValidImpl(void*);

template <typename F, typename... Args>
std::false_type isValidImpl(...);

inline constexpr auto isValid = [](auto f) {
  return [](auto&&... args) {
    return decltype(isValidImpl<decltype(f), decltype(args)&&...>(nullptr)){};
  };
};

constexpr auto hasFirst = isValid([](auto&& x) -> decltype((void)&x.first) {});

template <typename T>
using HasFirstT = decltype(hasFirst(std::declval<T>()));

constexpr auto hasSizeType =
    isValid([](auto&& x) -> typename std::decay_t<decltype(x)>::size_type {});

template <typename T>
using HasSizeTypeT = decltype(hasSizeType(std::declval<T>()));

constexpr auto hasLess = isValid([](auto&& x, auto&& y) -> decltype(x < y) {});

template <typename T, typename U>
using HasLessT = decltype(hasLess(std::declval<T>(), std::declval<U>()));

struct A {
  using size_type = std::size_t;
};

int main() {
  static_assert(HasFirstT<std::pair<int, int>>::value);
  static_assert(HasSizeTypeT<A>::value);
  static_assert(HasSizeTypeT<A&>::value);
  static_assert(HasLessT<int, char>::value);
  static_assert(HasLessT<std::string, std::string>::value);
  static_assert(!HasLessT<std::string, int>::value);
  static_assert(HasLessT<std::string, const char*>::value);
}
```

# 07 其他traits技术

## 7.1 If-Then-Else

* IfThenElse 通过一个布尔参数在两个类型参数进行选择

```cpp
template <bool b, typename T, typename U>
struct IfThenElseT {
  using Type = T;
};

template <typename T, typename U>
struct IfThenElseT<false, T, U> {
  using Type = U;
};

template <bool b, typename T, typename U>
using IfThenElse = typename IfThenElseT<b, T, U>::Type;
```

* 标准库提供了对应的 [std::conditional](https://en.cppreference.com/w/cpp/types/conditional)
* 下面的类型函数能确定某个值的最低级别整型

```cpp
#include <type_traits>

template <auto N>
struct SmallestIntT {
  using Type = std::conditional_t<
      N <= std::numeric_limits<char>::max(), char,
      std::conditional_t<
          N <= std::numeric_limits<short>::max(), short,
          std::conditional_t<
              N <= std::numeric_limits<int>::max(), int,
              std::conditional_t<
                  N <= std::numeric_limits<long>::max(), long,
                  std::conditional_t<N <= std::numeric_limits<long long>::max(),
                                     long long, void>>>>>;
};
```

* 这里所有分支的模板实参在被选择前都会被计算，所以不能有非法的代码

```cpp
#include <type_traits>

// T 是 bool 或不是整型将产生未定义行为
template <typename T>
struct UnsignedT {
  using Type =
      std::conditional_t<std::is_integral_v<T> && !std::is_same_v<T, bool>,
                         std::make_unsigned_t<T>,
                         T>;  // 无论是否被选择，所有分支都会被计算
};
```

* 添加一个类型函数作为中间层即可解决此问题

```cpp
#include <type_traits>

template <typename T>
struct IdentityT {
  using Type = T;
};

template <typename T>
struct MakeUnsignedT {
  using Type = std::make_unsigned_t<T>;
};

template <typename T>
struct UnsignedT {
  using Type =
      std::conditional_t<std::is_integral_v<T> && !std::is_same_v<T, bool>,
                         MakeUnsignedT<T>, IdentityT<T>>;
};
```

* 类型函数在需要计算 ::Type 时会实例化，所以别名模板不能有效地用于 IfThenElse 的分支

```cpp
template <typename T>
using MakeUnsigned = typename MakeUnsignedT<T>::Type;

template <typename T>
struct UnsignedT {
  using Type =
      std::conditional_t<std::is_integral_v<T> && !std::is_same_v<T, bool>,
                         MakeUnsigned<T>, T>;  // 仍会实例化，未解决之前的问题
};
```

## 7.2 检查不抛出异常的操作

* 可以通过 [noexcept 运算符](https://en.cppreference.com/w/cpp/language/noexcept)直接实现

```cpp
#include <type_traits>
#include <utility>

template <typename T>
struct IsNothrowMoveConstructibleT
    : std::bool_constant<noexcept(T(std::declval<T>()))> {};
```

* 但这个实现对无法移动构造的类型将报错而非产生 false，因为 `T(std::declval<T&&>())` 无效

```cpp
class A {
 public:
  A(A&&) = delete;
};

int main() {
  static_assert(!IsNothrowMoveConstructibleT<A>::value);  // 断言成功，编译错误
}
```

* 因此在使用 [noexcept 运算符](https://en.cppreference.com/w/cpp/language/noexcept)前必须确保移动构造函数有效

```cpp
#include <type_traits>
#include <utility>

template <typename T, typename = std::void_t<>>
struct IsNothrowMoveConstructibleT : std::false_type {};

template <typename T>
struct IsNothrowMoveConstructibleT<T,
                                   std::void_t<decltype(T(std::declval<T>()))>>
    : std::bool_constant<noexcept(T(std::declval<T>()))> {};

class A {
 public:
  A(A&&) = delete;
};

int main() { static_assert(!IsNothrowMoveConstructibleT<A>::value); }
```

* 这里 traits 名为 IsNothrowMoveConstructible 而不是 HasNothrowMoveConstructor 是因为，如果不能直接访问移动构造函数则无法检查它是否抛异常，此外对应的类型不能是抽象类（但可以是抽象类的引用或指针）
* 标准库提供了对应的 [std::is_move_constructible](https://en.cppreference.com/w/cpp/types/is_move_constructible)

## 7.3 简化 traits

* 用别名模板简化产生类型的 traits

```cpp
template <class T>
using remove_reference_t = typename remove_reference<T>::type;
```

* 用变量模板简化产生值的traits

```cpp
template <class T, class U>
inline constexpr bool is_same_v = is_same<T, U>::value;
```

* 别名模板可以简化代码，但使用别名模板也有一些缺点
  * 别名模板不能被特化，traits 的许多技术依赖于特化，此时只能把别名模板改回类模板
  * 一些 traits 有意让用户特化，大量调用别名模板时就容易和类模板特化混淆
  * 使用别名模板总会实例化类型，将使得对给定类型难以避免无意义的实例化 traits。换句话说，别名模板不能用于元函数转发


# 08 类型分类（Type Classification）

## 8.1 判断基本类型

```cpp
#include <cstddef>  // for std::nullptr_t
#include <iostream>
#include <type_traits>

template <typename T>
struct IsFundaT : std::false_type {};

#define MK_FUNDA_TYPE(T) \
  template <>            \
  struct IsFundaT<T> : std::true_type {};

MK_FUNDA_TYPE(void)
MK_FUNDA_TYPE(bool)
MK_FUNDA_TYPE(char)
MK_FUNDA_TYPE(signed char)
MK_FUNDA_TYPE(unsigned char)
MK_FUNDA_TYPE(wchar_t)
MK_FUNDA_TYPE(char16_t)
MK_FUNDA_TYPE(char32_t)
MK_FUNDA_TYPE(signed short)
MK_FUNDA_TYPE(unsigned short)
MK_FUNDA_TYPE(signed int)
MK_FUNDA_TYPE(unsigned int)
MK_FUNDA_TYPE(signed long)
MK_FUNDA_TYPE(unsigned long)
MK_FUNDA_TYPE(signed long long)
MK_FUNDA_TYPE(unsigned long long)
MK_FUNDA_TYPE(float)
MK_FUNDA_TYPE(double)
MK_FUNDA_TYPE(long double)
MK_FUNDA_TYPE(std::nullptr_t)

#undef MK_FUNDA_TYPE

template <typename T>
void test(const T&) {
  std::cout << std::boolalpha << IsFundaT<T>::value;
}

int main() {
  test(7);        // true
  test("hello");  // false
}
```

* 标准库提供了对应的 [std::is_fundamental](https://en.cppreference.com/w/cpp/types/is_fundamental)

## 8.2 判断函数类型

* 函数类型有任意数量的参数影响结果，因此在匹配函数类型的偏特化中，借用一个参数包来捕获所有的参数类型

```cpp
template <class>
struct is_function : std::false_type {};

template <class Ret, class... Args>
struct is_function<Ret(Args...)> : std::true_type {};

// variadic functions，如 std::printf
template <class Ret, class... Args>
struct is_function<Ret(Args..., ...)> : std::true_type {};
```

* 但这不能处理所有函数类型，因为函数类型还涉及 cv 限定符、左值右值引用限定符，以及 C++17 的 noexcept 限定符，比如

```cpp
using MyFuncType = void (int&) const;
```

* 标记为 const 的函数类型不是真的 const 类型，无法用 [std::remove_const](https://en.cppreference.com/w/cpp/types/remove_cv) 去除，比如对上述类型需要

```cpp
template <class Ret, class... Args>
struct is_function<Ret(Args...) const> : std::true_type {};

static_assert(is_function<MyFuncType>::value);
```

* 为了识别带限定符的函数类型，需要额外引入大量的偏特化来覆盖所有的限定符组合，以下仅列出部分

```cpp
template <class Ret, class... Args>
struct is_function<Ret(Args...) const volatile> : std::true_type {};

template <class Ret, class... Args>
struct is_function<Ret(Args...) &> : std::true_type {};

template <class Ret, class... Args>
struct is_function<Ret(Args...) &&> : std::true_type {};

template <class Ret, class... Args>
struct is_function<Ret(Args...) noexcept> : std::true_type {};

template <class Ret, class... Args>
struct is_function<Ret(Args...) const &> : std::true_type {};

template <class Ret, class... Args>
struct is_function<Ret(Args...) const volatile &&noexcept> : std::true_type {};

template <class Ret, class... Args>
struct is_function<Ret(Args..., ...) const volatile &&> : std::true_type {};
```

* 标准库提供了对应的 [std::is_function](https://en.cppreference.com/w/cpp/types/is_function)

## 8.3 判断复合类型

* 指针类型：[std::is_pointer](https://en.cppreference.com/w/cpp/types/is_pointer)

```cpp
#include <type_traits>

template <class T>
struct is_pointer_helper : std::false_type {};

template <class T>
struct is_pointer_helper<T*> : std::true_type {};

template <class T>
struct is_pointer : is_pointer_helper<typename std::remove_cv<T>::type> {};
```

* 左值引用： [std::is_lvalue_reference](https://en.cppreference.com/w/cpp/types/is_lvalue_reference)

```cpp
#include <type_traits>

template <class T>
struct is_lvalue_reference : std::false_type {};

template <class T>
struct is_lvalue_reference<T&> : std::true_type {};

template <class T>
constexpr bool is_lvalue_reference_v = is_lvalue_reference<T>::value;
```

* 右值引用：[std::is_rvalue_reference](https://en.cppreference.com/w/cpp/types/is_rvalue_reference)

```cpp
#include <type_traits>

template <class T>
struct is_rvalue_reference : std::false_type {};

template <class T>
struct is_rvalue_reference<T&&> : std::true_type {};

template <class T>
constexpr bool is_rvalue_reference_v = is_rvalue_reference<T>::value;
```

* 引用：[std::is_reference](https://en.cppreference.com/w/cpp/types/is_reference)

```cpp
#include <type_traits>

template <class T>
struct is_reference : std::false_type {};

template <class T>
struct is_reference<T&> : std::true_type {};

template <class T>
struct is_reference<T&&> : std::true_type {};

template <class T>
constexpr bool is_reference_v = is_reference<T>::value;
```

* 数组类型：[std::is_array](https://en.cppreference.com/w/cpp/types/is_array)，另外提供了 [std::rank](https://en.cppreference.com/w/cpp/types/rank) 和 [std::extent](https://en.cppreference.com/w/cpp/types/extent) 来允许查询大小

```cpp
#include <type_traits>

template <class T>
struct is_array : std::false_type {};

template <class T>
struct is_array<T[]> : std::true_type {};

template <class T, std::size_t N>
struct is_array<T[N]> : std::true_type {};

template <class T>
constexpr bool is_array_v = is_array<T>::value;
```

* non-static 成员对象指针或 non-static 成员函数指针： [std::is_member_pointer](https://en.cppreference.com/w/cpp/types/is_member_pointer)

```cpp
#include <type_traits>

template <class T>
struct is_member_pointer_helper : std::false_type {};

template <class T, class U>
struct is_member_pointer_helper<T U::*> : std::true_type {};

template <class T>
struct is_member_pointer
    : is_member_pointer_helper<typename std::remove_cv<T>::type> {};

template <class T>
constexpr bool is_member_pointer_v = is_member_pointer<T>::value;
```

* non-static 成员函数指针： [std::is_member_function_pointer](https://en.cppreference.com/w/cpp/types/is_member_function_pointer)

```cpp
#include <type_traits>

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
```

* non-static 成员对象指针： [std::is_member_object_pointer](https://en.cppreference.com/w/cpp/types/is_member_object_pointer)

```cpp
#include <type_traits>

template <class T>
struct is_member_object_pointer
    : std::bool_constant<std::is_member_pointer_v<T> &&
                         !std::is_member_function_pointer_v<T>> {};

template <class T>
constexpr bool is_member_object_pointer_v = is_member_object_pointer<T>::value;
```

## 8.4 判断类类型

* 表达式 `T::*` 中的 T 只能为类类型，对其使用 SFINAE 即可

```cpp
#include <type_traits>

template <typename T, typename = std::void_t<>>
struct is_class : std::false_type {};

template <typename T>
struct is_class<T, std::void_t<int T::*>> : std::true_type {};

template <class T>
constexpr bool is_class_v = is_class<T>::value;
```

* 下面是 C++98 中确定类类型的方法

```cpp
#include <iostream>

template <typename T>
class IsClassT {
 private:
  typedef char One;
  typedef struct {
    char a[2];
  } Two;
  template <typename C>
  static One test(int C::*);
  template <typename C>
  static Two test(...);

 public:
  enum { Yes = sizeof(IsClassT<T>::test<T>(0)) == 1 };
  enum { No = !Yes };
};

template <typename T>
void check() {
  if (IsClassT<T>::Yes) {
    std::cout << "Y" << '\n';
  } else {
    std::cout << "N" << '\n';
  }
}

template <typename T>
void checkT(T) {
  check<T>();
}

class MyClass {};
struct MyStruct {};
union MyUnion {};
void myfunc() {}
enum E { e1 } e;

int main() {
  check<int>();      // N
  check<MyClass>();  // Y
  MyStruct s;
  checkT(s);         // Y
  check<MyUnion>();  // Y
  checkT(e);         // N
  checkT(myfunc);    // N
}
```

* lambda表达式是一个匿名类，union 也是一种类类型，因此检查 lambda表达式和 union 也将产生 true
* 标准库提供了 [std::is_class](https://en.cppreference.com/w/cpp/types/is_class) 和 [std::is_union](https://en.cppreference.com/w/cpp/types/is_union)

## 8.5 判断枚举类型

* 对于枚举类型，排除其他类型就是枚举类型

```cpp
#include <type_traits>

template <typename T>
struct is_enum {
  static constexpr bool value =
      !std::is_fundamental_v<T> && !std::is_pointer_v<T> &&
      !std::is_reference_v<T> && !std::is_array_v<T> &&
      !std::is_member_pointer_v<T> && !std::is_function_v<T> &&
      !std::is_class_v<T>;
};

template <class T>
constexpr bool is_enum_v = is_enum<T>::value;
```

* 标准库提供了对应的 [std::is_enum](https://en.cppreference.com/w/cpp/types/is_enum)，实际上该 traits 无法用语言内置的方法实现，而是由编译器内部实现

# 09 Policy traits

* 目前给出的 traits 都是用于去确定模板参数的一些属性，这些 traits 称为 property traits，另外还存在其他类型的 traits，定义了如何对待这些类型，这类 traits 称为 policy traits，policy traits 针对的是模板参数相关的更加独有的属性。通常把 property traits 实现为类型函数，而 policy traits 通常把 policy 封装在成员函数内部

## 9.1 只读的参数类型

* 一个小的结构也可能有高开销的拷贝构造函数，这时应该以 const 引用方式传递只读参数。使用 policy traits 模板可以根据类型大小把实参类型 T 映射为 T 或 const T&

```cpp
template <typename T>
struct RParam {
  using Type = std::conditional_t<sizeof(T) <= 2 * sizeof(void*), T, const T&>;
};
```

* 另一方面，对容器类型，即使 sizeof 很小也可能涉及昂贵的拷贝构造函数，因此需要偏特化

```cpp
template <typename T>
struct RParam<Array<T>> {
  using Type = const Array<T>&;
};
```

* 对某些性能要求严格的类，可以选择性地设置类为传值方式

```cpp
template <typename T>
struct RParam {
  using Type = std::conditional_t<(sizeof(T) <= 2 * sizeof(void*) &&
                                   std::is_trivially_copy_constructible_v<T> &&
                                   std::is_trivially_move_constructible_v<T>),
                                  T, const T&>;
};

class A {};
class B {};

// 对 B 类型对象按值传递
template <>
class RParam<B> {
 public:
  using Type = B;
};

// 允许按值传递和按引用传递的函数
template <typename T, typename U>
void f(typename RParam<T>::Type a, typename RParam<U>::Type b) {}

int main() {
  A a;
  B b;
  f<A, B>(a, b);  // a 按引用传递，b 按值传递
}
```

* 这种做法的缺点是，函数声明变得格外复杂，且无法使用实参推断，调用时必须显式指定模板实参。一个解决方法是使用函数模板来完美转发

```cpp
template <typename T, typename U>
void f(typename RParam<T>::Type a, typename RParam<U>::Type b) {}

template <typename T, typename U>
void g(T&& a, U&& b) {
  f<T, U>(std::forward<T>(a), std::forward<U>(b));
}

int main() {
  A a;
  B b;
  g(a, b);
}
```

# 10 标准库中的type traits

* C++11中，[type traits](https://en.cppreference.com/w/cpp/header/type_traits) 变成了标准库的内置部分。他们或多或少包含上述所有的类型函数和 type traits，但对于其中一些，比如 trivial operation detection traits 和 [std::is_union](https://en.cppreference.com/w/cpp/types/is_union)，没有已知的 in-language solution，而是由编译器提供内置支持。因此如果需要 type traits，只要可行就推荐使用标准库
* 标准库也定义了一些 policy 和 property traits
  * 类模板 [std::char_traits](https://en.cppreference.com/w/cpp/string/char_traits) 被 string 和 I/O stream 类用作一个 policy traits 参数
  * 为了更容易给标准迭代器配置算法，提供了一个非常简单的 property traits 模板 [std::iterator_traits](https://en.cppreference.com/w/cpp/iterator/iterator_traits)
  * 模板 [std::numeric_limits](https://en.cppreference.com/w/cpp/types/numeric_limits) 也能被用作 property traits 模板
  * 标准容器类型的内存分配器使用了 policy traits 类进行处理，C++98 提供了 [std::allocator](https://en.cppreference.com/w/cpp/memory/allocator) 作为此目的的标准组件，C++11引入了 [std::allocator_traits](https://en.cppreference.com/w/cpp/memory/allocator_traits) 用于改变分配器的行为
* 改写 [std::char_traits](https://en.cppreference.com/w/cpp/string/char_traits) 即可实现自定义行为的 string，比如让 string 不区分大小写

```cpp
#include <cassert>
#include <iostream>

struct ci_char_traits : public std::char_traits<char> {
  static bool eq(char c1, char c2) { return toupper(c1) == toupper(c2); }
  static bool ne(char c1, char c2) { return toupper(c1) != toupper(c2); }
  static bool lt(char c1, char c2) { return toupper(c1) < toupper(c2); }
  static int compare(const char* s1, const char* s2, std::size_t n) {
    for (; n; ++s1, ++s2, --n) {
      const char diff = toupper(*s1) - toupper(*s2);
      if (diff < 0) {
        return -1;
      } else if (diff > 0) {
        return +1;
      }
    }
    return 0;
  }
  static const char* find(const char* p, int n, char a) {
    for (; n != 0; --n, ++p) {
      if (toupper(*p) == toupper(a)) return p;
    }
    return nullptr;
  }
};

// std::string 其实是 std::basic_string<char, char_traits<char>, std::allocator<char>>
using ci_string = std::basic_string<char, ci_char_traits>;

std::ostream& operator<<(std::ostream& os, const ci_string& str) {
  return os.write(str.data(), str.size());
}

int main() {
  ci_string s1 = "hello";
  ci_string s2 = "HeLLO";
  assert(s1 == s2);
}
```
