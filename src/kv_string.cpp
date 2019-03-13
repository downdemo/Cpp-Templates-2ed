#include <cassert>
#include <iostream>
#include <sstream>
#include <string>
#include <type_traits>

namespace jc {

template <
    typename K, typename V,
    std::enable_if_t<std::is_same_v<std::decay_t<V>, bool>, void*> = nullptr>
void append(std::ostringstream& os, const K& k, const V& v) {
  os << R"(")" << k << R"(":)" << std::boolalpha << v;
}

template <typename K, typename V,
          std::enable_if_t<!std::is_same_v<std::decay_t<V>, bool> &&
                               std::is_arithmetic_v<std::decay_t<V>>,
                           void*> = nullptr>
void append(std::ostringstream& os, const K& k, const V& v) {
  os << R"(")" << k << R"(":)" << v;
}

template <
    typename K, typename V,
    std::enable_if_t<std::is_constructible_v<std::string, std::decay_t<V>>,
                     void*> = nullptr>
void append(std::ostringstream& os, const K& k, const V& v) {
  os << R"(")" << k << R"(":")" << v << R"(")";
}

void kv_string_impl(std::ostringstream& os) {}

template <typename V, typename... Args>
std::void_t<decltype(std::cout << std::declval<std::decay_t<V>>())>
kv_string_impl(std::ostringstream& os, const std::string& k, const V& v,
               const Args&... args) {
  append(os, k, v);
  if constexpr (sizeof...(args) >= 2) {
    os << ",";
  }
  kv_string_impl(os, args...);
}

template <typename... Args>
std::string kv_string(const std::string& field, const Args&... args) {
  std::ostringstream os;
  os << field << ":{";
  kv_string_impl(os, args...);
  os << "}";
  return os.str();
}

}  // namespace jc

int main() {
  std::string a{R"(data:{})"};
  std::string b{R"(data:{"name":"jc","ID":1})"};
  std::string c{R"(data:{"name":"jc","ID":1,"active":true})"};
  assert(a == jc::kv_string("data"));
  assert(b == jc::kv_string("data", "name", "jc", "ID", 1));
  assert(c == jc::kv_string("data", "name", "jc", "ID", 1, "active", true));
}
