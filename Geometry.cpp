#include "Geometry.h"
#include <cmath>
#include <gl/freeglut.h>

#define NOT_IMPLEMENT -1.0

namespace hw6 {

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
		double mindist = line->getPointN(0).distance(this);
		for (size_t i = 0; i < line->numPoints() - 1; ++i) {
			double dist = 0;
			double x1 = line->getPointN(i).getX();
			double y1 = line->getPointN(i).getY();
			double x2 = line->getPointN(i + 1).getX();
			double y2 = line->getPointN(i + 1).getY();
			// Task calculate the distance between Point P(x, y) and Line [P1(x1,
			// y1), P2(x2, y2)] (less than 10 lines)
			double dx = x2 - x1;
			double dy = y2 - y1;
			double len2 = dx * dx + dy * dy;
			double t = 0;
			if (len2 > 0) {
				t = ((x - x1) * dx + (y - y1) * dy) / len2;
				t = std::max(0.0, std::min(1.0, t));
			}
			double projX = x1 + t * dx;
			double projY = y1 + t * dy;
			dist = sqrt((x - projX) * (x - projX) + (y - projY) * (y - projY));

			if (dist < mindist)
				mindist = dist;
		}
		return mindist;
	}

	double Point::distance(const Polygon* polygon) const {
		LineString line = polygon->getExteriorRing();
		size_t n = line.numPoints();

		bool inPolygon = false;
		// Task whether Point P(x, y) is within Polygon (less than 15 lines)
		int cnt = 0;
		for (size_t i = 0,j = n - 1; i < n; j = i++) {
			double xi = line.getPointN(i).getX();
			double yi = line.getPointN(i).getY();
			double xj = line.getPointN(j).getX();
			double yj = line.getPointN(j).getY();
			if (((yi > y) != (yj > y)) &&
				(x < (xj - xi) * (y - yi) / (yj - yi) + xi))
				cnt++;
		}

		if(cnt % 2 == 1)
			inPolygon = true;

		double mindist = 0;
		if (!inPolygon)
			mindist = this->distance(&line);
		return mindist;
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
		double minDist = this->getPointN(0).distance(&line->getPointN(0));
		size_t n1 = this->numPoints();
		size_t n2 = line->numPoints();
		if (n1 < 2 || n2 < 2)
			return NOT_IMPLEMENT;
		for (size_t i = 0; i < n1 - 1; ++i) {
			const Point& p1 = this->getPointN(i);
			const Point& p2 = this->getPointN(i + 1);
			for (size_t j = 0; j < n2 - 1; ++j) {
				const Point& q1 = line->getPointN(j);
				const Point& q2 = line->getPointN(j + 1);
				// calculate the distance between Line [P1(x1, y1), P2(x2, y2)]
				double d1 = p1.distance(&q1);
				double d2 = p1.distance(&q2);
				double d3 = p2.distance(&q1);
				double d4 = p2.distance(&q2);
				double d5 = p1.distance(new LineString(std::vector<Point>{q1, q2}));
				double d6 = p2.distance(new LineString(std::vector<Point>{q1, q2}));
				double d7 = q1.distance(new LineString(std::vector<Point>{p1, p2}));
				double d8 = q2.distance(new LineString(std::vector<Point>{p1, p2}));
				double dist = std::min({ d1, d2, d3, d4, d5, d6, d7, d8 });
				if (dist < minDist)
					minDist = dist;
			}
		}
		return minDist;
	}

	double LineString::distance(const Polygon* polygon) const {
		// 1. 取Polygon的外环
		LineString exterior = polygon->getExteriorRing();
		// 2. 计算本LineString到外环的距离
		double dist1 = this->distance(&exterior);
		// 3. 计算外环到本LineString的距离
		double dist2 = exterior.distance(this);
		// 4. 返回较小值
		return std::min(dist1, dist2);
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
		return std::min(exteriorRing.distance(polygon),
			polygon->getExteriorRing().distance(this));
	}

	bool Polygon::intersects(const Envelope& rect) const {
		// TODO
		return true;
	}

	void Polygon::draw() const { exteriorRing.draw(); }

	void Polygon::print() const {
		std::cout << "Polygon(";
		for (size_t i = 0; i < exteriorRing.numPoints(); ++i) {
			if (i != 0)
				std::cout << ", ";
			Point p = exteriorRing.getPointN(i);
			std::cout << p.getX() << " " << p.getY();
		}
		std::cout << ")";
	}

} // namespace hw6
