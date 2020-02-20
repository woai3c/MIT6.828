# 总结
## Lab 4
#### 练习6
在 `boot_aps()` 启动 AP CPU 后，创建了三个用户环境，运行过程如下（假设只有一个 CPU）：
1. 三个用户环境对应三个新的 env
2. 调用 `sched_yield()`，如果有 env 处于 `ENV_RUNNABLE` 状态，就给这个 env 分配 CPU 执行环境
3. 执行用户程序（user/yield.c），进入中断，打印 `"Hello, I am environment %08x.\n"`，再调用 `sys_yield()`
4. 此时会进入第 3 个步骤，如此循环
5. 循环完毕后输出用户程序最后一行语句，执行完毕
