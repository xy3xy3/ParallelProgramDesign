﻿**并行程序设计与算法实验**

**2-基于MPI的并行矩阵乘法（进阶）**

**提交格式说明**

按照实验报告模板填写报告，需要提供源代码及代码描述至https://easyhpc.net/course/221。实验报告模板使用PDF格式，命名方式为“并行程序设计\_学号\_姓名”。如有疑问，在课程群（群号1021067950）中询问细节。

**1. 实验要求**

改进上次实验中的MPI并行矩阵乘法(MPI-v1)，并讨论不同通信方式对性能的影响。

**输入：**m, n, k三个整数，每个整数的取值范围均为[128, 2048]

**问题描述：**随机生成m×n的矩阵A及n×k的矩阵B，并对这两个矩阵进行矩阵乘法运算，得到矩阵C.

**输出**：A, B, C三个矩阵，及矩阵计算所消耗的时间t。

**要求：1.**采用MPI集合通信实现并行矩阵乘法中的进程间通信；使用mpi\_type\_create\_struct聚合MPI进程内变量后通信（例如矩阵尺寸m、n、k或者其他变量）；尝试不同数据/任务划分方式（选做）。

\2. 对于不同实现方式，调整并记录不同线程数量（1-16）及矩阵规模（128-2048）下的时间开销，填写下页表格，并分析其性能及扩展性。



|进程数|矩阵规模|
| :-: | :-: |
||128|256|512|1024|2048|
|1||||||
|2||||||
|4||||||
|8||||||
|16||||||

