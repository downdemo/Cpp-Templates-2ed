## 基本Tuple设计

### 存储（Storage）

* `N>0`个元素的tuple可存储为一个单元素（首元素）和一个包含`N-1`个元素的tuple（尾），零元素tuple作特殊情况处理。如`Tuple<int, double, std::string>`存为`int`和`Tuple<double, std::string>`，`Tuple<double, std::string>`存为`double`和`Tuple<std::string>`，`Tuple<std::string>`存为`std::string`和一个`Tuple<>`

```cpp
template<typename... Types>
class Tuple;

// recursive case:
template<typename Head, typename... Tail>
class Tuple<Head, Tail...> {
 public:
  // constructors:
  Tuple() {}
  Tuple(const Head& head, const Tuple<Tail...>& tail)
  : head(head), tail(tail) {}

  Head& getHead() { return head; }
  const Head& getHead() const { return head; }
  Tuple<Tail...>& getTail() { return tail; }
  const Tuple<Tail...>& getTail() const { return tail; }
 private:
  Head head;
  Tuple<Tail...> tail;
};

// basis case:
template<>
class Tuple<> {
  // no storage required
};
```

* 对tuple t使用`get<Index>(t)`提取对应索引的元素，get实现如下

```cpp
// recursive case:
template<unsigned N>
struct TupleGet {
  template<typename Head, typename... Tail>
  static auto apply(const Tuple<Head, Tail...>& t)
  {
    return TupleGet<N-1>::apply(t.getTail());
  }
};

// basis case:
template<>
struct TupleGet<0> {
  template<typename Head, typename... Tail>
  static const Head& apply(const Tuple<Head, Tail...>& t)
  {
    return t.getHead();
  }
};

template<unsigned N, typename... Types>
auto get(const Tuple<Types...>& t)
{
  return TupleGet<N>::apply(t);
}
```

### 构造

* 已定义的构造函数不能满足如下的直接构造

```cpp
Tuple<int> t(1); // 错误：不能把int转换为Tuple<int>
```

* 为此还需要添加如下构造函数

```cpp
Tuple(const Head& head, const Tail&... tail)
: head(head), tail(tail...) {}
```

* 但这不是最通用的接口，用户可能希望移动构造，因此应该使用完美转发

```cpp
template<typename VHead, typename... VTail>
Tuple(VHead&& vhead, VTail&&... vtail)
: head(std::forward<VHead>(vhead)), tail(std::forward<VTail>(vtail)...) {}
```

* 同理，用完美转发实现从另一个tuple拷贝构造tuple

```cpp
template<typename VHead, typename... VTail>
Tuple(const Tuple<VHead, VTail...>& other)
: head(other.getHead()), tail(other.getTail()) {}
```

* 但这个构造不允许tuple的转换

```cpp
Tuple<int> t2(t); // 错误：t是Tuple<int>，无法转为int
```

* 原因在于，接受一系列值参数的构造模板比接受一个tuple参数的构造模板更优，为此必须使用[std::enable_if](https://en.cppreference.com/w/cpp/types/enable_if)，当尾部没有期望的长度时禁用成员函数模板。最终上述两个构造函数实现如下

```cpp
template<typename VHead, typename... VTail,
  typename = std::enable_if_t<sizeof...(VTail)==sizeof...(Tail)>>
Tuple(VHead&& vhead, VTail&&... vtail)
: head(std::forward<VHead>(vhead)), tail(std::forward<VTail>(vtail)...) {}

template<typename VHead, typename... VTail,
  typename = std::enable_if_t<sizeof...(VTail)==sizeof...(Tail)>>
Tuple(const Tuple<VHead, VTail...>& other)
: head(other.getHead()), tail(other.getTail()) {}
```

* 接着实现makeTuple，使用完美转发结合[std::decay](https://en.cppreference.com/w/cpp/types/decay)来转换字符串字面值和原始数组为指针，并移除cv和引用限定符

```cpp
template<typename... Types>
auto makeTuple(Types&&... elems)
{
  return Tuple<std::decay_t<Types>...>(std::forward<Types>(elems)...);
}

// 使用示例
Tuple<int, double, const char*> t = makeTuple(1, 3.14, "hi")
```

## 基本Tuple操作

### 比较

* 为了比较两个tuple，必须比较他们的元素

```cpp
// basis case:
bool operator==(const Tuple<>&, const Tuple<>&)
{
  return true;
}

// recursive case:
template<typename Head1, typename... Tail1,
  typename Head2, typename... Tail2,
  typename = std::enable_if_t<sizeof...(Tail1) == sizeof...(Tail2)>>
bool operator==(const Tuple<Head1, Tail1...>& lhs, const Tuple<Head2, Tail2...>& rhs)
{
  return lhs.getHead() == rhs.getHead() && lhs.getTail() == rhs.getTail();
}
```

* 其他比较运算符的实现同理

### 输出

```cpp
void printTuple(std::ostream& strm, const Tuple<>&, bool isFirst = true)
{
  strm << ( isFirst ? '(' : ')' );
}

template<typename Head, typename... Tail>
void printTuple(std::ostream& strm, const Tuple<Head, Tail...>& t,
  bool isFirst = true)
{
  strm << ( isFirst ? "(" : ", " );
  strm << t.getHead();
  printTuple(strm, t.getTail(), false);
}

template<typename... Types>
std::ostream& operator<<(std::ostream& strm, const Tuple<Types...>& t)
{
  printTuple(strm, t);
  return strm;
}

// 使用示例
std::cout << makeTuple(1, 3.14, std::string("hi")); // 打印(1, 3.14, hi)
```

## Tuple算法

### Tuple作为Typelist

* Tuple和Typelist一样接受任意数量的模板类型参数，因此通过一些偏特化，可以把Tuple转换成一个功能完整的Typelist

```cpp
template<typename T>
struct IsEmpty;
template<typename T>
struct FrontT;
template<typename T>
struct PopFrontT;
template<typename T, typename U>
struct PushFrontT;
template<typename T, typename U>
struct PushBackT;

template<>
struct IsEmpty<Tuple<>> {
  static constexpr bool value = true;
};

template<typename Head, typename... Tail>
class FrontT<Tuple<Head, Tail...>> {
 public:
  using Type = Head;
};

template<typename Head, typename... Tail>
class PopFrontT<Tuple<Head, Tail...>> {
 public:
  using Type = Tuple<Tail...>;
};

template<typename... Types, typename Element>
class PushFrontT<Tuple<Types...>, Element> {
 public:
  using Type = Tuple<Element, Types...>;
};

template<typename... Types, typename Element>
class PushBackT<Tuple<Types...>, Element> {
 public:
  using Type = Tuple<Types..., Element>;
};

template<typename T>
using Front = typename FrontT<T>::Type;

template<typename T>
using PopFront = typename PopFrontT<T>::Type;

template<typename T, typename U>
using PushFront = typename PushFrontT<T, U>::Type;

template<typename T, typename U>
using PushBack = typename PushBackT<T, U>::Type;
```

* 现在所有Typelist的算法也能用于Tuple

```cpp
Tuple<int, double, std::string> t1(42, 3.14, "hi");
using T2 = PopFront<PushBack<decltype(t1), bool>>;
// T2为Tuple<double, std::string, bool>
T2 t2(get<1>(t1), get<2>(t1), true);
std::cout << t2; // 打印(3.14, hi, 1)
```

### 添加删除

* 先从最简单的pushFront开始

```cpp
template<typename... Types, typename V>
PushFront<Tuple<Types...>, V>
pushFront(const Tuple<Types...>& tuple, const V& value)
{
  return PushFront<Tuple<Types...>, V>(value, tuple);
}
```

* pushBack复杂得多

```cpp
// basis case
template<typename V>
Tuple<V> pushBack(const Tuple<>&, const V& value)
{
  return Tuple<V>(value);
}

// recursive case
template<typename Head, typename... Tail, typename V>
Tuple<Head, Tail..., V>
pushBack(const Tuple<Head, Tail...>& tuple, const V& value)
{
  return Tuple<Head, Tail..., V>(tuple.getHead(),
    pushBack(tuple.getTail(), value));
}
```

* pushBack中返回的Tuple要求匹配一个参数为值、Tuple的构造函数，因此还需添加如下构造模板

```cpp
template<typename V, typename VHead, typename... VTail>
Tuple(const V& value, const Tuple<VHead, VTail...>& other)
: head(value), tail(other) {}
```

* PopFront很容易实现

```cpp
template<typename... Types>
PopFront<Tuple<Types...>>
popFront(const Tuple<Types...>& tuple)
{
  return tuple.getTail();
}
```

* 使用这些算法

```cpp
Tuple<int, double, std::string> t1(17, 3.14, "hi");
auto t2 = popFront(pushBack(t1, true));
std::cout << std::boolalpha << t2; // 打印(3.14, hi, true)
```

### Reverse

```cpp
// basis case
Tuple<> reverse(const Tuple<>& t)
{
  return t;
}

// recursive case
template<typename Head, typename... Tail>
Reverse<Tuple<Head, Tail...>>
reverse(const Tuple<Head, Tail...>& t)
{
  return pushBack(reverse(t.getTail()), t.getHead());
}
```

* 使用示例

```cpp
Tuple<int, double, std::string> t1(1, 3.14, "hi");
auto t2 = reverse(t1);
std::cout << t2; // 打印(hi, 3.14, 1)
```

* 和Typelist一样，现在可以用reverse结合popFront实现popBack

```cpp
template<typename... Types>
PopBack<Tuple<Types...>>
popBack(const Tuple<Types...>& tuple)
{
  return reverse(popFront(reverse(tuple)));
}
```

### 索引表

* 上面实现的reverse在运行期是低效的，为了解释此问题，引入一个计算拷贝次数的类

```cpp
template<int N>
struct CopyCounter {
  inline static unsigned numCopies = 0;
  CopyCounter() {}
  CopyCounter(const CopyCounter&)
  {
    ++numCopies;
  }
};

Tuple<CopyCounter<0>, CopyCounter<1>, CopyCounter<2>,
  CopyCounter<3>, CopyCounter<4>> copies;
auto reversed = reverse(copies);
std::cout << "0: " << CopyCounter<0>::numCopies << " copies\n";
std::cout << "1: " << CopyCounter<1>::numCopies << " copies\n";
std::cout << "2: " << CopyCounter<2>::numCopies << " copies\n";
std::cout << "3: " << CopyCounter<3>::numCopies << " copies\n";
std::cout << "4: " << CopyCounter<4>::numCopies << " copies\n";

// output
0: 5 copies
1: 8 copies
2: 9 copies
3: 8 copies
4: 5 copies
```

* 理想的reverse实现是每个元素只拷贝一次，直接从源tuple拷贝到目标tuple的正确位置，这点可以通过引用实现，但十分复杂。对于已知长度的tuple，消除无关拷贝的简单方法是使用makeTuple和get

```cpp
auto reversed = makeTuple(get<4>(copies), get<3>(copies),
  get<2>(copies), get<1>(copies), get<0>(copies));
```

* 这个程序将生成期望的输出，每个tuple元素只拷贝一次（实际测试是2次，get和makeTuple各一次），这种方法的思路是使用索引表。C++14提供了[std::integer_sequence](https://en.cppreference.com/w/cpp/utility/integer_sequence)用于表示索引表，这里将使用以前提过的Valuelist表示索引表。reverse对应的索引表为

```cpp
Valuelist<unsigned, 4, 3, 2, 1, 0>
```

* 生成这个索引表的方法是生成一个从0到N-1计数的索引表，再对其Reverse

```cpp
// recursive case
template<unsigned N, typename Result = Valuelist<unsigned>>
struct MakeIndexListT
: MakeIndexListT<N-1, PushFront<Result, CTValue<unsigned, N-1>>>
{};

// basis case
template<typename Result>
struct MakeIndexListT<0, Result> {
  using Type = Result;
};

template<unsigned N>
using MakeIndexList = typename MakeIndexListT<N>::Type;

using MyIndexList = Reverse<MakeIndexList<5>>;
// 等价于Valuelist<unsigned, 4, 3, 2, 1, 0>
```

* 现在使用索引表的reverse实现如下

```cpp
template<typename... Elements, unsigned... Indices>
auto reverseImpl(const Tuple<Elements...>& t, Valuelist<unsigned, Indices...>)
{
  return makeTuple(get<Indices>(t)...);
}

template<typename... Elements>
auto reverse(const Tuple<Elements...>& t)
{
  return reverseImpl(t, Reverse<MakeIndexList<sizeof...(Elements)>>());
}
```

* 如果不支持C++14，C++11中必须声明尾置返回类型

```cpp
template<typename... Elements, unsigned... Indices>
auto reverseImpl(const Tuple<Elements...>& t, Valuelist<unsigned, Indices...>)
-> decltype(makeTuple(get<Indices>(t)...))
{
  return makeTuple(get<Indices>(t)...);
}

template<typename... Elements>
auto reverse(const Tuple<Elements...>& t)
-> decltype(reverseImpl(t, Reverse<MakeIndexList<sizeof...(Elements)>>()))
{
  return reverseImpl(t, Reverse<MakeIndexList<sizeof...(Elements)>>());
}
```

### 洗牌算法和选择算法（Shuffle and Select）

* 基于如下select可以构建可以构建许多算法

```cpp
template<typename... Elements, unsigned... Indices>
auto select(const Tuple<Elements...>& t, Valuelist<unsigned, Indices...>)
{
  return makeTuple(get<Indices>(t)...);
}
```

* 一个基于select构建的简单算法是splat，它重复拷贝tuple中的一个元素一定次数，以创建另一个tuple

```cpp
Tuple<int, double, std::string> t1(1, 3.14, "hi");
auto t2 = splat<1, 4>(t1);
std::cout << t2; // (3.14, 3.14, 3.14, 3.14)
```

* splat实现如下

```cpp
template<unsigned I, unsigned N, typename IndexList = Valuelist<unsigned>>
class ReplicatedIndexListT;

template<unsigned I, unsigned N, unsigned... Indices>
class ReplicatedIndexListT<I, N, Valuelist<unsigned, Indices...>>
: public ReplicatedIndexListT<I, N-1, Valuelist<unsigned, Indices..., I>> {};

template<unsigned I, unsigned... Indices>
class ReplicatedIndexListT<I, 0, Valuelist<unsigned, Indices...>> {
 public:
  using Type = Valuelist<unsigned, Indices...>;
};

template<unsigned I, unsigned N>
using ReplicatedIndexList = typename ReplicatedIndexListT<I, N>::Type;

template<unsigned I, unsigned N, typename... Elements>
auto splat(const Tuple<Elements...>& t)
{
  return select(t, ReplicatedIndexList<I, N>());
}
```

* 也可以构建复杂算法，比如使用插入排序基于元素类型的大小排序一个tuple，为此需要实现一个sort函数，它接受一个比较tuple元素类型的元函数

```cpp
#include <complex>

template<typename T, typename U>
class SmallerThanT {
 public:
  static constexpr bool value = sizeof(T) < sizeof(U);
};

auto t1 = makeTuple(17LL, std::complex<double>(1, 2), 'c', 42, 3.14);
std::cout << t1 << '\n';
auto t2 = sort<SmallerThanT>(t1);
std::cout << t2;

// output
(17, (1,2), c, 42, 3.14)
(c, 42, 3.14, 17, (1,2))
```

* sort实现如下

```cpp
// metafunction wrapper that compares the elements in a tuple:
template<typename List, template<typename T, typename U> class F>
class MetafunOfNthElementT {
 public:
  template<typename T, typename U> class Apply;

  template<unsigned N, unsigned M>
  class Apply<CTValue<unsigned, M>, CTValue<unsigned, N>>
  : public F<NthElement<List, M>, NthElement<List, N>> {};
};

// sort a tuple based on comparing the element types:
template<
  template<typename T, typename U> class Compare,
  typename... Elements>
auto sort(const Tuple<Elements...>& t)
{
  return select(t,
    InsertionSort<
      MakeIndexList<sizeof...(Elements)>,
      MetafunOfNthElementT<Tuple<Elements...>, Compare>::template Apply
      >());
}
```

## Expanding Tuple

* 比如拆开Tuple，将其元素作为实参传递给函数

```cpp
template<typename F, typename... Elements, unsigned... Indices>
auto applyImpl(F f, const Tuple<Elements...>& t, Valuelist<unsigned, Indices...>)
->decltype(f(get<Indices>(t)...))
{
  return f(get<Indices>(t)...);
}

template<typename F, typename... Elements, unsigned N = sizeof...(Elements)>
auto apply(F f, const Tuple<Elements...>& t)
->decltype(applyImpl(f, t, MakeIndexList<N>()))
{
  return applyImpl(f, t, MakeIndexList<N>());
}

void f() {}

template<typename T, typename... Ts>
void f(T t, Ts... ts)
{
  std::cout << t << " ";
  f(ts...);
}

auto print = [] (auto... x) { f(x...); };

Tuple<std::string, const char*, int, char> t("Pi", "is roughly", 3, '!');
apply(print, t); // Pi is roughly 3 ! 
```

* C++17提供了[std::apply](https://en.cppreference.com/w/cpp/utility/apply)

## 优化

### Tuple与EBCO

* 严格来说，Tuple的存储实现比实际需要使用了更多的空间，一个问题在于tail成员最终将是一个空tuple，为此可使用EBCO，派生自尾tuple，而不是让它作为一个成员

```cpp
// recursive case:
template<typename Head, typename... Tail>
class Tuple<Head, Tail...> : private Tuple<Tail...> {
 public:
  Head& getHead() { return head; }
  const Head& getHead() const { return head; }
  Tuple<Tail...>& getTail() { return *this; }
  const Tuple<Tail...>& getTail() const { return *this; }
 private:
  Head head;
};
```

* 但这存在一个问题，之前head成员先于tail成员初始化，而这里tail在基类中，因此会先于head成员初始化。为此引入一个包裹head成员的类模板，将其作为基类并置于tail之前

```cpp
template<typename... Types>
class Tuple;

template<typename T>
class TupleElt {
 public:
  TupleElt() = default;

  template<typename U>
  TupleElt(U&& other) : value(std::forward<U>(other)) {}

  T& get() { return value; }
  const T& get() const { return value; }
 private:
  T value;
};

// recursive case:
template<typename Head, typename... Tail>
class Tuple<Head, Tail...>
: private TupleElt<sizeof...(Tail), Head>, private Tuple<Tail...> {
 public:
  Head& getHead()
  {
    return static_cast<HeadElt*>(this)->get();
  }
  const Head& getHead() const
  {
    return static_cast<const HeadElt*>(this)->get();
  }
  Tuple<Tail...>& getTail() { return *this; }
  const Tuple<Tail...>& getTail() const { return *this; }
 private:
  using HeadElt = TupleElt<sizeof...(Tail), Head>;
};

// basis case:
template<>
class Tuple<> {
  // no storage required
};
```

* 这个方法有一个潜在的问题：不能从有两个相同类型元素的tuple（比如`Tuple<int, int>`）提取元素，因为派生类到基类的TupleElt（比如`TupleElt<int>`）的转换将会是二义性的（会报warning）
* 为此需要确保每个TupleElt基类在给出的Tuple中是独一无二的，一个方法是将值的高度（即尾tuple的长度）设为模板参数，tuple中最后的元素高度为0，倒数第二个则为1，以此类推

```cpp
template<unsigned Height, typename T>
class TupleElt {
 public:
  TupleElt() = default;

  template<typename U>
  TupleElt(U&& other) : value(std::forward<U>(other)) {}

  T& get() { return value; }
  const T& get() const { return value; }
 private:
  T value;
};

template<typename... Types>
class Tuple;

// recursive case:
template<typename Head, typename... Tail>
class Tuple<Head, Tail...>
: private TupleElt<sizeof...(Tail), Head>, private Tuple<Tail...> {
 public:
  Head& getHead()
  {
    return static_cast<HeadElt*>(this)->get();
  }
  const Head& getHead() const
  {
    return static_cast<const HeadElt*>(this)->get();
  }
  Tuple<Tail...>& getTail() { return *this; }
  const Tuple<Tail...>& getTail() const { return *this; }
 private:
  using HeadElt = TupleElt<sizeof...(Tail), Head>;
};

// basis case:
template<>
class Tuple<> {
  // no storage required
};
```

* 这样就能在生成应用EBCO的Tuple时，保持初始化顺序并支持多个同类型元素

```cpp
struct A {
  A() { std::cout << "A()" << '\n'; }
};

struct B {
  B() { std::cout << "B()" << '\n'; }
};

int main()
{
  Tuple<A, char, A, char, B> t;
  std::cout << sizeof(t) << " bytes" << '\n';
}

// output
A()
A()
B()
5 bytes // VS实测是6
```

* EBCO消除了一个字节（对于空的`Tuple<>`）。注意到A和B都是空类，让TupleElt继承元素类型以进一步使用EBCO

```cpp
#include <type_traits>

template<unsigned Height, typename T,
  bool = std::is_class<T>::value && !std::is_final<T>::value>
class TupleElt;

template<unsigned Height, typename T>
class TupleElt<Height, T, false> {
 public:
  TupleElt() = default;

  template<typename U>
  TupleElt(U&& other) : value(std::forward<U>(other)) {}

  T& get() { return value; }
  const T& get() const { return value; }
 private:
  T value;
};

template<unsigned Height, typename T>
class TupleElt<Height, T, true> : private T {
 public:
  TupleElt() = default;

  template<typename U>
  TupleElt(U&& other) : T(std::forward<U>(other)) {}

  T& get() { return *this; }
  const T& get() const { return *this; }
};
```

* 之前的程序现在将输出

```cpp
A()
A()
B()
2 bytes // VS实测是3
```

### 常数时间的get

* 线性数量模板实例化会影响编译时间，EBCO优化允许一个更高效的get实现。关键在于匹配一个基类参数给一个派生类实参时，模板实参推断为基类。因此通过计算元素高度，就可以依赖于从Tuple特化到`TupleElt<H, T>`（T是被推断的）的转换来提取元素，而不需要手动遍历所有索引，这就只需要常数数量的模板实例化

```cpp
template<unsigned H, typename T>
T& getHeight(TupleElt<H, T>& te)
{
  return te.get();
}

template<typename... Types>
class Tuple;

template<unsigned I, typename... Elements>
auto get(Tuple<Elements...>& t)
-> decltype(getHeight<sizeof...(Elements)-I-1>(t))
{
  return getHeight<sizeof...(Elements)-I-1>(t);
}
```

* 注意getHeight必须声明为Tuple的友元以允许到私有基类的转换

```cpp
// inside the recursive case for class template Tuple:
template<unsigned I, typename... Elements>
friend auto get(Tuple<Elements...>& t)
-> decltype(getHeight<sizeof...(Elements)-I-1>(t));
```

## 下标（Subscript）

* 原则上可以定义一个`operator[]`访问元素，然而tuple的元素可以有不同类型，所以`operator[]`必须是模板，这就要求每个索引有一个不同类型，这需要使用Typelist中提过的类模板CTValue来表示一个类型中的数值索引

```cpp
template<typename T, T Index>
auto& operator[](CTValue<T, Index>)
{
  return get<Index>(*this);
}

// 使用示例
auto t = makeTuple(0, '1', 3.14f, std::string{"hi"});
auto a = t[CTValue<unsigned, 2>{}]; // a为3.14
auto b = t[CTValue<unsigned, 3>{}]; // b为"hi"

// 实测是上述模板返回auto&会报错（使用最初版本的get）
// 返回auto可行（但这与本来目的不符）
// 同理，对于如下调用get也会失败
// auto& x = get<0>(t);
// 而std::get是允许这样的
// 但std::tuple没有提供operator[]
```

* 之前为CTValue实现了字面值操作符初始化的模板，使用_c后缀即可生成CTValue，用在这里可以简化代码如下

```cpp
auto t = makeTuple(0, '1', 3.14f, std::string{"hi"});
auto a = t[2_c];
auto b = t[3_c];
```
