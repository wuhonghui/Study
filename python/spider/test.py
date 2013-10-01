#coding:utf-8
import re

pattern = re.compile(r'hello')
str = pattern.match('hello world')
if str:
    print str.group()
else:
    print "not matched"

pattern = re.compile(r'hello', re.I)
str = pattern.match('hEllo')
if str:
    print str.group()

m = re.match(r'(\w+)(\w+)(?P<sign>.*)', 'hello world')
print "m.string :",  m.string
print "m.re:"    ,    m.re
print "m.pos:",      m.pos
print "m.endpos:" ,  m.endpos
print "m.lastindex:", m.lastindex
print "m.lastgroup:", m.lastgroup

print "m.group:", m.group()
print "m.group(1,2):", m.group(1,2)
print "m.groups()", m.groups()
print "m.groupdict():", m.groupdict()
print "m.start(2):", m.start(2)
print "m.end(2):", m.end(2)
print "m.span(2):", m.span(2)
print r"m.expand(r'\g<2> \g<1> \g<3>'):",m.expand(r'\2 \1 \3')

m = re.compile(r'\d')
print m.findall('five5six6seven7')
print m.split('five5six6seven7')

m = re.compile(r'(\w+) (\w+)')
for i in m.finditer("fff sss ff fs"):
    print i.group()

print m.subn(r'\2 f', "aaa vvv ccc lll iii")
print m.subn(r'\2 \1', "fff sss fff ppp")
