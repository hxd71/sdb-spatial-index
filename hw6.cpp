// hw6.cpp : 定义控制台应用程序的入口点。
//

#include "Common.h"
#include "Geometry.h"
#include "shapelib/shapefil.h"

#include "CMakeIn.h" // 配置注入

#include <gl/freeglut.h> // GLUT库头文件

#include <cstdio>
#include <ctime>
#include <iostream>
#include <list>
#include <map>
#include <memory>
#include <vector>

#ifdef USE_RTREE
#include "RTree.h"
using TreeTy = hw6::RTree;
#else
#include "QuadTree.h"
using TreeTy = hw6::QuadTree;
#endif


using namespace std;

int screenWidth = 640;
int screenHeight = 480;

double pointSize = 2.0;

int mode;

// 数据容器
vector<hw6::Feature> features; // 点数据
vector<hw6::Feature> roads;    // 线数据
vector<hw6::Feature> polygons; // 新增：多边形数据

bool showRoad = true;
bool showPolygon = true;       // 新增：多边形显示开关
bool showTree = false;

// 索引树
unique_ptr<hw6::Tree> pointTree;
unique_ptr<hw6::Tree> roadTree;
unique_ptr<hw6::Tree> polygonTree; // 新增：多边形索引

// 交互与查询结果
hw6::Feature nearestFeature;
bool firstPoint = true;
hw6::Point corner[2];
hw6::Envelope selectedRect;
vector<hw6::Feature> selectedFeatures;

int kValue = 5; // k-NN 的 k 值

/*
 * shapefile文件中name和geometry属性读取
 */
vector<string> readName(const char* filename) {
	DBFHandle file = DBFOpen(filename, "r");

	// --- 修复开始：增加空指针检查 ---
	if (file == NULL) {
		cout << "Error: Failed to open DBF file: " << filename << ".dbf" << endl;
		return vector<string>(); // 返回空向量，避免崩溃
	}
	// --- 修复结束 ---

	vector<string> res;
	int cct = DBFGetRecordCount(file);
	res.reserve(cct);
	for (int i = 0; i < cct; ++i) {
		string a = DBFReadStringAttribute(file, i, 0);
		res.push_back(a);
	}

	DBFClose(file);

	return res;
}

vector<hw6::Geometry*> readGeom(const char* filename) {
	SHPHandle file = SHPOpen(filename, "r");

	// --- 修复开始：增加空指针检查 ---
	if (file == NULL) {
		cout << "Error: Failed to open SHP file: " << filename << ".shp" << endl;
		return vector<hw6::Geometry*>(); // 返回空向量
	}
	// --- 修复结束 ---

	int pnEntities, pnShapeType;
	double padfMinBound[4], padfMaxBound[4];
	SHPGetInfo(file, &pnEntities, &pnShapeType, padfMinBound, padfMaxBound);

	vector<hw6::Point> points;
	vector<hw6::Geometry*> geoms;
	geoms.reserve(pnEntities);
	switch (pnShapeType) {
	case SHPT_POINT:
		for (int i = 0; i < pnEntities; ++i) {
			SHPObject* pt = SHPReadObject(file, i);
			geoms.push_back(new hw6::Point(pt->padfY[0], pt->padfX[0]));
			SHPDestroyObject(pt);
		}
		break;

	case SHPT_ARC:
		for (int i = 0; i < pnEntities; ++i) {
			points.clear();
			SHPObject* pt = SHPReadObject(file, i);
			for (int j = 0; j < pt->nVertices; ++j) {
				points.push_back(hw6::Point(pt->padfY[j], pt->padfX[j]));
			}
			SHPDestroyObject(pt);
			geoms.push_back(new hw6::LineString(points));
		}
		break;

	case SHPT_POLYGON:
		for (int i = 0; i < pnEntities; ++i) {
			points.clear();
			SHPObject* pt = SHPReadObject(file, i);
			for (int j = 0; j < pt->nVertices; ++j) {
				points.push_back(hw6::Point(pt->padfY[j], pt->padfX[j]));
			}
			SHPDestroyObject(pt);
			hw6::LineString line(points);
			hw6::Polygon* poly = new hw6::Polygon(line);
			geoms.push_back(new hw6::Polygon(line));
		}
		break;
	}

	SHPClose(file);
	return geoms;
}

void transformValue(double& res, const char* format = "%.2lf") {
	char buf[20];
	sprintf(buf, format, res);
	sscanf(buf, "%lf", &res);
}

void wrongMessage(hw6::Envelope e1, hw6::Envelope e2, bool cal) {
	cout << "Your answer is " << cal << " for test between ";
	e1.print();
	cout << " and ";
	e2.print();
	cout << ", but the answer is " << !cal << endl;
}

void wrongMessage(const hw6::Point& pt1, const hw6::Point& pt2, double dis,
	double res) {
	cout << "Your answer is " << dis << " for test ";
	pt1.print();
	cout << " and ";
	pt2.print();
	cout << ", but the answer is " << res << endl;
}

void wrongMessage(hw6::Envelope e1, hw6::Envelope e2, hw6::Envelope cal,
	hw6::Envelope res) {
	cout << "Your answer is ";
	cal.print();
	cout << " for test between ";
	e1.print();
	cout << " and ";
	e2.print();
	cout << ", but the answer is ";
	res.print();
	cout << endl;
}

/*
 * 输出几何信息
 */
void printGeom(vector<hw6::Geometry*>& geom) {
	cout << "Geometry:" << endl;
	for (vector<hw6::Geometry*>::iterator it = geom.begin(); it != geom.end();
		++it) {
		(*it)->print();
	}
}

/*
 * 删除几何信息
 */
void deleteGeom(vector<hw6::Geometry*>& geom) {
	for (vector<hw6::Geometry*>::iterator it = geom.begin(); it != geom.end();
		++it) {
		delete* it;
		*it = NULL;
	}
	geom.clear();
}

// --- 新增：加载多边形数据 ---
void loadPolygonData() {
	// 1. 读取几何数据 (.shp)
	// 只要 polygon.shp 存在，readGeom 就能正常工作
	vector<hw6::Geometry*> geom = readGeom(PROJ_SRC_DIR "/data/polygon");

	// 2. 注释掉 readName
	// 因为 data/polygon.dbf 不存在，调用此函数会导致崩溃或返回空
	// vector<string> name = readName(PROJ_SRC_DIR "/data/polygon"); 

	polygons.clear();

	if (geom.empty()) {
		cout << "Warning: No polygon geometry found." << endl;
		return;
	}

	// 3. 构建 Feature，使用默认名称
	for (size_t i = 0; i < geom.size(); ++i) {
		// 由于没有 .dbf 文件提供真实名称，我们使用索引生成默认名称
		string defaultName = "Polygon_" + to_string(i);

		polygons.push_back(hw6::Feature(defaultName, geom[i]));
	}

	cout << "polygon number: " << geom.size() << endl;

	// 4. 构建多边形四叉树索引
	if (polygonTree) polygonTree.reset();
	polygonTree = make_unique<TreeTy>(10); // 设置容量
	polygonTree->constructTree(polygons);
}

/*
 * 读取纽约道路数据
 */
void loadRoadData() {
	vector<hw6::Geometry*> geom = readGeom(PROJ_SRC_DIR "/data/highway");

	roads.clear();
	for (size_t i = 0; i < geom.size(); ++i)
		roads.push_back(hw6::Feature(to_string(i), geom[i]));

	cout << "road number: " << geom.size() << endl;
	roadTree->setCapacity(20);
	roadTree->constructTree(roads);
}

/*
 * 读取纽约自行车租赁点数据
 */
void loadStationData() {
	vector<hw6::Geometry*> geom = readGeom(PROJ_SRC_DIR "/data/station");
	vector<string> name = readName(PROJ_SRC_DIR "/data/station");

	features.clear();
	for (size_t i = 0; i < geom.size(); ++i)
		features.push_back(hw6::Feature(name[i], geom[i]));

	cout << "station number: " << geom.size() << endl;
	pointTree->setCapacity(5);
	pointTree->constructTree(features);
}

/*
 * 读取纽约出租车打车点数据
 */
void loadTaxiData() {
	vector<hw6::Geometry*> geom = readGeom(PROJ_SRC_DIR "/data/taxi");
	vector<string> name = readName(PROJ_SRC_DIR "/data/taxi");

	features.clear();
	for (size_t i = 0; i < geom.size(); ++i)
		features.push_back(hw6::Feature(name[i], geom[i]));

	cout << "taxi number: " << geom.size() << endl;
	pointTree->setCapacity(100);
	pointTree->constructTree(features);
}

/*
 * 区域查询
 */
 // --- 修改：区域查询 ---
void rangeQuery() {
	vector<hw6::Feature> candidateFeatures;

	if (mode == RANGEPOINT)
		pointTree->rangeQuery(selectedRect, candidateFeatures);
	else if (mode == RANGELINE)
		roadTree->rangeQuery(selectedRect, candidateFeatures);
	else if (mode == RANGEPOLYGON) // 处理多边形
		polygonTree->rangeQuery(selectedRect, candidateFeatures);

	selectedFeatures.clear();
	for (const auto& feature : candidateFeatures) {
		// 利用 Geometry 多态调用 intersects
		if (feature.getGeom()->intersects(selectedRect)) {
			selectedFeatures.push_back(feature);
		}
	}
}

// --- 修改：邻近查询 ---
void NNQuery(hw6::Point p) {
	vector<hw6::Feature> candidateFeatures;
	hw6::Tree* currentTree = nullptr;

	if (mode == NNPOINT) currentTree = pointTree.get();
	else if (mode == NNLINE) currentTree = roadTree.get();
	else if (mode == NNPOLYGON) currentTree = polygonTree.get(); // 处理多边形

	if (!currentTree) return;

	currentTree->NNQuery(p.getX(), p.getY(), candidateFeatures);

	double minDist = std::numeric_limits<double>::max();
	nearestFeature = hw6::Feature();

	for (const auto& feature : candidateFeatures) {
		// 利用 Geometry 多态调用 distance
		double dist = p.distance(feature.getGeom());
		if (dist < minDist) {
			minDist = dist;
			nearestFeature = feature;
		}
	}
}

// --- 新增：k-NN 查询 ---
void KNNQuery(hw6::Point p) {
	// 这里演示对 点数据 (Stations) 进行 k-NN
	// 如果想对多边形 k-NN，只需把 tree 指针换成 polygonTree.get()

	// 需要将基类 Tree 指针转换为 QuadTree (TreeTy) 指针才能调用 kNNQuery
	TreeTy* qTree = dynamic_cast<TreeTy*>(pointTree.get());

	if (qTree) {
		selectedFeatures.clear(); // 复用 selectedFeatures 存储结果
		qTree->kNNQuery(p.getX(), p.getY(), kValue, selectedFeatures);

		// 控制台输出结果调试
		// cout << "k-NN Results (k=" << kValue << "):" << endl;
		// for(const auto& f : selectedFeatures) {
		//    cout << " - " << f.getName() << endl;
		// }
	}
}

/*
 * 从屏幕坐标转换到地理坐标
 */
void transfromPt(hw6::Point& pt) {
	const hw6::Envelope bbox = pointTree->getEnvelope();
	double width = bbox.getMaxX() - bbox.getMinX() + 0.002;
	double height = bbox.getMaxY() - bbox.getMinY() + 0.002;

	double x = pt.getX() * width / screenWidth + bbox.getMinX() - 0.001;
	double y = pt.getY() * height / screenHeight + bbox.getMinY() - 0.001;

	x = max(bbox.getMinX(), x);
	x = min(bbox.getMaxX(), x);
	y = max(bbox.getMinY(), y);
	y = min(bbox.getMaxY(), y);
	pt = hw6::Point(x, y);
}

/*
 * 绘制代码
 */
void display() {
	glClearColor(1.0, 1.0, 1.0, 0.0);
	glClear(GL_COLOR_BUFFER_BIT);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	// ... (设置视口 glOrtho 代码保持不变) ...
	// 确定视口范围
	const hw6::Envelope* bbox = nullptr;
	if (pointTree && pointTree->getEnvelope().getWidth() > 0) bbox = &pointTree->getEnvelope();
	else if (polygonTree && polygonTree->getEnvelope().getWidth() > 0) bbox = &polygonTree->getEnvelope();

	if (bbox) {
		glOrtho(bbox->getMinX(), bbox->getMaxX(), bbox->getMinY(), bbox->getMaxY(), -1.0, 1.0);
	}
	else {
		glOrtho(-74.05, -73.90, 40.70, 40.85, -1.0, 1.0); // 默认纽约范围
	}

	// 绘制多边形
	if (showPolygon) {
		glColor3d(0.8, 0.8, 0.8); // 灰色填充
		for (const auto& poly : polygons) poly.draw();
	}

	// 绘制道路
	if (showRoad) {
		glColor3d(252 / 255.0, 214 / 255.0, 164 / 255.0);
		for (const auto& road : roads) road.draw();
	}

	// 绘制点
	if (!(mode == RANGELINE || mode == NNLINE || mode == RANGEPOLYGON || mode == NNPOLYGON)) {
		glPointSize((float)pointSize);
		glColor3d(0.0, 146 / 255.0, 247 / 255.0);
		for (const auto& feat : features) feat.draw();
	}

	// --- 插入开始：四叉树绘制逻辑 ---
	if (showTree) {
		glLineWidth(1.0);
		glColor3d(0.0, 0.6, 0.0); // 使用绿色或深蓝色绘制树的边界

		// 根据当前模式决定画哪棵树
		if (mode == RANGELINE || mode == NNLINE) {
			if (roadTree) roadTree->draw();
		}
		else if (mode == RANGEPOLYGON || mode == NNPOLYGON) {
			if (polygonTree) polygonTree->draw();
		}
		else {
			// 默认模式、点模式、KNN模式都画点树
			if (pointTree) pointTree->draw();
		}
	}
	// --- 插入结束 ---

	// 绘制高亮选区或结果 (Range 或 k-NN)
	if (mode == RANGEPOINT || mode == RANGELINE || mode == RANGEPOLYGON || mode == KNN) {
		// ... (保持不变) ...
		if (mode != KNN) {
			glColor3d(0.0, 0.0, 0.0);
			selectedRect.draw();
		}
		// ...
		glColor3d(1.0, 0.0, 0.0);
		glLineWidth(2.0);
		if (mode == KNN) glPointSize(6.0);
		for (const auto& f : selectedFeatures) f.draw();
		glLineWidth(1.0);
		glPointSize((float)pointSize);
	}

	// 绘制最近邻 (NN)
	if (mode == NNPOINT || mode == NNLINE || mode == NNPOLYGON) {
		// ... (保持不变) ...
		if (mode == NNPOINT) glPointSize(6.0);
		else glLineWidth(3.0);
		glColor3d(0.9, 0.0, 0.0);
		nearestFeature.draw();
		glLineWidth(1.0);
		glPointSize((float)pointSize);
	}

	glFlush();
	glutSwapBuffers();
}

/*
 * 鼠标和键盘交互
 */
void mouse(int button, int state, int x, int y) {
	if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
		if (mode == RANGEPOINT || mode == RANGELINE || mode == RANGEPOLYGON) {
			if (firstPoint) {
				selectedFeatures.clear();
				corner[0] = hw6::Point(x, screenHeight - y);
				transfromPt(corner[0]);
			}
			else {
				corner[1] = hw6::Point(x, screenHeight - y);
				transfromPt(corner[1]);
				selectedRect = hw6::Envelope(
					min(corner[0].getX(), corner[1].getX()),
					max(corner[0].getX(), corner[1].getX()),
					min(corner[0].getY(), corner[1].getY()),
					max(corner[0].getY(), corner[1].getY()));
				rangeQuery();
			}
			firstPoint = !firstPoint;
			glutPostRedisplay();
		}
	}
}

void passiveMotion(int x, int y) {
	hw6::Point p(x, screenHeight - y);

	// Range Query 预览
	if ((mode == RANGEPOINT || mode == RANGELINE || mode == RANGEPOLYGON) && !firstPoint) {
		corner[1] = p;
		transfromPt(corner[1]);
		selectedRect = hw6::Envelope(
			min(corner[0].getX(), corner[1].getX()),
			max(corner[0].getX(), corner[1].getX()),
			min(corner[0].getY(), corner[1].getY()),
			max(corner[0].getY(), corner[1].getY()));
		rangeQuery();
		glutPostRedisplay();
	}
	// NN Query 实时
	else if (mode == NNPOINT || mode == NNLINE || mode == NNPOLYGON) {
		transfromPt(p);
		NNQuery(p);
		glutPostRedisplay();
	}
	// k-NN Query 实时
	else if (mode == KNN) {
		transfromPt(p);
		KNNQuery(p);
		glutPostRedisplay();
	}
}


void changeSize(int w, int h) {
	screenWidth = w;
	screenHeight = h;
	glViewport(0, 0, w, h);
	glutPostRedisplay();
}

void processNormalKeys(unsigned char key, int x, int y) {
	switch (key) {
	case 27:
		exit(0);
		break;
	case 'N':
		mode = NNLINE;
		break;
	case 'n':
		mode = NNPOINT;
		break;
	case 'S':
		mode = RANGELINE;
		firstPoint = true;
		break;
	case 's':
		mode = RANGEPOINT;
		firstPoint = true;
		break;
	// Polygon Modes (新增按键 P/p)
	case 'p': mode = RANGEPOLYGON; firstPoint = true; cout << "Mode: Polygon Range" << endl; break;
	case 'P': mode = NNPOLYGON; cout << "Mode: Polygon NN" << endl; break;
	// k-NN Mode (新增按键 K/k)
	case 'k':
	case 'K':
		mode = KNN;
		cout << "Mode: k-NN (k=" << kValue << ")" << endl;
		break;

	// k 值调整
	case ']':
		kValue++;
		cout << "k increased to " << kValue << endl;
		if (mode == KNN) passiveMotion(x, y); // 刷新视图
		break;
	case '[':
		if (kValue > 1) kValue--;
		cout << "k decreased to " << kValue << endl;
		if (mode == KNN) passiveMotion(x, y); // 刷新视图
		break;
	case 'B':
	case 'b':
		loadStationData();
		mode = Default;
		break;
	case 'T':
	case 't':
		loadTaxiData();
		mode = Default;
		break;
	case 'R':
	case 'r':
		showRoad = !showRoad;
		break;
	case 'Q':
	case 'q':
		showTree = !showTree;
		break;
	case '+':
		pointSize *= 1.1;
		break;
	case '-':
		pointSize /= 1.1;
		break;
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
		TreeTy::test(key - '0');
		break;
	default:
		mode = Default;
		break;
	}
	glutPostRedisplay();
}

int main(int argc, char* argv[]) {
	cout << "Key Usage:\n"
		<< "  S  : range search for roads\n"
		<< "  s  : range search for stations\n"
		<< "  N  : nearest road search\n"
		<< "  n  : nearest station search\n"
		<< "  P/p: Polygon data (RANGE/NN)\n"
		<< "  K/k: k-NN search for stations\n"
		<< "     ]: increase k\n"
		<< "     [: decrease k\n"
		<< "  B/b: Bicycle data\n"
		<< "  T/t: Taxi data\n"
		<< "  R/r: show Road\n"
		<< "  Q/q: show Tree\n"
		<< "  +  : increase point size\n"
		<< "  -  : decrease point size\n"
		<< "  1  : Test Envelope contain, interset and union\n"
		<< "  2  : Test distance between Point and LineString\n"
		<< "  3  : Test distance between Point and Polygon\n"
		<< "  4  : Test tree construction\n"
		<< "  5  : Test (your option here)\n"
		<< "  8  : Tree performance analysis\n"
		<< "  ESC: quit\n"
		<< endl;

	pointTree = make_unique<TreeTy>();
	roadTree = make_unique<TreeTy>();
	polygonTree = make_unique<TreeTy>();

	loadRoadData();

	loadStationData();

	loadPolygonData(); // 新增：加载多边形数据

	glutInit(&argc, argv);
	glutInitWindowSize(screenWidth, screenHeight);
	glutInitWindowPosition(0, 0);
	glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE);
	glutCreateWindow("New York");

	glutMouseFunc(mouse);
	glutDisplayFunc(display);
	glutPassiveMotionFunc(passiveMotion);
	glutReshapeFunc(changeSize);
	glutKeyboardFunc(processNormalKeys);

	glutMainLoop();

	return 0;
}
