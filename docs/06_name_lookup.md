## [ADL（Argument-Dependent Lookup，Koenig Lookup）](https://en.cppreference.com/w/cpp/language/adl)

* [Name lookup](https://en.cppreference.com/w/cpp/language/lookup) 是当程序中出现一个名称时，将其与引入它的声明联系起来的过程，它分为 [qualified name lookup](https://en.cppreference.com/w/cpp/language/qualified_lookup) 和 [unqualified name lookup](https://en.cppreference.com/w/cpp/language/lookup)，[unqualified name lookup](https://en.cppreference.com/w/cpp/language/lookup) 对于函数名查找会使用 [ADL](https://en.cppreference.com/w/cpp/language/adl)

```cpp
namespace jc {

struct A {};
struct B {};
void f1(int) {}
void f2(A) {}

}  // namespace jc

namespace jd {

void f1(int i) {
  f1(i);  // 调用 jd::f1()，造成无限递归
}

void f2(jc::A t) {
  f2(t);  // 通过 t 的类型 jc::A 看到 jc，通过 jc 看到 jc::f2()
          // 因为 jd::f2() 也可见，此处产生二义性调用错误
}

void f3(jc::B t) {
  f3(t);  // 通过 t 的类型 jc::B 看到 jc，但 jc 中无 jc::f3()
          // 此处调用 jd::f3()，造成无限递归
}

}  // namespace jd

int main() {}
```

* [Qualified name lookup](https://en.cppreference.com/w/cpp/language/qualified_lookup) 即对使用了作用域运算符的名称做查找，查找在受限的作用域内进行

```cpp
namespace jc {

int x;

struct Base {
  int i;
};

struct Derived : Base {};

void f(Derived* p) {
  p->i = 0;        // 找到 Base::i
  Derived::x = 0;  // 错误：在受限作用域中找不到 ::x
}

}  // namespace jc

int main() {}
```

* [Unqualified name lookup](https://en.cppreference.com/w/cpp/language/lookup) 即对不指定作用域的名称做查找，先查找当前作用域，若找不到再继续查找外围作用域

```cpp
namespace jc {

extern int x;  // 1

int f(int x) {  // 2
  if (x < 0) {
    int x = 1;  // 3
    f(x);       // 使用 3
  }
  return x + ::x;  // 分别使用 2、1
}

}  // namespace jc

int main() {}
```

* [ADL](https://en.cppreference.com/w/cpp/language/adl) 即实参依赖查找，对于一个类，其成员函数与使用了它的非成员函数，都是该类的逻辑组成部分，如果函数接受一个类作为参数，编译器查找函数名时，不仅会查找局部作用域，还会查找类所在的命名空间

```cpp
#include <iostream>
#include <string>

namespace jc {

struct A {};

void f(const A&) {}  // f() 是 A 的逻辑组成部分

}  // namespace jc

jc::A a;

int main() {
  f(a);  // 通过 ADL 找到 jc::f()，如果没有 ADL，就要写成 jc::f(a)
  std::string s;
  std::cout << s;  // std::operator<<() 是 std::string 的逻辑组成部分
  // 如果没有 ADL，就要写成 std::operator<<(std::cout, s)
}
```

* ADL 是通过实参查找，对于函数模板，查找前无法先得知其为函数，也就无法确定实参，因此不会使用 ADL

```cpp
namespace jc {

class A {};

template <typename>
void f(A*) {}

}  // namespace jc

void g(jc::A* p) {
  f<int>(p);  // 错误，不知道 f<int> 是函数，所以不知道 p 是实参，不会用 ADL
}

int main() {}
```

* ADL 会忽略 using 声明

```cpp
namespace jc {

template <typename T>
constexpr int f(T) {
  return 1;
}

}  // namespace jc

namespace jd {

using namespace jc;  // 忽略 using 声明，不会调用 jc::f

enum Color { red };
constexpr int f(Color) { return 2; }

}  // namespace jd

constexpr int f(int) { return 3; }

static_assert(::f(jd::red) == 3);  // 受限的函数名称，不使用 ADL
static_assert(f(jd::red) == 2);    // 使用 ADL 找到 jd::f()

int main() {}
```

* ADL 会查找实参关联的命名空间和类，关联的命名空间和类组成的集合定义如下
  * 内置类型：集合为空
  * 指针和数组类型：所引用类型关联的命名空间和类
  * 枚举类型：关联枚举声明所在的命名空间
  * 类成员：关联成员所在的类
  * 类类型：关联的类包括该类本身、外围类型、直接和间接基类，关联的命名空间为每个关联类所在的命名空间，如果类是一个类模板实例则还包含模板实参本身类型、模板的模板实参所在的类和命名空间
  * 函数类型：所有参数和返回类型关联的命名空间和类
  * 类成员指针类型：成员和类关联的命名空间和类
* 友元声明在外围作用域不可见，因为如果可见的话，实例化类模板会使普通函数的声明可见，如果没有先实例化类就调用函数，将导致编译错误，但如果友元函数所在类属于 ADL 的关联类集合，则在外围作用域可以找到该友元声明，且调用时，未实例化的类会被实例化

```cpp
namespace jc {

template <typename T>
class A {
  friend void f() {}
  friend void f(A<T>) {}
};

void g(const A<int>& a) {
  f();   // f() 无参数，不能使用 ADL，不可见
  f(a);  // f(A<int>) 关联类 A<int> 所以可见，若类 A<int> 未实例化则调用时实例化
}

}  // namespace jc

int main() {}
```

## [注入类名（Injected Class Name）](https://en.cppreference.com/w/cpp/language/injected-class-name)

* 为了便于查找，在类作用域中，类名称是自身类型的 public 别名，该名称称为注入类名

```cpp
namespace jc {

int A;

struct A {
  void f() {
    A* p;    // OK：A 是注入类名
    ::A* q;  // 错误：查找到变量名 A，隐藏了 struct A 的名称
  }
};

}  // namespace jc

int main() {}
```

* 类模板的注入类名可以被用作模板名或类型名

```cpp
namespace jc {

template <template <typename> class>
struct A {};

template <typename T>
struct B {
  B* a;            // B 被当作类型名，等价于 B<T>
  B<void>* b;      // B 被当作模板名
  using c = A<B>;  // B 被当作模板名
  A<jc::B> d;      // jc::B 不是注入类名，总会被当作模板名
};

}  // namespace jc

int main() {}
```

## 非模板中的上下文相关性

* 解析理论主要面向上下文无关语言，而 C++ 是上下文相关语言，为了解决这个问题，编译器使用一张符号表结合扫描器和解析器
* 解析某个声明时会把它添加到表中，扫描器找到一个标识符时，会在符号表中查找，如果发现该符号是一个类型就会注释这个标记，如编译器看见 `x*`，扫描器会查找 x，如果发现 x 是一个类型，解析器会看到标记如下，认为表达式是一个声明

```
identifier, type, x
symbol, *
```

* 如果 x 不是类型，则解析器从扫描器获得标记如下，表达式被视为一个乘积

```
identifier, nontype, x
symbol, *
```

* 对于 `A<1>(0)`，如果 A 是类模板，则表达式是把 0 转换成 `A<1>` 类型。如果不是类模板，表达式等价于 `(A<1)>0`，计算表达式 A 小于 1 的结果，再将结果与 0 比较大小。因此解析器先查找 `<` 前的名称，如果名称是模板才会把 `<` 看作左尖括号，其他情况则看作小于号

```cpp
namespace jc {

template <bool B>
struct A {
  static const bool value = B;
};

static_assert(A<(1 > 0)>::value);  // 必须使用小括号

}  // namespace jc

int main() {}
```

## [Dependent name](https://en.cppreference.com/w/cpp/language/dependent_name)

### 当前实例化（current instantiation）和未知特化（unknown specialization）

* Name lookup 对 dependent name 与 non-dependent name 有不同的查找规则，在模板定义中，依赖于模板参数的名称称为 dependent name，dependent name 包含当前实例化和未知特化。类模板的注入类名属于当前实例化，依赖于模板参数但不是当前实例化的为未知特化（unknown specialization）

```cpp
namespace jc {

template <typename T>
struct A {
  using type = T;

  A* a;        // A 是当前实例化
  A<type>* b;  // A<type> 是当前实例化
  A<T*>* c;    // A<T*> 是未知特化

  struct B {
    A* a;        // A 是当前实例化
    A<type>* b;  // A<type> 是当前实例化
    B* c;        // B 是当前实例化
  };

  struct C {
    A* a;        // A 是当前实例化
    A<type>* b;  // A<type> 是当前实例化
    B* c;        // 不在 B 的作用域内，B 是未知特化
    C* d;        // C 是当前实例化
  };
};

template <>
struct A<int>::B {
  int i;
};

}  // namespace jc

int main() {
  jc::A<double>::C{}.c->a;
  jc::A<int>::C{}.c->i;  // 使用特化的 A<int>::B
}
```

### typename 消歧义符

* 模板名称的问题主要是不能有效确定名称，模板中不能引用其他模板的名称，因为其他模板可能有使原名称失效的特化

```cpp
namespace jc {

template <typename T>
struct A {
  static constexpr int x = 0;  // x 是值
};

template <typename T>
struct B {
  int y;

  void f() {
    A<T>::x* y;  // 默认被看作乘法表达式
  }
};

template <>
struct A<int> {
  using x = int;  // x 是类型
};

}  // namespace jc

int main() {
  jc::B<int>{}.f();   // A<int>::x* y 是声明，int* y
  jc::B<void>{}.f();  // A<void>::x* y 是乘法，0 * y
}
```

* Dependent name 默认不会被看作类型，如果要表明是类型则需要加上 typename 消歧义符

```cpp
namespace jc {

template <typename T>
struct A {
  static constexpr int x = 0;  // x 是值
};

template <typename T>
struct B {
  int y;

  void f() {
    typename A<T>::x* y;  // 默认被看作声明
  }
};

template <>
struct A<int> {
  using x = int;  // x 是类型
};

}  // namespace jc

int main() {
  jc::B<int>{}.f();   // A<int>::x* y 是声明，int* y
  jc::B<void>{}.f();  // A<void>::x* y 是乘法，0 * y
}
```

* typename 消歧义符只能用于不在基类列表和初始化列表中的 dependent name，用作用域运算符访问 dependent name 中的成员类型时，必须指定 typename 消歧义符

```cpp
namespace jc {

struct Base {
  int i;
};

template <typename T>
struct A {
  using type = T;
};

template <typename T>
struct Derived : A<T>::type {  // 基类列表中不能加 typename 消歧义符
  Derived()
      : A<T>::type  // 初始化列表中不能加 typename 消歧义符
        (typename A<T>::type{0})  // 必须加 typename 消歧义符
  {}

  A<T> f() {
    typename A<T>::type* p;  // 必须加 typename 消歧义符
    return {};
  }

  A<int>::type* s;  // non-dependent name，typename 消歧义符可有可无
};

}  // namespace jc

int main() { jc::Derived<jc::Base>{}.f(); }
```

### template 消歧义符

* 访问模板参数的 dependent name 时，要在 dependent name 前加 template 消歧义符，才能让编译器知道引用的是一个模板，否则 `<` 会被视为小于号

```cpp
namespace jc {

template <typename T>
struct A {
  template <typename U>
  struct Impl {
    template <typename Y>
    static void f() {}
  };

  template <typename U>
  static void f() {}
};

}  // namespace jc

template <typename T>
void test() {
  T::template Impl<T>::template f<T>();
  T::template f<T>();
}

int main() { test<jc::A<int>>(); }
```

## Non-dependent base

* Non-dependent base 是不用知道模板实参就可以推断类型的基类，派生类中查找 non-dependent name 时会先查找 non-dependent base，再查找模板参数列表

```cpp
#include <type_traits>

namespace jc {

template <typename>
struct Base {
  using T = char;
};

template <typename T>
struct Derived1 : Base<void> {  // non-dependent base
  using type = T;               // T 是 Base<void>::T
};

template <typename T>
struct Derived2 : Base<T> {  // dependent base
  using type = T;            // T 是模板参数
};

static_assert(std::is_same_v<Derived1<int>::type, char>);
static_assert(std::is_same_v<Derived2<int>::type, int>);

}  // namespace jc

int main() {}
```

## Dependent base

* 对于 non-dependent name，不会在 dependent base 中做查找

```cpp
namespace jc {

template <typename>
struct Base {
  static constexpr int value = 1;
};

template <typename T>
struct Derived : Base<T> {  // dependent base
  constexpr int get_value() const {
    return value;  // 错误：不会在 dependent base 中查找 non-dependent name
  }
};

}  // namespace jc

int main() {}
```

* 如果要在 dependent base 中查找，则可以使用 `this->` 或作用域运算符将 non-dependent name 变为 dependent name

```cpp
namespace jc {

template <typename>
struct Base {
  static constexpr int value = 1;
};

template <typename T>
struct Derived : Base<T> {  // dependent base
  constexpr int get_value() const {
    return this->value;  // dependent name，会在 dependent base 中查找
  }
};

template <>
struct Base<bool> {
  static constexpr int value = 2;
};

}  // namespace jc

int main() {
  static_assert(jc::Derived<int>{}.get_value() == 1);
  static_assert(jc::Derived<bool>{}.get_value() == 2);
}
```

* 或者使用 using 声明，这样只需要引入一次

```cpp
namespace jc {

template <typename>
struct Base {
  static constexpr int value = 1;
};

template <typename T>
struct Derived : Base<T> {  // dependent base
  using Base<T>::value;

  constexpr int get_value() const {
    return value;  // dependent name，会在 dependent base 中查找
  }
};

template <>
struct Base<bool> {
  static constexpr int value = 2;
};

}  // namespace jc

int main() {
  static_assert(jc::Derived<int>{}.get_value() == 1);
  static_assert(jc::Derived<bool>{}.get_value() == 2);
}
```

* 使用作用域运算符不会访问虚函数

```cpp
#include <cassert>

namespace jc {

template <typename>
struct Base {
  virtual int f() const { return 1; }
};

template <typename T>
struct Derived : Base<T> {  // dependent base
  virtual int f() const { return 2; }
  int get_value() const { return Base<T>::f(); }
};

template <>
struct Base<bool> {
  virtual int f() const { return 3; }
};

}  // namespace jc

int main() {
  assert(jc::Derived<int>{}.get_value() == 1);
  assert(jc::Derived<bool>{}.get_value() == 3);
}
```

* 如果需要使用虚函数，则只能使用 `this->` 或 using 声明

```cpp
#include <cassert>

namespace jc {

template <typename>
struct Base {
  virtual int f() const { return 1; }
};

template <typename T>
struct Derived1 : Base<T> {  // dependent base
  virtual int f() const { return 2; }
  int get_value() const { return this->f(); }
};

template <typename T>
struct Derived2 : Base<T> {  // dependent base
  using Base<T>::f;
  virtual int f() const { return 2; }
  int get_value() const { return f(); }
};

template <>
struct Base<bool> {
  virtual int f() const { return 3; }
};

}  // namespace jc

int main() {
  assert(jc::Derived1<int>{}.get_value() == 2);
  assert(jc::Derived1<bool>{}.get_value() == 2);
  assert(jc::Derived2<int>{}.get_value() == 2);
  assert(jc::Derived2<bool>{}.get_value() == 2);
}
```
