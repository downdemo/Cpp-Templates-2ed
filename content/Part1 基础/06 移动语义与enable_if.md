## 完美转发（Perfect Forwarding）

* 要用一个函数将实参的如下基本属性转发给另一个函数
  * 可修改的对象转发后应该仍可以被修改
  * 常量对象应该作为只读对象转发
  * 可移动的对象应该作为可移动对象转发
* 如果不使用模板实现这些功能，必须编写全部的三种情况

```cpp
void f(int&) { std::cout << 1; }
void f(const int&) { std::cout << 2; }
void f(int&&) { std::cout << 3; }

// 用多个重载转发给对应版本比较繁琐
void g(int& x)
{
  f(x);
}

void g(const int& x)
{
  f(x);
}

void g(int&& x)
{
  f(std::move(x));
}

// 同样可以用一个模板来替代上述功能
template<typename T>
void h(T&& x)
{
  f(std::forward<T>(x)); // 注意std::forward的模板参数是T
}

int main()
{
  int a = 1;
  const int b = 1;

  g(a); h(a); // 11
  g(b); h(b); // 22
  g(std::move(a)); h(std::move(a)); // 33
  g(1); h(1); // 33
}
```

* 结合可变参数模板，完美转发可以转发任意数量的实参

```cpp
template<typename... Ts>
void f(Ts&&... args)
{
  g(std::forward<Ts>(args)...); // 把任意数量的实参转发给g
}
```

* lambda中也可以使用完美转发

```cpp
auto f = [](auto&& x) { return g(std::forward<decltype(x)>(x)); };

// 转发任意数量实参
auto f = [](auto&&... args) {
  return g(std::forward<decltype(args)>(args)...);
};
```

* 如果想在转发前修改要转发的值，可以用auto&&存储结果，修改后再转发

```cpp
template<typename T>
void f(T x)
{
  auto&& res = doSomething(x);
  doSomethingElse(res);
  set(std::forward<decltype(res)>(res));
}
```

## 特殊成员函数模板

* 模板也能用于特殊的成员函数，如构造函数，但这可能导致意外的行为
* 下面是未使用模板的代码

```cpp
class Person {
 public:
  explicit Person(const std::string& n) : name(n) {} // 拷贝初始化函数
  explicit Person(std::string&& n) : name(std::move(n)) {} // 移动初始化函数
  Person(const Person& p) : name(p.name) {} // 拷贝构造函数
  Person(Person&& p) : name(std::move(p.name)) {} // 移动构造函数
 private:
  std::string name;
};

int main()
{
  std::string s = "sname";
  Person p1(s); // 调用拷贝初始化函数
  Person p2("tmp"); // 调用移动初始化函数
  Person p3(p1); // 调用拷贝构造函数
  Person p4(std::move(p1)); // 调用移动构造函数
}
```

* 现在用模板作为构造函数，完美转发实参给成员name，替代原有的两个构造函数

```cpp
class Person {
 public:
  template<typename STR> // 完美转发构造函数
  explicit Person(STR&& n) : name(std::forward<STR>(n)) {}
  Person(const Person& p) : name(p.name) {} // 拷贝构造函数
  Person(Person&& p) : name(std::move(p.name)) {} // 移动构造函数
 private:
  std::string name;
};
```

* 如下构造函数仍能正常工作

```cpp
std::string s = "sname";
Person p1(s); // 调用完美转发构造函数
Person p2("tmp"); // 调用完美转发构造函数
Person p4(std::move(p1)); // 调用移动构造函数
```

* 但调用拷贝构造函数时将出错

```cpp
Person p3(p1); // 错误
```

* 但拷贝一个const对象却不会出错

```cpp
const Person p2c("ctmp"); // 调用完美转发构造函数
Person p3c(p2c); // 调用拷贝构造函数
```

* 原因是，成员模板比拷贝构造函数更匹配non-const左值，于是调用完美转换构造函数，导致使用Person类型初始化std::string的错误

```cpp
// 对于Person p1的匹配
template<typename STR>
Person(STR&&)
// 优于
Person(const Person&)
```

* 一种解决方法是添加一个接受non-const实参的拷贝构造函数

```cpp
Person(Person&);
```

* 但这只是一个局限的解决方案，因为对于派生类对象，成员模板仍然是更好的匹配。最佳方案是在传递实参为Person或一个能转换为Person的表达式时，禁用成员模板

## 使用enable_if禁用成员模板

* C++11提供的[std::enable_if](https://en.cppreference.com/w/cpp/types/enable_if)允许在某个编译期条件下忽略函数模板

```cpp
#include <type_traits>

template<typename T>
typename std::enable_if<(sizeof(T) > 4)>::type
f() {}
```

* 如果(sizeof(T) > 4)为false，模板的定义将被忽略，若为true则模板实例扩展为

```cpp
void f() {}
```

* [std::enable_if](https://en.cppreference.com/w/cpp/types/enable_if)是一个[type traits](https://en.cppreference.com/w/cpp/header/type_traits)，编译期表达式作为首个模板实参传递。若表达式为false，则enable_if::type未定义，由于SFINAE，这个带有enable_if的函数模板将被忽略。若表达式为true，enable_if::type产生一个类型，若有第二个实参，则类型为第二个实参类型，否则为void

```cpp
template<typename T>
std::enable_if<(sizeof(T) > 4), T>::type f()
{
  return T();
}
```

* C++14中为所有[type traits](https://en.cppreference.com/w/cpp/header/type_traits)提供了一个别名模板，以省略后缀::type，enable_if::type在C++14中可以简写为enable_if_t

```cpp
template<typename T>
std::enable_if_t<(sizeof(T) > 4)>
f() {}
```

* enable_if表达式出现在声明中很影响可读性，因此std::enable_if常用作一个额外的模板实参的默认值

```cpp
template<typename T, typename = std::enable_if_t<(sizeof(T) > 4)>>
void f() {}
```

* 如果`sizeof(T) > 4`，扩展为

```cpp
template<typename T, typename = void>
void f() {}
```

* 利用别名模板还可以进一步简化代码

```cpp
template<typename T>
using EnableIfSizeGreater4 = std::enable_if_t<(sizeof(T) > 4)>;

template<typename T, typename = EnableIfSizeGreater4<T>>
void f() {}
```

## 使用enable_if解决完美转发构造函数的优先匹配问题

* 现在来解决之前的构造函数模板的问题，当实参STR有正确的类型（[std::string](https://en.cppreference.com/w/cpp/string/basic_string)或可以转换为[std::string](https://en.cppreference.com/w/cpp/string/basic_string)的类型）时禁用完美转发构造函数。为此，需要使用另一个[type traits](https://en.cppreference.com/w/cpp/header/type_traits)，[std::is_convertible](https://en.cppreference.com/w/cpp/types/is_convertible)

```cpp
template<typename STR,
  typename = std::enable_if_t<std::is_convertible<STR, std::string>::value>>
explicit Person(STR&& n) : name(std::forward<STR>(n)) {}
```

* 如果STR不能转换为[std::string](https://en.cppreference.com/w/cpp/string/basic_string)，则模板将被忽略。如果STR可转换为[std::string](https://en.cppreference.com/w/cpp/string/basic_string)，则整个声明扩展为

```cpp
template<typename STR, typename = void>
explicit Person(STR&& n) : name(std::forward<STR>(n)) {}
```

* 同样也可以使用别名模板定义自己的名称，此外C++17中可以用别名模板std::is_convertible_v替代std::is_convertible::value，最终代码如下

```cpp
#include <iostream>
#include <string>
#include <type_traits>

template<typename T>
using EnableIfString = std::enable_if_t<std::is_convertible_v<T, std::string>>;

class Person {
 public:
  template<typename STR, typename = EnableIfString<STR>>
  explicit Person(STR&& n) : name(std::forward<STR>(n)) {}
  Person(const Person& p) : name(p.name) {}
  Person(Person&& p) : name(std::move(p.name)) {}
 private:
  std::string name;
};

int main()
{
  std::string s = "sname";
  Person p1(s); // 调用完美转发构造函数
  Person p2("tmp"); // 调用完美转发构造函数
  Person p3(p1); // 调用拷贝构造函数
  Person p4(std::move(p1)); // 调用移动构造函数
}
```

* 也可以用[std::is_constructible](https://en.cppreference.com/w/cpp/types/is_constructible)替代[std::is_convertible](https://en.cppreference.com/w/cpp/types/is_convertible)，但要注意[std::is_convertible](https://en.cppreference.com/w/cpp/types/is_convertible)判断类型可以隐式转换，而[std::is_constructible](https://en.cppreference.com/w/cpp/types/is_constructible)判断的是显式转换，实参顺序相反

```cpp
template<typename T>
using EnableIfString = std::enable_if_t<std::is_constructible_v<std::string, T>>;
```

## 模板与预定义的特殊成员函数

* 通常不能用enable_if禁用预定义的拷贝/移动构造函数和赋值运算符，因为成员函数模板不会被当作特殊的成员函数，比如当需要拷贝构造函数时，成员模板将被忽略

```cpp
class C {
 public:
  template<typename T>
  C(const T&) {}
};

C x;
C y{x}; // 仍然使用预定义合成的拷贝构造函数，上面的模板被忽略
```

* 同样不能删除预定义的拷贝构造函数，但有一个tricky方案，可以为cv限定符修饰的实参声明一个拷贝构造函数，这样会禁止合成拷贝构造函数。再将自定义的拷贝构造函数声明为=delete，这样模板就会成为唯一选择

```cpp
class C {
 public:
  C(const volatile C&) = delete; // 显式声明将阻止默认合成拷贝构造函数
  template<typename T>
  C(const T&) {}
};

C x;
C y{x}; // 使用模板
```

* 此时就可以用enable_if添加限制，比如模板参数类型为整型时禁用拷贝构造

```cpp
template<typename T>
class C {
 public:
  C(const volatile C&) = delete;
  template<typename U, typename = std::enable_if_t<!std::is_integral_v<U>>>
  C(const C<U>&) {}
};
```

## 使用concepts替代enable_if以简化表达式

* concepts经过长时间讨论仍然还未成为C++17标准的一部分，但一些编译器提供了实验性的支持，对于之前的

```cpp
template<typename STR,
  typename = std::enable_if_t<std::is_convertible_v<STR, std::string>>>
explicit Person(STR&& n) : name(std::forward<STR>(n)) {}
```

* 使用concepts可以写为

```cpp
template<typename STR>
requires std::is_convertible_v<STR, std::string>
explicit Person(STR&& n) : name(std::forward<STR>(n)) {}
```

* 同样为了方便，类似于别名模板，可以把requirement制定为一个通用的concept

```cpp
template<typename T>
concept ConvertibleToString = std::is_convertible_v<T,std::string>;
```

* 再将这个concept作为requirement使用

```cpp
template<typename STR>
requires ConvertibleToString<STR>
explicit Person(STR&& n) : name(std::forward<STR>(n)) {}
```

* 也可以直接写为

```cpp
template<ConvertibleToString STR>
explicit Person(STR&& n) : name(std::forward<STR>(n)) {}
```
