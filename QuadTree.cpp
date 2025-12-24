#include "QuadTree.h"
#include <set>
#include <utility> 

#include <queue> 

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

		// 辅助结构：用于搜索队列，存储节点及其到查询点的最小距离
		struct SearchEntry {
			double minDist;
			QuadNode* node;

			// 优先队列默认是最大堆，为了实现最小堆（Min-Heap），需要反转比较逻辑
			bool operator>(const SearchEntry& other) const {
				return minDist > other.minDist;
			}
		};

		// 辅助结构：用于结果队列，存储特征及其确切距离
		struct ResultEntry {
			double dist;
			Feature feature;

			// 结果队列需要是最大堆（Max-Heap），用于快速获取第 k 个最近邻（即当前结果中最远的）
			bool operator<(const ResultEntry& other) const {
				return dist < other.dist;
			}
		};

		void QuadTree::kNNQuery(double x, double y, int k, std::vector<Feature>& result) {
			result.clear();
			if (!root || k <= 0) return;

			// 1. 搜索队列 (Min-Heap): 存储待访问的节点，按到 (x,y) 的最小距离排序
			std::priority_queue<SearchEntry, std::vector<SearchEntry>, std::greater<SearchEntry>> searchQueue;

			// 2. 结果队列 (Max-Heap): 存储当前找到的最近的 k 个特征
			std::priority_queue<ResultEntry> resultQueue;

			// 初始将根节点加入搜索队列
			// 注意：maxDistance2Envelope 是点到矩形的最短距离
			// 这里我们需要计算点 (x,y) 到 root envelope 的最小欧几里得距离。
			// 由于 Feature 没有直接提供 point2EnvelopeMinDist，我们在这里手动计算或复用现有逻辑。
			// 假设 feature.maxDistance2Envelope 计算的是 max dist，我们需要 min dist。
			// 为了简化，我们直接在 SearchEntry 中计算距离。

			// 简易 lambda 计算点到 Envelope 的最小平方距离 (用于比较)
			auto calcMinDist = [](double px, double py, const Envelope& env) -> double {
				double dx = 0.0, dy = 0.0;
				if (px < env.getMinX()) dx = env.getMinX() - px;
				else if (px > env.getMaxX()) dx = px - env.getMaxX();

				if (py < env.getMinY()) dy = env.getMinY() - py;
				else if (py > env.getMaxY()) dy = py - env.getMaxY();

				return sqrt(dx * dx + dy * dy);
				};

			searchQueue.push({ calcMinDist(x, y, root->getEnvelope()), root });

			Point queryPt(x, y); // 用于计算精确距离

			while (!searchQueue.empty()) {
				SearchEntry current = searchQueue.top();
				searchQueue.pop();

				// 剪枝 (Pruning):
				// 如果当前节点的最短距离 已经大于 已找到的第 k 个最近邻的距离，
				// 那么这个节点下的所有特征都不可能是 k-NN，直接放弃。
				if (resultQueue.size() == k && current.minDist > resultQueue.top().dist) {
					continue; // 甚至可以 break，因为 SearchEntry 是有序的，后面的肯定更远
				}

				if (current.node->isLeafNode()) {
					// 处理叶节点中的特征
					for (const auto& feat : current.node->getFeatures()) {
						// 计算精确几何距离
						double d = queryPt.distance(feat.getGeom());

						if (resultQueue.size() < k) {
							resultQueue.push({ d, feat });
						}
						else if (d < resultQueue.top().dist) {
							resultQueue.pop();
							resultQueue.push({ d, feat });
						}
					}
				}
				else {
					// 处理内部节点：将子节点加入搜索队列
					for (int i = 0; i < 4; ++i) {
						QuadNode* child = current.node->getChildNode(i);
						if (child) {
							double d = calcMinDist(x, y, child->getEnvelope());
							// 优化：入队前的剪枝
							if (resultQueue.size() < k || d < resultQueue.top().dist) {
								searchQueue.push({ d, child });
							}
						}
					}
				}
			}

			// 将结果从优先队列转入 vector (注意：堆弹出的顺序是从大到小，我们需要反转)
			while (!resultQueue.empty()) {
				result.push_back(resultQueue.top().feature);
				resultQueue.pop();
			}
			std::reverse(result.begin(), result.end()); // 现在是从小到大
		}


} // namespace hw6
