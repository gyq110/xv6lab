# xv6-RISC-V 学习仓库
## 仓库简介
本仓库用于存放 **MIT 6.S081 xv6-RISC-V 操作系统实验**，
使用 Git 做版本管理，每完成一个 Lab 单独提交一个版本，方便回溯、复盘学习过程。

所有实验具体步骤、源码修改、原理理解、踩坑记录，均放在 `docs/` 目录下单独文档。

## 仓库目录结构

xv6-riscv/
├── kernel/ # xv6 内核源码，各 Lab 内核修改均在此
├── user/ # 用户态程序 & 系统调用桩代码
├── docs/ # 各实验详细笔记 & 做题过程
│ ├── lab1.md
│ └── lab2.md
└── README.md # 仓库总说明

## 实验列表 导航
- [Lab1 实验笔记](docs/lab1.md)
- [Lab2 自定义系统调用实现](docs/lab2.md)

## 运行环境
- 架构：RISC-V
- 模拟器：QEMU
- 编译工具：RISC-V 交叉编译链

启动命令：
```bash
make qemu