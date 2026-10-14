# EasyBC: SPN 结构分组密码自动化截断差分分析

本仓库提供论文《基于EasyBC的SPN结构分组密码自动化截断差分分析》相关的实现代码和补充材料. 系统以 `.cl` 格式的 EasyBC 密码描述及分析参数为输入, 基于分支特性表(Branching Property Table, BPT)生成截断差分传播约束, 使用 MILP 或 SAT 后端搜索单条截断差分轨迹.

本研究主要面向具有并行双射 S 盒、字级置换, 且线性扩散层可分解为局部列混合操作的字级 SPN 分组密码. 仓库提供 CRAFT、LED64、Midori64、RAIN64 和 SKINNY64 的 WordWise 密码描述与配置示例. 本文所用 BPT 传播语义及 MILP 截断差分建模主要沿用已有方法; 本项目将其集成到 EasyBC 自动化分析流程, 并实现 SAT 后端和精度可设置的概率负对数离散化编码.

## 仓库内容

| 路径 | 内容 |
| --- | --- |
| [`附录材料/`](./附录材料/) | **论文正文与附录 A-C 整合的完整技术报告**, 以及按附录分类整理的实验补充文件. |
| [`EasyBC-TruncatedDifferential/`](./EasyBC-TruncatedDifferential/) | 完整代码工程, 包含构建文件、密码描述、参数示例及源代码. |
| `EasyBC-TruncatedDifferential/parameter_examples/` | MILP/SAT 分析的参数文件示例. |
| `EasyBC-TruncatedDifferential/include/`、`EasyBC-TruncatedDifferential/lib/` | EasyBC 解析、预处理、BPT 构建与差分分析等模块的头文件和实现. |

**附录对应关系:** 

- 附录 A 为运行时间分解; 
- 附录 B 为分支特性表(BPT), 包含五种密码的原始及不同精度下的离散化 BPT, 收录在完整技术报告内, 不单独设置 `附录B/` 目录; 
- 附录 C 为代表性截断差分轨迹. 其中轨迹图位于 `附录材料/附录C/figures/`, 相应文本数据位于 `附录材料/附录C/trajectories/`. 

请以完整技术报告中的附录编号、图表编号为准.

## 系统架构

```text
EasyBC .cl 密码描述 + 分析参数
  → Flex/Bison 解析
  → 语义解释与三地址中间表示
  → BPT 构建及概率负对数离散化
  → MILP / SAT 约束模型自动生成
  → Gurobi / Mallob 求解
  → 逐轮求解结果与单条截断差分轨迹
```

 `EasyBC-TruncatedDifferential/` 下的 `include/truncatedDifferentialBPT/` 和 `lib/truncatedDifferentialBPT/` 包含截断差分分析的主要实现;

 `differential/` 模块提供已有差分分析功能. 截断差分MILP后端使用 Gurobi, SAT 后端使用 Mallob, 支持 $SAT_{(+)}$、$SAT_{(-)}$ 和 $SAT_{(bs)}$ 三种外层目标搜索策略, 以及 SC、SWC、BDD 三种伪布尔目标约束的 CNF 编码方式.

## 环境与依赖

项目面向 Linux 环境, 使用 C++17 和 POSIX shell. 构建需要 CMake 3.16 或更高版本、C++ 编译器、OpenMP、Flex 和 Bison. 其他主要依赖如下:

| 依赖 | 用途 |
| --- | --- |
| Gurobi | MILP 模型求解; 需要安装求解器并配置有效 License. |
| Mallob | SAT 模型的 CNF 可满足性求解. |
| Z3 | MaxSMT 求解及概率权重的布尔编码. |
| PBLib | 将伪布尔约束转换为 CNF 公式. |
| SageMath | 有限域运算及 BPT 相关计算. |
| JsonCpp | 读取和生成 JSON 数据. |

从仓库根目录执行以下命令, 首先进入代码工程目录 `EasyBC-TruncatedDifferential/`. 依赖路径均为**占位符**, 需要按实际服务器安装位置修改; Gurobi 库版本为`gurobi130` .

```bash
cd EasyBC-TruncatedDifferential

export GUROBI_HOME=/path/to/gurobi/linux64
export Z3_DIR=/path/to/z3
export PBLIB_DIR=/path/to/pblib
export MALLOB_BIN_DIR=/path/to/mallob/build
export PATH="$MALLOB_BIN_DIR:$GUROBI_HOME/bin:$PATH"
export LD_LIBRARY_PATH="$GUROBI_HOME/lib:$Z3_DIR/lib:$Z3_DIR/lib64:$Z3_DIR/bin${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"

cmake -S . -B build \
  -DCMAKE_BUILD_TYPE=Debug \
  -DGUROBI_DIR="$GUROBI_HOME" \
  -DGUROBI_LIBRARY_NAME=gurobi130 \
  -DZ3_DIR="$Z3_DIR" \
  -DPBLIB_DIR="$PBLIB_DIR"
cmake --build build --target EasyBC -j 4
```

运行前还需准备包含 SageMath、Python 和 `optimodel.boolean` 的运行环境.

## 快速运行

EasyBC 在不带命令行参数时, 从执行目录的 `../parameters.txt` 读取配置. 以下示例接续上述构建命令, 在代码目录 `EasyBC-TruncatedDifferential/` 中准备配置文件, 再进入其 `build/` 子目录运行.  可先使用小轮数的 CRAFT 示例检查 MILP 和 SAT 两个入口:

```bash
# MILP
cp parameter_examples/craft-milp-a0.txt parameters.txt
(cd build && ./EasyBC)

# SAT
cp parameter_examples/craft-sat-a0-sc-minus.txt parameters.txt
(cd build && ./EasyBC)
```

代码工程下的 `parameter_examples/` 提供 CRAFT 的 MILP `alpha=0/1` 配置, 以及 SAT 在不同目标搜索策略、编码方法和精度下的配置示例. 如需更换密码或轮数, 可修改相应参数文件.

## 参数文件说明

参数文件每行一个值, 不含空行或行内注释. 首行为**包含首行自身在内的总行数**; 第 2-5 行依次为密码描述路径、方法、分析目标和 S 盒约简方法, 此后以“参数名、值”成对填写. 密码文件路径相对于程序运行目录 `EasyBC-TruncatedDifferential/build/`.

以下为搜索 CRAFT 第 1-12 轮、`alpha=1` 的 MILP 参数格式示例:

```text
11
../benchmarks/WordWise/CRAFT.cl
w
truncBPTcryptanalysis
1
startRound
1
endRound
12
alpha
1
```

仓库的 `craft-milp-a0.txt` 和 `craft-milp-a1.txt` 使用相同格式, 但配置了较小的结束轮数, 适合初步运行检查. SAT 配置还需要指定权重编码及目标约束编码, 例如 `craft-sat-a0-sc-minus.txt` 包含:

```text
encoding_type
ONE_HOT
objective_encoding
SC
```

| 参数 | 说明 |
| --- | --- |
| `startRound`、`endRound` | 搜索的起止轮数, 包含两个端点. |
| `alpha` | BPT 传播概率负对数的离散化精度参数; 示例包括 0 和 1. |
| `threadsNum` | 可选求解线程数; 程序默认值为 16. |
| `timer` | 可选的单次求解时限, 单位为秒; 程序默认值为 86400. |
| `encoding_type` | SAT 的 BPT 权重编码选项, 如 `ONE_HOT`、`MAX_SMT`. |
| `objective_encoding` | SAT 的目标约束 CNF 编码选项, 如 `SC`、`SWC`、`BDD`. |

提供的 SAT 参数示例中, `alpha=0` 对应 `ONE_HOT+SC`, `alpha=1` 对应 `MAX_SMT+SWC/BDD`. 更换密码时, 将第 2 行改为 `../benchmarks/WordWise/<密码>.cl` 中相应文件的实际路径. 仓库 README 所列示例名称为 `CRAFT.cl`、`LED64.cl`、`Midori64.cl`、`RAIN64.cl` 和 `SKINNY64.cl`.

## 结果

程序执行后, 结果默认写入 `EasyBC-TruncatedDifferential/data/trunBPTdifferential/`. MILP 对应 `WordWiseMILP/truncBPTcryptanalysis/`, SAT 对应 `WordWiseSAT/cryptanalysis/`.

相应配置的 `results/` 保存逐轮分析结果及 JSON 数据; 模型文件、求解赋值和轨迹数据则保存于各自子目录. JSON 中的 `status` 表示求解状态, `optimality_proven` 表示是否证明最优, `trajectory_available` 表示是否得到可用轨迹. `raw_total_weight` 与 `raw_probability` 由单条轨迹中局部 BPT 传播概率计算.

计时字段单位为秒: `build_time` 为构建时间, `solve_time` 为求解阶段时间, `other_time` 为其他开销, `total_time` 为单轮总时间, 满足 `total_time = build_time + solve_time + other_time`. 共享预处理时间通过 `shared_preprocessing_time` 单独报告. SAT 在 `sat_initial_build_v1` 口径下, 构建时间统计首次进入分析至首次调用求解器前的时间, 后续搜索中的模型与编码开销计入求解阶段. 附录 A 提供用于论文分析的逐轮运行时间分解数据.

## 论文附录

论文正文和附录 A-C 的**整合技术报告位于 [`附录材料/`](./附录材料/)**, 可用于交叉查阅正文、附录表格和轨迹图.

- **附录 A: 运行时间分解.** `附录材料/附录A/milp/` 和 `附录材料/附录A/sat/` 分别存放 MILP 与 SAT 的逐轮计时数据. 建模、求解及总时间的具体定义以论文正文和附录说明为准.
- **附录 B: 分支特性表.** 包含五种测试密码的原始 BPT 及 `alpha=0/1` 条件下的离散化 BPT, 直接收录于完整技术报告, 无独立文件夹.
- **附录 C: 代表性截断差分轨迹.** `附录材料/附录C/figures/` 保存轨迹图, `附录材料/附录C/trajectories/` 保存相应轨迹文本. 图中的活跃字模式及概率权重可结合轨迹文本、附录 B 与论文第 4.2 节进行核对.
