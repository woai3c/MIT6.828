# Lab3 总结
## 用户环境
Lab3 介绍了用户环境系统，一个用户环境就是一个进程。
创建一个用户环境的过程如下
```
内核调用 `ENV_CREATE(x,type)` 
ENV_CREATE()
  env_create()
    env_alloc() // 创建一个新的 env 并初始化
      env_setup_vm() // 继承内核的页目录
    load_icode() // 根据程序文件头部加载数据段、代码段等
      region_alloc() // 为用户环境映射一页内存作为栈空间（USTACKTOP - PGSIZE）
```
创建完成后，只要调用 `env_run()` 即可运行用户环境。`env_run()` 先加载环境的页目录，以代替内核的页目录，然后通过 `env_pop_tf()` 将 tf 结构指针指向的栈的值弹出到各个寄存器，再执行 `iret`，假返回，就像刚从中断调用返回来一样切换到用户环境。
```
void env_pop_tf(struct Trapframe *tf)
{
	asm volatile(
		"\tmovl %0,%%esp\n"
		"\tpopal\n"
		"\tpopl %%es\n"
		"\tpopl %%ds\n"
		"\taddl $0x8,%%esp\n" /* skip tf_trapno and tf_errcode */
		"\tiret\n"
		: : "g" (tf) : "memory");
	panic("iret failed");  /* mostly to placate the compiler */
}
```
```
IRET(interrupt return)中断返回，中断服务程序的最后一条指令。
IRET指令将推入堆栈的段地址和偏移地址弹出，使程序返回到原来发生中断的地方。
其作用是从中断中恢复中断前的状态，具体作用有如下三点：
1.恢复IP(instruction pointer)：IP←（（SP）+1:（SP）），SP←SP+2
2.恢复CS(code segment)：CS←（（SP）+1:（SP））， SP←SP+2
3.恢复中断前的PSW(program status word),即恢复中断前的标志寄存器（EFLAGS）的状态。
FR←（（SP）+1:（SP）），SP←SP+2
4.恢复ESP（返回权限发生变化）
5.恢复SS（返回权限发生变化）
```
## 中断系统
通过 `trap_init()` 设置了中断向量表及对应的处理函数。

用户模式与内核模式切换时，栈（用户栈与内核栈）也会切换，记录在 TSS 中。
#### 系统调用
>现在回顾一下系统调用的完成流程：以user/hello.c为例，其中调用了cprintf()，注意这是lib/print.c中的cprintf，该cprintf()最终会调用lib/syscall.c中的sys_cputs()，sys_cputs()又会调用lib/syscall.c中的syscall()，该函数将系统调用号放入%eax寄存器，五个参数依次放入in DX, CX, BX, DI, SI，然后执行指令int 0x30，发生中断后，去IDT中查找中断处理函数，最终会走到kern/trap.c的trap_dispatch()中，我们根据中断号0x30，又会调用kern/syscall.c中的syscall()函数（注意这时候我们已经进入了内核模式CPL=0），在该函数中根据系统调用号调用kern/print.c中的cprintf()函数，该函数最终调用kern/console.c中的cputchar()将字符串打印到控制台。当trap_dispatch()返回后，trap()会调用env_run(curenv);，该函数前面讲过，会将curenv->env_tf结构中保存的寄存器快照重新恢复到寄存器中，这样又会回到用户程序系统调用之后的那条指令运行，只是这时候已经执行了系统调用并且寄存器eax中保存着系统调用的返回值。任务完成重新回到用户模式CPL=3。

中断发生时，CPU 自动压入 ss esp eflags cs eip error_code，trapno 在 `trapentry.S` 中注册函数时压入， 然后再手动压入 ds es。

此时仍处于用户态，然后将 ds es 指向内核数据段，执行 call trap，cs eip 指向内核代码段，到这一步就已经从用户态转为内核态了。
在 trap 中，根据中断号调用对应的代码，最后再通过执行 env_pop_tf 将 tf 指针指向的堆栈值弹出，恢复寄存器的值，恢复用户态（刚才压入栈的那一堆寄存器的值）。
* [参考资料](https://www.cnblogs.com/gatsby123/p/9838304.html)