# RISC_V_Simulator

### 简介

使用C++模拟RISC-V架构cpu五级流水运行

### 五级流水架构图

<img src="C:/Users/StillMe/AppData/Roaming/Typora/typora-user-images/image-20220701172039670.png" alt="image-20220701172039670" style="zoom:25%;" />

### 模拟方式

五级流水各个阶段如下：

| 缩写    | 全称               |
| ------- | :----------------- |
| **IF**  | Instruction Fetch  |
| **ID**  | Instruction Decode |
| **EX**  | Execute            |
| **MEM** | Memory Access      |
| **WB**  | Write Back         |

按照IF,ID,EX,WB均消耗一个时钟周期，MEM为load/store指令时消耗三个时钟周期，其余指令消耗一个时钟周期来模拟cpu运行过程。

当时钟周期为上升沿时，更新所有Stage之间的Buffer，然后执行所有阶段的模拟。这种方式支持五个Stage之间的乱序执行，可以模拟真实cpu执行过程中的并行执行过程。

### 文件说明

+ main.cpp
+ utility.h
  + namespace `INSTRUCTION`
    + 指令相关常量
    + `Instruction`类，负责decode等
  + namespace `STORAGE`
    + `Storage`类，存储相关功能的基类
      + 派生类`Register`
      + 派生类`Memory`
+ cpu.hpp
  + `cpu`类
    + 成员变量：寄存器、内存类对象，五个Stage对象，预测器对象，其他flag
    + 成员函数：
      + `init()`：从输入流初始化内存
      + `run()`：开始模拟cpu运行
+ predictor.hpp
  + `predictor`类:
    + 包含四种不同的预测方式，接口相同
    + 成员函数：
      + `predictPC()`:给出预测的pc
      + `update()`:用传入的预测结果更新预测器
+ stage.h / stage.cpp
  + 纯虚基类`Stage`
    + 定义Stage间的Buffer
  + 派生类`StageIF`,`StageID`,`StageEX`,`StageMEM`,`StageWB`
    + 各自拥有重载的成员函数`work()`，负责不同Stage的模拟

### 分支预测

实现了四种分支预测方式：

+ 一级分支预测

  + 通过取`pc`的后12位，映射到大小为`4096`的`BHT(Branch History Table)`，其中每个元素都是一个二位饱和计数器，来预测是否跳转
  + 通过取`pc`的后8位，映射到大小为`256`的`BTB(Branch Target Buffer)`，来预测跳转地址

+ 基于局部历史的二级分支预测

  + 通过取`pc`的后8位，映射到大小为`256`的`BHT(Branch History Table)`，来记录该分支的跳转历史
  + 通过取`pc`的后8位和`BHT`，映射到大小为`256x64`的`PHT(Pattern History Table)`，来预测是否跳转
  + 通过取`pc`的后8位，映射到大小为`256`的`BTB(Branch Target Buffer)`，来预测跳转地址

+ 基于全局历史的预测

  + 用长度为12位的`GHR(Global History Register)`记录全局所有指令的跳转历史，来替代局部历史方法中的`BHT`，其余相同
  + 另一种方法是`PHT`只开一维，以减小空间消耗增长`GHR`，利用`pc ^ GHR`进行寻址，测试效果不佳，故未呈现在代码中

+ 竞争预测

  + 将局部历史和全局历史的方法融合
  + 将`12`位的`GHR`映射到大小为`4096`的`CPHT(Choice Pattern History Table)`，即`4096`个二位饱和计数器，来预测采用哪种预测方法
  + 若局部历史方法预测成功而全局历史方法预测失败，则将对应的`CPHT++`；反之则`--`
  + 其余部分与局部历史和全局历史的方法相同

+ 预测结果

  | 一级分支预测        | 准确率 | 预测成功数 | 总预测数 |
  | ------------------- | ------ | ---------- | -------- |
  | array_test1.data    | 54.55% | 12         | 22       |
  | array_test2.data    | 57.69% | 15         | 26       |
  | basicopt1.data      | 82.40% | 127840     | 155139   |
  | bulgarian.data      | 94.27% | 67396      | 71493    |
  | expr.data           | 84.68% | 94         | 111      |
  | gcd.data            | 67.50% | 81         | 120      |
  | hanoi.data          | 61.10% | 10667      | 17457    |
  | lvalue2.data        | 66.67% | 4          | 6        |
  | magic.data          | 78.42% | 53220      | 67869    |
  | manyarguments.data  | 60.00% | 6          | 10       |
  | multiarray.data     | 83.33% | 135        | 162      |
  | naive.data          | /      | 0          | 0        |
  | pi.data             | 82.40% | 32925342   | 39956380 |
  | qsort.data          | 87.42% | 174888     | 200045   |
  | queens.data         | 73.38% | 56588      | 77116    |
  | statement_test.data | 60.40% | 122        | 202      |
  | superloop.data      | 93.82% | 408156     | 435027   |
  | tak.data            | 73.81% | 44755      | 60639    |

  | 二级分支预测        | 准确率 | 预测成功数 | 总预测数 |
  | ------------------- | ------ | ---------- | -------- |
  | array_test1.data    | 54.55% | 12         | 22       |
  | array_test2.data    | 50.00% | 13         | 26       |
  | basicopt1.data      | 99.20% | 153893     | 155139   |
  | bulgarian.data      | 94.49% | 67555      | 71493    |
  | expr.data           | 68.47% | 76         | 111      |
  | gcd.data            | 60.83% | 73         | 120      |
  | hanoi.data          | 98.38% | 17175      | 17457    |
  | lvalue2.data        | 66.67% | 4          | 6        |
  | magic.data          | 85.24% | 57850      | 67869    |
  | manyarguments.data  | 80.00% | 8          | 10       |
  | multiarray.data     | 55.56% | 90         | 162      |
  | naive.data          | /      | 0          | 0        |
  | pi.data             | 84.45% | 33742332   | 39956380 |
  | qsort.data          | 96.14% | 192319     | 200045   |
  | queens.data         | 81.09% | 62531      | 77116    |
  | statement_test.data | 61.88% | 125        | 202      |
  | superloop.data      | 99.27% | 431843     | 435027   |
  | tak.data            | 80.53% | 48832      | 60639    |

  | 全局分支预测        | 准确率 | 预测成功数 | 总预测数 |
  | ------------------- | ------ | ---------- | -------- |
  | array_test1.data    | 54.55% | 12         | 22       |
  | array_test2.data    | 50.00% | 13         | 26       |
  | basicopt1.data      | 89.41% | 138717     | 155139   |
  | bulgarian.data      | 90.20% | 64486      | 71493    |
  | expr.data           | 52.25% | 58         | 111      |
  | gcd.data            | 65.00% | 78         | 120      |
  | hanoi.data          | 93.83% | 16380      | 17457    |
  | lvalue2.data        | 66.67% | 4          | 6        |
  | magic.data          | 71.08% | 48241      | 67869    |
  | manyarguments.data  | 80.00% | 8          | 10       |
  | multiarray.data     | 56.79% | 92         | 162      |
  | naive.data          | /      | 0          | 0        |
  | pi.data             | 84.09% | 33599940   | 39956380 |
  | qsort.data          | 93.40% | 186840     | 200045   |
  | queens.data         | 73.69% | 56823      | 77116    |
  | statement_test.data | 59.90% | 121        | 202      |
  | superloop.data      | 88.35% | 384340     | 435027   |
  | tak.data            | 87.53% | 53078      | 60639    |

  | 竞争预测            | 准确率 | 预测成功数 | 总预测数 |
  | ------------------- | ------ | ---------- | -------- |
  | array_test1.data    | 45.45% | 10         | 22       |
  | array_test2.data    | 38.46% | 10         | 26       |
  | basicopt1.data      | 97.31% | 150971     | 155139   |
  | bulgarian.data      | 95.16% | 68030      | 71493    |
  | expr.data           | 79.28% | 88         | 111      |
  | gcd.data            | 65.00% | 78         | 120      |
  | hanoi.data          | 85.30% | 14891      | 17457    |
  | lvalue2.data        | 66.67% | 4          | 6        |
  | magic.data          | 81.00% | 54974      | 67869    |
  | manyarguments.data  | 80.00% | 8          | 10       |
  | multiarray.data     | 72.84% | 118        | 162      |
  | naive.data          | /      | 0          | 0        |
  | pi.data             | 84.97% | 33951724   | 39956380 |
  | qsort.data          | 95.29% | 190615     | 200045   |
  | queens.data         | 79.05% | 60957      | 77116    |
  | statement_test.data | 66.34% | 134        | 202      |
  | superloop.data      | 98.77% | 429687     | 435027   |
  | tak.data            | 85.44% | 51811      | 60639    |

