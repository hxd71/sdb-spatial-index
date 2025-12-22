# Geometry.cpp 代码完善与原理说明

## 1. Envelope 部分
### contain(const Envelope& envelope)
- 判断当前 Envelope 是否包含另一个 Envelope。
- 原理：只要当前 Envelope 的最小/最大 x/y 坐标分别小于等于/大于等于目标 Envelope，即可判定包含。

### intersect(const Envelope& envelope)
- 判断两个 Envelope 是否相交。
- 原理：只要不满足“一个在另一个左/右/上/下方”，即有交集。

### unionEnvelope(const Envelope& envelope)
- 合并两个 Envelope，生成一个能包住两者的新 Envelope。
- 原理：取两个 Envelope 的 minX/minY 的最小值，maxX/maxY 的最大值。

## 2. Point 部分
### distance(const LineString* line)
- 计算点到折线的最短距离。
- 原理：遍历折线的每一段，计算点到线段的投影距离，取最小值。
- 关键步骤：
  1. 计算线段向量(dx, dy)和长度平方len2。
  2. 计算投影参数 t，限定在[0,1]。
  3. 得到投影点坐标，计算欧氏距离。

### distance(const Polygon* polygon)
- 计算点到多边形的距离。
- 原理：先判断点是否在多边形内（射线法），若在则距离为0，否则为点到外环的最短距离。
- 关键步骤：
  1. 射线法统计穿越次数，奇数为内，偶数为外。
  2. 若在外部，调用 distance(&line) 计算距离。

## 3. LineString 部分
### distance(const LineString* line)
- 计算两条折线的最短距离。
- 原理：遍历两条折线的所有线段组合，计算端点间距离和端点到对方线段的距离，取最小值。

### distance(const Polygon* polygon)
- 计算折线到多边形的最短距离。
- 原理：分别计算折线到多边形外环的距离和外环到折线的距离，取较小值。
- 这样可以兼容两者互为内外的情况。

## 4. 代码补充说明
- 所有 distance 相关函数均采用欧氏距离。
- 射线法判断点在多边形内外。
- 折线与多边形距离采用对称最小距离法，保证鲁棒性。
- Envelope 的交并等操作均为区间比较。

## 5. 主要逻辑流程
- 点到线/多边形距离：遍历+投影+最小值。
- 线到线/多边形距离：双重遍历+端点/线段距离。
- 多边形到多边形距离：外环互算距离。

---

