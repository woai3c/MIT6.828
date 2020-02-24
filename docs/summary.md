# 总结
## Lab 3
#### 系统调用
>现在回顾一下系统调用的完成流程：以user/hello.c为例，其中调用了cprintf()，注意这是lib/print.c中的cprintf，该cprintf()最终会调用lib/syscall.c中的sys_cputs()，sys_cputs()又会调用lib/syscall.c中的syscall()，该函数将系统调用号放入%eax寄存器，五个参数依次放入in DX, CX, BX, DI, SI，然后执行指令int 0x30，发生中断后，去IDT中查找中断处理函数，最终会走到kern/trap.c的trap_dispatch()中，我们根据中断号0x30，又会调用kern/syscall.c中的syscall()函数（注意这时候我们已经进入了内核模式CPL=0），在该函数中根据系统调用号调用kern/print.c中的cprintf()函数，该函数最终调用kern/console.c中的cputchar()将字符串打印到控制台。当trap_dispatch()返回后，trap()会调用env_run(curenv);，该函数前面讲过，会将curenv->env_tf结构中保存的寄存器快照重新恢复到寄存器中，这样又会回到用户程序系统调用之后的那条指令运行，只是这时候已经执行了系统调用并且寄存器eax中保存着系统调用的返回值。任务完成重新回到用户模式CPL=3。

中断发生时，CPU 自动压入 ss esp eflags cs eip error_code，然后再手动压入 ds es。

此时仍处于用户态，然后将 ds es 指向内核数据段，执行 call trap，cs eip 指向内核代码段，到这一步就已经从用户态转为内核态了。
在 trap 中，根据中断号调用对应的代码，最后再通过执行 env_pop_tf 将 tf 指针指向的堆栈值弹出，恢复寄存器的值，恢复用户态（刚才压入栈的那一堆寄存器的值）。
* [参考资料](https://www.cnblogs.com/gatsby123/p/9838304.html)
## Lab 4
#### 练习6
在 `boot_aps()` 启动 AP CPU 后，创建了三个用户环境，运行过程如下（假设只有一个 CPU）：
1. 三个用户环境对应三个新的 env
2. 调用 `sched_yield()`，如果有 env 处于 `ENV_RUNNABLE` 状态，就给这个 env 分配 CPU 执行环境
3. 执行用户程序（user/yield.c），进入中断，打印 `"Hello, I am environment %08x.\n"`，再调用 `sys_yield()`
4. 此时会进入第 3 个步骤，如此循环
5. 循环完毕后输出用户程序最后一行语句，执行完毕

#### fork 流程
1. 使用 `set_pgfault_handler()` 设置缺页处理函数。
2. 调用 `sys_exofork()` 系统调用，创建子进程。
3. 将父进程的页映射到子进程，对于可写的页，将对应的 PTE 的 `PTE_COW` 位设置为1。
4. 为子进程设置 `_pgfault_upcall`。
5. 将子进程状态设置为 `ENV_RUNNABLE`。
6. 为子进程的异常栈分配一个页的内存。

#### 一次 fork 两个返回值
```c
envid_t envid = sys_exofork();
if (envid < 0)
  panic("sys_exofork: %e", envid);
if (envid == 0) {
  thisenv = &envs[ENVX(sys_getenvid())];
  return 0;
}
```
为什么每次 fork 后，要判断 envid 是否为零？

因为 fork 时，子进程会把父进程的各个寄存器值也复制，包括 CS 和 IP。
```c
struct Trapframe {
	struct PushRegs tf_regs;
	uint16_t tf_es;
	uint16_t tf_padding1;
	uint16_t tf_ds;
	uint16_t tf_padding2;
	uint32_t tf_trapno;
	/* below here defined by x86 hardware */
	uint32_t tf_err;
	uintptr_t tf_eip;
	uint16_t tf_cs;
	uint16_t tf_padding3;
	uint32_t tf_eflags;
	/* below here only when crossing rings, such as from user to kernel */
	uintptr_t tf_esp;
	uint16_t tf_ss;
	uint16_t tf_padding4;
} __attribute__((packed));
```
所以在 `sys_exofork()` 后父子进程的指令是相同的。

`envid_t envid = sys_exofork();` 要分两步看。

第一步是 `sys_exofork()`，第二步是将函数返回值（函数返回值存放在 eax）赋给 envid，在 fork 完之后，父子进程同样会执行 envid 的赋值操作，父进程将 eax 的值赋给 envid，这个值是刚创建的子进程 id，而子进程由于在初始化时 eax 被手动设为 0，所以 envid 的值为 0。
```c
// kern/syscall.c sys_exofork 函数
child->env_tf.tf_regs.reg_eax = 0;
```

>如果system_call执行调度，父进程就会被换入到就绪队列中，失去CPU的使用权。那么可能是子进程也可能是其他进程继而得到CPU的使用权，开始执行。总之，子进程总会有执行的时刻，而子进程的pcb中的cs,eip寄存器保存的是和父进程通过int 0x80编程异常进入内核处理函数时一样的指令地址，即子进程也是从int 0x80后面的指令movl eax, res; return res开始执行，而子进程的eax的值被设置为0，从而子进程开始执行时的指令流程，和父进程从异常处理函数system_call返回到用户空间后的执行流程是一样的，将eax的值作为fork()API的返回值，从而完成子进程像是从fork()API返回的样子，但实质只是其执行流程是从fork()API返回处开始执行的，且返回值是0。

[fork如何实现执行一次返回两个值的？](https://www.zhihu.com/question/24173190/answer/244790670)
