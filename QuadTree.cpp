#include "QuadTree.h"
#include <set>
#include <utility> 

namespace hw6 {

	/*
	 * QuadNode
	 */
	void QuadNode::split(size_t capacity) {
		// 1. 清理旧子节点
		for (int i = 0; i < 4; ++i) {
			delete children[i];
			children[i] = nullptr;
		}

		// 2. 计算四个子节点的包围盒
		double minX = bbox.getMinX();
		double minY = bbox.getMinY();
		double maxX = bbox.getMaxX();
		double maxY = bbox.getMaxY();
		double midX = (minX + maxX) / 2.0;
		double midY = (minY + maxY) / 2.0;

		Envelope boxes[4] = {
			Envelope(minX, midX, minY, midY), // 左下
			Envelope(midX, maxX, minY, midY), // 右下
			Envelope(minX, midX, midY, maxY), // 左上
			Envelope(midX, maxX, midY, maxY)  // 右上
		};

		// 3. 创建子节点
		for (int i = 0; i < 4; ++i) {
			children[i] = new QuadNode(boxes[i]);
		}

		// 4. 将特征分配到所有重叠的子节点
		for (const auto& feature : features) {
			const Envelope& fbox = feature.getEnvelope();
			for (int i = 0; i < 4; ++i) {
				if (boxes[i].intersect(fbox)) {
					children[i]->add(feature);
				}
			}
		}

		// 5. 清空当前节点的特征
		features.clear();

		// 6. 对子节点递归分裂
		for (int i = 0; i < 4; ++i) {
			if (children[i]->getFeatureNum() > capacity) {
				children[i]->split(capacity);
			}
		}
	}

	void QuadNode::countNode(int& interiorNum, int& leafNum) {
		if (isLeafNode()) {
			++leafNum;
		}
		else {
			++interiorNum;
			for (int i = 0; i < 4; ++i)
				children[i]->countNode(interiorNum, leafNum);
		}
	}

	int QuadNode::countHeight(int height) {
		++height;
		if (!isLeafNode()) {
			int cur = height;
			for (int i = 0; i < 4; ++i) {
				height = std::max(height, children[i]->countHeight(cur));
			}
		}
		return height;
	}

	void QuadNode::rangeQuery(const Envelope& rect, std::vector<Feature>& features) {
		if (!bbox.intersect(rect))
			return;

		// Task range query
		if (isLeafNode()) {
			for (const auto& feature : this->features) {
				if (feature.getEnvelope().intersect(rect)) {
					features.push_back(feature);
				}
			}
		}
		else {
			for (int i = 0; i < 4; ++i) {
				if (children[i]) {
					children[i]->rangeQuery(rect, features);
				}
			}
		}
	}

	QuadNode* QuadNode::pointInLeafNode(double x, double y) {
		// Task NN query
		if(!bbox.contain(x, y))
			return nullptr;

		if (isLeafNode()) {
			return this;
		}
		else {
			for (int i = 0; i < 4; ++i) {
				if(children[i] && children[i]->getEnvelope().contain(x, y)) {
					return children[i]->pointInLeafNode(x, y);
				}
			}
		}

		return nullptr;
	}

	void QuadNode::draw() {
		if (isLeafNode()) {
			bbox.draw();
		}
		else {
			for (int i = 0; i < 4; ++i)
				children[i]->draw();
		}
	}

	/*
	 * QuadTree
	 */
	bool QuadTree::constructTree(const std::vector<Feature>& features) {
		if (features.empty())
			return false;

		// Task construction
		double minX = features[0].getEnvelope().getMinX();
		double minY = features[0].getEnvelope().getMinY();
		double maxX = features[0].getEnvelope().getMaxX();
		double maxY = features[0].getEnvelope().getMaxY();
		for (const auto& feature : features) {
			const Envelope& fbox = feature.getEnvelope();
			minX = std::min(minX, fbox.getMinX());
			minY = std::min(minY, fbox.getMinY());
			maxX = std::max(maxX, fbox.getMaxX());
			maxY = std::max(maxY, fbox.getMaxY());
		}

		bbox = Envelope(minX, maxX, minY, maxY); // 注意此行代码需要更新为features的包围盒，或根节点的包围盒

		if (root) {
			delete root;
			root = nullptr;
		}
		root = new QuadNode(bbox);
		root->add(features);
		if(root->getFeatureNum() > capacity) {
			root->split(capacity);
		}
		return true;
	}

	void QuadTree::countNode(int& interiorNum, int& leafNum) {
		interiorNum = 0;
		leafNum = 0;
		if (root)
			root->countNode(interiorNum, leafNum);
	}

	void QuadTree::countHeight(int& height) {
		height = 0;
		if (root)
			height = root->countHeight(0);
	}

	void QuadTree::rangeQuery(const Envelope& rect, std::vector<Feature>& features) {
		features.clear();

		// Task range query

		// filter step (选择查询区域与几何对象包围盒相交的几何对象)
		if (root) {
			root->rangeQuery(rect, features);
		}

		if (!features.empty()) {
			// 按 Geometry 指针地址排序
			std::sort(features.begin(), features.end(), [](const Feature& a, const Feature& b) {
				return a.getGeom() < b.getGeom();
				});
			// 移除重复项
			features.erase(std::unique(features.begin(), features.end(), [](const Feature& a, const Feature& b) {
				return a.getGeom() == b.getGeom();
				}), features.end());
		}

		// 注意四叉树区域查询仅返回候选集，精炼步在hw6的rangeQuery中完成
	}

	bool QuadTree::NNQuery(double x, double y, std::vector<Feature>& features) {
		if (!root || !(root->getEnvelope().contain(x, y)))
			return false;

		// Task NN query
		

		// filter step
		// (使用maxDistance2Envelope函数，获得查询点到几何对象包围盒的最短的最大距离，然后区域查询获得候选集)

		QuadNode* leaf = root->pointInLeafNode(x, y);
		if(!leaf)
			return false;

		double minDist = std::numeric_limits<double>::max();
		for(const auto& feature : leaf->getFeatures()) {
			double dist = feature.maxDistance2Envelope(x, y);
			if(dist < minDist) {
				minDist = dist;
			}
		}
		Envelope queryRect(x - minDist, x + minDist, y - minDist, y + minDist);
		rangeQuery(queryRect, features);

		// 注意四叉树邻近查询仅返回候选集，精炼步在hw6的NNQuery中完成

		return true;
	}

		void QuadTree::draw() {
		if (root)
			root->draw();
	}

	/*
	 * Spatial Join
	 */
		void QuadTree::spatialJoin(const std::vector<Feature>& queryFeatures, double distanceThreshold, std::vector<std::pair<Feature, Feature>>& result) {
			result.clear();

			// Outer Loop: Iterate through the query features (e.g., Roads)
			for (const auto& qFeat : queryFeatures) {
				const Geometry* qGeom = qFeat.getGeom();
				Envelope qEnv = qFeat.getEnvelope();

				// 1. Calculate Search Window
				// Expand the feature's envelope by the threshold distance
				Envelope searchEnv(
					qEnv.getMinX() - distanceThreshold,
					qEnv.getMaxX() + distanceThreshold,
					qEnv.getMinY() - distanceThreshold,
					qEnv.getMaxY() + distanceThreshold
				);

				// 2. Filter Step (Index Lookup)
				// Use QuadTree to find features potentially within distance
				std::vector<Feature> candidates;
				rangeQuery(searchEnv, candidates);

				// 3. Refine Step (Exact Geometry Distance)
				for (const auto& cFeat : candidates) {
					const Geometry* cGeom = cFeat.getGeom();

					// Avoid self-comparison if the datasets overlap (optional check)
					// Calculate exact distance
					if (qGeom->distance(cGeom) <= distanceThreshold) {
						result.push_back(std::make_pair(qFeat, cFeat));
					}
				}
			}
		}

} // namespace hw6
