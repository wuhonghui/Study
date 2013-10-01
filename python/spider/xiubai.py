#coding:utf-8
from __future__ import print_function
import urllib2
import urllib
import re

# 获取一个页面
def get_page(category, n):
    print('正在努力加载中...')
    realurl = 'http://www.qiushibaike.com/' + category + '/page/' + str(n)
    headers = {'User-Agent' : 'Mozilla/5.0 (Windows NT 6.1) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/29.0.1547.76 Safari/537.36',
               'Host' : 'www.qiushibaike.com'}
    req = urllib2.Request(url=realurl, headers=headers)
    response = urllib2.urlopen(req)
    page = response.read()
    unicodePage = page.decode("utf-8")

    # 正则表达式提取
    alldiv = re.compile(r'<a href="/article/(\d*)" [^>]*>(.*?)\s*</a>.*?<div.*?class="content".*?title="(.*?)">(.*?)</div>', re.S)
    divs = {}
    print()
    print('--------------------------------------------------------------------------------------------------------------------------')
    for div in re.finditer(alldiv, unicodePage):
        id = div.group(1)
        brief = div.group(2) 
        date = div.group(3)
        content = div.group(4).replace("\n", "").replace("<br/>", "\n")
        print(id, brief, date)
        print(content, "\n")

# 浏览页面，接收用户输入，并返回结果
def browse_page(category):
    page = 1
    while True:
        if page >= 1:
            get_page(category, page)

        hint = '正在浏览第' + str(page) + '页'
        key = raw_input(hint + '，下一页(n), 上一页(p), 返回分类(b): ')
        if key == 'n':
            page = page + 1
        elif key == 'p':
            page = page - 1
        elif key == 'b':
            break
        else:
            try:
                value = int(key)
                page = value
            except ValueError, e:
                print('页数错误, 请重新输入!')
                page = 0
   
# 打印主菜单
def print_menu():
    menu=u"""
+----------------------------------------------+
|                  糗事百科                    |
|   分类：                                     |
|        1. 最新发布                           |
|        2. 8小时精华                          |
|        3. 24小时精华                         |
|        4. 一周精华                           |
|        5. 30天精华                           |
|        0. 退出浏览                           |
|                                              |
+-----------------------------------------------
"""
    print(menu)

# main 函数
if __name__ == "__main__":
    while True:
        print_menu()
        choose = raw_input('请选择分类: ')
        categories = ['', 'late', '8hr', 'hot', 'week', 'month']
        if choose == '0':
            print('谢谢使用！')
            break;
        browse_page(categories[int(choose)])
        
