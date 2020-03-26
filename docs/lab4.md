## Lab 4
我们将使JOS支持"symmetric multiprocessing" (SMP)，这是一种所有CPU共享系统资源的多处理器模式。在启动阶段这些CPU将被分为两类：
1. 启动CPU（BSP）：负责初始化系统，启动操作系统。
2. 应用CPU（AP）：操作系统启动后由BSP激活。

`mp_init()` 收集 CPU 信息，初始化相关信息。

每个CPU如下信息是当前CPU私有的
1. 内核栈：内核代码中的数组percpu_kstacks[NCPU][KSTKSIZE]为每个CPU都保留了KSTKSIZE大小的内核栈。从内核线性地址空间看CPU 0的栈从KSTACKTOP开始，CPU 1的内核栈将从CPU 0栈后面KSTKGAP字节处开始，以此类推，参见inc/memlayout.h。
2. TSS和TSS描述符：每个CPU都需要单独的TSS和TSS描述符来指定该CPU对应的内核栈。
3. 进程结构指针：每个CPU都会独立运行一个进程的代码，所以需要Env指针。
4. 系统寄存器：比如cr3, gdt, ltr这些寄存器都是每个CPU私有的，每个CPU都需要单独设置。
### 调用用户环境过程
在 `boot_aps()` 启动 AP CPU 后，创建了三个用户环境，运行过程如下（假设只有一个 CPU）：
1. 三个用户环境对应三个新的 env
2. 调用 `sched_yield()`，如果有 env 处于 `ENV_RUNNABLE` 状态，就给这个 env 分配 CPU 执行环境
3. 执行用户程序（user/yield.c），进入中断，打印 `"Hello, I am environment %08x.\n"`
4. 重复第2个步骤
5. 循环完毕后输出用户程序最后一行语句，执行完毕

### fork 流程
1. 使用 `set_pgfault_handler()` 设置缺页处理函数。
2. 调用 `sys_exofork()` 系统调用，创建子进程。
3. 将父进程的页映射到子进程，对于可写的页，将对应的 PTE 的 `PTE_COW` 位设置为1。
4. 为子进程设置 `_pgfault_upcall`。
5. 将子进程状态设置为 `ENV_RUNNABLE`。
6. 为子进程的异常栈分配一个页的内存。

### 一次 fork 两个返回值
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

### CPU 是怎么切换去执行用户环境的
CPU 在执行 bootloader 时，就已经通过 cli 指令屏蔽了中断，并且一直没有开启。

在两种情况下，内核会开启中断
1. 切换到用户环境时（在创建用户环境时，通过执行 `e->env_tf.tf_eflags |= FL_IF;` 开启中断 ）
2. `sched_yield()` 调度时，如果没有用户环境，执行 `sched_halt()`，此函数将重新开启中断

所以可以看到在内核初始化完成后，要执行 `sched_yield();`，否则内核将因为没有下一条指令将停摆（此时中断没有开启，时钟中断无任何作用）。

执行 `sched_yield();`，如果有用户环境处于 `ENV_RUNNABLE` 状态，则执行此用户环境，同时开启中断，时钟中断也开始起作用。

每个 CPU 都会触发自己的时钟中断，所以如果有 4 个用户环境，在第一个用户环境执行后，因为开启了中断，此时另外 3 个 CPU 都会触发时钟中断，并执行时钟中断对应的处理函数，即  `sched_yield();`，此时，4 个用户环境运行在不同的 CPU 上。

通过中断进入内核，此时要看中断是屏蔽还是开启，主要看设置中断向量表对应的处理函数时，将门设置为中断门还是陷阱门。中断门会清除 IF 标志屏蔽中断，陷阱门不会。

### 写时复制
在执行 `fork` 函数时，父子进程共享页面被标记为 `PTE_COW | PTE_U | PTE_P`。 当尝试往共享的页面写入，并且页面权限为 `PTE_COW` 时，才会触发缺页中断。这时系统会分配一个新的物理页，并将之前出现错误的页的内容拷贝到新的物理页，然后重新映射线性地址到新的物理页。

### 进程间通信
sys_ipc_recv()和sys_ipc_try_send()是这么协作的：

1. 当某个进程调用sys_ipc_recv()后，该进程会阻塞（状态被置为ENV_NOT_RUNNABLE），直到另一个进程向它发送“消息”。当进程调用sys_ipc_recv()传入dstva参数时，表明当前进程准备接收页映射。
2. 进程可以调用sys_ipc_try_send()向指定的进程发送“消息”，如果目标进程已经调用了sys_ipc_recv()，那么就发送数据，然后返回0，否则返回-E_IPC_NOT_RECV，表示目标进程不希望接受数据。当传入srcva参数时，表明发送进程希望和接收进程共享srcva对应的物理页。如果发送成功了发送进程的srcva和接收进程的dstva将指向相同的物理页。

[参考资料](https://www.cnblogs.com/gatsby123/p/9930630.html)
