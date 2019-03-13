* C++ [模板](https://en.cppreference.com/w/cpp/language/templates)技术是泛型编程的核心，但囿于编译器技术限制，不得不带着缺陷诞生，语法晦涩，报错冗长，难以调试，应用层开发较少使用，相关技术书籍匮乏，因此掌握难度较大。模板相关的经典技术书籍主要有三本，分别是 2001 年出版的 [*Modern C++ Design*](https://book.douban.com/subject/1755195/)、2002 年出版的 [*C++ Templates*](https://book.douban.com/subject/1455780/)、2004 年出版的 [*C++ Template Metaprogramming*](https://book.douban.com/subject/1920800/)。三者基于的 C++ 标准都是 C++98，*Modern C++ Design* 涉及 [Andrei Alexandrescu](https://en.wikipedia.org/wiki/Andrei_Alexandrescu) 写书时配套的 [Loki](http://loki-lib.sourceforge.net/)，*C++ Template Metaprogramming* 涉及 [Boost](https://www.boost.org/)，二者以介绍元编程（模板技术的一种应用）为主，只有 *C++ Templates* 主要介绍 C++98 标准的模板技术。时过境迁，C++ 标准的更新逐步修复了一些语法缺陷，减少了使用者的心智负担，并引入了语法糖和工具，让编写模板越来越简单。2017 年 9 月 25 日，基于 C++17 标准，[*C++ Templates 2ed*](https://book.douban.com/subject/11939436/) 出版，填补了十多年间模板技术进化时相关书籍的空白，堪称最全面的模板教程，也是对 C++11/14/17 特性介绍最为全面的书籍之一。个人完整学习[原书](https://www.safaribooksonline.com/library/view/c-templates-the/9780134778808/)后，梳理精简章节脉络，补充 C++20 相关特性，如 [concepts](https://en.cppreference.com/w/cpp/concepts)、支持模板参数的 [lambda](https://en.cppreference.com/w/cpp/language/lambda) 等，运行验证所有代码结果，最终记录至此。

## Contents

1. [Function template](01_function_template.html)
2. [Class template](02_class_template.html)
3. [Non-type template parameter](03_non_type_template_parameter.html)
4. [Variadic template](04_variadic_template.html)
5. [Move semantics and perfect forwarding](05_move_semantics_and_perfect_forwarding.html)
6. [Name lookup](06_name_lookup.html)
7. [Instantiation](07_instantiation.html)
8. [Template argument deduction](08_template_argument_deduction.html)
9. [Specialization and overloading](09_specialization_and_overloading.html)
10. [Traits](10_traits.html)
11. [Inheritance](11_inheritance.html)
12. [Type erasure](12_type_erasure.html)
13. [Metaprogramming](13_metaprogramming.html)
14. [Expression template](14_expression_template.html)
15. [Debugging](15_debugging.html)
