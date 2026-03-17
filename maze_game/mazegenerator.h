// mazegenerator.h
#ifndef MAZEGENERATOR_H
#define MAZEGENERATOR_H

#include <vector>
#include <random>
#include <stack>
#include <algorithm>
#include <QPoint>
#include <chrono>
#include <queue>
#include <functional>

// 迷宫单元格状态枚举
enum class CellState {
    EMPTY,      // 可通行
    WALL,       // 墙壁
    START,      // 起点
    END,        // 终点
    PATH,       // 路径（用于可视化）
    VISITED     // 已访问（用于生成算法）
};

// 寻路状态枚举
enum class PathfindingState {
    NONE,           // 无寻路状态
    SEARCHING,      // 正在搜索
    PATH_FOUND      // 找到路径
};

// 方向枚举
enum class Direction {
    UP,
    DOWN,
    LEFT,
    RIGHT,
    NONE
};

// A*算法节点结构
struct AStarNode {
    QPoint position;
    int g;  // 从起点到当前节点的实际代价
    int h;  // 从当前节点到终点的启发式估计代价
    int f;  // f = g + h，总估计代价

    AStarNode(QPoint pos, int gCost, int hCost)
        : position(pos), g(gCost), h(hCost), f(gCost + hCost) {}

    bool operator>(const AStarNode& other) const {
        return f > other.f;
    }
};

// 难度枚举
enum class Difficulty {
    EASY,    // 简单 - 递归分割算法 (30x30)
    MEDIUM,  // 中等 - Prim算法 (50x50)
    HARD     // 困难 - 迭代回溯算法 (70x70)
};

class MazeGenerator {
public:
    MazeGenerator(int width = 20, int height = 20);

    // 生成新迷宫
    void generate(Difficulty difficulty = Difficulty::MEDIUM);

    // 重置并生成新尺寸的迷宫
    void resetAndGenerate(int newWidth, int newHeight, Difficulty difficulty = Difficulty::MEDIUM);

    // 获取迷宫数据
    const std::vector<std::vector<CellState>>& getMaze() const { return maze; }

    // 获取迷宫尺寸
    int getWidth() const { return width; }
    int getHeight() const { return height; }

    // 设置起点和终点
    void setStartPoint(const QPoint& start);
    void setEndPoint(const QPoint& end);

    // 获取起点和终点
    QPoint getStartPoint() const { return startPoint; }
    QPoint getEndPoint() const { return endPoint; }

    // 检查位置是否有效且可通行
    bool isValidPosition(int x, int y) const;
    bool isWalkable(int x, int y) const;

    // 验证起点到终点是否有路径
    bool validatePath() const;

    // 确保迷宫有路径（如果没有则修复）
    void ensurePathExists();

    // 重置迷宫（保留墙壁结构，重置其他状态）
    void resetMaze();

    // 保存和加载迷宫（简单序列化）
    void saveMaze(const QString& filename) const;
    bool loadMaze(const QString& filename);

    // 寻找空单元格
    QPoint findEmptyCell(int startX, int startY);

    // A*寻路功能
    void startAStarPathfinding(const QPoint& start, const QPoint& end);
    void clearPathfinding();
    std::vector<QPoint> getVisitedNodes() const { return visitedNodes; }
    std::vector<QPoint> getPath() const { return path; }
    PathfindingState getPathfindingState() const { return pathfindingState; }
    int getCurrentPathfindingIndex() const { return currentPathfindingIndex; }
    void advancePathfinding();
    bool isPathfindingComplete() const;

private:
    // 递归分割算法（简单难度）
    void generateWithRecursiveDivision();
    void recursiveDivision(int x, int y, int width, int height, bool horizontal);

    // Prim算法（中等难度）
    void generateWithPrim();

    // 迭代版本的递归回溯算法（困难难度）
    void generateWithIterativeBacktracking();

    // 曼哈顿距离计算（用于A*启发式函数）
    int manhattanDistance(const QPoint& a, const QPoint& b) const;

    // 添加额外的复杂性（死胡同和环）
    void addExtraComplexity();

    // 递归回溯算法（已弃用，保留兼容性）
    void recursiveBacktracking(int x, int y);
    std::vector<Direction> getShuffledDirections();
    QPoint getNeighbor(int x, int y, Direction dir, int step = 2) const;

    // 辅助数据结构
    std::vector<std::vector<bool>> visited;

    // 检查是否在迷宫范围内
    bool inBounds(int x, int y) const;

    // 迭代版本的递归回溯算法（避免栈溢出）
    void iterativeBacktracking(int startX, int startY);

    // 新添加的函数声明
    QPoint findFarthestCell(const QPoint& start);  // 添加这个
    void addExtraDeadEnds();  // 添加这个

    // 辅助函数
    Direction getDirection(const QPoint& from, const QPoint& to) const;

private:
    int width;
    int height;
    std::vector<std::vector<CellState>> maze;
    QPoint startPoint;
    QPoint endPoint;

    // 随机数生成器
    std::random_device rd;
    std::mt19937 gen;

    // BFS寻路相关
    std::vector<QPoint> visitedNodes;
    std::vector<QPoint> path;
    PathfindingState pathfindingState;
    int currentPathfindingIndex;
    std::vector<std::vector<QPoint>> parentMap;
};

#endif // MAZEGENERATOR_H
