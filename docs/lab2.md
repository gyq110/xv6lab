# Lab2：xv6 剩余内存查询系统调用实现与复盘

---

## 一、实验目标
在 xv6 内核中实现一个自定义系统调用，功能为**查询并返回整个系统当前剩余的可用内存大小**。

---

## 二、核心问题拆解
要完成这个实验，必须先搞懂两个核心底层逻辑：

### 问题1：系统调用的完整执行流程
从用户态触发系统调用，到内核执行具体函数，完整链路如下：
1.  用户态将系统调用编号写入 `a7` 寄存器
2.  执行 `ecall` 指令，触发从用户态到内核态的特权级切换
3.  内核通过 `a7` 中的编号，映射到对应的系统调用处理函数
4.  执行内核态函数，完成系统调用功能

### 问题2：xv6 的内存分配原理
xv6 中所有空闲内存，都会以「页」为单位挂载到一个**空闲链表（freelist）**中。
`kalloc` 函数分配内存时，会从这个链表中取出节点；因此，**遍历空闲链表的节点数量，即可统计出剩余内存大小**。

---

## 三、内核态代码实现步骤

### 步骤1：添加系统调用编号
在 `kernel/syscall.h` 中，新增系统调用编号：
```c
#define SYS_mmcheck 22
```

### 步骤 2：映射系统调用处理函数
在 `kernel/syscall.c` 中，将编号与自定义内核函数绑定：
```C
uint64 sys_mmcheck(void);

static uint64 (*syscalls[])(void) = {
    // ... 其他系统调用
    [SYS_mmcheck] sys_mmcheck,
};
```
### 步骤 3：添加到系统调用号库
将系统调用号加入   `kernel/syscall.h` 定义的系统调用号列表中，完成内核的注册。
### 步骤 4：实现核心统计函数
因为空闲链表 freelist 是 `kernel/kalloc.c` 中的私有数据结构，统计函数必须在 kalloc.c 中实现，同时要注意：
1. NULL 判断：xv6 中没有定义 NULL 宏，链表结束的判断条件是指针值为 0
2. 加锁保护：空闲链表是临界资源，统计时必须先获取 kalloc 的全局锁，否则会出现统计错误
```C

uint64 
mmfree(void)
{
  uint64 size=0;
  struct run *r;
  acquire(&kmem.lock);
  r=kmem.freelist;
  while(r!=0){
    size+=PGSIZE;
    r=r->next;
  }
  release(&kmem.lock);
  return size;

}

```

### 步骤 5：声明内核函数
在 `kernel/defs.h` 中声明 kalloc.c 里实现的函数，方便 sysproc.c 调用：
```C
uint64 mmfree(void);
```
---

## 四、用户态代码实现步骤
### 步骤 1：编写用户态测试程序
新建 `user/testos.c`，在 main 函数中调用系统调用并打印结果：
```C
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"
#include "kernel/fcntl.h"
int main(int argc,char* argv[]){
    uint64 size=mmcheck();
    printf("the size of memory is %lu",size);
    return 0;
}
```
⚠️ 注意：使用 uint64 类型时，需要包含 kernel/type.h 头文件。

### 步骤 2：声明用户态接口
在 `user/user.h` 中声明用户态可调用的 mmcheck 函数：
```C
uint64 mmcheck(void);
```

### 步骤 3：自动生成系统调用汇编桩
修改 `user/usys.pl` 脚本，添加我们的系统调用，脚本会自动生成 usys.S 汇编文件，完成从用户态函数到内核 ecall 的封装：
```perl
entry("mmcheck");
```
脚本会自动生成对应的汇编代码，将调用号写入 a7 寄存器，再执行 ecall 指令。

---

## 五、核心原理复盘（个人思考）
### 问题 1：为什么内核和用户态的函数不能直接通过 #include "kernel/sysproc.h" 关联？
    -这是操作系统设计的核心原则：内核态与用户态深度解耦。
    -内核的私有头文件、数据结构，不应该暴露给用户态，现代工业级操作系统会通过文件隔离、宏防护等手段，从根源禁止这种直接关联，避免用户态破坏内核的安全与稳定性。

### 问题 2：ecall 指令到底做了什么？如何从用户态跳转到内核？
1. ecall 是 RISC-V 硬件提供的特权级切换指令，由 CPU 硬件自动完成以下操作：
2. 保存状态：将当前用户态的权限状态保存到 sstatus 寄存器，用于后续返回
3. 切换特权级：从低权限的用户态（U 模式），切换到高权限的内核态（S 模式）
4. 保存返回地址：将 ecall 指令的下一条指令地址，保存到 sepc 寄存器，内核返回时会直接跳回这里
5. 读取入口地址：读取 stvec 寄存器中预存的内核陷阱入口地址
6. 跳转执行：跳转到内核陷阱入口，开始执行内核代码

### 问题 3：实验技巧总结
这是从学长的博客中学到的高效调试方法：
先找一个 xv6 自带的、功能正常的系统调用（比如 write/read）
使用 GDB 调试这个正常的系统调用，一步步跟踪从用户态到内核态的完整跳转流程
按照这个成功的流程，一步步添加自己的新系统调用，避免因某一步出错导致整体排查困难


## 六、踩坑记录
### 用户态函数未定义错误：只在 user.h 声明了函数，但没有修改 usys.pl 生成对应的汇编桩
### 统计结果错误：遍历空闲链表时忘记加锁，多进程并发访问导致链表数据错乱
### 编译报错：使用 uint64 类型时，未包含 kernel/type.h 头文件


---