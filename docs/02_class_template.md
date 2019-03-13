## [类模板](https://en.cppreference.com/w/cpp/language/class_template)

* 和函数类似，类也支持泛型，比如实现一个基于拓扑排序遍历的有向无环图的森林

```cpp
#include <algorithm>
#include <cassert>
#include <functional>
#include <map>
#include <memory>
#include <queue>
#include <set>
#include <unordered_set>
#include <vector>

namespace jc {

template <typename K, typename V>
struct DAGNode {
  K k;
  V v;
  std::set<DAGNode<K, V>*> in;
  std::set<DAGNode<K, V>*> out;
};

template <typename K, typename V>
class DAGGraph {
 public:
  bool AddEdge(const K& from, const K& to);

  V& operator[](const K& key);

  bool Exist(const K& key) const;

  void Clear();

  std::size_t Size() const;

  void Walk(std::function<void(const K& k, const V& v)> f,
            bool start_from_head = true);

  void WalkHeads(std::function<void(const K& k, const V& v)> f);

  void WalkTails(std::function<void(const K& k, const V& v)> f);

  std::unordered_set<K> NextKeys();

  std::unordered_set<K> NextKeys(const K& key);

 private:
  bool IsCyclic(const DAGNode<K, V>& from, const DAGNode<K, V>& to) const;

  void RefreshWalkSequences();

  std::vector<std::set<K>> ConnectedComponents() const;

  void DFS(const K& k, std::unordered_set<K>* visited,
           std::set<K>* connected_components) const;

  std::vector<K> TopologicalSequence(const std::set<K>& connected_components,
                                     bool start_from_head) const;

 private:
  std::map<K, DAGNode<K, V>> bucket_;
  std::unordered_set<K> heads_;
  std::unordered_set<K> tails_;
  std::vector<std::vector<K>> sequences_start_from_head_;
  std::vector<std::vector<K>> sequences_start_from_tail_;

 private:
  bool allow_modify_ = true;
  std::vector<std::vector<K>> sequences_start_from_head_for_next_;
  std::unordered_set<K> current_heads_for_next_;
};

template <typename K, typename V>
inline bool DAGGraph<K, V>::AddEdge(const K& from, const K& to) {
  assert(allow_modify_);
  if (from == to || !bucket_.count(from) || !bucket_.count(to) ||
      IsCyclic(bucket_.at(from), bucket_.at(to))) {
    return false;
  }
  bucket_.at(from).out.emplace(&bucket_.at(to));
  bucket_.at(to).in.emplace(&bucket_.at(from));
  heads_.erase(to);
  tails_.erase(from);
  sequences_start_from_head_.clear();
  sequences_start_from_tail_.clear();
  return true;
}

template <typename K, typename V>
inline V& DAGGraph<K, V>::operator[](const K& key) {
  if (!bucket_.count(key)) {
    assert(allow_modify_);
    bucket_[key].k = key;
    heads_.emplace(key);
    tails_.emplace(key);
    sequences_start_from_head_.clear();
    sequences_start_from_tail_.clear();
  }
  return bucket_.at(key).v;
}

template <typename K, typename V>
inline bool DAGGraph<K, V>::Exist(const K& key) const {
  return bucket_.count(key);
}

template <typename K, typename V>
inline void DAGGraph<K, V>::Clear() {
  allow_modify_ = true;
  bucket_.clear();
  heads_.clear();
  tails_.clear();
  sequences_start_from_head_.clear();
  sequences_start_from_tail_.clear();
}

template <typename K, typename V>
inline std::size_t DAGGraph<K, V>::Size() const {
  return bucket_.size();
}

template <typename K, typename V>
inline void DAGGraph<K, V>::Walk(std::function<void(const K& k, const V& v)> f,
                                 bool start_from_head) {
  if (sequences_start_from_head_.empty()) {
    RefreshWalkSequences();
  }
  const std::vector<std::vector<K>>& seqs_to_walk =
      start_from_head ? sequences_start_from_head_ : sequences_start_from_tail_;
  for (const std::vector<K>& seq : seqs_to_walk) {
    std::for_each(std::begin(seq), std::end(seq), [&](const K& key) {
      const DAGNode<K, V>& node = bucket_.at(key);
      f(node.k, node.v);
    });
  }
}

template <typename K, typename V>
inline void DAGGraph<K, V>::WalkHeads(
    std::function<void(const K& k, const V& v)> f) {
  if (sequences_start_from_head_.empty()) {
    RefreshWalkSequences();
  }
  for (const std::vector<K>& seq : sequences_start_from_head_) {
    std::for_each(std::begin(seq), std::end(seq), [&](const K& key) {
      if (heads_.count(key)) {
        const DAGNode<K, V>& node = bucket_.at(key);
        f(node.k, node.v);
      }
    });
  }
}

template <typename K, typename V>
inline void DAGGraph<K, V>::WalkTails(
    std::function<void(const K& k, const V& v)> f) {
  if (sequences_start_from_head_.empty()) {
    RefreshWalkSequences();
  }
  for (const std::vector<K>& seq : sequences_start_from_tail_) {
    std::for_each(std::begin(seq), std::end(seq), [&](const K& key) {
      if (tails_.count(key)) {
        const DAGNode<K, V>& node = bucket_.at(key);
        f(node.k, node.v);
      }
    });
  }
}

template <typename K, typename V>
inline std::unordered_set<K> DAGGraph<K, V>::NextKeys() {
  assert(allow_modify_);  // allowed call once unless Clear()
  allow_modify_ = false;
  current_heads_for_next_ = heads_;
  if (sequences_start_from_head_.empty()) {
    RefreshWalkSequences();
  }
  return heads_;
}

template <typename K, typename V>
inline std::unordered_set<K> DAGGraph<K, V>::NextKeys(const K& key) {
  assert(!allow_modify_);  // must call NextKeys() before
  assert(current_heads_for_next_.count(key));
  current_heads_for_next_.erase(key);

  std::unordered_set<K> res;
  for (std::vector<K>& seq : sequences_start_from_head_for_next_) {
    auto it = std::find(begin(seq), std::end(seq), key);
    if (it == std::end(seq)) {
      continue;
    }
    seq.erase(it);
    const std::set<DAGNode<K, V>*>& nodes = bucket_.at(key).out;
    for (DAGNode<K, V>* v : nodes) {
      const std::set<DAGNode<K, V>*>& prev_nodes = v->in;
      bool no_prev_node_in_seq =
          std::all_of(std::begin(prev_nodes), std::end(prev_nodes),
                      [&](DAGNode<K, V>* in_node) {
                        return std::find(std::begin(seq), std::end(seq),
                                         in_node->k) == std::end(seq);
                      });
      if (no_prev_node_in_seq) {
        current_heads_for_next_.emplace(v->k);
        res.emplace(v->k);
      }
    }
    break;
  }
  return res;
}

template <typename K, typename V>
inline bool DAGGraph<K, V>::IsCyclic(const DAGNode<K, V>& from,
                                     const DAGNode<K, V>& to) const {
  std::queue<DAGNode<K, V>*> q;
  for (DAGNode<K, V>* v : from.in) {
    q.emplace(v);
  }

  std::unordered_set<DAGNode<K, V>*> visited;
  while (!q.empty()) {
    DAGNode<K, V>* node = q.front();
    q.pop();
    if (visited.count(node)) {
      continue;
    }
    if (node == &to) {
      return true;
    }
    visited.emplace(node);
    for (DAGNode<K, V>* v : node->in) {
      q.emplace(v);
    }
  }

  return false;
}

template <typename K, typename V>
inline void DAGGraph<K, V>::RefreshWalkSequences() {
  sequences_start_from_head_.clear();
  sequences_start_from_tail_.clear();

  const std::vector<std::set<K>> connected_components = ConnectedComponents();
  for (const std::set<K>& x : connected_components) {
    const std::vector<K> seq_from_head = TopologicalSequence(x, true);
    const std::vector<K> seq_from_tail = TopologicalSequence(x, false);
    assert(!seq_from_head.empty());
    assert(!seq_from_tail.empty());
    sequences_start_from_head_.emplace_back(seq_from_head);
    sequences_start_from_tail_.emplace_back(seq_from_tail);
  }

  sequences_start_from_head_for_next_ = sequences_start_from_head_;
}

template <typename K, typename V>
inline std::vector<std::set<K>> DAGGraph<K, V>::ConnectedComponents() const {
  std::vector<std::set<K>> res;
  std::unordered_set<K> visited;
  for (auto& x : bucket_) {
    std::set<K> tmp;
    DFS(x.second.k, &visited, &tmp);
    if (!tmp.empty()) {
      res.emplace_back(tmp);
    }
  }
  std::sort(std::begin(res), std::end(res),
            [&](const std::set<K>& lhs, const std::set<K>& rhs) {
              return lhs.size() < rhs.size();
            });
  return res;
}

template <typename K, typename V>
inline void DAGGraph<K, V>::DFS(const K& k, std::unordered_set<K>* visited,
                                std::set<K>* connected_components) const {
  if (visited->count(k)) {
    return;
  }
  visited->emplace(k);
  connected_components->emplace(k);
  if (!bucket_.at(k).in.empty()) {
    for (DAGNode<K, V>* v : bucket_.at(k).in) {
      DFS(v->k, visited, connected_components);
    }
  }
  if (!bucket_.at(k).out.empty()) {
    for (DAGNode<K, V>* v : bucket_.at(k).out) {
      DFS(v->k, visited, connected_components);
    }
  }
}

template <typename K, typename V>
inline std::vector<K> DAGGraph<K, V>::TopologicalSequence(
    const std::set<K>& connected_components, bool start_from_head) const {
  std::map<K, std::vector<K>> adjacency_list;
  std::map<K, int32_t> in_degree;

  for (const K& key : connected_components) {
    if (!in_degree.count(key)) {
      in_degree.emplace(key, 0);
    }
    const std::set<DAGNode<K, V>*>& nodes =
        start_from_head ? bucket_.at(key).out : bucket_.at(key).in;
    for (DAGNode<K, V>* v : nodes) {
      adjacency_list[key].emplace_back(v->k);
      ++in_degree[v->k];
    }
  }

  std::queue<K> q;
  for (auto& x : in_degree) {
    if (x.second == 0) {
      q.emplace(x.first);
    }
  }

  std::vector<K> res;
  while (!q.empty()) {
    const K key = q.front();
    q.pop();
    res.emplace_back(key);
    for (const K& k : adjacency_list[key]) {
      if (--in_degree.at(k) == 0) {
        q.emplace(k);
      }
    }
  }

  assert(res.size() == connected_components.size());  // graph is DAG
  return res;
}

}  // namespace jc

namespace jc::test {

class MockPipelineEngine {
 public:
  void Start() {}
  void Stop() {}
  void Destroy() {}
};

void test() {
  DAGGraph<int, std::unique_ptr<MockPipelineEngine>> d;
  // Make Direct Acyclic Graph:
  //    0    6      11  13
  //   / \   |      |
  //  1   3  7  8   12
  //  | x |      \ /
  //  2   4       9
  //   \ /        |
  //    5         10
  // Traverse each child graph in order whose size smaller

  // Start Order:
  // 13
  // 6 -> 7
  // 8 -> 11 -> 12 -> 9 -> 10
  // 0 -> 1 -> 3 -> 2 -> 4 -> 5
  // Stop Order:
  // 13
  // 7 -> 6
  // 10 -> 9 -> 8 -> 12 -> 11
  // 5 -> 2 -> 4 -> 1 -> 3 -> 0

  constexpr int nodes_count = 14;
  for (int i = 0; i < nodes_count; ++i) {
    d[i].reset(new MockPipelineEngine);
  }
  assert(d.AddEdge(0, 1));
  assert(d.AddEdge(0, 3));
  assert(d.AddEdge(1, 2));
  assert(d.AddEdge(3, 4));
  assert(d.AddEdge(1, 4));
  assert(d.AddEdge(3, 2));
  assert(d.AddEdge(2, 5));
  assert(d.AddEdge(4, 5));
  assert(d.AddEdge(6, 7));
  assert(d.AddEdge(8, 9));
  assert(d.AddEdge(9, 10));
  assert(d.AddEdge(11, 12));
  assert(d.AddEdge(12, 9));

  assert(d.Size() == nodes_count);

  for (int i = 0; i < nodes_count; ++i) {
    assert(d.Exist(i));
  }

  assert(!d.AddEdge(1, 0));
  assert(!d.AddEdge(2, 0));
  assert(!d.AddEdge(4, 0));
  assert(!d.AddEdge(7, 6));
  assert(!d.AddEdge(10, 11));
  assert(!d.AddEdge(13, 13));
  assert(!d.AddEdge(13, 14));

  constexpr bool start_from_head = true;
  {
    std::vector<int> v;
    std::vector<int> start_order{13, 6, 7, 8, 11, 12, 9, 10, 0, 1, 3, 2, 4, 5};
    d.Walk(
        [&](int key, const std::unique_ptr<MockPipelineEngine>& pipeline) {
          pipeline->Start();
          v.emplace_back(key);
        },
        start_from_head);
    assert(v == start_order);
  }

  {
    std::vector<int> v;
    std::vector<int> stop_order{13, 7, 6, 10, 9, 8, 12, 11, 5, 2, 4, 1, 3, 0};
    d.Walk(
        [&](int key, const std::unique_ptr<MockPipelineEngine>& pipeline) {
          pipeline->Stop();
          v.emplace_back(key);
        },
        !start_from_head);
    assert(v == stop_order);
  }

  {
    std::vector<int> v;
    std::vector<int> heads_order{13, 6, 8, 11, 0};
    d.WalkHeads(
        [&](int key, const std::unique_ptr<MockPipelineEngine>& pipeline) {
          pipeline->Destroy();
          v.emplace_back(key);
        });
    assert(v == heads_order);
  }

  {
    std::vector<int> v;
    std::vector<int> tails_order{13, 7, 10, 5};
    d.WalkTails(
        [&](int key, const std::unique_ptr<MockPipelineEngine>& pipeline) {
          pipeline->Destroy();
          v.emplace_back(key);
        });
    assert(v == tails_order);
  }

  {
    std::vector<int> test_sequence{13, 6, 7, 0,  1,  3, 4,
                                   2,  5, 8, 11, 12, 9, 10};

    std::unordered_set<int> heads{0, 6, 8, 11, 13};
    assert(d.NextKeys() == heads);

    std::vector<std::unordered_set<int>> next_keys{
        {}, {7}, {}, {1, 3}, {}, {2, 4}, {}, {5}, {}, {}, {12}, {9}, {10}, {},
    };

    assert(test_sequence.size() == nodes_count);
    assert(next_keys.size() == nodes_count);
    for (int i = 0; i < nodes_count; ++i) {
      assert(d.NextKeys(test_sequence[i]) == next_keys[i]);
    }
  }

  d.Clear();
  assert(d.Size() == 0);
  for (int i = 0; i < nodes_count; ++i) {
    assert(!d.Exist(i));
  }
}

}  // namespace jc::test

int main() { jc::test::test(); }
```

* 对于不同类型模板参数的类模板，会为每个类型实例化出不同的类，类的函数被调用时才实例化，类模板的 static 数据成员会分别在每个不同的类中实例化，static 数据成员和成员函数只被同一个类共享。通过 [C++ Insights](https://github.com/andreasfertig/cppinsights) 可以查看类的实例化代码，该工具提供了[在线版本](https://cppinsights.io/)

```cpp
namespace jc {

template <typename T>
class A {
 public:
  static int value();

 private:
  static int n;
};

template <typename T>
inline int A<T>::value() {
  return n;
}

template <typename T>
int A<T>::n = 0;

}  // namespace jc

int main() {
  using namespace jc;
  A<void> a;    // 实例化 A<void>::n
  A<int> b, c;  // 实例化 A<int>::n，bc 共享 A<int>::value() 和 A<int>::n
  int n = A<double>::value();  // 实例化 A<double>::value()
  n = b.value();               // 使用 A<int>::value()
  n = A<int>::value();  // 必须指定模板参数以确定实例化版本
}
```

* 由于函数被调用时才实例化，如果不调用实例化时会出错的函数，代码也能通过编译

```cpp
namespace jc {

template <typename T>
class A {
 public:
  static int value();

 private:
  static int n;
};

template <typename T>
inline int A<T>::value() {
  return f(n);
}

template <typename T>
int A<T>::n = 0;

}  // namespace jc

int main() {
  using namespace jc;
  A<void> a;  // OK，实例化 A<void>::n
  //   a.value();         // 实例化错误，f 未声明
  //   A<void>::value();  // 实例化错误，f 未声明
}
```

## 友元

* 类内定义友元可以省略模板参数，但友元函数在类模板实例化后才会实例化，如果类模板中的友元函数不包含模板参数，则会出现重定义的错误

```cpp
#include <iostream>
#include <typeinfo>

namespace jc {

template <typename T>
class A {
  // 类作用域内的 A 是注入类名，等价于 A<T>
  friend std::ostream& operator<<(std::ostream& os, const A& rhs) {
    return os << "A<" << typeid(T).name() << "> = " << rhs.n;
  }

  friend void f() {}

 private:
  int n = 0;
};

}  // namespace jc

int main() {
  jc::A<void> a;  // 实例化 operator<<(std::ostream&, const A<void>&) 和 f()
  std::cout << a;  // A<void> = 0
  // jc::A<int> b;    // 错误：第二次实例化 f()，重定义
}
```

* 类外定义友元不会有重定义的问题，需要在类内声明处为类模板额外指定不同的模板参数

```cpp
#include <iostream>
#include <typeinfo>

namespace jc {

template <typename T>
class A {
  template <typename U>
  friend std::ostream& operator<<(std::ostream& os, const A<U>& rhs);

  friend void f();

 private:
  int n = 0;
};

template <typename T>
std::ostream& operator<<(std::ostream& os, const A<T>& rhs) {
  return os << "A<" << typeid(T).name() << "> = " << rhs.n;
}

void f() {}

}  // namespace jc

int main() {
  jc::A<void> a;
  std::cout << a;  // A<void> = 0
  jc::A<int> b;
  std::cout << b;  // A<int> = 0
}
```

* 如果要在类外定义友元，又不想在类内声明额外指定模板参数，则可以将友元声明为函数模板，在类内使用模板实例作为友元

```cpp
#include <iostream>
#include <typeinfo>

namespace jc {

template <typename T>
class A;

template <typename T>
std::ostream& operator<<(std::ostream& os, const A<T>& rhs);

template <typename T>
class A {
  friend std::ostream& operator<< <T>(std::ostream& os, const A<T>& rhs);

 private:
  int n = 0;
};

template <typename T>
std::ostream& operator<<(std::ostream& os, const A<T>& rhs) {
  return os << "A<" << typeid(T).name() << "> = " << rhs.n;
}

}  // namespace jc

int main() {
  jc::A<void> a;
  std::cout << a;  // A<void> = 0
}
```

* 如果将类模板实例声明为友元，则类模板必须已声明并可见

```cpp
namespace jc {

template <typename T>
struct Node;

template <typename T>
struct Tree {
  friend class Node<T>;  // 友元类模板必须已声明并可见
  friend class A;        // 友元类可以未声明
};

}  // namespace jc

int main() {}
```

* 模板参数可以是友元

```cpp
namespace jc {

template <typename T>
class A {
  friend T;  // 如果 T 不是 class 则忽略

 private:
  int n = 0;
};

class B {
 public:
  static int f(const A<B>& a) { return a.n; }
};

}  // namespace jc

int main() {}
```

## [特化（Specialization）](https://en.cppreference.com/w/cpp/language/template_specialization)

* 特化一般指的是全特化，即为一种特定类型指定一个特定实现，该类型将不使用模板的实例化版本

```cpp
#include <cassert>

namespace jc {

template <typename T>
class A {
 public:
  int f() { return 1; }
};

template <>
class A<int> {
 public:
  int f() { return 2; }
  int g() { return 3; }
};

}  // namespace jc

int main() {
  jc::A<void> a;
  assert(a.f() == 1);
  jc::A<int> b;
  assert(b.f() == 2);
  assert(b.g() == 3);
}
```

## [偏特化（Partial Specialization）](https://en.cppreference.com/w/cpp/language/partial_specialization)

* 偏特化是为一类类型指定特定实现，是一种更通用的特化

```cpp
#include <cassert>

namespace jc {

template <typename T>
class A {
 public:
  int f() { return 1; }
};

template <typename T>
class A<T*> {
 public:
  int f() { return 2; }
  int g() { return 3; }
};

}  // namespace jc

int main() {
  jc::A<int> a;
  assert(a.f() == 1);
  jc::A<int*> b;
  assert(b.f() == 2);
  assert(b.g() == 3);
  jc::A<jc::A<int>*> c;
  assert(c.f() == 2);
  assert(c.g() == 3);
}
```

* 偏特化可以指定多个模板参数之间的关系，如果多个偏特化匹配程度相同，将产生二义性错误。如果模板声明是一个普通声明（没有在模板名称后添加尖括号），这个声明就是一个主模板（primary template），编写偏特化通常会有一个主模板和其他偏特化模板

```cpp
namespace jc {

template <typename T, typename U>
struct A;  // primary template

template <typename T>
struct A<T, T> {
  static constexpr int i = 1;
};

template <typename T>
struct A<T, int> {
  static constexpr int j = 2;
};

template <typename T, typename U>
struct A<T*, U*> {
  static constexpr int k = 3;
};

}  // namespace jc

using namespace jc;

static_assert(A<double, double>::i == 1);
static_assert(A<double, int>::j == 2);
static_assert(A<int*, double*>::k == 3);

int main() {
  //   A<int, int>{};    // 错误，匹配 A<T, T> 和 A<T, int>
  //   A<int*, int*>{};  // 错误，匹配 A<T, T> 和 A<T*, U*>
}
```

* 如果多个特化中，有一个匹配程度最高，则不会有二义性错误

```cpp
namespace jc {

template <typename T, typename U>
struct A;

template <typename T>
struct A<T, T> {
  static constexpr int i = 1;
};

template <typename T>
struct A<T, int> {
  static constexpr int j = 2;
};

template <typename T, typename U>
struct A<T*, U*> {
  static constexpr int k = 3;
};

template <typename T>
struct A<T*, T*> {
  static constexpr int k = 4;
};

}  // namespace jc

static_assert(jc::A<double, double>::i == 1);
static_assert(jc::A<double, int>::j == 2);
static_assert(jc::A<int*, double*>::k == 3);
static_assert(jc::A<double*, int*>::k == 3);
static_assert(jc::A<int*, int*>::k == 4);
static_assert(jc::A<double*, double*>::k == 4);

int main() {}
```

* 偏特化常用于元编程

```cpp
#include <tuple>
#include <type_traits>

namespace jc {

template <typename T, typename Tuple>
struct is_among;

template <typename T, template <typename...> class Tuple, typename... List>
struct is_among<T, Tuple<List...>>
    : std::disjunction<std::is_same<T, List>...> {};

template <typename T, typename Tuple>
inline constexpr bool is_among_v = is_among<T, Tuple>::value;

}  // namespace jc

static_assert(jc::is_among_v<int, std::tuple<char, int, double>>);
static_assert(!jc::is_among_v<float, std::tuple<char, int, double>>);

int main() {}
```

* 偏特化遍历 [std::tuple](https://en.cppreference.com/w/cpp/utility/tuple)

```cpp
#include <cstddef>
#include <iostream>
#include <tuple>

namespace jc {

template <std::size_t Index, std::size_t N, typename... List>
struct PrintImpl {
  static void impl(const std::tuple<List...>& t) {
    std::cout << std::get<Index>(t) << " ";
    PrintImpl<Index + 1, N, List...>::impl(t);
  }
};

template <std::size_t N, typename... List>
struct PrintImpl<N, N, List...> {
  static void impl(const std::tuple<List...>& t) {}
};

template <typename... List>
void Print(const std::tuple<List...>& t) {
  PrintImpl<0, sizeof...(List), List...>::impl(t);
}

}  // namespace jc

int main() {
  auto t = std::make_tuple(3.14, 42, "hello world");
  jc::Print(t);  // 3.14 42 hello world
}
```

* [成员模板](https://en.cppreference.com/w/cpp/language/member_template)也能特化或偏特化

```cpp
#include <cassert>
#include <string>

namespace jc {

struct A {
  template <typename T = std::string>
  T as() const {
    return s;
  }

  std::string s;
};

template <>
inline bool A::as<bool>() const {
  return s == "true";
}

}  // namespace jc

int main() {
  jc::A a{"hello"};
  assert(a.as() == "hello");
  assert(!a.as<bool>());
  jc::A b{"true"};
  assert(b.as<bool>());
}
```

* 成员函数模板不能为虚函数，因为虚函数表的大小是固定的，而成员函数模板的实例化个数要编译完成后才能确定

```cpp

namespace jc {

template <typename T>
class Dynamic {
 public:
  virtual ~Dynamic() {}  // OK，每个 Dynamic<T> 对应一个析构函数

  template <typename U>
  virtual void f(const U&) {}  // 错误，编译器不知道一个 Dynamic<T> 中 f() 个数
};

}  // namespace jc

int main() {}
```

## 模板的模板参数（Template Template Parameter）

* 如果模板参数的类型是类模板，则需要使用模板的模板参数。对于模板的模板参数，C++11 之前只能用 class 关键字修饰，C++11 及其之后可以用别名模板的名称来替代，C++17 可以用 typename 修饰

```cpp
#include <set>
#include <vector>

namespace jc {

template <typename T, template <typename...> class Container>
void f(const Container<T>&) {}

}  // namespace jc

int main() {
  jc::f(std::vector<int>{});
  jc::f(std::vector<double>{});
  jc::f(std::set<int>{});
}
```

* 实际上容器还有一个模板参数，即内存分配器 allocator

```cpp
#include <cassert>
#include <deque>
#include <string>

namespace jc {

template <typename T, template <typename Elem, typename = std::allocator<Elem>>
                      class Container = std::deque>
class Stack {
 public:
  using reference = T&;
  using const_reference = const T&;

  template <typename, template <typename, typename> class>
  friend class Stack;

  template <typename U,
            template <typename Elem2, typename = std::allocator<Elem2>>
            class Container2>
  Stack<T, Container>& operator=(const Stack<U, Container2>&);

  void push(const T&);

  void pop();

  reference top();

  const_reference top() const;

  std::size_t size() const;

  bool empty() const;

 private:
  Container<T> container_;
};

template <typename T, template <typename, typename> class Container>
template <typename U, template <typename, typename> class Container2>
inline Stack<T, Container>& Stack<T, Container>::operator=(
    const Stack<U, Container2>& rhs) {
  container_.assign(rhs.container_.begin(), rhs.container_.end());
  return *this;
}

template <typename T, template <typename, typename> class Container>
inline void Stack<T, Container>::push(const T& x) {
  container_.emplace_back(x);
}

template <typename T, template <typename, typename> class Container>
inline void Stack<T, Container>::pop() {
  assert(!empty());
  container_.pop_back();
}

template <typename T, template <typename, typename> class Container>
inline typename Stack<T, Container>::reference Stack<T, Container>::top() {
  assert(!empty());
  return container_.back();
}

template <typename T, template <typename, typename> class Container>
inline typename Stack<T, Container>::const_reference Stack<T, Container>::top()
    const {
  assert(!empty());
  return container_.back();
}

template <typename T, template <typename, typename> class Container>
inline std::size_t Stack<T, Container>::size() const {
  return container_.size();
}

template <typename T, template <typename, typename> class Container>
inline bool Stack<T, Container>::empty() const {
  return container_.empty();
}

}  // namespace jc

int main() {
  jc::Stack<std::string> s;
  s.push("hello");
  s.push("world");
  assert(s.size() == 2);
  assert(s.top() == "world");
  s.pop();
  assert(s.size() == 1);
  assert(s.top() == "hello");
  s.pop();
  assert(s.empty());
}
```
