## Lab 3
#### 系统调用
>现在回顾一下系统调用的完成流程：以user/hello.c为例，其中调用了cprintf()，注意这是lib/print.c中的cprintf，该cprintf()最终会调用lib/syscall.c中的sys_cputs()，sys_cputs()又会调用lib/syscall.c中的syscall()，该函数将系统调用号放入%eax寄存器，五个参数依次放入in DX, CX, BX, DI, SI，然后执行指令int 0x30，发生中断后，去IDT中查找中断处理函数，最终会走到kern/trap.c的trap_dispatch()中，我们根据中断号0x30，又会调用kern/syscall.c中的syscall()函数（注意这时候我们已经进入了内核模式CPL=0），在该函数中根据系统调用号调用kern/print.c中的cprintf()函数，该函数最终调用kern/console.c中的cputchar()将字符串打印到控制台。当trap_dispatch()返回后，trap()会调用env_run(curenv);，该函数前面讲过，会将curenv->env_tf结构中保存的寄存器快照重新恢复到寄存器中，这样又会回到用户程序系统调用之后的那条指令运行，只是这时候已经执行了系统调用并且寄存器eax中保存着系统调用的返回值。任务完成重新回到用户模式CPL=3。

中断发生时，CPU 自动压入 ss esp eflags cs eip error_code，trapno 在 `trapentry.S` 中注册函数时压入， 然后再手动压入 ds es。

此时仍处于用户态，然后将 ds es 指向内核数据段，执行 call trap，cs eip 指向内核代码段，到这一步就已经从用户态转为内核态了。
在 trap 中，根据中断号调用对应的代码，最后再通过执行 env_pop_tf 将 tf 指针指向的堆栈值弹出，恢复寄存器的值，恢复用户态（刚才压入栈的那一堆寄存器的值）。
* [参考资料](https://www.cnblogs.com/gatsby123/p/9838304.html)