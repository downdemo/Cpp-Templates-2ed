#include <cassert>
#include <exception>
#include <new>  // for std::launder()
#include <sstream>
#include <string>
#include <type_traits>
#include <utility>

#include "typelist.hpp"

namespace jc {

class computed_result_type;

template <typename Visitor, typename T>
using visit_element_result =
    decltype(std::declval<Visitor>()(std::declval<T>()));

template <typename R, typename Visitor, typename... Types>
struct visit_result {
  using type = R;
};

template <typename Visitor, typename... Types>
struct visit_result<computed_result_type, Visitor, Types...> {
  using type = std::common_type_t<visit_element_result<Visitor, Types>...>;
};

template <typename R, typename Visitor, typename... Types>
using visit_result_t = typename visit_result<R, Visitor, Types...>::type;

struct empty_variant : std::exception {};

template <typename R, typename V, typename Visitor, typename Head,
          typename... Tail>
R variant_visit_impl(V&& variant, Visitor&& vis, typelist<Head, Tail...>) {
  if (variant.template is<Head>()) {
    return static_cast<R>(std::forward<Visitor>(vis)(
        std::forward<V>(variant).template get<Head>()));
  } else if constexpr (sizeof...(Tail) > 0) {
    return variant_visit_impl<R>(std::forward<V>(variant),
                                 std::forward<Visitor>(vis),
                                 typelist<Tail...>{});
  } else {
    throw empty_variant();
  }
}

template <typename... Types>
class variant_storage {
 public:
  unsigned char get_discriminator() const { return discriminator_; }

  void set_discriminator(unsigned char d) { discriminator_ = d; }

  void* get_raw_buffer() { return buffer_; }

  const void* get_raw_buffer() const { return buffer_; }

  template <typename T>
  T* get_buffer_as() {
    return std::launder(reinterpret_cast<T*>(buffer_));
  }

  template <typename T>
  const T* get_buffer_as() const {
    return std::launder(reinterpret_cast<const T*>(buffer_));
  }

 private:
  using largest_t = largest_type_t<typelist<Types...>>;
  alignas(Types...) unsigned char buffer_[sizeof(largest_t)];
  unsigned char discriminator_ = 0;
};

template <typename... Types>
class variant;

template <typename T, typename... Types>
class variant_choice {
  using Derived = variant<Types...>;

  Derived& get_derived() { return *static_cast<Derived*>(this); }

  const Derived& get_derived() const {
    return *static_cast<const Derived*>(this);
  }

 protected:
  static constexpr unsigned Discriminator =
      find_index_of_t<typelist<Types...>, T>::value + 1;

 public:
  variant_choice() = default;

  variant_choice(const T& value) {
    new (get_derived().get_raw_buffer()) T(value);  // CRTP
    get_derived().set_discriminator(Discriminator);
  }

  variant_choice(T&& value) {
    new (get_derived().get_raw_buffer()) T(std::move(value));
    get_derived().set_discriminator(Discriminator);
  }

  bool destroy() {
    if (get_derived().get_discriminator() == Discriminator) {
      get_derived().template get_buffer_as<T>()->~T();
      return true;
    }
    return false;
  }

  Derived& operator=(const T& value) {
    if (get_derived().get_discriminator() == Discriminator) {
      *get_derived().template get_buffer_as<T>() = value;
    } else {
      get_derived().destroy();
      new (get_derived().get_raw_buffer()) T(value);
      get_derived().set_discriminator(Discriminator);
    }
    return get_derived();
  }

  Derived& operator=(T&& value) {
    if (get_derived().get_discriminator() == Discriminator) {
      *get_derived().template get_buffer_as<T>() = std::move(value);
    } else {
      get_derived().destroy();
      new (get_derived().get_raw_buffer()) T(std::move(value));
      get_derived().set_discriminator(Discriminator);
    }
    return get_derived();
  }
};

/*
 * class variant<int, double, std::string>
 *     : private variant_storage<int, double, std::string>,
 *       private variant_choice<int, int, double, std::string>,
 *       private variant_choice<double, int, double, std::string>,
 *       private variant_choice<std::string, int, double, std::string> {};
 *
 * variant_choice<int, int, double, std::string>::discriminator_ == 1;
 * variant_choice<double, int, double, std::string>::discriminator_ == 2;
 * variant_choice<std::string, int, double, std::string>::discriminator_ == 3;
 */
template <typename... Types>
class variant : private variant_storage<Types...>,
                private variant_choice<Types, Types...>... {
  template <typename T, typename... OtherTypes>
  friend class variant_choice;  // enable CRTP

 public:
  /*
   * ctor of variant<int, double, string>:
   * variant(const int&);
   * variant(int&&);
   * variant(const double&);
   * variant(double&&);
   * variant(const string&);
   * variant(string&&);
   */
  using variant_choice<Types, Types...>::variant_choice...;
  using variant_choice<Types, Types...>::operator=...;

  variant() { *this = front_t<typelist<Types...>>(); }

  variant(const variant& rhs) {
    if (!rhs.empty()) {
      rhs.visit([&](const auto& value) { *this = value; });
    }
  }

  variant(variant&& rhs) {
    if (!rhs.empty()) {
      std::move(rhs).visit([&](auto&& value) { *this = std::move(value); });
    }
  }

  template <typename... SourceTypes>
  variant(const variant<SourceTypes...>& rhs) {
    if (!rhs.empty()) {
      rhs.visit([&](const auto& value) { *this = value; });
    }
  }

  template <typename... SourceTypes>
  variant(variant<SourceTypes...>&& rhs) {
    if (!rhs.empty()) {
      std::move(rhs).visit([&](auto&& value) { *this = std::move(value); });
    }
  }

  variant& operator=(const variant& rhs) {
    if (!rhs.empty()) {
      rhs.visit([&](const auto& value) { *this = value; });
    } else {
      destroy();
    }
    return *this;
  }

  variant& operator=(variant&& rhs) {
    if (!rhs.empty()) {
      std::move(rhs).visit([&](auto&& value) { *this = std::move(value); });
    } else {
      destroy();
    }
    return *this;
  }

  template <typename... SourceTypes>
  variant& operator=(const variant<SourceTypes...>& rhs) {
    if (!rhs.empty()) {
      rhs.visit([&](const auto& value) { *this = value; });
    } else {
      destroy();
    }
    return *this;
  }

  template <typename... SourceTypes>
  variant& operator=(variant<SourceTypes...>&& rhs) {
    if (!rhs.empty()) {
      std::move(rhs).visit([&](auto&& value) { *this = std::move(value); });
    } else {
      destroy();
    }
    return *this;
  }

  bool empty() const { return this->get_discriminator() == 0; }

  ~variant() { destroy(); }

  void destroy() {
    (variant_choice<Types, Types...>::destroy(), ...);
    this->set_discriminator(0);
  }

  template <typename T>
  bool is() const {
    return this->get_discriminator() ==
           variant_choice<T, Types...>::Discriminator;
  }

  template <typename T>
  T& get() & {
    if (empty()) {
      throw empty_variant();
    }
    assert(is<T>());
    return *this->template get_buffer_as<T>();
  }

  template <typename T>
  const T& get() const& {
    if (empty()) {
      throw empty_variant();
    }
    assert(is<T>());
    return *this->template get_buffer_as<T>();
  }

  template <typename T>
  T&& get() && {
    if (empty()) {
      throw empty_variant();
    }
    assert(is<T>());
    return std::move(*this->template get_buffer_as<T>());
  }

  template <typename R = computed_result_type, typename Visitor>
  visit_result_t<R, Visitor, Types&...> visit(Visitor&& vis) & {
    using Result = visit_result_t<R, Visitor, Types&...>;
    return variant_visit_impl<Result>(*this, std::forward<Visitor>(vis),
                                      typelist<Types...>{});
  }

  template <typename R = computed_result_type, typename Visitor>
  visit_result_t<R, Visitor, const Types&...> visit(Visitor&& vis) const& {
    using Result = visit_result_t<R, Visitor, const Types&...>;
    return variant_visit_impl<Result>(*this, std::forward<Visitor>(vis),
                                      typelist<Types...>{});
  }

  template <typename R = computed_result_type, typename Visitor>
  visit_result_t<R, Visitor, Types&&...> visit(Visitor&& vis) && {
    using Result = visit_result_t<R, Visitor, Types&&...>;
    return variant_visit_impl<Result>(
        std::move(*this), std::forward<Visitor>(vis), typelist<Types...>{});
  }
};

}  // namespace jc

namespace jc::test {

struct copied_noncopyable : std::exception {};

struct noncopyable {
  noncopyable() = default;

  noncopyable(const noncopyable&) { throw copied_noncopyable(); }

  noncopyable(noncopyable&&) = default;

  noncopyable& operator=(const noncopyable&) { throw copied_noncopyable(); }

  noncopyable& operator=(noncopyable&&) = default;
};

template <typename V, typename Head, typename... Tail>
void print_impl(std::ostream& os, const V& v) {
  if (v.template is<Head>()) {
    os << v.template get<Head>();
  } else if constexpr (sizeof...(Tail) > 0) {
    print_impl<V, Tail...>(os, v);
  }
}

template <typename... Types>
void print(std::ostream& os, const variant<Types...>& v) {
  print_impl<variant<Types...>, Types...>(os, v);
}

}  // namespace jc::test

void test_variant() {
  jc::variant<int, double, std::string> v{42};
  assert(!v.empty());
  assert(v.is<int>());
  assert(v.get<int>() == 42);
  v = 3.14;
  assert(v.is<double>());
  assert(v.get<double>() == 3.14);
  v = "hello";
  assert(v.is<std::string>());
  assert(v.get<std::string>() == "hello");

  std::stringstream os;
  v.visit([&os](const auto& value) { os << value; });
  assert(os.str() == "hello");

  os.str("");
  jc::test::print(os, v);
  assert(os.str() == "hello");

  jc::variant<int, double, std::string> v2;
  assert(!v2.empty());
  assert(v2.is<int>());
  v2 = std::move(v);
  assert(v.is<std::string>());
  assert(v.get<std::string>().empty());
  assert(v2.is<std::string>());
  assert(v2.get<std::string>() == "hello");
  v2.destroy();
  assert(v2.empty());
}

void test_noncopyable() {
  jc::variant<int, jc::test::noncopyable> v(42);
  try {
    jc::test::noncopyable nc;
    v = nc;
  } catch (jc::test::copied_noncopyable) {
    assert(!v.is<int>() && !v.is<jc::test::noncopyable>());
  }
}

int main() {
  test_variant();
  test_noncopyable();
}
