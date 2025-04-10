﻿**并行程序设计与算法实验**

**3-Pthreads并行矩阵乘法与数组求和**

**提交格式说明**

按照实验报告模板填写报告，需要提供源代码及代码描述至https://easyhpc.net/course/221。实验报告模板使用PDF格式，命名方式为“并行程序设计\_学号\_姓名”。如有疑问，在课程群（群号1021067950）中询问细节。

**1. 并行矩阵乘法**

使用Pthreads实现并行矩阵乘法，并通过实验分析其性能。

**输入：**m, n, k三个整数，每个整数的取值范围均为[128, 2048]

**问题描述：**随机生成m×n的矩阵A及n×k的矩阵B，并对这两个矩阵进行矩阵乘法运算，得到矩阵C.

**输出**：A, B, C三个矩阵，及矩阵计算所消耗的时间t。

**要求：**1. 使用Pthread创建多线程实现并行矩阵乘法，调整线程数量（1-16）及矩阵规模（128-2048），根据结果分析其并行性能（包括但不限于，时间、效率、可扩展性）。2. 选做：可分析不同数据及任务划分方式的影响。

**2. 并行数组求和**

使用Pthreads实现并行数组求和，并通过实验分析其性能。

**输入：**整数n，取值范围为[1M, 128M]

**问题描述：**随机生成长度为n的整型数组A，计算其元素和s=i=1nAi。

**输出**：数组A，元素和s，及求和计算所消耗的时间t。

**要求：**1. 使用Pthreads实现并行数组求和，调整线程数量（1-16）及数组规模（1M, 128M），根据结果分析其并行性能（包括但不限于，时间、效率、可扩展性）。2. 选做：可分析不同聚合方式的影响。
