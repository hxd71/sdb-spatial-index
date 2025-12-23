#include "Geometry.h"
#include <cmath>
#include <gl/freeglut.h>

#define NOT_IMPLEMENT -1.0

namespace hw6 {
namespace {
	// 叉乘：(B-A) x (C-A)
	double cross(double ax, double ay, double bx, double by, double cx, double cy) {
		return (bx - ax) * (cy - ay) - (by - ay) * (cx - ax);
	}

	// 判断点 P 是否在线段 AB 上（含端点）
	bool onSegment(double ax, double ay, double bx, double by, double px, double py) {
		if (cross(ax, ay, bx, by, px, py) != 0.0) return false;

		double minx = std::min(ax, bx), maxx = std::max(ax, bx);
		double miny = std::min(ay, by), maxy = std::max(ay, by);

		return (px >= minx && px <= maxx && py >= miny && py <= maxy);
	}

	// 线段 AB 与线段 CD 是否相交（含共线重叠/端点相触）
	bool segmentsIntersect(double ax, double ay, double bx, double by,
						double cx, double cy, double dx, double dy) {
		double c1 = cross(ax, ay, bx, by, cx, cy);
		double c2 = cross(ax, ay, bx, by, dx, dy);
		double c3 = cross(cx, cy, dx, dy, ax, ay);
		double c4 = cross(cx, cy, dx, dy, bx, by);

		// 严格相交（两边异号）
		if ((c1 > 0 && c2 < 0 || c1 < 0 && c2 > 0) &&
			(c3 > 0 && c4 < 0 || c3 < 0 && c4 > 0)) {
			return true;
		}

		// 处理共线/端点落在线段上
		if (c1 == 0.0 && onSegment(ax, ay, bx, by, cx, cy)) return true;
		if (c2 == 0.0 && onSegment(ax, ay, bx, by, dx, dy)) return true;
		if (c3 == 0.0 && onSegment(cx, cy, dx, dy, ax, ay)) return true;
		if (c4 == 0.0 && onSegment(cx, cy, dx, dy, bx, by)) return true;

		return false;
	}

	// 点到线段距离（投影到线段并截断）
	double pointSegmentDistance(double px, double py,
								double ax, double ay,
								double bx, double by) {
		double vx = bx - ax, vy = by - ay;
		double wx = px - ax, wy = py - ay;
		double len2 = vx * vx + vy * vy;

		// 退化线段：A==B
		if (len2 == 0.0) {
			double dx = px - ax, dy = py - ay;
			return std::sqrt(dx * dx + dy * dy);
		}

		double t = (wx * vx + wy * vy) / len2;
		if (t < 0.0) t = 0.0;
		if (t > 1.0) t = 1.0;

		double projx = ax + t * vx;
		double projy = ay + t * vy;

		double dx = px - projx, dy = py - projy;
		return std::sqrt(dx * dx + dy * dy);
	}

	// 线段到线段距离：若相交则为 0，否则为端点到对方线段的最小值
	double segmentSegmentDistance(double ax, double ay, double bx, double by,
								double cx, double cy, double dx, double dy) {
		if (segmentsIntersect(ax, ay, bx, by, cx, cy, dx, dy)) return 0.0;

		double d1 = pointSegmentDistance(ax, ay, cx, cy, dx, dy);
		double d2 = pointSegmentDistance(bx, by, cx, cy, dx, dy);
		double d3 = pointSegmentDistance(cx, cy, ax, ay, bx, by);
		double d4 = pointSegmentDistance(dx, dy, ax, ay, bx, by);

		return std::min(std::min(d1, d2), std::min(d3, d4));
	}

	// 环（折线）边界到环边界的最短距离（按边枚举）
	double ringToRingBoundaryDistance(const LineString& r1, const LineString& r2) {
		size_t n1 = r1.numPoints();
		size_t n2 = r2.numPoints();
		if (n1 < 2 || n2 < 2) return NOT_IMPLEMENT;

		double best = std::numeric_limits<double>::infinity();

		auto edgePoint = [](const LineString& r, size_t idx) -> Point {
			return r.getPointN(idx);
		};

		for (size_t i = 0; i < n1; ++i) {
			size_t i2 = (i + 1) % n1;
			Point a = edgePoint(r1, i);
			Point b = edgePoint(r1, i2);

			double ax = a.getX(), ay = a.getY();
			double bx = b.getX(), by = b.getY();

			for (size_t j = 0; j < n2; ++j) {
				size_t j2 = (j + 1) % n2;
				Point c = edgePoint(r2, j);
				Point d = edgePoint(r2, j2);

				double cx = c.getX(), cy = c.getY();
				double dx = d.getX(), dy = d.getY();

				double dist = segmentSegmentDistance(ax, ay, bx, by, cx, cy, dx, dy);
				if (dist < best) best = dist;
				if (best == 0.0) return 0.0;
			}
		}
		return best;
	}

	// 点是否在环内（含边界），射线法 + 边界判定
	bool pointInRingInclusive(const LineString& ring, double px, double py) {
		size_t n = ring.numPoints();
		if (n < 2) return false;

		// 1) 边界：在任意边上
		for (size_t i = 0; i < n; ++i) {
			size_t j = (i + 1) % n;
			double ax = ring.getPointN(i).getX();
			double ay = ring.getPointN(i).getY();
			double bx = ring.getPointN(j).getX();
			double by = ring.getPointN(j).getY();
			if (onSegment(ax, ay, bx, by, px, py)) return true;
		}

		// 2) 射线法：奇偶规则
		bool inside = false;
		for (size_t i = 0, j = n - 1; i < n; j = i++) {
			double xi = ring.getPointN(i).getX();
			double yi = ring.getPointN(i).getY();
			double xj = ring.getPointN(j).getX();
			double yj = ring.getPointN(j).getY();

			bool cond = ((yi > py) != (yj > py));
			if (cond) {
				double xInt = (xj - xi) * (py - yi) / (yj - yi) + xi;
				if (px < xInt) inside = !inside;
			}
		}
		return inside;
	}

	// 点是否在多边形实体内（含外边界，排除孔洞，孔洞边界也视为“不在实体内”）
	bool pointInPolygonSolidInclusive(const Polygon* poly, double px, double py) {
		LineString exterior = poly->getExteriorRing();
		if (!pointInRingInclusive(exterior, px, py)) return false;

		// 在孔洞内部或孔洞边界上：不属于实体
		for (size_t k = 0; k < poly->numInteriorRings(); ++k) {
			LineString hole = poly->getInteriorRingN(k);
			if (pointInRingInclusive(hole, px, py)) return false;
		}
		return true;
	}

} // anonymous namespace

	/*
	 * Envelope functions
	 */
	bool Envelope::contain(double x, double y) const {
		return x >= minX && x <= maxX && y >= minY && y <= maxY;
	}

	bool Envelope::contain(const Envelope& envelope) const {
		// Task 测试Envelope是否包含或相等关系
		return minX <= envelope.minX && maxX >= envelope.maxX &&
			minY <= envelope.minY && maxY >= envelope.maxY;
	}

	bool Envelope::intersect(const Envelope& envelope) const {
		// Task 测试Envelope是否相交
		return !(maxX < envelope.minX || minX > envelope.maxX
			|| minY > envelope.maxY || maxY < envelope.minY);
	}

	Envelope Envelope::unionEnvelope(const Envelope& envelope) const {
		// Task 合并两个Envelope生成一个新的Envelope
		double newminX = std::min(this->minX, envelope.minX);
		double newmaxX = std::max(this->maxX, envelope.maxX);
		double newminY = std::min(this->minY, envelope.minY);
		double newmaxY = std::max(this->maxY, envelope.maxY);
		return Envelope(newminX,newmaxX,newminY,newmaxY);
	}

	void Envelope::draw() const {
		glBegin(GL_LINE_STRIP);

		glVertex2d(minX, minY);
		glVertex2d(minX, maxY);
		glVertex2d(maxX, maxY);
		glVertex2d(maxX, minY);
		glVertex2d(minX, minY);

		glEnd();
	}

	/*
	 * Points functions
	 */
	double Point::distance(const Point* point) const {
		return sqrt((x - point->x) * (x - point->x) +
			(y - point->y) * (y - point->y));
	}

	double Point::distance(const LineString* line) const {
		// 点到折线距离：取点到每一条线段的最小值
		double best = line->getPointN(0).distance(this);
		for (size_t i = 0; i + 1 < line->numPoints(); ++i) {
			double x1 = line->getPointN(i).getX();
			double y1 = line->getPointN(i).getY();
			double x2 = line->getPointN(i + 1).getX();
			double y2 = line->getPointN(i + 1).getY();
			double d = pointSegmentDistance(x, y, x1, y1, x2, y2);
			if (d < best) best = d;
		}
		return best;
	}

	double Point::distance(const Polygon* polygon) const {
		// 点到多边形距离：
		// 1) 若点在实体内（含外边界），且不在孔洞内/孔洞边界上，则距离为 0
		// 2) 若点在孔洞内（含孔洞边界），返回点到该孔洞边界的距离
		// 3) 否则返回点到外环边界的距离
		if (pointInPolygonSolidInclusive(polygon, x, y)) return 0.0;

		// 若点落在某个孔洞内部/边界上：返回点到孔洞边界距离
		for (size_t k = 0; k < polygon->numInteriorRings(); ++k) {
			LineString hole = polygon->getInteriorRingN(k);
			if (pointInRingInclusive(hole, x, y)) {
				return this->distance(&hole);
			}
		}

		LineString exterior = polygon->getExteriorRing();
		return this->distance(&exterior);
	}

	bool Point::intersects(const Envelope& rect) const {
		return (x >= rect.getMinX()) && (x <= rect.getMaxX()) &&
			(y >= rect.getMinY()) && (y <= rect.getMaxY());
	}

	void Point::draw() const {
		glBegin(GL_POINTS);
		glVertex2d(x, y);
		glEnd();
	}

	/*
	 * LineString functions
	 */
	void LineString::constructEnvelope() {
		double minX, minY, maxX, maxY;
		maxX = minX = points[0].getX();
		maxY = minY = points[0].getY();
		for (size_t i = 1; i < points.size(); ++i) {
			maxX = std::max(maxX, points[i].getX());
			maxY = std::max(maxY, points[i].getY());
			minX = std::min(minX, points[i].getX());
			minY = std::min(minY, points[i].getY());
		}
		envelope = Envelope(minX, maxX, minY, maxY);
	}

	double LineString::distance(const LineString* line) const {
		// 折线到折线距离：枚举两条折线的所有线段，取线段到线段距离最小值
		size_t n1 = this->numPoints();
		size_t n2 = line->numPoints();
		if (n1 < 2 || n2 < 2) return NOT_IMPLEMENT;

		double best = std::numeric_limits<double>::infinity();

		for (size_t i = 0; i + 1 < n1; ++i) {
			double ax = this->getPointN(i).getX();
			double ay = this->getPointN(i).getY();
			double bx = this->getPointN(i + 1).getX();
			double by = this->getPointN(i + 1).getY();

			for (size_t j = 0; j + 1 < n2; ++j) {
				double cx = line->getPointN(j).getX();
				double cy = line->getPointN(j).getY();
				double dx = line->getPointN(j + 1).getX();
				double dy = line->getPointN(j + 1).getY();

				double d = segmentSegmentDistance(ax, ay, bx, by, cx, cy, dx, dy);
				if (d < best) best = d;
				if (best == 0.0) return 0.0;
			}
		}
		return best;
	}

	double LineString::distance(const Polygon* polygon) const {
		// 折线到多边形距离：
		// 1) 若折线任意顶点在多边形实体内（含外边界）-> 距离 0
		// 2) 若折线任意线段与多边形边界（外环/孔洞环）相交/相触 -> 距离 0
		// 3) 否则取折线到所有环边界的最小距离
		if (polygon == nullptr) return NOT_IMPLEMENT;
		if (this->numPoints() < 2) return NOT_IMPLEMENT;

		// 1) 顶点落在实体内
		for (size_t i = 0; i < this->numPoints(); ++i) {
			Point p = this->getPointN(i);
			if (pointInPolygonSolidInclusive(polygon, p.getX(), p.getY())) return 0.0;
		}

		// 2) 检查与外环/孔洞环边界相交
		auto polyBoundaryHit = [&](const LineString& ring) -> bool {
			size_t rn = ring.numPoints();
			if (rn < 2) return false;

			for (size_t i = 0; i + 1 < this->numPoints(); ++i) {
				double ax = this->getPointN(i).getX();
				double ay = this->getPointN(i).getY();
				double bx = this->getPointN(i + 1).getX();
				double by = this->getPointN(i + 1).getY();

				for (size_t k = 0; k < rn; ++k) {
					size_t k2 = (k + 1) % rn;
					double cx = ring.getPointN(k).getX();
					double cy = ring.getPointN(k).getY();
					double dx = ring.getPointN(k2).getX();
					double dy = ring.getPointN(k2).getY();

					if (segmentsIntersect(ax, ay, bx, by, cx, cy, dx, dy)) return true;
				}
			}
			return false;
		};

		LineString exterior = polygon->getExteriorRing();
		if (polyBoundaryHit(exterior)) return 0.0;

		for (size_t h = 0; h < polygon->numInteriorRings(); ++h) {
			LineString hole = polygon->getInteriorRingN(h);
			if (polyBoundaryHit(hole)) return 0.0;
		}

		// 3) 计算到所有环边界的最小距离
		double best = ringToRingBoundaryDistance(*this, exterior);
		for (size_t h = 0; h < polygon->numInteriorRings(); ++h) {
			LineString hole = polygon->getInteriorRingN(h);
			double d = ringToRingBoundaryDistance(*this, hole);
			if (d < best) best = d;
		}
		return best;
	}

	typedef int OutCode;

	const int INSIDE = 0; // 0000
	const int LEFT = 1;   // 0001
	const int RIGHT = 2;  // 0010
	const int BOTTOM = 4; // 0100
	const int TOP = 8;    // 1000

	// Compute the bit code for a point (x, y) using the clip rectangle
	// bounded diagonally by (xmin, ymin), and (xmax, ymax)
	// ASSUME THAT xmax, xmin, ymax and ymin are global constants.
	OutCode ComputeOutCode(double x, double y, double xmin, double xmax,
		double ymin, double ymax) {
		OutCode code;

		code = INSIDE; // initialised as being inside of [[clip window]]

		if (x < xmin) // to the left of clip window
			code |= LEFT;
		else if (x > xmax) // to the right of clip window
			code |= RIGHT;
		if (y < ymin) // below the clip window
			code |= BOTTOM;
		else if (y > ymax) // above the clip window
			code |= TOP;

		return code;
	}

	// Cohen–Sutherland clipping algorithm clips a line from
	// P0 = (x0, y0) to P1 = (x1, y1) against a rectangle with
	// diagonal from (xmin, ymin) to (xmax, ymax).
	bool intersectTest(double x0, double y0, double x1, double y1, double xmin,
		double xmax, double ymin, double ymax) {
		// compute outcodes for P0, P1, and whatever point lies outside the clip
		// rectangle
		OutCode outcode0 = ComputeOutCode(x0, y0, xmin, xmax, ymin, ymax);
		OutCode outcode1 = ComputeOutCode(x1, y1, xmin, xmax, ymin, ymax);
		bool accept = false;

		while (true) {
			if (!(outcode0 | outcode1)) {
				// bitwise OR is 0: both points inside window; trivially accept and
				// exit loop
				accept = true;
				break;
			}
			else if (outcode0 & outcode1) {
				// bitwise AND is not 0: both points share an outside zone (LEFT,
				// RIGHT, TOP, or BOTTOM), so both must be outside window; exit loop
				// (accept is false)
				break;
			}
			else {
				// failed both tests, so calculate the line segment to clip
				// from an outside point to an intersection with clip edge
				double x, y;

				// At least one endpoint is outside the clip rectangle; pick it.
				OutCode outcodeOut = outcode0 ? outcode0 : outcode1;

				// Now find the intersection point;
				// use formulas:
				//   slope = (y1 - y0) / (x1 - x0)
				//   x = x0 + (1 / slope) * (ym - y0), where ym is ymin or ymax
				//   y = y0 + slope * (xm - x0), where xm is xmin or xmax
				// No need to worry about divide-by-zero because, in each case, the
				// outcode bit being tested guarantees the denominator is non-zero
				if (outcodeOut & TOP) { // point is above the clip window
					x = x0 + (x1 - x0) * (ymax - y0) / (y1 - y0);
					y = ymax;
				}
				else if (outcodeOut & BOTTOM) { // point is below the clip window
					x = x0 + (x1 - x0) * (ymin - y0) / (y1 - y0);
					y = ymin;
				}
				else if (outcodeOut &
					RIGHT) { // point is to the right of clip window
					y = y0 + (y1 - y0) * (xmax - x0) / (x1 - x0);
					x = xmax;
				}
				else if (outcodeOut &
					LEFT) { // point is to the left of clip window
					y = y0 + (y1 - y0) * (xmin - x0) / (x1 - x0);
					x = xmin;
				}

				// Now we move outside point to intersection point to clip
				// and get ready for next pass.
				if (outcodeOut == outcode0) {
					x0 = x;
					y0 = y;
					outcode0 = ComputeOutCode(x0, y0, xmin, xmax, ymin, ymax);
				}
				else {
					x1 = x;
					y1 = y;
					outcode1 = ComputeOutCode(x1, y1, xmin, xmax, ymin, ymax);
				}
			}
		}
		return accept;
	}

	bool LineString::intersects(const Envelope& rect) const {
		double xmin = rect.getMinX();
		double xmax = rect.getMaxX();
		double ymin = rect.getMinY();
		double ymax = rect.getMaxY();

		for (size_t i = 1; i < points.size(); ++i)
			if (intersectTest(points[i - 1].getX(), points[i - 1].getY(),
				points[i].getX(), points[i].getY(), xmin, xmax, ymin,
				ymax))
				return true;
		return false;
	}

	void LineString::draw() const {
		if (points.empty()) {
			std::cerr << "Warning: LineString points array is empty." << std::endl;
			return;
		}

		glBegin(GL_LINE_STRIP);
		for (size_t i = 0; i < points.size(); ++i)
			glVertex2d(points[i].getX(), points[i].getY());
		glEnd();

		GLenum err;
		while ((err = glGetError()) != GL_NO_ERROR) {
			std::cerr << "OpenGL Error in draw(): " << gluErrorString(err) << std::endl;
		}
	}

	void LineString::print() const {
		std::cout << "LineString(";
		for (size_t i = 0; i < points.size(); ++i) {
			if (i != 0)
				std::cout << ", ";
			std::cout << points[i].getX() << " " << points[i].getY();
		}
		std::cout << ")";
	}

	/*
	 * Polygon
	 */
	double Polygon::distance(const Polygon* polygon) const {
		// 多边形到多边形距离：
		// 1) 若任意边界相交/相触 -> 距离 0
		// 2) 若一个多边形实体包含另一个多边形的顶点 -> 距离 0
		// 3) 否则返回所有边界环之间的最短距离
		if (polygon == nullptr) return NOT_IMPLEMENT;

		// 1) 边界相交：外环/孔洞环两两检查
		std::vector<LineString> ringsA;
		ringsA.push_back(this->exteriorRing);
		for (size_t i = 0; i < this->numInteriorRings(); ++i) {
			ringsA.push_back(this->getInteriorRingN(i));
		}

		std::vector<LineString> ringsB;
		ringsB.push_back(polygon->getExteriorRing());
		for (size_t i = 0; i < polygon->numInteriorRings(); ++i) {
			ringsB.push_back(polygon->getInteriorRingN(i));
		}

		for (size_t i = 0; i < ringsA.size(); ++i) {
			for (size_t j = 0; j < ringsB.size(); ++j) {
				if (ringToRingBoundaryDistance(ringsA[i], ringsB[j]) == 0.0) return 0.0;
			}
		}

		// 2) 实体包含：检查 A 外环顶点是否落入 B 实体，或 B 外环顶点是否落入 A 实体
		for (size_t i = 0; i < this->exteriorRing.numPoints(); ++i) {
			Point v = this->exteriorRing.getPointN(i);
			if (pointInPolygonSolidInclusive(polygon, v.getX(), v.getY())) return 0.0;
		}

		LineString eB = polygon->getExteriorRing();
		for (size_t i = 0; i < eB.numPoints(); ++i) {
			Point v = eB.getPointN(i);
			if (pointInPolygonSolidInclusive(this, v.getX(), v.getY())) return 0.0;
		}

		// 3) 不相交：计算边界环最短距离
		double best = std::numeric_limits<double>::infinity();
		for (size_t i = 0; i < ringsA.size(); ++i) {
			for (size_t j = 0; j < ringsB.size(); ++j) {
				double d = ringToRingBoundaryDistance(ringsA[i], ringsB[j]);
				if (d < best) best = d;
			}
		}
		return best;
	}

	bool Polygon::intersects(const Envelope& rect) const {
		// 多边形与包围盒是否相交：
		// 1) 先用包围盒快速排除
		// 2) 多边形任一环边界与矩形相交 -> true
		// 3) 矩形任一角点在多边形实体内 -> true
		// 4) 多边形外环任一点在矩形内 -> true
		if (!envelope.intersect(rect)) return false;

		// 2) 任一环边界与矩形相交
		if (exteriorRing.intersects(rect)) return true;
		for (size_t h = 0; h < interiorRings.size(); ++h) {
			if (interiorRings[h].intersects(rect)) return true;
		}

		// 3) 矩形角点是否在多边形实体内
		double minX = rect.getMinX(), maxX = rect.getMaxX();
		double minY = rect.getMinY(), maxY = rect.getMaxY();

		const Point corners[4] = {
			Point(minX, minY),
			Point(maxX, minY),
			Point(maxX, maxY),
			Point(minX, maxY)
		};

		for (int i = 0; i < 4; ++i) {
			double px = corners[i].getX();
			double py = corners[i].getY();
			if (pointInPolygonSolidInclusive(this, px, py)) return true;
		}

		// 4) 多边形外环顶点是否落在矩形内
		for (size_t i = 0; i < exteriorRing.numPoints(); ++i) {
			if (exteriorRing.getPointN(i).intersects(rect)) return true;
		}

		return false;
	}

	void Polygon::draw() const {
		exteriorRing.draw();
		for (size_t i = 0; i < interiorRings.size(); ++i) {
			interiorRings[i].draw();
		}
	}

	void Polygon::print() const {
		std::cout << "POLYGON((";
		for (size_t i = 0; i < exteriorRing.numPoints(); ++i) {
			if (i != 0) std::cout << ", ";
			Point p = exteriorRing.getPointN(i);
			std::cout << p.getX() << " " << p.getY();
		}
		std::cout << ")";

		for (size_t k = 0; k < interiorRings.size(); ++k) {
			std::cout << ", (";
			for (size_t i = 0; i < interiorRings[k].numPoints(); ++i) {
				if (i != 0) std::cout << ", ";
				Point p = interiorRings[k].getPointN(i);
				std::cout << p.getX() << " " << p.getY();
			}
			std::cout << ")";
		}
		std::cout << ")";
	}

} // namespace hw6
