## 隐式实例化

* 编译器遇到模板特化时会用所给的模板实参替换对应的模板参数，从而产生特化。如果声明类模板的指针或引用，不需要看到类模板定义，但如果要访问特化的成员或想知道模板特化的大小，就要先看到定义

```cpp
namespace jc {

template <typename T>
struct A;

A<int>* p = 0;  // OK：不需要类模板定义

template <typename T>
struct A {
  void f();
};

void g(A<int>& a) {  // 只使用类模板声明
  a.f();             // 使用了类模板定义，需要 A::f() 的定义
}

template <typename T>
void A<T>::f() {}

}  // namespace jc

int main() {}
```

* 函数重载时，如果候选函数的参数是类类型，则该类必须可见。如果重载函数的参数是类模板，为了检查重载匹配，就需要实例化类模板。通过 [C++ Insights](https://cppinsights.io/) 或在 Visual Studio 中使用 `/d1templateStats` 命令行参数查看模板的实例化结果

```cpp
namespace jc {

template <typename T>
struct A {
  A(int);
};

void f(A<double>) {}
void f(int) {}

}  // namespace jc

int main() {
  jc::f(42);  // 两个函数声明都匹配，调用第二个，但仍会实例化第一个
}
```

## 延迟实例化（Lazy Instantiation）

* 隐式实例化类模板时，也会实例化每个成员声明，但不会实例化定义。例外的是匿名 union 和虚函数，union 成员会被实例化，虚函数是否实例化依赖于具体实现

```cpp
namespace jc {

template <int N>
struct A {
  int a[N];  // 编译器会假设 N 是正整数，实例化时 N <= 0 则失败
};

template <typename T, int N>
struct B {
  void f() {
    A<N> a;  // 如果 N <= 0，调用时出错
  }

  //   void error() {  // 即使不被调用也会引发错误
  //     A<-1> a;  // 要求给出 A<-1> 的完整定义，定义 -1 大小的数组出错
  //   }

  //   virtual void g();  // 虚函数只有声明没有定义会导致链接错误

  struct Nested {  // N <= 0 时使用该定义出错
    A<N> a;
  };

  //   union {    // union 的所有成员声明都会被生成
  //     A<N> a;  // N <= 0 时出错
  //   };
};

}  // namespace jc

int main() {
  jc::B<int, -1> b;
  //   b.f();                     // 调用则出错
  //   jc::B<int, -1>::Nested{};  // 错误
}
```

## 两阶段查找（Two-Phase Lookup）

* 编译器解析模板时，不能解析 [dependent name](https://en.cppreference.com/w/cpp/language/dependent_name)，于是会在 POI（point of instantiation）再次查找 dependent name，而 non-dependent name 在首次看到模板时就会进行查找，因此就有了两阶段查找，第一阶段发生在模板解析阶段，第二阶段在模板实例化阶段
  * 第一阶段使用 [unqualified name lookup](https://en.cppreference.com/w/cpp/language/lookup)（对于函数名查找会使用 [ADL](https://en.cppreference.com/w/cpp/language/adl)）查找 non-dependent name 和非受限的 dependent name（如具有 dependent name 实参的函数名称），但后者的查找不完整，在实例化时还会再次查找
  * 第二阶段发生的地点称为 POI，该阶段查找受限的 dependent name，并对非受限的 dependent name 再次进行 ADL

## POI（Points of Instantiation）

* 编译器会在模板中的某个位置访问模板实例的声明或定义，实例化相应的模板定义时就会产生 POI，POI 是代码中的一个点，在该点会插入替换后的模板实例

```cpp
namespace jc {

struct A {
  A(int i) : i(i) {}
  int i;
};

A operator-(const A& a) { return A{-a.i}; }

bool operator<(const A& lhs, const A& rhs) { return lhs.i < rhs.i; }

using Int = A;  // 若使用 int 而不使用 A 则无法使用 ADL 找到 g

template <typename T>
void f(T i) {
  if (i < 0) {
    g(-i);  // POI 二阶段查找，T 为 A 可以使用 ADL，T 为 int 则找不到 g
  }
}

// 此处不能为 POI，因为 g() 不可见，无法解析 g(-i)
void g(Int) {
  // 此处不能为 POI，不允许在此处插入 f<Int>(Int) 的定义
  f<Int>(42);  // 调用点
  // 此处不能为 POI，不允许在此处插入 f<Int>(Int) 的定义
}
// 是 POI，此时 g() 可见，实例化 f<Int>(Int)

}  // namespace jc

int main() {}
```

* 类模板实例的 POI 位置只能定义在包含该实例的声明或定义前的最近作用域

```cpp
namespace jc {

template <typename T>
struct A {
  T x;
};

// POI
int f() {
  // 不能是 POI，A<int> 的定义不能出现在函数作用域内
  return sizeof(A<int>);
  // 不能是 POI，A<int> 的定义不能出现在函数作用域内
}
// 不能是 POI，如果是 POI 则 sizeof(A<int>) 无效，因为编译后才知道大小

}  // namespace jc

int main() {}
```

* 实例化一个模板时，可能附带实例化其他模板

```cpp
namespace jc {

template <typename T>
struct A {
  using type = int;
};

// A<char> 的 POI
template <typename T>
void f() {
  A<char>::type a = 0;
  typename A<T>::type b = 0;
}

}  // namespace jc

int main() {
  jc::f<double>();
  // A<double> 的 POI
  // f<double> 的 POI
  // f 使用了 dependent name A<T>，需要一个二次 POI
  // 此处有两个 POI，对于类实例，二次 POI 位于主 POI 之前（函数实例则位置相同）
}
```

* 一个编译单元通常会包含一个实例的多个 POI，对类模板实例，每个编译单元只保留首个 POI，忽略其他 POI（它们不会被真正认为是 POI），对函数模板和变量模板的实例，所有 POI 都会被保留

## 模板的链接（Linkage of Template）

* 类模板不能和其他实例共用一个名称

```cpp
namespace jc {

int A;

class A;  // OK：两者名称在不同的空间

int B;

template <typename T>
struct B;  // 错误：名称冲突

struct C;

template <typename T>
struct C;  // 错误：名称冲突

}  // namespace jc

int main() {}
```

* 模板不能有 C linkage

```cpp
namespace jc {

extern "C++" template <typename T>
void normal();  // 默认方式，链接规范可以省略不写

extern "C" template <typename T>
void invalid();  // 错误：不能使用 C 链接

extern "Java" template <typename T>
void java_link();  // 非标准链接，某些编译器可能支持

}  // namespace jc

int main() {}
```

* 模板通常具有外链接（external linkage），唯一例外的是 static 修饰的函数模板

```cpp
template <typename T>  // 与其他文件中同名的声明指向相同的实例
void external();

template <typename T>  // 与其他文件中同名的模板无关
static void internal();

template <typename T>  // 重复声明
static void internal();

namespace {
template <typename>  // 与其他文件中同名的模板无关
void other_internal();
}

namespace {
template <typename>  // 重复声明
void other_internal();
}

struct {
  template <typename T>
  void f(T) {}  // 无链接：不能被重复声明
} x;

int main() {}
```

## 链接错误

* 和普通的函数不同，如果将模板的声明和实现分离，将出现链接错误，原因是编译器在函数调用处未看到实例化的函数定义，只是假设在别处提供了定义，并产生一个指向该定义的引用，并让链接器利用该引用解决此问题

```cpp
// a.hpp
#pragma once

namespace jc {

template <typename T>
class A {
 public:
  void f();
};

}  // namespace jc

// a.cpp
#include "a.hpp"

namespace jc {

template <typename T>
void A<T>::f() {}

}  // namespace jc

// main.cpp
#include "a.hpp"

int main() {
  jc::A<int>{}.f();  // 链接错误
}
```

* 推荐的做法是直接在头文件中实现模板

```cpp
// a.hpp
#pragma once

namespace jc {

template <typename T>
class A {
 public:
  void f();
};

template <typename T>
inline void A<T>::f() {}

}  // namespace jc

// main.cpp
#include "a.hpp"

int main() { jc::A<int>{}.f(); }
```

## [显式实例化（Explicit Instantiation）](https://en.cppreference.com/w/cpp/language/class_template#Explicit_instantiation)

* 如果希望在头文件中不暴露模板实现，则可以使用显式实例化，显式实例化相当于为模板手动实例化指定的类型，但用户仅能使用已指定类型的模板，可以在头文件中使用 extern 声明显式实例化，告知用户支持的实例化类型

```cpp
// a.hpp
#pragma once

namespace jc {

template <typename T>
class A {
 public:
  void f();
};

extern template class A<int>;         // 声明
extern template void A<double>::f();  // 声明

}  // namespace jc

// a.cpp
#include "a.hpp"

namespace jc {

template <typename T>
void A<T>::f() {}

template class A<int>;  // 实例化 A<int>，同时会实例化其所有成员
template void A<double>::f();  // 仅实例化该成员

}  // namespace jc

// main.cpp
#include "a.hpp"

int main() {
  jc::A<int>{}.f();
  jc::A<double>{}.f();
}
```

* 可以把显式实例化可提取到一个单独的文件中，注意这个文件要包含定义模板的文件

```cpp
// a.hpp
#pragma once

namespace jc {

template <typename T>
class A {
 public:
  void f();
};

extern template class A<int>;
extern template void A<double>::f();

}  // namespace jc

// a.cpp
#include "a.hpp"

namespace jc {

template <typename T>
void A<T>::f() {}

template class A<int>;
template void A<double>::f();

}  // namespace jc

// a_init.cpp
#include "a.cpp"

namespace jc {

template class A<int>;
template void A<double>::f();

}  // namespace jc

// main.cpp
#include "a.hpp"

int main() {
  jc::A<int>{}.f();
  jc::A<double>{}.f();
}
```

* 显式实例化不会影响类型推断规则，它只是实例化了一个实例，并不是一个可以优先匹配的非模板函数。从函数模板实例化而来的函数永远不和普通函数等价

```cpp
namespace jc {

template <typename T>
void f(T, T) {}

template void f<double>(double, double);

}  // namespace jc

int main() {
  jc::f<double>(1, 3.14);  // OK
  jc::f(1, 3.14);  // 错误：推断类型不一致，不存在普通函数 f(double, double)
}
```

* 显式实例化的本质是创建一个特化的实例，因此显式实例化之后，不能定义同类型的特化

```cpp
namespace jc {

template <typename T>
struct A {
  void f();
};

template <typename T>
void A<T>::f() {}

// template<> struct A<int> { void f() {} };
template struct A<int>;  // 相当于创建如上实例

// template <>
// struct A<int> {};  // 不允许重定义

}  // namespace jc

int main() {}
```

* 通过显式实例化给已有的类增加友元函数

```cpp
#include <cassert>
#include <iostream>
#include <string>

namespace jc {

template <auto>
struct A;

template <typename T, typename Class, T Class::*Member>
struct A<Member> {
  friend T& get(Class& c) { return c.*Member; }
};

}  // namespace jc

class Data {
 public:
  std::string value() const { return value_; }

 private:
  std::string value_ = "downdemo";
};

template struct jc::A<&Data::value_>;
std::string& jc::get(Data&);

int main() {
  Data data;
  assert(data.value() == "downdemo");
  jc::get(data) = "june";
  assert(data.value() == "june");
}
```
