template <typename... Elements>
class Typelist {};

template <typename List>
class FrontT;

template <typename Head, typename... Tail>
class FrontT<Typelist<Head, Tail...>> {
 public:
  using Type = Head;
};

template <typename List>
using Front = typename FrontT<List>::Type;

// PopFront
template <typename List>
class PopFrontT;

template <typename Head, typename... Tail>
class PopFrontT<Typelist<Head, Tail...>> {
 public:
  using Type = Typelist<Tail...>;
};

template <typename List>
using PopFront = typename PopFrontT<List>::Type;

// PushFront
template <typename List, typename NewElement>
class PushFrontT;

template <typename... Elements, typename NewElement>
class PushFrontT<Typelist<Elements...>, NewElement> {
 public:
  using Type = Typelist<NewElement, Elements...>;
};

template <typename List, typename NewElement>
using PushFront = typename PushFrontT<List, NewElement>::Type;

// NthElement
template <typename List, unsigned N>
class NthElementT : public NthElementT<PopFront<List>, N - 1> {};

template <typename List>
class NthElementT<List, 0> : public FrontT<List> {};

template <typename List, unsigned N>
using NthElement = typename NthElementT<List, N>::Type;

// IfThenElse
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

// IsEmpty
template <typename List>
class IsEmpty {
 public:
  static constexpr bool value = false;
};

template <>
class IsEmpty<Typelist<>> {
 public:
  static constexpr bool value = true;
};

// LargestType
template <typename List, bool Empty = IsEmpty<List>::value>
class LargestTypeT;

template <typename List>
class LargestTypeT<List, false> {
 private:
  using Contender = Front<List>;
  using Best = typename LargestTypeT<PopFront<List>>::Type;

 public:
  using Type = IfThenElse<(sizeof(Contender) >= sizeof(Best)), Contender, Best>;
};

template <typename List>
class LargestTypeT<List, true> {
 public:
  using Type = char;
};

template <typename List>
using LargestType = typename LargestTypeT<List>::Type;

// PushBack
template <typename List, typename NewElement, bool = IsEmpty<List>::value>
class PushBackRecT;

template <typename List, typename NewElement>
class PushBackRecT<List, NewElement, false> {
  using Head = Front<List>;
  using Tail = PopFront<List>;
  using NewTail = typename PushBackRecT<Tail, NewElement>::Type;

 public:
  using Type = PushFront<NewTail, Head>;
};

template <typename List, typename NewElement>
class PushBackRecT<List, NewElement, true> {
 public:
  using Type = PushFront<List, NewElement>;
};

template <typename List, typename NewElement>
class PushBackT : public PushBackRecT<List, NewElement> {};

template <typename List, typename NewElement>
using PushBack = typename PushBackT<List, NewElement>::Type;

// Reverse
template <typename List, bool Empty = IsEmpty<List>::value>
class ReverseT;

template <typename List>
using Reverse = typename ReverseT<List>::Type;

template <typename List>
class ReverseT<List, false>
    : public PushBackT<Reverse<PopFront<List>>, Front<List>> {};

template <typename List>
class ReverseT<List, true> {
 public:
  using Type = List;
};

// PopBack
template <typename List>
class PopBackT {
 public:
  using Type = Reverse<PopFront<Reverse<List>>>;
};

template <typename List>
using PopBack = typename PopBackT<List>::Type;

// Transform
template <typename List, template <typename T> class MetaFun,
          bool Empty = IsEmpty<List>::value>
class TransformT;

// 使用包扩展的优化版本
template <typename... Elements, template <typename T> class MetaFun>
class TransformT<Typelist<Elements...>, MetaFun, false> {
 public:
  using Type = Typelist<typename MetaFun<Elements>::Type...>;
};

// 不使用包扩展的版本
// template <typename List, template <typename T> class MetaFun>
// class TransformT<List, MetaFun, false>
//     : public PushFrontT<typename TransformT<PopFront<List>, MetaFun>::Type,
//                         typename MetaFun<Front<List>>::Type> {};

template <typename List, template <typename T> class MetaFun>
class TransformT<List, MetaFun, true> {
 public:
  using Type = List;
};

template <typename List, template <typename T> class MetaFun>
using Transform = typename TransformT<List, MetaFun>::Type;

// Accumulate
template <typename List, template <typename X, typename Y> class F,  // 元函数
          typename I,  // 初始类型
          bool = IsEmpty<List>::value>
class AccumulateT;

template <typename List, template <typename X, typename Y> class F, typename I>
class AccumulateT<List, F, I, false>
    : public AccumulateT<PopFront<List>, F, typename F<I, Front<List>>::Type> {
};

template <typename List, template <typename X, typename Y> class F, typename I>
class AccumulateT<List, F, I, true> {
 public:
  using Type = I;
};

template <typename List, template <typename X, typename Y> class F, typename I>
using Accumulate = typename AccumulateT<List, F, I>::Type;

// InsertSorted
template <typename T>
struct IdentityT {
  using Type = T;
};

template <typename List, typename Element,
          template <typename T, typename U> class Compare,
          bool = IsEmpty<List>::value>
class InsertSortedT;

template <typename List, typename Element,
          template <typename T, typename U> class Compare>
class InsertSortedT<List, Element, Compare, false> {
  // compute the tail of the resulting list:
  using NewTail = typename IfThenElse<
      Compare<Element, Front<List>>::value, IdentityT<List>,
      InsertSortedT<PopFront<List>, Element, Compare>>::Type;

  // compute the head of the resulting list:
  using NewHead =
      IfThenElse<Compare<Element, Front<List>>::value, Element, Front<List>>;

 public:
  using Type = PushFront<NewTail, NewHead>;
};

template <typename List, typename Element,
          template <typename T, typename U> class Compare>
class InsertSortedT<List, Element, Compare, true>
    : public PushFrontT<List, Element> {};

template <typename List, typename Element,
          template <typename T, typename U> class Compare>
using InsertSorted = typename InsertSortedT<List, Element, Compare>::Type;

// InsertionSort
template <typename List, template <typename T, typename U> class Compare,
          bool = IsEmpty<List>::value>
class InsertionSortT;

template <typename List, template <typename T, typename U> class Compare>
using InsertionSort = typename InsertionSortT<List, Compare>::Type;

template <typename List, template <typename T, typename U> class Compare>
class InsertionSortT<List, Compare, false>
    : public InsertSortedT<InsertionSort<PopFront<List>, Compare>, Front<List>,
                           Compare> {};

template <typename List, template <typename T, typename U> class Compare>
class InsertionSortT<List, Compare, true> {
 public:
  using Type = List;
};

// Multiply
template <typename T, T Value>
struct CTValue {
  static constexpr T value = Value;
};

template <typename T, typename U>
struct MultiplyT;

template <typename T, T Value1, T Value2>
struct MultiplyT<CTValue<T, Value1>, CTValue<T, Value2>> {
 public:
  using Type = CTValue<T, Value1 * Value2>;
};

template <typename T, typename U>
using Multiply = typename MultiplyT<T, U>::Type;

// Valuelist
template <typename T, T... Values>
struct Valuelist {};

template <typename T, T... Values>
struct IsEmpty<Valuelist<T, Values...>> {
  static constexpr bool value = sizeof...(Values) == 0;
};

template <typename T, T Head, T... Tail>
struct FrontT<Valuelist<T, Head, Tail...>> {
  using Type = CTValue<T, Head>;
  static constexpr T value = Head;
};

template <typename T, T Head, T... Tail>
struct PopFrontT<Valuelist<T, Head, Tail...>> {
  using Type = Valuelist<T, Tail...>;
};

template <typename T, T... Values, T New>
struct PushFrontT<Valuelist<T, Values...>, CTValue<T, New>> {
  using Type = Valuelist<T, New, Values...>;
};

template <typename T, T... Values, T New>
struct PushBackT<Valuelist<T, Values...>, CTValue<T, New>> {
  using Type = Valuelist<T, Values..., New>;
};

// Select
template <typename Types, typename Indices>
class SelectT;

template <typename Types, unsigned... Indices>
class SelectT<Types, Valuelist<unsigned, Indices...>> {
 public:
  using Type = Typelist<NthElement<Types, Indices>...>;
};

template <typename Types, typename Indices>
using Select = typename SelectT<Types, Indices>::Type;
