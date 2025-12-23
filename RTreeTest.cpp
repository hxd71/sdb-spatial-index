#include "Common.h"
#include "RTree.h"
#include <ctime>
#include <cmath>
#include "CMakeIn.h"

using namespace hw6;

extern int mode;
extern std::vector<Geometry*> readGeom(const char* filename);
extern std::vector<std::string> readName(const char* filename);
extern void transformValue(double& res, const char* format);
extern void wrongMessage(Envelope e1, Envelope e2, bool cal);
extern void wrongMessage(const Point& pt1, const Point& pt2, double dis,
	double res);
extern void wrongMessage(Envelope e1, Envelope e2, Envelope cal, Envelope res);

namespace hw6 {

	void RTree::test(int t) {
		using namespace std;

		std::cout << "*********************Start*********************" << std::endl;
		if (t == TEST1) {
			std::cout << "TEST1: Envelope Contain, Intersect, and Union" << endl;

			int failedCase = 0;
			Envelope e1(-1, 1, -1, 1);
			vector<Envelope> tests;
			tests.push_back(Envelope(-0.5, 0.5, -0.5, 0.5));
			tests.push_back(Envelope(-0.5, 0.5, 0.5, 1.5));
			tests.push_back(Envelope(0.5, 1.5, -0.5, 0.5));
			tests.push_back(Envelope(-1.5, -0.5, -1.5, -0.5));
			tests.push_back(Envelope(-2, -1, -0.5, 0.5));
			tests.push_back(Envelope(1, 1.5, 1, 1.5));
			tests.push_back(Envelope(-2, -1.5, -0.5, 0.5));
			tests.push_back(Envelope(-0.5, 0.5, 1.5, 2));
			tests.push_back(Envelope(-2, -1.5, 0.5, 1.5));
			tests.push_back(Envelope(0.5, 1.5, 1.5, 2));

			for (size_t i = 0; i < tests.size(); ++i) {
				if (e1.contain(tests[i]) != (i == 0)) {
					failedCase += 1;
					wrongMessage(e1, tests[i], (i != 0));
				}
				if (tests[i].contain(e1) == true) {
					failedCase += 1;
					wrongMessage(tests[i], e1, true);
				}
			}
			cout << "Envelope Contain: " << tests.size() * 2 - failedCase << " / "
				<< tests.size() * 2 << " tests are passed" << endl;

			failedCase = 0;
			for (size_t i = 0; i < tests.size(); ++i) {
				if (e1.intersect(tests[i]) != (i < 6)) {
					failedCase += 1;
					wrongMessage(e1, tests[i], (i < 6));
				}
				if (tests[i].intersect(e1) != (i < 6)) {
					failedCase += 1;
					wrongMessage(tests[i], e1, (i < 6));
				}
			}
			cout << "Envelope Intersect: " << tests.size() * 2 - failedCase << " / "
				<< tests.size() * 2 << " tests are passed" << endl;

			failedCase = 0;
			vector<Envelope> results;
			results.push_back(Envelope(-1, 1, -1, 1));
			results.push_back(Envelope(-1, 1, -1, 1.5));
			results.push_back(Envelope(-1, 1.5, -1, 1));
			results.push_back(Envelope(-1.5, 1, -1.5, 1));
			results.push_back(Envelope(-2, 1, -1, 1));
			results.push_back(Envelope(-1, 1.5, -1, 1.5));
			results.push_back(Envelope(-2, 1, -1, 1));
			results.push_back(Envelope(-1, 1, -1, 2));
			results.push_back(Envelope(-2, 1, -1, 1.5));
			results.push_back(Envelope(-1, 1.5, -1, 2));
			for (size_t i = 0; i < tests.size(); ++i) {
				if (e1.unionEnvelope(tests[i]) != results[i]) {
					failedCase += 1;
					wrongMessage(e1, tests[i], e1.unionEnvelope(tests[i]),
						results[i]);
				}
				if (tests[i].unionEnvelope(e1) != results[i]) {
					failedCase += 1;
					wrongMessage(tests[i], e1, e1.unionEnvelope(tests[i]),
						results[i]);
				}
			}
			cout << "Envelope Union: " << tests.size() * 2 - failedCase << " / "
				<< tests.size() * 2 << " tests are passed" << endl;
		}
		else if (t == TEST2) {
			cout << "TEST2: Distance between Point and LineString" << endl;

			vector<Point> points;
			points.push_back(Point(0, 0));
			points.push_back(Point(10, 10));
			LineString line(points);

			points.push_back(Point(-10, -10));
			points.push_back(Point(20, 20));
			points.push_back(Point(5, 5));
			points.push_back(Point(10, 0));
			points.push_back(Point(10, -10));
			points.push_back(Point(0, 10));
			points.push_back(Point(0, 20));
			points.push_back(Point(20, 0));

			double dists[] = { 0,       0,       14.1421, 14.1421, 0,
							  7.07107, 14.1421, 7.07107, 14.1421, 14.1421 };

			int failedCase = 0;
			for (size_t i = 0; i < points.size(); ++i) {
				double dist = points[i].distance(&line);
				if (fabs(dist - dists[i]) > 0.0001) {
					failedCase += 1;
					cout << "Your answer is " << dist << " for test between ";
					line.print();
					cout << " and ";
					points[i].print();
					cout << ", but the answer is " << dists[i] << endl;
				}
			}
			cout << "Distance between Point and LineString: "
				<< points.size() - failedCase << " / " << points.size()
				<< " tests are passed" << endl;
		}
		else if (t == TEST3) {
			cout << "TEST3: Distance between Point and Polygon" << endl;

			const double EPS = 1e-5;

			int totalCases = 0;
			int failedCases = 0;

			cout << "[Part A] Distance between Point and Polygon" << endl;

			vector<Point> ringPts;
			ringPts.push_back(Point(5, 0));
			ringPts.push_back(Point(3, 6));
			ringPts.push_back(Point(2, 4));
			ringPts.push_back(Point(-2, 4));
			ringPts.push_back(Point(-3, 5));
			ringPts.push_back(Point(-5, 0));
			ringPts.push_back(Point(0, -3));
			ringPts.push_back(Point(5, 0));
			LineString ring(ringPts);
			Polygon poly(ring);

			vector<Point> testPts;
			testPts.push_back(Point(5, 4));
			testPts.push_back(Point(3, 4));
			testPts.push_back(Point(0, 4));
			testPts.push_back(Point(-3, 4));
			testPts.push_back(Point(-5, 4));
			testPts.push_back(Point(5, 5));
			testPts.push_back(Point(3, 5));
			testPts.push_back(Point(0, 5));
			testPts.push_back(Point(-3, 5));
			testPts.push_back(Point(0, 0));

			double expected[] = { 1.26491, 0, 0, 0, 1.48556, 1.58114, 0, 1, 0, 0 };

			for (size_t i = 0; i < testPts.size(); ++i) {
				totalCases++;
				double dist = testPts[i].distance(&poly);
				if (fabs(dist - expected[i]) > EPS) {
					failedCases++;
					cout << "FAILED [Part A, case " << i + 1 << "]: got " << dist
						<< ", expected " << expected[i] << " for point ";
					testPts[i].print();
					cout << " with polygon ";
					poly.print();
					cout << endl;
				}
			}

			cout << "[Part B] Distance between Point and Polygon with Interior Rings" << endl;

			// 外环: [0,10]x[0,10]
			vector<Point> exteriorPoints;
			exteriorPoints.push_back(Point(0, 0));
			exteriorPoints.push_back(Point(10, 0));
			exteriorPoints.push_back(Point(10, 10));
			exteriorPoints.push_back(Point(0, 10));
			exteriorPoints.push_back(Point(0, 0));
			LineString exteriorRing(exteriorPoints);

			// 孔洞1: [2,4]x[2,4]
			vector<Point> hole1Points;
			hole1Points.push_back(Point(2, 2));
			hole1Points.push_back(Point(4, 2));
			hole1Points.push_back(Point(4, 4));
			hole1Points.push_back(Point(2, 4));
			hole1Points.push_back(Point(2, 2));
			LineString hole1(hole1Points);

			// 孔洞2: [6,8]x[6,8]
			vector<Point> hole2Points;
			hole2Points.push_back(Point(6, 6));
			hole2Points.push_back(Point(8, 6));
			hole2Points.push_back(Point(8, 8));
			hole2Points.push_back(Point(6, 8));
			hole2Points.push_back(Point(6, 6));
			LineString hole2(hole2Points);

			vector<LineString> holes;
			holes.push_back(hole1);
			holes.push_back(hole2);

			Polygon polyWithHoles(exteriorRing, holes);

			vector<Point> testPoints;
			testPoints.push_back(Point(5, 5));     // 0: 在外环内，不在孔洞内
			testPoints.push_back(Point(3, 3));     // 1: 在孔洞1中心
			testPoints.push_back(Point(7, 7));     // 2: 在孔洞2中心
			testPoints.push_back(Point(1, 1));     // 3: 外环内 solid
			testPoints.push_back(Point(-1, 5));    // 4: 外环外左侧
			testPoints.push_back(Point(12, 5));    // 5: 外环外右侧
			testPoints.push_back(Point(5, -2));    // 6: 外环外下方
			testPoints.push_back(Point(5, 12));    // 7: 外环外上方
			testPoints.push_back(Point(2.5, 2));   // 8: 在孔洞1边界上
			testPoints.push_back(Point(3, 2.5));   // 9: 孔洞1内靠边
			testPoints.push_back(Point(0, 0));     // 10: 外环顶点
			testPoints.push_back(Point(9, 9));     // 11: 外环内靠角落

			double expectedDists[] = {0, 1, 1, 0, 1, 2, 2, 2, 0, 0.5, 0, 0};

			for (size_t i = 0; i < testPoints.size(); ++i) {
				totalCases++;
				double dist = testPoints[i].distance(&polyWithHoles);
				if (fabs(dist - expectedDists[i]) > EPS) {
					failedCases++;
					cout << "FAILED [Part B, case " << i + 1 << "]: got " << dist
						<< ", expected " << expectedDists[i] << " for point ";
					testPoints[i].print();
					cout << " with polygon-with-holes" << endl;
				}
			}

			cout << "Distance between Point and Polygon: "
				<< (totalCases - failedCases) << " / " << totalCases
				<< " tests are passed" << endl;
		}
		else if (t == TEST4) {
			cout << "TEST4: Distance between LineString and Polygon" << endl;

			const double EPS = 1e-5;

			auto makeRectRing = [&](double minX, double minY, double maxX, double maxY) -> LineString {
				vector<Point> pts;
				pts.push_back(Point(minX, minY));
				pts.push_back(Point(maxX, minY));
				pts.push_back(Point(maxX, maxY));
				pts.push_back(Point(minX, maxY));
				pts.push_back(Point(minX, minY));
				LineString ring(pts);
				return ring;
			};

			auto makeLine2 = [&](double x1, double y1, double x2, double y2) -> LineString {
				vector<Point> pts;
				pts.push_back(Point(x1, y1));
				pts.push_back(Point(x2, y2));
				LineString line(pts);
				return line;
			};

			// -------- polygon with holes: outer [0,10]x[0,10], hole1 [2,4]x[2,4], hole2 [6,8]x[6,8]
			LineString outer = makeRectRing(0, 0, 10, 10);
			LineString hole1 = makeRectRing(2, 2, 4, 4);
			LineString hole2 = makeRectRing(6, 6, 8, 8);
			vector<LineString> holes;
			holes.push_back(hole1);
			holes.push_back(hole2);
			Polygon poly(outer, holes);

			// -------- test lines
			vector<LineString> lines;
			vector<double> expected;

			// 0) 完全在实体内（solid），距离=0
			lines.push_back(makeLine2(1, 1, 1, 9));
			expected.push_back(0.0);

			// 1) 穿过外环边界进入实体，距离=0
			lines.push_back(makeLine2(-1, 5, 1, 5));
			expected.push_back(0.0);

			// 2) 完全在外部左侧，最近到外环 x=0 的距离=1
			lines.push_back(makeLine2(-1, 5, -1, 6));
			expected.push_back(1.0);

			// 3) 完全在孔洞1内部（不触边），到孔洞边界最近距离=0.5
			lines.push_back(makeLine2(2.5, 2.5, 3.5, 3.5));
			expected.push_back(0.5);

			// 4) 贴在孔洞1边界上（y=2），距离=0
			lines.push_back(makeLine2(2.5, 2.0, 3.5, 2.0));
			expected.push_back(0.0);

			// 5) 从孔洞1内部跨到实体（穿过孔洞边界），距离=0
			lines.push_back(makeLine2(3, 3, 1, 3));
			expected.push_back(0.0);

			// 6) 两端在实体，但线穿过孔洞1（触孔洞边界两次），距离=0
			lines.push_back(makeLine2(1, 3, 5, 3));
			expected.push_back(0.0);

			// 7) 沿外环边界（x=0），距离=0
			lines.push_back(makeLine2(0, 1, 0, 9));
			expected.push_back(0.0);

			int failed = 0;
			for (size_t i = 0; i < lines.size(); ++i) {
				double dist = lines[i].distance(&poly);
				if (fabs(dist - expected[i]) > EPS) {
					failed++;
					cout << "Case " << i + 1 << " failed: expected " << expected[i]
						<< ", got " << dist << " for line ";
					lines[i].print();
					cout << endl;
				}
			}

			cout << "Distance between LineString and Polygon: "
				<< (lines.size() - failed) << " / " << lines.size()
				<< " tests are passed" << endl;
		}
		else if (t == TEST5) {
			cout << "TEST5: Distance between Polygon and Polygon" << endl;

			const double EPS = 1e-6;

			auto makeRectRing = [&](double minX, double minY, double maxX, double maxY) -> LineString {
				vector<Point> pts;
				pts.push_back(Point(minX, minY));
				pts.push_back(Point(maxX, minY));
				pts.push_back(Point(maxX, maxY));
				pts.push_back(Point(minX, maxY));
				pts.push_back(Point(minX, minY));
				LineString ring(pts);
				return ring;
			};

			auto checkDist = [&](const string& name, Polygon& A, Polygon& B, double expected) -> int {
				double d1 = A.distance(&B);
				double d2 = B.distance(&A);

				bool ok1 = (fabs(d1 - expected) <= EPS);
				bool ok2 = (fabs(d2 - expected) <= EPS);
				bool sym = (fabs(d1 - d2) <= EPS);

				if (!ok1 || !ok2 || !sym) {
					cout << "[FAILED] " << name << endl;
					cout << "  expected: " << expected << endl;
					cout << "  A.distance(B) = " << d1 << endl;
					cout << "  B.distance(A) = " << d2 << endl;
					cout << "  symmetric? " << (sym ? "true" : "false") << endl;
					return 1;
				}
				return 0;
			};

			int failedCase = 0;

			// ========= 构造一个带孔洞的多边形 P1 =========
			LineString outer1 = makeRectRing(0, 0, 10, 10);

			LineString hole1 = makeRectRing(2, 2, 4, 4);
			LineString hole2 = makeRectRing(6, 6, 8, 8);

			vector<LineString> holes1;
			holes1.push_back(hole1);
			holes1.push_back(hole2);

			Polygon P1(outer1, holes1);

			// ========= Case0: 远离（右侧），距离=20-10=10 =========
			LineString ring_far = makeRectRing(20, 0, 30, 10);
			Polygon P_far(ring_far);
			failedCase += checkDist("Case0: far apart (expect 10)", P1, P_far, 10.0);

			// ========= Case1: 与外环右边界触碰（x=10），距离=0 =========
			LineString ring_touchOuter = makeRectRing(10, 3, 12, 5);
			Polygon P_touchOuter(ring_touchOuter);
			failedCase += checkDist("Case1: touch exterior boundary (expect 0)", P1, P_touchOuter, 0.0);

			// ========= Case2: 与外环重叠相交，距离=0 =========
			LineString ring_overlap = makeRectRing(8, 8, 12, 12);
			Polygon P_overlap(ring_overlap);
			failedCase += checkDist("Case2: overlap (expect 0)", P1, P_overlap, 0.0);

			// ========= Case3: 完全在外环 solid 区域内部（不触边），理论距离=0 =========
			// 如果此项失败，说明当前实现没有处理“包含/相交 => 距离0”的情况（只算边界距离）
			LineString ring_insideSolid = makeRectRing(1, 1, 2, 2);
			Polygon P_insideSolid(ring_insideSolid);
			failedCase += checkDist("Case3: contained in solid (expect 0)", P1, P_insideSolid, 0.0);

			// ========= Case4: 完全在孔洞1内部（不触孔洞边界），距离=0.5 =========
			LineString ring_insideHole = makeRectRing(2.5, 2.5, 3.5, 3.5);
			Polygon P_insideHole(ring_insideHole);
			failedCase += checkDist("Case4: contained in hole1 (expect 0.5)", P1, P_insideHole, 0.5);

			// ========= Case5: 正好等于孔洞1区域（边界重合），距离=0 =========
			LineString ring_equalHole = makeRectRing(2, 2, 4, 4);
			Polygon P_equalHole(ring_equalHole);
			failedCase += checkDist("Case5: equal to hole1 boundary (expect 0)", P1, P_equalHole, 0.0);

			// ========= Case6: 无孔洞 polygon 的包含关系，理论距离=0 =========
			LineString ring_Q1 = makeRectRing(0, 0, 10, 10);
			Polygon Q1(ring_Q1);

			LineString ring_Q2 = makeRectRing(1, 1, 2, 2);
			Polygon Q2(ring_Q2);

			failedCase += checkDist("Case6: simple containment no holes (expect 0)", Q1, Q2, 0.0);

			int total = 7;
			cout << "Distance between Polygon and Polygon: "
				<< (total - failedCase) << " / " << total
				<< " tests are passed" << endl;
		}
		else if (t == TEST6) {
			cout << "TEST6: Intersect between Polygon and Envelope" << endl;

			// 创建一个带孔洞的多边形
			// 外环: 矩形 (0,0) -> (10,0) -> (10,10) -> (0,10) -> (0,0)
			vector<Point> exteriorPoints;
			exteriorPoints.push_back(Point(0, 0));
			exteriorPoints.push_back(Point(10, 0));
			exteriorPoints.push_back(Point(10, 10));
			exteriorPoints.push_back(Point(0, 10));
			exteriorPoints.push_back(Point(0, 0));
			LineString exteriorRing(exteriorPoints);

			// 内环1: 矩形 (2,2) -> (4,2) -> (4,4) -> (2,4) -> (2,2)
			vector<Point> hole1Points;
			hole1Points.push_back(Point(2, 2));
			hole1Points.push_back(Point(4, 2));
			hole1Points.push_back(Point(4, 4));
			hole1Points.push_back(Point(2, 4));
			hole1Points.push_back(Point(2, 2));
			LineString hole1(hole1Points);

			// 内环2: 矩形 (6,6) -> (8,6) -> (8,8) -> (6,8) -> (6,6)
			vector<Point> hole2Points;
			hole2Points.push_back(Point(6, 6));
			hole2Points.push_back(Point(8, 6));
			hole2Points.push_back(Point(8, 8));
			hole2Points.push_back(Point(6, 8));
			hole2Points.push_back(Point(6, 6));
			LineString hole2(hole2Points);

			vector<LineString> holes;
			holes.push_back(hole1);
			holes.push_back(hole2);

			Polygon polyWithHoles(exteriorRing, holes);

			// 测试带孔洞多边形与Envelope的相交判断
			vector<Envelope> testEnvelopes = {
				Envelope(-2, -1, 5, 6),         // 0: 完全在外环外（左侧远离） -> false
				Envelope(11, 12, 5, 6),         // 1: 完全在外环外（右侧远离） -> false

				Envelope(0.5, 1.5, 0.5, 1.5),   // 2: 完全在外环内 solid 区域 -> true

				Envelope(2.5, 3.5, 2.5, 3.5),   // 3: 完全在孔洞1内部（不触边） -> false
				Envelope(6.5, 7.5, 6.5, 7.5),   // 4: 完全在孔洞2内部（不触边） -> false

				Envelope(-1, 1, -1, 1),         // 5: 穿过/接触外环边界（左下角附近） -> true
				Envelope(10, 12, 5, 6),         // 6: 恰好“贴着”外环右边界 x=10（触边） -> true

				Envelope(1, 3, 1, 3),           // 7: 覆盖 solid + 孔洞1一部分（有实体交集） -> true

				Envelope(2.5, 3.5, 2.0, 3.0),   // 8: 在孔洞1内但“触到孔洞边界 y=2”（触边） -> true (OGC)
				Envelope(2, 4, 2, 4),           // 9: 正好等于孔洞1（边界完全重合） -> true (OGC)

				Envelope(0, 10, 0, 10),         // 10: 正好等于外环包围矩形（覆盖实体+孔洞） -> true
				Envelope(-1, 11, -1, 11)        // 11: 完全覆盖整个多边形（rect contains polygon） -> true
			};

			bool expectedIntersects[] = {
				false,
				false,
				true,
				false,
				false,
				true,
				true,
				true,
				true,
				true,
				true,
				true 
			};

			int failedCase = 0;
			for (size_t i = 0; i < testEnvelopes.size(); ++i) {
				bool intersects = polyWithHoles.intersects(testEnvelopes[i]);
				if (intersects != expectedIntersects[i]) {
					failedCase++;
					cout << "Test case " << i + 1 << " failed: ";
					testEnvelopes[i].print();
					cout << " with polygon, expected " << expectedIntersects[i]
						<< ", got " << intersects << endl;
				}
			}
			cout << "Intersect between Polygon and Envelope: "
				<< testEnvelopes.size() - failedCase << " / " << testEnvelopes.size()
				<< " tests are passed" << endl;
		}
		else if (t == TEST7) {
			cout << "TEST7: RTree Construction" << endl;
			int ncase, cct;
			ncase = cct = 2;

			RTree rtree(8);
			vector<Geometry*> geom = readGeom(PROJ_SRC_DIR "/data/station");
			vector<Feature> features;

			for (size_t i = 0; i < geom.size(); ++i)
				features.push_back(Feature("", geom[i]));

			rtree.constructTree(features);

			int height, interiorNum, leafNum;
			rtree.countHeight(height);
			rtree.countNode(interiorNum, leafNum);

			if (!(height == 4 && interiorNum == 18 && leafNum == 86)) {
				cout << "Case 1: "
					<< "Your answer is height: " << height
					<< ", interiorNum: " << interiorNum << ", leafNum: " << leafNum
					<< ". One possible answer is height: 4, interiorNum: "
					"18, "
					"leafNum: 86\n";
				--cct;
			}

			features.clear();
			for (size_t i = 0; i < geom.size(); ++i)
				delete geom[i];
			geom.clear();

			vector<Geometry*> geom2 = readGeom(PROJ_SRC_DIR "/data/highway");
			vector<Feature> features2;
			RTree rtree2(8);

			for (size_t i = 0; i < geom2.size(); ++i)
				features2.push_back(Feature("", geom2[i]));

			rtree2.constructTree(features2);

			int height2, interiorNum2, leafNum2;
			rtree2.countHeight(height2);
			rtree2.countNode(interiorNum2, leafNum2);

			if (!(height2 == 6 && interiorNum2 == 484 && leafNum2 == 2305)) {
				cout << "Case 2: "
					<< "Your answer is height: " << height2
					<< ", interiorNum: " << interiorNum2
					<< ", leafNum: " << leafNum2
					<< ". One possible answer is height: 6, interiorNum: "
					"484, leafNum: 2305\n";
				--cct;
			}

			features2.clear();
			for (size_t i = 0; i < geom2.size(); ++i)
				delete geom2[i];
			geom2.clear();

			//cout << "RTree Construction: " << cct << " / " << ncase
			//     << " tests are passed" << endl;
		}
		else if (t == TEST8) {
			cout << "TEST8: RTreeAnalysis" << endl;
			analyse();
		}

		cout << "**********************End**********************" << endl;
	}

	void forConstCapAnalyseRTree(const std::vector<Feature>& features, int childNum, int maxNum, int step) {
		if (childNum <= maxNum) {
			RTree rtree(childNum);
			rtree.constructTree(features);

			// TODO 与四叉树进行比较

			forConstCapAnalyseRTree(features, childNum + step, maxNum, step);
		}
	}

	void RTree::analyse() {
		using namespace std;

		vector<Feature> features;
		vector<Geometry*> geom = readGeom(PROJ_SRC_DIR "/data/taxi");
		vector<string> name = readName(PROJ_SRC_DIR "/data/taxi");

		features.clear();
		features.reserve(geom.size());
		for (size_t i = 0; i < geom.size(); ++i)
			features.push_back(Feature(name[i], geom[i]));

		cout << "taxi number: " << geom.size() << endl;

		srand(time(nullptr));

		/*TODO:实现forConstCapAnalyseRTree */
		forConstCapAnalyseRTree(features, 70, 200, 10);
	}


} // namespace hw6
