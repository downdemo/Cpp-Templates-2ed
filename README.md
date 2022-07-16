* C++ 模板技术是泛型编程的核心，但囿于编译器技术限制，不得不带着缺陷诞生，语法晦涩，报错冗长，难以调试，应用层开发较少使用，相关技术书籍匮乏，因此掌握难度较大。模板相关的经典技术书籍主要有三本，分别是 2001 年出版的 [*Modern C++ Design*](https://book.douban.com/subject/1755195/)、2002 年出版的 [*C++ Templates*](https://book.douban.com/subject/1455780/)、2004 年出版的 [*C++ Template Metaprogramming*](https://book.douban.com/subject/1920800/)。三者基于的 C++ 标准都是 C++98，*Modern C++ Design* 涉及 [Andrei Alexandrescu](https://en.wikipedia.org/wiki/Andrei_Alexandrescu) 写书时配套的 [Loki](http://loki-lib.sourceforge.net/)，*C++ Template Metaprogramming* 涉及 [Boost](https://www.boost.org/)，二者以介绍元编程（模板技术的一种应用）为主，只有 *C++ Templates* 主要介绍 C++98 标准的模板技术。时过境迁，C++ 标准的更新逐步修复了一些语法缺陷，减少了使用者的心智负担，并引入了语法糖和工具，让编写模板越来越简单。2017 年 9 月 25 日，基于 C++17 标准，[*C++ Templates 2ed*](https://book.douban.com/subject/11939436/) 出版，填补了十多年间模板技术进化时相关书籍的空白，堪称最全面的模板教程，也是对 C++11/14/17 特性介绍最为全面的书籍之一。此为个人笔记，精简并覆盖了[原书](https://www.safaribooksonline.com/library/view/c-templates-the/9780134778808/)所有关键知识点（略过涉及代码较少的 [ch 10 模板术语](https://learning.oreilly.com/library/view/c-templates-the/9780134778808/ch10.xhtml#ch10)、[ch 17 未来的方向](https://learning.oreilly.com/library/view/c-templates-the/9780134778808/ch17.xhtml#ch17)、[ch 18 模板的多态威力](https://learning.oreilly.com/library/view/c-templates-the/9780134778808/ch18.xhtml#ch18) 三章），对[书中示例代码](http://www.tmplbook.com/code/code.html)使用 VS 2017 编译过并对错误代码全部进行了纠正（仅一处因语法复杂编译失败），此外个人会在相应处补充 C++20 相关特性，如 [concepts](https://en.cppreference.com/w/cpp/concepts)。或是学习源码时遇到模板黑魔法不得其解，或是见他人轻松把玩模板心生羡慕，或是希望无限逼近精通 C++ 的状态，C++ 爱好者对模板求知若渴却又望而生畏，分享该笔记，希望帮助更多 C++ 爱好者掌握模板的使用。

## part1：基础

1. [函数模板（Function Template）](content/Part1%20%E5%9F%BA%E7%A1%80/01%20%E5%87%BD%E6%95%B0%E6%A8%A1%E6%9D%BF.md)
2. [类模板（Class Template）](content/Part1%20%E5%9F%BA%E7%A1%80/02%20%E7%B1%BB%E6%A8%A1%E6%9D%BF.md)
3. [非类型模板参数（Nontype Template Parameter）](content/Part1%20%E5%9F%BA%E7%A1%80/03%20%E9%9D%9E%E7%B1%BB%E5%9E%8B%E6%A8%A1%E6%9D%BF%E5%8F%82%E6%95%B0.md)
4. [可变参数模板（Variadic Template）](content/Part1%20%E5%9F%BA%E7%A1%80/04%20%E5%8F%AF%E5%8F%98%E5%8F%82%E6%95%B0%E6%A8%A1%E6%9D%BF.md)
5. [Tricky Basic](content/Part1%20%E5%9F%BA%E7%A1%80/05%20Tricky%20Basic.md)
6. [移动语义与 enable_if](content/Part1%20%E5%9F%BA%E7%A1%80/06%20%E7%A7%BB%E5%8A%A8%E8%AF%AD%E4%B9%89%E4%B8%8Eenable_if.md)
7. [按值传递与按引用传递（By Value or by Reference?）](content/Part1%20%E5%9F%BA%E7%A1%80/07%20%E6%8C%89%E5%80%BC%E4%BC%A0%E9%80%92%E4%B8%8E%E6%8C%89%E5%BC%95%E7%94%A8%E4%BC%A0%E9%80%92.md)
8. [编译期编程（Compile-Time Programming）](content/Part1%20%E5%9F%BA%E7%A1%80/08%20%E7%BC%96%E8%AF%91%E6%9C%9F%E7%BC%96%E7%A8%8B.md)
9. [模板实战（Using Templates in Practice）](content/Part1%20%E5%9F%BA%E7%A1%80/09%20%E6%A8%A1%E6%9D%BF%E5%AE%9E%E6%88%98.md)
10. [泛型库（Generic Library）](content/Part1%20%E5%9F%BA%E7%A1%80/10%20%E6%B3%9B%E5%9E%8B%E5%BA%93.md)

## part2：深入模板

11. [深入模板基础（Fundamentals in Depth）](content/Part2%20%E6%B7%B1%E5%85%A5%E6%A8%A1%E6%9D%BF/11%20%E6%B7%B1%E5%85%A5%E6%A8%A1%E6%9D%BF%E5%9F%BA%E7%A1%80.md)
12. [模板中的名称（Names in Template）](content/Part2%20%E6%B7%B1%E5%85%A5%E6%A8%A1%E6%9D%BF/12%20%E6%A8%A1%E6%9D%BF%E4%B8%AD%E7%9A%84%E5%90%8D%E7%A7%B0.md)
13. [实例化（Instantiation）](content/Part2%20%E6%B7%B1%E5%85%A5%E6%A8%A1%E6%9D%BF/13%20%E5%AE%9E%E4%BE%8B%E5%8C%96.md)
14. [模板实参推断（Template Argument Deduction）](content/Part2%20%E6%B7%B1%E5%85%A5%E6%A8%A1%E6%9D%BF/14%20%E6%A8%A1%E6%9D%BF%E5%AE%9E%E5%8F%82%E6%8E%A8%E6%96%AD.md)
15. [特化与重载（Specialization and Overloading）](content/Part2%20%E6%B7%B1%E5%85%A5%E6%A8%A1%E6%9D%BF/15%20%E7%89%B9%E5%8C%96%E4%B8%8E%E9%87%8D%E8%BD%BD.md)

## part3：模板与设计

16. [Traits 的实现（Implementing Traits）](content/Part3%20%E6%A8%A1%E6%9D%BF%E4%B8%8E%E8%AE%BE%E8%AE%A1/16%20Traits%E7%9A%84%E5%AE%9E%E7%8E%B0.md)
17. [基于类型属性的重载（Overloading on Type Property）](content/Part3%20%E6%A8%A1%E6%9D%BF%E4%B8%8E%E8%AE%BE%E8%AE%A1/17%20%E5%9F%BA%E4%BA%8E%E7%B1%BB%E5%9E%8B%E5%B1%9E%E6%80%A7%E7%9A%84%E9%87%8D%E8%BD%BD.md)
18. [模板与继承（Template and Inheritance）](content/Part3%20%E6%A8%A1%E6%9D%BF%E4%B8%8E%E8%AE%BE%E8%AE%A1/18%20%E6%A8%A1%E6%9D%BF%E4%B8%8E%E7%BB%A7%E6%89%BF.md)
19. [桥接静态多态与动态多态（Bridging Static and Dynamic Polymorphism）](content/Part3%20%E6%A8%A1%E6%9D%BF%E4%B8%8E%E8%AE%BE%E8%AE%A1/19%20%E6%A1%A5%E6%8E%A5%E9%9D%99%E6%80%81%E5%A4%9A%E6%80%81%E4%B8%8E%E5%8A%A8%E6%80%81%E5%A4%9A%E6%80%81.md)
20. [元编程（Metaprogramming）](content/Part3%20%E6%A8%A1%E6%9D%BF%E4%B8%8E%E8%AE%BE%E8%AE%A1/20%20%E5%85%83%E7%BC%96%E7%A8%8B.md)
21. [Typelist](content/Part3%20%E6%A8%A1%E6%9D%BF%E4%B8%8E%E8%AE%BE%E8%AE%A1/21%20Typelist.md)
22. [Tuple](content/Part3%20%E6%A8%A1%E6%9D%BF%E4%B8%8E%E8%AE%BE%E8%AE%A1/22%20Tuple.md)
23. [标签联合（Discriminated Union）](content/Part3%20%E6%A8%A1%E6%9D%BF%E4%B8%8E%E8%AE%BE%E8%AE%A1/23%20%E6%A0%87%E7%AD%BE%E8%81%94%E5%90%88.md)
24. [表达式模板（Expression Template）](content/Part3%20%E6%A8%A1%E6%9D%BF%E4%B8%8E%E8%AE%BE%E8%AE%A1/24%20%E8%A1%A8%E8%BE%BE%E5%BC%8F%E6%A8%A1%E6%9D%BF.md)
25. [模板的调试（Debugging Template）](content/Part3%20%E6%A8%A1%E6%9D%BF%E4%B8%8E%E8%AE%BE%E8%AE%A1/25%20%E6%A8%A1%E6%9D%BF%E7%9A%84%E8%B0%83%E8%AF%95.md)
