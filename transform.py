import re
import os
from urllib.parse import quote


def default_head(title: str = "", description: str = "") -> str:
    return f"""---
layout: default
title: {title}
description: {description}
---

"""


def deal_str_github(s: str) -> str:
    return (
        deal_str_cpprefer(s)
        .replace("https://github.com/downdemo/Cpp-Templates-2ed/blob/master/", "")
        .replace("content/", "")
        .replace(".md)", ".html)")
    )


def deal_str_cpprefer(s: str) -> str:
    return s.replace("https://en.cppreference.com", "https://zh.cppreference.com")


if not os.path.exists("docs"):
    os.mkdir("docs")

with open("README.md", "r", encoding="utf-8") as i:
    with open("docs/index.md", "w", encoding="utf-8") as o:
        for line in i.readlines():
            o.write(deal_str_github(line))

for dirname in os.listdir("content"):
    dir_path_o = f"docs/{dirname}"
    dir_path_i = f"content/{dirname}"
    desp_prefix: str = re.findall("Part\d (.*)", dirname)[0]
    if not os.path.exists(dir_path_o):
        os.mkdir(dir_path_o)
    file_list = os.listdir(f"content/{dirname}")
    if file_list:
        file_now = file_list[0]  # 当前小节
        file_list.remove(file_now)
    else:
        file_list = None
    file_pre = None  # 上一小节
    file_next = None  # 下一小节
    while file_now is not None:
        if file_list:
            file_next = file_list[0]
            file_list.remove(file_next)
        else:
            file_next = None
        with open(dir_path_i + f"/{file_now}", "r", encoding="utf-8") as i:
            with open(dir_path_o + f"/{file_now}", "w", encoding="utf-8") as o:
                ch_name = re.findall("\d{2} (.*).md", file_now)[0]
                o.write(default_head(file_now.replace(".md", ""), f"{desp_prefix}-{ch_name}"))
                if file_pre is not None:
                    o.write(
                        f"**[Back: {file_pre.replace('.md','')}]({quote(file_pre.replace('.md','.html'))})**    "
                    )  # 添加上一章链接
                else:
                    o.write(f"**[Home](../../index.html)**")  # 添加目录链接

                o.write("\n")
                for line in i.readlines():  # 复制正文
                    o.write(deal_str_cpprefer(line))
                o.write("\n")

                if file_next is not None:
                    o.write(
                        f"**[Next: {file_next.replace('.md','')}]({quote(file_next.replace('.md','.html'))})**"
                    )  # 添加下一章链接
                else:
                    o.write(f"**[Home](../../index.html)**")  # 添加目录链接
        file_pre = file_now
        file_now = file_next
