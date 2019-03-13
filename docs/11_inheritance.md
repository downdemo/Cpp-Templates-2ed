## 空基类优化（EBCO，Empty Base Class Optimization）

* 为了保证给类动态分配内存时有不同的地址，C++ 规定空类大小必须大于 0

```cpp
namespace jc {

struct A {};
struct B {};

static_assert(sizeof(A) > 0);
static_assert(sizeof(B) > 0);

}  // namespace jc

int main() {
  jc::A a;
  jc::B b;
  static_assert((void*)&a != (void*)&b);
}
```

* 一般编译器将空类大小设为 1 字节，对于空类存在继承关系的情况，如果支持 EBCO，可以优化派生类的空间占用大小

```cpp
/* 不支持 EBCO 的内存布局：
 * [    ] } A } B } C
 * [    ]     }   }
 * [    ]         }
 *
 * 支持 EBCO 的内存布局：
 * [    ] } A } B } C
 */

namespace jc {

struct A {
  using Int = int;
};

struct B : A {};
struct C : B {};

static_assert(sizeof(A) == 1);
static_assert(sizeof(A) == sizeof(B));
static_assert(sizeof(A) == sizeof(C));

}  // namespace jc

int main() {}
```

* 模板参数可能是空类

```cpp
namespace jc {

struct A {};
struct B {};

template <typename T, typename U>
struct C {
  T a;
  U b;
};

static_assert(sizeof(C<A, B>) == 2);

}  // namespace jc

int main() {}
```

* 为了利用 EBCO 压缩内存空间，可以将模板参数设为基类

```cpp
namespace jc {

struct A {};
struct B {};

template <typename T, typename U>
struct C : T, U {};

static_assert(sizeof(C<A, B>) == 1);

}  // namespace jc

int main() {}
```

* 但模板参数可能是相同类型，或者不一定是类，此时将其设为基类在实例化时会报错。如果已知一个模板参数类型为空类，把可能为空的类型参数与一个不为空的成员利用 EBCO 合并起来，即可把空类占用的空间优化掉

```cpp
namespace jc {

template <typename Base, typename Member>
class Pair : private Base {
 public:
  Pair(const Base& b, const Member& m) : Base(b), member_(m) {}

  const Base& first() const { return (const Base&)*this; }

  Base& first() { return (Base&)*this; }

  const Member& second() const { return this->member_; }

  Member& second() { return this->member_; }

 private:
  Member member_;
};

template <typename T>
struct Unoptimizable {
  T info;
  void* storage;
};

template <typename T>
struct Optimizable {
  Pair<T, void*> info_and_storage;
};

}  // namespace jc

struct A {};

static_assert(sizeof(jc::Unoptimizable<A>) == 2 * sizeof(void*));
static_assert(sizeof(jc::Optimizable<A>) == sizeof(void*));

int main() {}
```

## 奇异递归模板模式（CRTP，The Curiously Recurring Template Pattern）

* CRTP 的实现手法是将派生类作为基类的模板参数

```cpp
#include <cassert>

namespace jc {

template <typename T>
class Base {
 public:
  static int count() { return i; }

 protected:
  Base() { ++i; }
  Base(const Base<T> &) { ++i; }
  Base(Base<T> &&) noexcept { ++i; }
  ~Base() { --i; }

 private:
  inline static int i = 0;
};

template <typename T>
class Derived : public Base<Derived<T>> {};

}  // namespace jc

int main() {
  jc::Derived<int> a, b;
  jc::Derived<char> c;
  assert(jc::Derived<int>::count() == 2);
  assert(jc::Derived<char>::count() == 1);
}
```

* 通常大量运算符重载会一起出现，但通常这些运算符只需要一个定义，其他运算符可以提取到基类中基于这一个来实现

```cpp
#include <cassert>

namespace jc {

template <typename T>
class Base {
  friend bool operator!=(const T& lhs, const T& rhs) { return !(lhs == rhs); }
};

class Derived : public Base<Derived> {
  friend bool operator==(const Derived& lhs, const Derived& rhs) {
    return lhs.i_ == rhs.i_;
  }

 public:
  Derived(int i) : i_(i) {}

 private:
  int i_ = 0;
};

}  // namespace jc

int main() {
  jc::Derived a{1};
  jc::Derived b{2};
  assert(a != b);
}
```

* CRTP 基类可以基于 CRTP 派生类暴露的小得多的接口定义大部分接口，这个模式称为 facade 模式

```cpp
#include <cassert>
#include <iterator>
#include <type_traits>
#include <vector>

namespace jc {

template <typename Derived, typename Value, typename Category>
class IteratorFacade {
 public:
  using value_type = std::remove_const_t<Value>;
  using reference = Value&;
  using pointer = Value*;
  using difference_type = std::ptrdiff_t;
  using iterator_category = Category;

 public:
  reference operator*() const { return as_derived().dereference(); }

  Derived& operator++() {
    as_derived().increment();
    return as_derived();
  }

  Derived operator++(int) {
    Derived tmp(as_derived());
    as_derived().increment();
    return tmp;
  }

  friend bool operator==(const IteratorFacade& lhs, const IteratorFacade& rhs) {
    return lhs.as_derived().equals(rhs.as_derived());
  }

  friend bool operator!=(const IteratorFacade& lhs, const IteratorFacade& rhs) {
    return !operator==(lhs, rhs);
  }

 private:
  Derived& as_derived() { return *static_cast<Derived*>(this); }

  const Derived& as_derived() const {
    return *static_cast<const Derived*>(this);
  }
};

template <typename T>
struct ListNode {
  ListNode(T x) : value(x) {}

  T value;
  ListNode<T>* next = nullptr;
};

template <typename T>
class ListNodeIterator
    : public IteratorFacade<ListNodeIterator<T>, T, std::forward_iterator_tag> {
 public:
  ListNodeIterator(ListNode<T>* t = nullptr) : t_(t) {}
  T& dereference() const { return t_->value; }
  void increment() { t_ = t_->next; }
  bool equals(const ListNodeIterator& rhs) const { return t_ == rhs.t_; }

 private:
  ListNode<T>* t_ = nullptr;
};

}  // namespace jc

int main() {
  auto a = new jc::ListNode<int>{1};
  auto b = new jc::ListNode<int>{2};
  auto c = new jc::ListNode<int>{3};
  a->next = b;
  b->next = c;

  auto first = jc::ListNodeIterator{a};
  auto last = ++jc::ListNodeIterator{c};

  std::vector<int> v;
  for (auto it = first; it != last; ++it) {
    v.emplace_back(*it);
  }
  assert((v == std::vector<int>{1, 2, 3}));

  delete c;
  delete b;
  delete a;
}
```

## Mixins

* 使用 Mixins 手法可以更方便地引入额外信息

```cpp
#include <cassert>
#include <string>

namespace jc {

template <typename... Mixins>
struct Point : Mixins... {
  Point() : Mixins()..., x(0.0), y(0.0) {}
  Point(double x, double y) : Mixins()..., x(x), y(y) {}
  double x;
  double y;
};

struct Label {
  std::string label = "point";
};

struct Color {
  enum { red, green, blue };
};

using CustomPoint = Point<Label, Color>;

}  // namespace jc

int main() {
  jc::CustomPoint p;
  assert(p.label == "point");
  assert(p.red == jc::Color::red);
  assert(p.green == jc::Color::green);
  assert(p.blue == jc::Color::blue);
}
```

* CRTP-mixin

```cpp
#include <cassert>
#include <string>

namespace jc {

template <typename T>
class Base {
 public:
  static int count() { return i; }

 protected:
  Base() { ++i; }
  Base(const Base<T> &) { ++i; }
  Base(Base<T> &&) noexcept { ++i; }
  ~Base() { --i; }

 private:
  inline static int i = 0;
};

template <template <typename> class... Mixins>
struct Point : Mixins<Point<>>... {
  Point() : Mixins<Point<>>()..., x(0.0), y(0.0) {}
  Point(double x, double y) : Mixins<Point<>>()..., x(x), y(y) {}
  double x;
  double y;
};

template <typename T>
struct Label {
  std::string label = "point";
};

template <typename T>
struct Color {
  enum { red, green, blue };
};

using PointCount = Point<Base, Label, Color>;

}  // namespace jc

int main() {
  jc::PointCount a, b, c;
  assert(jc::PointCount::count() == 3);
  assert(a.label == "point");
  assert(a.red == jc::Color<void>::red);
  assert(a.green == jc::Color<void>::green);
  assert(a.blue == jc::Color<void>::blue);
}
```

* Mixins 参数化成员函数的虚拟性

```cpp
#include <cassert>

namespace jc {

template <typename... Mixins>
class Base : private Mixins... {
 public:
  int f() { return 1; }  // 是否为虚函数由 Mixins 中的声明决定
};

template <typename... Mixins>
class Derived : public Base<Mixins...> {
 public:
  int f() { return 2; }
};

}  // namespace jc

struct A {};

struct B {
  virtual int f() = 0;
};

int main() {
  jc::Base<A>* p = new jc::Derived<A>;
  assert(p->f() == 1);

  jc::Base<B>* q = new jc::Derived<B>;
  assert(q->f() == 2);
}
```

## 指定模板参数

* 模板常常带有一长串类型参数，不过通常都设有默认值

```cpp
struct A {};
struct B {};
struct C {};

template <typename T1 = A, typename T2 = B, typename T3 = C>
struct MyClass {};
```

* 现在想指定某个实参，而其他参数依然使用默认实参

```cpp
namespace jc {

struct A {};
struct B {};
struct C {
  static constexpr int f() { return 1; }
};

struct Alias {
  using P1 = A;
  using P2 = B;
  using P3 = C;
};

template <typename T>
struct SetT1 : virtual Alias {
  using P1 = T;
};

template <typename T>
struct SetT2 : virtual Alias {
  using P2 = T;
};

template <typename T>
struct SetT3 : virtual Alias {
  using P3 = T;
};

// 由于不能从多个相同类直接继承，需要一个中间层用于区分
template <typename T, int N>
struct Mid : T {};

template <typename T1, typename T2, typename T3>
struct SetBase : Mid<T1, 1>, Mid<T2, 2>, Mid<T3, 3> {};

/* Alias 要被用作默认实参
 * 但 SetBase 会将其多次指定为 Mid 的基类
 * 为了防止多次继承产生二义性
 * 虚派生一个新类替代 Alias 作为默认实参
 */
struct Args : virtual Alias {};  // Args 即包含了别名 P1、P2、P3

template <typename T1 = Args, typename T2 = Args, typename T3 = Args>
struct MyClass {
  using Policies = SetBase<T1, T2, T3>;

  constexpr int f() { return Policies::P3::f(); }
};

struct D {
  static constexpr int f() { return 2; }
};

static_assert(MyClass{}.f() == 1);
static_assert(MyClass<SetT3<D>>{}.f() == 2);

}  // namespace jc

int main() {}
```
