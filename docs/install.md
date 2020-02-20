# MIT6.828 实验环境安装教程
## 前期准备
### 安装工具包
```shell
sudo apt-get install -y build-essential libtool libglib2.0-dev libpixman-1-dev zlib1g-dev git libfdt-dev gcc-multilib gdb
```
### 克隆代码和QEMU
```git
// 代码
git clone https://pdos.csail.mit.edu/6.828/2018/jos.git lab
// QEMU
git clone https://github.com/mit-pdos/6.828-qemu.git qemu
```


### 正式安装
##### 1. 切换到 QEMU 目录下
执行 `./configure --disable-kvm --target-list="i386-softmmu x86_64-softmmu"`

出现：`ERROR: Python not found. Use --python=/path/to/python`

解决：添加`--python=python3`，还是不行提示`Note that Python 3 or later is not yet supported`。安装`python2.7`，然后使用`--python=python2.7`选项。

##### 2. 继续执行 `make && make install`
报错
```shell
  CC    qga/commands-posix.o
qga/commands-posix.c: In function ‘dev_major_minor’:
qga/commands-posix.c:633:13: error: In the GNU C Library, "major" is defined
 by <sys/sysmacros.h>. For historical compatibility, it is
 currently defined by <sys/types.h> as well, but we plan to
 remove this soon. To use "major", include <sys/sysmacros.h>
 directly. If you did not intend to use a system-defined macro
 "major", you should undefine it after including <sys/types.h>. [-Werror]
         *devmajor = major(st.st_rdev);
             ^~~~~~~~~~~~~~~~~~~~~~~~~~                                                                                                                                                                                                                                                                                                                                              
qga/commands-posix.c:634:13: error: In the GNU C Library, "minor" is defined
 by <sys/sysmacros.h>. For historical compatibility, it is
 currently defined by <sys/types.h> as well, but we plan to
 remove this soon. To use "minor", include <sys/sysmacros.h>
 directly. If you did not intend to use a system-defined macro
 "minor", you should undefine it after including <sys/types.h>. [-Werror]
         *devminor = minor(st.st_rdev);
             ^~~~~~~~~~~~~~~~~~~~~~~~~~                                                                                                                                                                                                                                                                                                                                              
cc1: all warnings being treated as errors
```
解决方法：
在 `qga/commands-posix.c` 文件中加 `#include <sys/sysmacros.h>`
##### 3. 继续执行 `make && make install`
再报错：
```
block/blkdebug.c: In function ‘blkdebug_refresh_filename’:
block/blkdebug.c:749:31: error: ‘%s’ directive output may be truncated writing up to 4095 bytes into a region of size 4086 [-Werror=format-truncation=]
                  "blkdebug:%s:%s",
                               ^~
In file included from /usr/include/stdio.h:862:0,
                 from /home/wzd/qemu/include/qemu-common.h:27,
                 from block/blkdebug.c:25:
/usr/include/x86_64-linux-gnu/bits/stdio2.h:64:10: note: ‘__builtin___snprintf_chk’ output 11 or more bytes (assuming 4106) into a destination of size 4096
   return __builtin___snprintf_chk (__s, __n, __USE_FORTIFY_LEVEL - 1,
          ^~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        __bos (__s), __fmt, __va_arg_pack ());
        ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
cc1: all warnings being treated as errors
/home/wzd/qemu/rules.mak:57: recipe for target 'block/blkdebug.o' failed
```
解决方法：`config-host.mak`文件中的`-Werror`去掉 

再重新执行 `make && make install`

##### 4.  在 `lab` 源码根目录下执行 `make`，如果看到

```
+ as kern/entry.S
+ cc kern/entrypgdir.c
+ cc kern/init.c
+ cc kern/console.c
+ cc kern/monitor.c
+ cc kern/printf.c
+ cc kern/kdebug.c
+ cc lib/printfmt.c
+ cc lib/readline.c
+ cc lib/string.c
+ ld obj/kern/kernel
ld: warning: section `.bss' type changed to PROGBITS
+ as boot/boot.S
+ cc -Os boot/main.c
+ ld boot/boot
boot block is 390 bytes (max 510)
+ mk obj/kern/kernel.img
```
就说明编译成功了。

然后执行`make qemu`，看到如下信息就说明环境搭建好了。
```
sed "s/localhost:1234/localhost:26000/" < .gdbinit.tmpl > .gdbinit
qemu-system-i386 -drive file=obj/kern/kernel.img,index=0,media=disk,format=raw -serial mon:stdio -gdb tcp::26000 -D qemu.log 
VNC server running on `127.0.0.1:5900'
6828 decimal is XXX octal!
entering test_backtrace 5
entering test_backtrace 4
entering test_backtrace 3
entering test_backtrace 2
entering test_backtrace 1
entering test_backtrace 0
leaving test_backtrace 0
leaving test_backtrace 1
leaving test_backtrace 2
leaving test_backtrace 3
leaving test_backtrace 4
leaving test_backtrace 5
Welcome to the JOS kernel monitor!
Type 'help' for a list of commands.
K> 

```
## 参考
* [MIT-6.828-JOS-环境搭建](https://www.cnblogs.com/gatsby123/p/9746193.html)
* [MIT6.828课程实验环境搭建](https://www.jianshu.com/p/3d6a9df84056)
* [mit6.828 实验环境配置](https://www.jianshu.com/p/1ca94cdd9c89)
