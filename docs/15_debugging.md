## 浅实例化（Shallow Instantiation）

* 模板的报错会跟踪导致问题的所有层次，冗长的报错信息使调试变得更为繁琐，真正的问题一般出现在一长串实例化之后

```cpp
template <typename T>
void f1(T& i) {
  *i = 0;  // 假设 T 为指针类型
}

template <typename T>
void f2(T& i) {
  f1(i);
}

template <typename T>
void f3(typename T::Type i) {
  f2(i);
}

template <typename T>
void f4(const T&) {
  typename T::Type i = 42;
  f3<T>(i);
}

struct A {
  using Type = int;
};

int main() {
  f4(A{});  // 错误，只能在实例化时被检测到
            // 实例化 f4<A>(const A&)
            // 实例化 f3<A>(int)
            // 实例化 f2<int>(int&)
            // 实例化 f1<int>(int&)，解引用 int 出错
}

/*
 * error C2100: 非法的间接寻址
 * message : 查看对正在编译的函数 模板 实例化“void f1<T>(T &)”的引用
 *         with
 *         [
 *             T=A::Type
 *         ]
 * message : 查看对正在编译的函数 模板 实例化“void f2<A::Type>(T &)”的引用
 *         with
 *         [
 *             T=A::Type
 *         ]
 * message : 查看对正在编译的函数 模板 实例化“void f3<T>(A::Type)”的引用
 *         with
 *         [
 *             T=A
 *         ]
 * message : 查看对正在编译的函数 模板 实例化“void f4<A>(const T &)”的引用
 *         with
 *         [
 *             T=A
 *         ]
 */
```

* 一种简单的减少报错信息长度的方式是提前使用参数

```cpp
template <typename T>
void f1(T& i) {
  *i = 0;  // 假设 T 为指针类型
}

template <typename T>
void f2(T& i) {
  f1(i);
}

template <typename T>
void f3(typename T::Type i) {
  f2(i);
}

template <typename T>
void f4(const T&) {
  class ShallowChecks {  // 未调用，不影响运行期
    static void deref(typename T::Type p) { *p; }
  };
  typename T::Type i = 42;
  f3<T>(i);
}

struct A {
  using Type = int;
};

int main() {
  f4(A{});  // 实例化 f4<A>(const A&) 时检测到错误
}

/*
 * error C2100: 非法的间接寻址
 * message : 查看对正在编译的函数 模板 实例化“void f4<A>(const T &)”的引用
 *         with
 *         [
 *             T=A
 *         ]
 */
```

## 静态断言（Static Assertion）

* C++11 引入了 [static_assert](https://en.cppreference.com/w/cpp/language/static_assert)，在编译期进行断言，比如下列静态断言确保编译平台带 64 位指针

```cpp
static_assert(sizeof(void*) * CHAR_BIT == 64, "Not a 64-bit platform");
```

* 创建一个检查解引用的 traits，用 [static_assert](https://en.cppreference.com/w/cpp/language/static_assert) 提供更明确的诊断信息

```cpp
#include <type_traits>

template <typename T>
class has_dereference {
 private:
  template <typename U>
  struct Identity;

  template <typename U>
  static std::true_type test(Identity<decltype(*std::declval<U>())>*);

  template <typename U>
  static std::false_type test(...);

 public:
  static constexpr bool value = decltype(test<T>(nullptr))::value;
};

template <typename T>
inline constexpr bool has_dereference_v = has_dereference<T>::value;

template <typename T>
void f(T& i) {
  static_assert(has_dereference_v<T>, "T is not dereferenceable");
  *i = 0;
}

int main() {
  int i = 42;
  f(i);  // static_assert 报错：T is not dereferenceable
}
```

* C++17 可以用 [std::void_t](https://en.cppreference.com/w/cpp/types/void_t) 简化 traits 的实现

```cpp
#include <type_traits>

template <typename, typename = std::void_t<>>
struct has_dereference : std::false_type {};

template <typename T>
struct has_dereference<T, std::void_t<decltype(*std::declval<T>())>>
    : std::true_type {};

template <typename T>
inline constexpr bool has_dereference_v = has_dereference<T>::value;
```

## [Concepts](https://en.cppreference.com/w/cpp/concepts)

* C++20 可以用 concepts 约束类型，代码更简洁

```cpp
template <typename T>
concept Dereferenceable = requires(T x) {
  *x;
};

template <typename T>
requires Dereferenceable<T>
void f(T& i) { *i = 0; }

/* 等价写法
 * template <typename T>
 *   requires requires(T x) { *x; }
 * void f(T& i) {
 *   *i = 0;
 * }
 */

int main() {
  int i = 42;
  f(i);  // 未满足关联约束
}
```

## 原型（Archetype）

* 模板的一个挑战是确保满足特定约束的实参都能通过编译，为了测试满足要求的模板参数，引入原型的概念。原型是用户定义的类，以尽可能小的方式来满足模板大多数要求，而不提供任何外来的操作

```cpp
// 要求 T 是可比较类型
template <typename T>
int find(const T* a, int n, const T& v) {
  int i = 0;
  while (i != n && a[i] != v) {
    ++i;
  }
  return i;
}

struct EqualityComparable {};

struct ConvertibleToBool {
  operator bool() const { return true; }  // 提供本类型到 bool 的隐式转换
};

ConvertibleToBool  // 返回类型要求能转换为 bool
operator==(const EqualityComparable&, const EqualityComparable&) {
  return ConvertibleToBool{};
}

// 实例化 find<EqualityComparable>
template int find(const EqualityComparable*, int, const EqualityComparable&);

int main() {}
```

* 实例化将失败，改用 `operator==` 比较即可解决此问题

```cpp
template <typename T>
int find(const T* a, int n, const T& v) {
  int i = 0;
  while (i != n && !(a[i] == v)) {
    ++i;
  }
  return i;
}
```

* 但这又在无意中对结果使用了 `operator!`，如果要发现这点，在 ConvertibleToBool 中禁用 `operator!` 即可，当其被使用时将报错

```cpp
struct ConvertibleToBool {
  operator bool() const { return true; }
  bool operator!() = delete;
};
```

* 可以再对原型做其他扩展，比如禁用 `operator&&` 和 `operator||` 来找出其他的一些模板定义中的问题

## 跟踪程序（Tracer）

* 以上都是编译或链接时的 bug，更大的挑战是确保程序在运行期表现正确
* Tracer 是一个用户定义的类，它能用作要测试的模板的实参。通常 tracer 也是一个原型，但包含一些额外的信息。下面是一个用于测试 [std::sort](https://en.cppreference.com/w/cpp/algorithm/sort) 的 tracer，它提供 [std::sort](https://en.cppreference.com/w/cpp/algorithm/sort) 需要的功能（比如 `operator==` 和 `operator>`），并给出算法开销的直观结果，但不揭示排序模板的正确性

```cpp
#include <algorithm>
#include <iostream>

class SortTracer {
 public:
  static long creations() { return n_created; }
  static long destructions() { return n_destroyed; }
  static long assignments() { return n_assigned; }
  static long comparisons() { return n_compared; }
  static long max_live() { return n_max_live; }

 public:
  SortTracer(int v = 0) : value(v), generation(1) {
    ++n_created;
    update_max_live();
    std::cerr << "SortTracer #" << n_created << ", created generation "
              << generation << " (total: " << n_created - n_destroyed << ")\n";
  }

  SortTracer(const SortTracer& rhs)
      : value(rhs.value), generation(rhs.generation + 1) {
    ++n_created;
    update_max_live();
    std::cerr << "SortTracer #" << n_created << ", copied as generation "
              << generation << " (total: " << n_created - n_destroyed << ")\n";
  }

  ~SortTracer() {
    ++n_destroyed;
    update_max_live();
    std::cerr << "SortTracer generation " << generation
              << " destroyed (total: " << n_created - n_destroyed << ")\n";
  }

  SortTracer& operator=(const SortTracer& rhs) {
    ++n_assigned;
    std::cerr << "SortTracer assignment #" << n_assigned << " (generation "
              << generation << " = " << rhs.generation << ")\n";
    value = rhs.value;
    return *this;
  }

  friend bool operator<(const SortTracer& lhs, const SortTracer& rhs) {
    ++n_compared;
    std::cerr << "SortTracer comparison #" << n_compared << " (generation "
              << lhs.generation << " < " << rhs.generation << ")\n";
    return lhs.value < rhs.value;
  }

  int val() const { return value; }

 private:
  int value;                           // integer value to be sorted
  int generation;                      // generation of this tracer
  inline static long n_created = 0;    // number of constructor calls
  inline static long n_destroyed = 0;  // number of destructor calls
  inline static long n_assigned = 0;   // number of assignments
  inline static long n_compared = 0;   // number of comparisons
  inline static long n_max_live = 0;   // maximum of existing objects

  // recompute maximum of existing objects
  static void update_max_live() {
    if (n_created - n_destroyed > n_max_live) {
      n_max_live = n_created - n_destroyed;
    }
  }
};

int main() {
  SortTracer input[] = {7, 3, 5, 6, 4, 2, 0, 1, 9, 8};

  // 打印初始值
  for (int i = 0; i < 10; ++i) {
    std::cerr << input[i].val() << ' ';
  }
  std::cerr << '\n';

  // 记录初始条件
  long created_at_start = SortTracer::creations();
  long max_live_at_start = SortTracer::max_live();
  long assigned_at_start = SortTracer::assignments();
  long compared_at_start = SortTracer::comparisons();

  // 执行
  std::cerr << "---[ Start std::sort() ]--------------------\n";
  std::sort<>(&input[0], &input[9] + 1);
  std::cerr << "---[ End std::sort() ]----------------------\n";

  // 检查结果
  for (int i = 0; i < 10; ++i) {
    std::cerr << input[i].val() << ' ';
  }
  std::cerr << "\n\n";

  // final report
  std::cerr << "std::sort() of 10 SortTracer's was performed by:\n"
            << SortTracer::creations() - created_at_start
            << " temporary tracers\n"
            << "up to " << SortTracer::max_live()
            << " tracers at the same time (" << max_live_at_start
            << " before)\n"
            << SortTracer::assignments() - assigned_at_start << " assignments\n"
            << SortTracer::comparisons() - compared_at_start
            << " comparisons\n\n";
}

/*
 * SortTracer #1, created generation 1 (total: 1)
 * SortTracer #2, created generation 1 (total: 2)
 * SortTracer #3, created generation 1 (total: 3)
 * SortTracer #4, created generation 1 (total: 4)
 * SortTracer #5, created generation 1 (total: 5)
 * SortTracer #6, created generation 1 (total: 6)
 * SortTracer #7, created generation 1 (total: 7)
 * SortTracer #8, created generation 1 (total: 8)
 * SortTracer #9, created generation 1 (total: 9)
 * SortTracer #10, created generation 1 (total: 10)
 * 7 3 5 6 4 2 0 1 9 8
 * ---[ Start std::sort() ]--------------------
 * SortTracer #11, copied as generation 2 (total: 11)
 * SortTracer comparison #1 (generation 2 < 1)
 * SortTracer comparison #2 (generation 1 < 2)
 * SortTracer assignment #1 (generation 1 = 1)
 * SortTracer assignment #2 (generation 1 = 2)
 * SortTracer generation 2 destroyed (total: 10)
 * SortTracer #12, copied as generation 2 (total: 11)
 * SortTracer comparison #3 (generation 2 < 1)
 * SortTracer comparison #4 (generation 2 < 1)
 * SortTracer comparison #5 (generation 1 < 2)
 * SortTracer assignment #3 (generation 1 = 1)
 * SortTracer comparison #6 (generation 2 < 1)
 * SortTracer assignment #4 (generation 1 = 2)
 * SortTracer generation 2 destroyed (total: 10)
 * SortTracer #13, copied as generation 2 (total: 11)
 * SortTracer comparison #7 (generation 2 < 1)
 * SortTracer comparison #8 (generation 2 < 1)
 * SortTracer comparison #9 (generation 1 < 2)
 * SortTracer assignment #5 (generation 1 = 1)
 * SortTracer comparison #10 (generation 2 < 1)
 * SortTracer assignment #6 (generation 1 = 2)
 * SortTracer generation 2 destroyed (total: 10)
 * SortTracer #14, copied as generation 2 (total: 11)
 * SortTracer comparison #11 (generation 2 < 1)
 * SortTracer comparison #12 (generation 2 < 1)
 * SortTracer comparison #13 (generation 1 < 2)
 * SortTracer assignment #7 (generation 1 = 1)
 * SortTracer comparison #14 (generation 2 < 1)
 * SortTracer comparison #15 (generation 1 < 2)
 * SortTracer assignment #8 (generation 1 = 1)
 * SortTracer comparison #16 (generation 2 < 1)
 * SortTracer comparison #17 (generation 1 < 2)
 * SortTracer assignment #9 (generation 1 = 1)
 * SortTracer comparison #18 (generation 2 < 1)
 * SortTracer assignment #10 (generation 1 = 2)
 * SortTracer generation 2 destroyed (total: 10)
 * SortTracer #15, copied as generation 2 (total: 11)
 * SortTracer comparison #19 (generation 2 < 1)
 * SortTracer comparison #20 (generation 1 < 2)
 * SortTracer assignment #11 (generation 1 = 1)
 * SortTracer assignment #12 (generation 1 = 1)
 * SortTracer assignment #13 (generation 1 = 1)
 * SortTracer assignment #14 (generation 1 = 1)
 * SortTracer assignment #15 (generation 1 = 1)
 * SortTracer assignment #16 (generation 1 = 2)
 * SortTracer generation 2 destroyed (total: 10)
 * SortTracer #16, copied as generation 2 (total: 11)
 * SortTracer comparison #21 (generation 2 < 1)
 * SortTracer comparison #22 (generation 1 < 2)
 * SortTracer assignment #17 (generation 1 = 1)
 * SortTracer assignment #18 (generation 1 = 1)
 * SortTracer assignment #19 (generation 1 = 1)
 * SortTracer assignment #20 (generation 1 = 1)
 * SortTracer assignment #21 (generation 1 = 1)
 * SortTracer assignment #22 (generation 1 = 1)
 * SortTracer assignment #23 (generation 1 = 2)
 * SortTracer generation 2 destroyed (total: 10)
 * SortTracer #17, copied as generation 2 (total: 11)
 * SortTracer comparison #23 (generation 2 < 1)
 * SortTracer comparison #24 (generation 2 < 1)
 * SortTracer comparison #25 (generation 1 < 2)
 * SortTracer assignment #24 (generation 1 = 1)
 * SortTracer comparison #26 (generation 2 < 1)
 * SortTracer comparison #27 (generation 1 < 2)
 * SortTracer assignment #25 (generation 1 = 1)
 * SortTracer comparison #28 (generation 2 < 1)
 * SortTracer comparison #29 (generation 1 < 2)
 * SortTracer assignment #26 (generation 1 = 1)
 * SortTracer comparison #30 (generation 2 < 1)
 * SortTracer comparison #31 (generation 1 < 2)
 * SortTracer assignment #27 (generation 1 = 1)
 * SortTracer comparison #32 (generation 2 < 1)
 * SortTracer comparison #33 (generation 1 < 2)
 * SortTracer assignment #28 (generation 1 = 1)
 * SortTracer comparison #34 (generation 2 < 1)
 * SortTracer comparison #35 (generation 1 < 2)
 * SortTracer assignment #29 (generation 1 = 1)
 * SortTracer comparison #36 (generation 2 < 1)
 * SortTracer assignment #30 (generation 1 = 2)
 * SortTracer generation 2 destroyed (total: 10)
 * SortTracer #18, copied as generation 2 (total: 11)
 * SortTracer comparison #37 (generation 2 < 1)
 * SortTracer comparison #38 (generation 2 < 1)
 * SortTracer assignment #31 (generation 1 = 2)
 * SortTracer generation 2 destroyed (total: 10)
 * SortTracer #19, copied as generation 2 (total: 11)
 * SortTracer comparison #39 (generation 2 < 1)
 * SortTracer comparison #40 (generation 2 < 1)
 * SortTracer comparison #41 (generation 1 < 2)
 * SortTracer assignment #32 (generation 1 = 1)
 * SortTracer comparison #42 (generation 2 < 1)
 * SortTracer assignment #33 (generation 1 = 2)
 * SortTracer generation 2 destroyed (total: 10)
 * ---[ End std::sort() ]----------------------
 * 0 1 2 3 4 5 6 7 8 9
 *
 * std::sort() of 10 SortTracer's was performed by:
 * 9 temporary tracers
 * up to 11 tracers at the same time (10 before)
 * 33 assignments
 * 42 comparisons
 *
 * SortTracer generation 1 destroyed (total: 9)
 * SortTracer generation 1 destroyed (total: 8)
 * SortTracer generation 1 destroyed (total: 7)
 * SortTracer generation 1 destroyed (total: 6)
 * SortTracer generation 1 destroyed (total: 5)
 * SortTracer generation 1 destroyed (total: 4)
 * SortTracer generation 1 destroyed (total: 3)
 * SortTracer generation 1 destroyed (total: 2)
 * SortTracer generation 1 destroyed (total: 1)
 * SortTracer generation 1 destroyed (total: 0)
 */
```
