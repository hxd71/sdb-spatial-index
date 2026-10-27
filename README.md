# SDB Spatial Index

一个基于 C++ 的二维空间索引与可视化示例项目。项目围绕点、线、面几何数据，实现空间对象的包围盒管理、空间索引构建以及交互式查询，并使用 OpenGL/FreeGLUT 对查询结果和索引结构进行可视化。

## 功能概览

- **几何模型**
  - `Point`
  - `LineString`
  - `Polygon`
  - `Envelope`（轴对齐包围盒）
- **几何计算**
  - 包围盒包含、相交与合并
  - 点、线、面之间的欧式距离计算
  - 几何对象与查询矩形的相交判断
- **空间索引**
  - 四叉树（`QuadTree`）
  - R 树（`RTree`）
  - 区域查询（Range Query）
  - 最近邻候选查询（Nearest Neighbor）
  - 四叉树 k 近邻查询（k-NN）
  - 基于距离阈值的空间连接（Spatial Join）
- **可视化交互**
  - 加载纽约地区的自行车站点、出租车点、道路和多边形数据
  - 鼠标框选区域进行查询
  - 鼠标移动实时执行最近邻和 k 近邻查询
  - 显示或隐藏道路、空间索引边界及查询结果

> 当前程序首先通过空间索引筛选候选对象，再使用几何对象自身的精确相交或距离计算完成精炼。因此，索引查询结果适合作为候选集，而不是直接替代精确几何判断。

## 项目结构

```text
.
├── Geometry.h/.cpp       # 几何类型与几何运算
├── Tree.h                # Feature 和空间索引基类
├── QuadTree.h/.cpp       # 四叉树索引
├── RTree.h/.cpp          # R 树索引
├── QuadTreeTest.cpp      # 四叉树测试与分析
├── RTreeTest.cpp         # R 树测试与分析
├── hw6.cpp               # OpenGL/FreeGLUT 可视化程序与交互入口
├── data/                 # Shapefile 示例数据
├── 3rd/freeglut/         # FreeGLUT
└── 3rd/shapelib/         # Shapefile 读取支持
```

## 构建环境

- CMake 3.16 或更高版本
- 支持 C++17 的编译器
- OpenGL 和 GLU
- Linux/macOS 通常还需要系统提供 X11 相关开发库；Windows 可使用 Visual Studio 的 OpenGL 环境

项目已在 `3rd/` 中包含 FreeGLUT 和 Shapelib 的源码，普通构建不需要额外下载这两个依赖。OpenGL 本身仍需由系统提供。

## 构建与运行

```bash
cmake -S . -B build
cmake --build build --config Release
```

构建完成后，可执行文件通常位于：

- Linux/macOS：`build/hw6_app`
- Visual Studio 多配置生成器：`build/Release/hw6_app.exe`

如果构建目录或生成器的输出路径不同，请以 CMake 实际生成结果为准。

## 选择索引实现

默认使用四叉树：

```cpp
// Common.h
//#define USE_RTREE
```

取消注释后将使用 R 树：

```cpp
#define USE_RTREE
```

该宏会影响 `hw6.cpp` 中点、道路和多边形索引树的具体类型。修改后请重新配置或重新构建项目。

## 交互操作

程序启动后会打开 OpenGL 窗口，主要快捷键如下：

| 按键 | 操作 |
| --- | --- |
| `s` | 点数据区域查询，鼠标拖拽框选矩形 |
| `S` | 道路数据区域查询，鼠标拖拽框选矩形 |
| `n` | 点数据最近邻查询，移动鼠标选择查询点 |
| `N` | 道路数据最近邻查询 |
| `p` | 多边形区域查询 |
| `P` | 多边形最近邻查询 |
| `k` / `K` | 点数据 k-NN 查询 |
| `]` | 增大 k |
| `[` | 减小 k |
| `b` / `B` | 加载自行车站点数据 |
| `t` / `T` | 加载出租车数据 |
| `r` / `R` | 显示或隐藏道路 |
| `q` / `Q` | 显示或隐藏当前空间索引边界 |
| `+` / `-` | 调整点大小 |
| `1`–`8` | 运行内置测试或性能分析 |
| `Esc` | 退出程序 |

## 数据格式

示例数据使用 Shapefile 格式。程序会从 `data/` 目录读取对应的 `.shp` 文件，并在可用时读取 `.dbf` 中的名称属性，例如：

```text
data/station.shp
 data/station.dbf
 data/highway.shp
 data/polygon.shp
```

其中点、道路和多边形数据分别用于构建不同的空间索引。若数据文件缺失，程序会输出错误或警告信息。

## 设计说明

查询流程大致分为两步：

1. **索引过滤**：四叉树或 R 树根据对象包围盒快速返回候选集。
2. **精确精炼**：调用 `Geometry::intersects` 或 `Geometry::distance` 对候选对象进行精确判断，并去除重复结果。

四叉树通过递归划分空间构建层次结构；R 树则以节点包围盒组织空间对象。两种索引都提供统一的 `Tree` 接口，因此可通过 `USE_RTREE` 在不修改上层交互代码的情况下切换实现。

## 许可证与第三方组件

本项目自身未在仓库中声明独立许可证。`3rd/` 目录中的第三方组件保留其各自的许可证和版权声明；使用或分发项目时，请同时遵守对应组件的许可条款。
