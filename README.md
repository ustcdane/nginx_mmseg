 需要先下载 pcre软件包,假设下载了pcre-8.37 并放在user目录下。<br>
  **下载源码**<br>
 git clone  https://github.com/ustcdane/nginx_mmseg.git  /user/nginx_mmseg<br>
**configure**
进入nginx目录，cd nginx-1.8.0，运行configure生成Makefile文件，Makefile文件在objs目录下：<br>
./configure  --prefix=/user/nginx-1.8.0/bin --add-module=/user/nginx_mmseg/  --with-pcre=/user/pcre-8.37<br>
解释下各个意义：<br>
—prefix=nginx的运行目录，还有—add-module=自己的module目录，—with-pcre=pcre目录。<br>
**修改Makefile**<br>
因为 nginx_mmseg 是 C++ 源码，所以作为 nginx 模块编译的时候需要 修改 obj/Makefile,<br>
修改后的Makefile一定要备份，因为重新configure或者make clean都会把这个makefile给删掉，这样心血就白费了。<br>
1.	首先把CC=cc改为CC=gcc，然后加入CXX=g++，并把LINK指定为CXX</br>
CXXFLAGS=$(CFLAGS) -std=c++11 -g<br>
2.	在ALL_INCS中添加你用到的.h文件的路径<br>
3.     在ADDON_DEPS中添加你的模块需要依赖的.h文件（不需要扯到外文输入法的.h）<br>
4.	在objs目录下添加你的module下的.c或者.cpp文件生成的.o文件 <br>
5.    在LINK后面也加上这些.o</br>
6.    加上这些.o文件所需要的编译选项，cpp用CXX编译，c用CC编译，然后再make就完事了。把objs目录下的nginx可执行文件拷贝到运行目录下/user/nginx-1.8.0/bin，然后修改bin/conf的nginx.conf文件，添加我们需要的words_path和charFreq_path参数,即词典的路径,如下面的代码所示，执行bin/nginx就能提供mmseg的HTTP服务了。</br>
```
  location / { 
            root   html;
            index  index.html index.htm;
        }   
    location /test {
            words_path /user/nginx_mmseg/mmseg/data/words.dic;
            charFreq_path /user/nginx_mmseg/mmseg/data/chars.dic; 
        }   
```
有不清楚的可以参考目录下 Makefile_example 文件 Makefile.bk.hpp和 Makefile.bk.so，
这是把mmseg算法两种形式供nginx模块调用的方式：Makefile.bk.hpp是在模块开发时直接包含分词算法Mmseg.hpp 无需额外的链接；Makefile.bk.so是把分词算法mmseg包装成.so的动态链接库形式供nginx模块调用。两个 Makefile不同之处在于\$(LINK) 的时候，.so在LINK时的写法是：</br>
\-L$(MMSEG_PATH)/../build -lmmseg -lpthread -lcrypt /search/daniel/pcre-8.37/.libs/l     ibpcre.a -lz
</br>
*注意* 在使用动态链接库 .so会出现如下情况：</br>
./nginx: error while loading shared libraries: libmmseg.so: cannot open shared object file: No such file or directory。
</br>
这是因为nginx 找不到我们的动态链接库 libmmseg.so,因此需要在路径LD_LIBRARY_PATH中添加，方法如下：</br>
在 /etc/profile 中添加</br>
export LD_LIBRARY_PATH=\$LD_LIBRARY_PATH:/user/nginx_mmseg/mmseg/build/</br>
然后 source /etc/profile 即可。</br>

**测试**

- GET</br>
curl "http://127.0.0.1/test?data=研究生命起源"</br>
结果：研究 生命 起源</br>
浏览器打开上述链接也可以，浏览器的页面编码设置为 utf-8 。
- POST</br>
curl --data "研究生命起源" "http://127.0.0.1/test"</br>
结果：研究 生命 起源</br>
具体源码注释见：
<a href="https://github.com/ustcdane/nginx_mmseg" target="_blank"> github/ </a>。
