# Lab5 总结
### 磁盘布局
![](https://github.com/woai3c/MIT6.828/blob/master/docs/disk.png)

磁盘块映射到文件系统环境的内存区域，从 0x10000000（DISKMAP）到 0xD0000000，例如，磁盘块0映射到虚拟地址0x10000000，磁盘块1映射到虚拟地址0x10001000，
依此类推。fs/bc.c 中的 diskaddr 函数 实现了从磁盘块号到虚拟地址的转换（以及一些完整性检查）。

在 fs/bc.c 和 fs/fs.c 文件中实现了文件的增删改查，并通过 flush_block() 函数将修改过的内存内容写到硬盘（通过 dirty 位来判断，由硬件设置）。

### 文件系统
```
 Regular env           FS env
   +---------------+   +---------------+
   |      read     |   |   file_read   |
   |   (lib/fd.c)  |   |   (fs/fs.c)   |
...|.......|.......|...|.......^.......|...............
   |       v       |   |       |       | RPC mechanism
   |  devfile_read |   |  serve_read   |
   |  (lib/file.c) |   |  (fs/serv.c)  |
   |       |       |   |       ^       |
   |       v       |   |       |       |
   |     fsipc     |   |     serve     |
   |  (lib/file.c) |   |  (fs/serv.c)  |
   |       |       |   |       ^       |
   |       v       |   |       |       |
   |   ipc_send    |   |   ipc_recv    |
   |       |       |   |       ^       |
   +-------|-------+   +-------|-------+
           |                   |
           +-------------------+
```

### spawn
spawn()，该函数创建一个新的进程，并从磁盘加载程序运行，类似UNIX中的fork()后执行exec()。
