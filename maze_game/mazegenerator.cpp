#include "mazegenerator.h"
#include <fstream>
#include <iostream>
#include <QFile>
#include <QTextStream>
#include <chrono>
#include <queue>
#include <climits>

MazeGenerator::MazeGenerator(int width, int height)
    : width(width), height(height), gen(rd()), pathfindingState(PathfindingState::NONE), currentPathfindingIndex(0) {
    // 使用当前时间作为随机种子
    gen.seed(std::chrono::system_clock::now().time_since_epoch().count());
    // 初始化迷宫，所有单元格都是墙壁
    maze.resize(height, std::vector<CellState>(width, CellState::WALL));
}

void MazeGenerator::generate(Difficulty difficulty) {
    // 重置迷宫，全部设为墙壁
    maze.assign(height, std::vector<CellState>(width, CellState::WALL));

    // 初始化访问数组
    visited.assign(height, std::vector<bool>(width, false));

    // 根据难度选择算法
    switch (difficulty) {
        case Difficulty::EASY:
            // 简单难度：递归分割算法
            generateWithRecursiveDivision();
            break;
        case Difficulty::MEDIUM:
            // 中等难度：Prim算法
            generateWithPrim();
            break;
        case Difficulty::HARD:
            // 困难难度：迭代回溯算法
            generateWithIterativeBacktracking();
            break;
    }

    // 设置起点
    int startX, startY;
    if (difficulty == Difficulty::EASY) {
        startX = 1;
        startY = 1;
    } else {
        // 随机选择奇数坐标作为起点
        std::uniform_int_distribution<> xDis(1, width/2);
        std::uniform_int_distribution<> yDis(1, height/2);
        startX = xDis(gen) * 2 - 1;
        startY = yDis(gen) * 2 - 1;
        startX = std::min(startX, width - 2);
        startY = std::min(startY, height - 2);
    }

    startPoint = QPoint(startX, startY);

    // 寻找终点
    if (difficulty == Difficulty::EASY) {
        endPoint = findEmptyCell(width - 2, height - 2);
    } else {
        endPoint = findFarthestCell(startPoint);
    }

    if (endPoint == startPoint) {
        endPoint = QPoint(width - 2, height - 2);
    }

    maze[startPoint.y()][startPoint.x()] = CellState::START;
    maze[endPoint.y()][endPoint.x()] = CellState::END;

    // 根据难度添加额外的复杂性
    if (difficulty == Difficulty::MEDIUM || difficulty == Difficulty::HARD) {
        addExtraComplexity();
    }

    // 对于困难难度，添加更多死胡同
    if (difficulty == Difficulty::HARD) {
        addExtraDeadEnds();
    }

    // 确保起点到终点至少有一条路径
    ensurePathExists();
}

// 递归分割算法（简单难度）
void MazeGenerator::generateWithRecursiveDivision() {
    // 首先将整个区域设为空地
    for (int y = 1; y < height - 1; ++y) {
        for (int x = 1; x < width - 1; ++x) {
            maze[y][x] = CellState::EMPTY;
        }
    }

    // 从整个区域开始递归分割
    recursiveDivision(1, 1, width - 2, height - 2, false);
}

void MazeGenerator::recursiveDivision(int x, int y, int w, int h, bool horizontal) {
    // 如果区域太小，停止分割
    if (w < 3 || h < 3) {
        return;
    }

    // 决定分割方向
    if (horizontal) {
        // 水平分割（添加水平墙）
        if (h < 3) return;

        // 选择一个随机位置放置墙（必须是奇数位置）
        std::uniform_int_distribution<> wallYDis(y + 1, y + h - 2);
        int wallY = wallYDis(gen);
        if (wallY % 2 == 0) wallY++;

        // 选择一个随机位置放置通道（必须是偶数位置）
        std::uniform_int_distribution<> passageXDis(x, x + w - 1);
        int passageX = passageXDis(gen);
        if (passageX % 2 != 0) passageX++;

        // 添加水平墙
        for (int i = x; i < x + w; ++i) {
            if (i != passageX) {
                maze[wallY][i] = CellState::WALL;
            }
        }

        // 递归分割上下两个区域
        recursiveDivision(x, y, w, wallY - y, !horizontal);
        recursiveDivision(x, wallY + 1, w, y + h - wallY - 1, !horizontal);
    } else {
        // 垂直分割（添加垂直墙）
        if (w < 3) return;

        // 选择一个随机位置放置墙（必须是奇数位置）
        std::uniform_int_distribution<> wallXDis(x + 1, x + w - 2);
        int wallX = wallXDis(gen);
        if (wallX % 2 == 0) wallX++;

        // 选择一个随机位置放置通道（必须是偶数位置）
        std::uniform_int_distribution<> passageYDis(y, y + h - 1);
        int passageY = passageYDis(gen);
        if (passageY % 2 != 0) passageY++;

        // 添加垂直墙
        for (int i = y; i < y + h; ++i) {
            if (i != passageY) {
                maze[i][wallX] = CellState::WALL;
            }
        }

        // 递归分割左右两个区域
        recursiveDivision(x, y, wallX - x, h, !horizontal);
        recursiveDivision(wallX + 1, y, x + w - wallX - 1, h, !horizontal);
    }
}

// Prim算法（中等难度）
void MazeGenerator::generateWithPrim() {
    // 使用Prim算法生成迷宫
    // 首先选择一个随机起点
    int startX = 1;
    int startY = 1;

    // 标记起点为已访问
    visited[startY][startX] = true;
    maze[startY][startX] = CellState::EMPTY;

    // 存储墙壁的列表
    std::vector<std::pair<int, int>> walls;

    // 添加起点的相邻墙壁
    const int dx[] = {0, 2, 0, -2};
    const int dy[] = {-2, 0, 2, 0};

    for (int i = 0; i < 4; ++i) {
        int nx = startX + dx[i];
        int ny = startY + dy[i];
        if (inBounds(nx, ny) && !visited[ny][nx]) {
            walls.push_back({nx, ny});
        }
    }

    // 主循环
    while (!walls.empty()) {
        // 随机选择一堵墙
        std::uniform_int_distribution<> dis(0, walls.size() - 1);
        int index = dis(gen);
        std::pair<int, int> wall = walls[index];
        walls.erase(walls.begin() + index);

        int wx = wall.first;
        int wy = wall.second;

        if (visited[wy][wx]) continue;

        // 找到这堵墙连接的已访问单元格
        std::vector<std::pair<int, int>> visitedNeighbors;
        for (int i = 0; i < 4; ++i) {
            int nx = wx + dx[i];
            int ny = wy + dy[i];
            if (inBounds(nx, ny) && visited[ny][nx]) {
                visitedNeighbors.push_back({nx, ny});
            }
        }

        if (visitedNeighbors.empty()) continue;

        // 随机选择一个已访问的邻居
        std::uniform_int_distribution<> neighborDis(0, visitedNeighbors.size() - 1);
        int neighborIndex = neighborDis(gen);
        std::pair<int, int> neighbor = visitedNeighbors[neighborIndex];

        // 打通墙壁
        int wallX = (wx + neighbor.first) / 2;
        int wallY = (wy + neighbor.second) / 2;
        maze[wallY][wallX] = CellState::EMPTY;

        // 标记新单元格为已访问
        visited[wy][wx] = true;
        maze[wy][wx] = CellState::EMPTY;

        // 添加新单元格的相邻墙壁
        for (int i = 0; i < 4; ++i) {
            int nx = wx + dx[i];
            int ny = wy + dy[i];
            if (inBounds(nx, ny) && !visited[ny][nx]) {
                walls.push_back({nx, ny});
            }
        }
    }
}

// 迭代回溯算法（困难难度）
void MazeGenerator::generateWithIterativeBacktracking() {
    // 选择起点（必须是奇数坐标）
    int startX = 1;
    int startY = 1;

    // 标记起点为已访问
    if (inBounds(startX, startY)) {
        visited[startY][startX] = true;
        maze[startY][startX] = CellState::EMPTY;
    }

    // 使用栈进行迭代回溯
    std::stack<QPoint> cellStack;
    cellStack.push(QPoint(startX, startY));

    while (!cellStack.empty()) {
        QPoint current = cellStack.top();

        // 获取随机顺序的未访问邻居
        std::vector<QPoint> unvisitedNeighbors;
        std::vector<Direction> directions = getShuffledDirections();

        for (Direction dir : directions) {
            QPoint neighbor = getNeighbor(current.x(), current.y(), dir, 2);

            if (inBounds(neighbor.x(), neighbor.y()) && !visited[neighbor.y()][neighbor.x()]) {
                unvisitedNeighbors.push_back(neighbor);
            }
        }

        if (!unvisitedNeighbors.empty()) {
            // 随机选择一个未访问的邻居
            std::uniform_int_distribution<> dis(0, unvisitedNeighbors.size() - 1);
            QPoint chosen = unvisitedNeighbors[dis(gen)];

            // 打通中间的墙
            QPoint wall = getNeighbor(current.x(), current.y(),
                getDirection(current, chosen), 1);

            if (inBounds(wall.x(), wall.y())) {
                maze[wall.y()][wall.x()] = CellState::EMPTY;
            }

            // 标记并打通选择的单元格
            visited[chosen.y()][chosen.x()] = true;
            maze[chosen.y()][chosen.x()] = CellState::EMPTY;

            // 将新单元格压入栈
            cellStack.push(chosen);
        } else {
            // 回溯
            cellStack.pop();
        }
    }
}

QPoint MazeGenerator::findFarthestCell(const QPoint& start) {
    // 使用BFS找到离起点最远的可达位置
    std::vector<std::vector<bool>> bfsVisited(height, std::vector<bool>(width, false));
    std::vector<std::vector<int>> distance(height, std::vector<int>(width, -1));
    std::queue<QPoint> q;

    q.push(start);
    bfsVisited[start.y()][start.x()] = true;
    distance[start.y()][start.x()] = 0;

    QPoint farthest = start;
    int maxDist = 0;

    const int dx[] = {0, 1, 0, -1};
    const int dy[] = {-1, 0, 1, 0};

    while (!q.empty()) {
        QPoint current = q.front();
        q.pop();

        int currentDist = distance[current.y()][current.x()];

        // 更新最远点
        if (currentDist > maxDist && current != start) {
            maxDist = currentDist;
            farthest = current;
        }

        // 检查四个方向
        for (int i = 0; i < 4; i++) {
            int nx = current.x() + dx[i];
            int ny = current.y() + dy[i];

            if (inBounds(nx, ny) && !bfsVisited[ny][nx] && maze[ny][nx] != CellState::WALL) {
                bfsVisited[ny][nx] = true;
                distance[ny][nx] = currentDist + 1;
                q.push(QPoint(nx, ny));
            }
        }
    }

    return farthest;
}

void MazeGenerator::addExtraDeadEnds() {
    // 可以随机选择一些墙壁，检查是否只能通向一个方向，如果是就打通
    std::uniform_int_distribution<> countDis(5, (width * height) / 20);
    int extraCount = countDis(gen);

    for (int i = 0; i < extraCount; i++) {
        // 随机选择一个位置
        std::uniform_int_distribution<> xDis(1, width - 2);
        std::uniform_int_distribution<> yDis(1, height - 2);

        int x = xDis(gen);
        int y = yDis(gen);

        // 确保是墙
        if (maze[y][x] == CellState::WALL) {
            int pathCount = 0;
            QPoint connectedPath;

            const int dx[] = {0, 1, 0, -1};
            const int dy[] = {-1, 0, 1, 0};

            // 检查相邻的通道
            for (int d = 0; d < 4; d++) {
                int nx = x + dx[d];
                int ny = y + dy[d];

                if (inBounds(nx, ny) && maze[ny][nx] == CellState::EMPTY) {
                    pathCount++;
                    connectedPath = QPoint(nx, ny);
                }
            }

            // 如果只与一个通道相邻，打通它（创建死胡同）
            if (pathCount == 1) {
                // 检查这个死胡同不会创建环
                int connections = 0;
                for (int d = 0; d < 4; d++) {
                    int nx = connectedPath.x() + dx[d];
                    int ny = connectedPath.y() + dy[d];

                    if (inBounds(nx, ny) && maze[ny][nx] == CellState::EMPTY) {
                        connections++;
                    }
                }

                // 如果原始通道已经有3个或更多连接，添加死胡同
                if (connections >= 3) {
                    maze[y][x] = CellState::EMPTY;
                }
            }
        }
    }
}

Direction MazeGenerator::getDirection(const QPoint& from, const QPoint& to) const {
    if (to.x() > from.x()) return Direction::RIGHT;
    if (to.x() < from.x()) return Direction::LEFT;
    if (to.y() > from.y()) return Direction::DOWN;
    if (to.y() < from.y()) return Direction::UP;
    return Direction::NONE;
}

// 添加迭代版本的递归回溯算法
void MazeGenerator::iterativeBacktracking(int startX, int startY) {
    std::stack<QPoint> cellStack;

    // 标记起点为已访问
    if (inBounds(startX, startY)) {
        visited[startY][startX] = true;
        maze[startY][startX] = CellState::EMPTY;
        cellStack.push(QPoint(startX, startY));
    }

    while (!cellStack.empty()) {
        QPoint current = cellStack.top();

        // 获取随机顺序的未访问邻居
        std::vector<QPoint> unvisitedNeighbors;
        std::vector<Direction> directions = getShuffledDirections();

        for (Direction dir : directions) {
            QPoint neighbor = getNeighbor(current.x(), current.y(), dir, 2);

            if (inBounds(neighbor.x(), neighbor.y()) && !visited[neighbor.y()][neighbor.x()]) {
                unvisitedNeighbors.push_back(neighbor);
            }
        }

        if (!unvisitedNeighbors.empty()) {
            // 随机选择一个未访问的邻居
            std::uniform_int_distribution<> dis(0, unvisitedNeighbors.size() - 1);
            QPoint chosen = unvisitedNeighbors[dis(gen)];

            // 打通中间的墙
            QPoint wall = getNeighbor(current.x(), current.y(),
                getDirection(current, chosen), 1);

            if (inBounds(wall.x(), wall.y())) {
                maze[wall.y()][wall.x()] = CellState::EMPTY;
            }

            // 标记并打通选择的单元格
            visited[chosen.y()][chosen.x()] = true;
            maze[chosen.y()][chosen.x()] = CellState::EMPTY;

            // 将新单元格压入栈
            cellStack.push(chosen);
        } else {
            // 回溯
            cellStack.pop();
        }
    }
}

// 实现递归回溯算法
void MazeGenerator::recursiveBacktracking(int x, int y) {
    if (!inBounds(x, y) || visited[y][x]) return;

    // 标记为已访问并打通
    visited[y][x] = true;
    maze[y][x] = CellState::EMPTY;

    // 获取随机方向顺序
    std::vector<Direction> directions = getShuffledDirections();

    // 尝试每个方向
    for (Direction dir : directions) {
        // 获取邻居单元格（跳过一堵墙）
        QPoint neighbor = getNeighbor(x, y, dir, 2);

        if (inBounds(neighbor.x(), neighbor.y()) && !visited[neighbor.y()][neighbor.x()]) {
            // 打通中间的墙
            QPoint wall = getNeighbor(x, y, dir, 1);
            if (inBounds(wall.x(), wall.y())) {
                maze[wall.y()][wall.x()] = CellState::EMPTY;
            }

            // 递归访问邻居
            recursiveBacktracking(neighbor.x(), neighbor.y());
        }
    }
}

// 获取随机顺序的方向
std::vector<Direction> MazeGenerator::getShuffledDirections() {
    std::vector<Direction> directions = {
        Direction::UP,
        Direction::DOWN,
        Direction::LEFT,
        Direction::RIGHT
    };

    // 使用Fisher-Yates洗牌算法
    for (int i = directions.size() - 1; i > 0; --i) {
        std::uniform_int_distribution<> dis(0, i);
        int j = dis(gen);
        std::swap(directions[i], directions[j]);
    }

    return directions;
}

// 获取邻居位置
QPoint MazeGenerator::getNeighbor(int x, int y, Direction dir, int step) const {
    switch (dir) {
        case Direction::UP:
            return QPoint(x, y - step);
        case Direction::DOWN:
            return QPoint(x, y + step);
        case Direction::LEFT:
            return QPoint(x - step, y);
        case Direction::RIGHT:
            return QPoint(x + step, y);
        default:
            return QPoint(x, y);
    }
}

void MazeGenerator::addExtraComplexity() {
    std::uniform_int_distribution<> countDis(10, width * height / 10);
    int extraCount = countDis(gen);

    for (int i = 0; i < extraCount; i++) {
        std::uniform_int_distribution<> xDis(1, width-2);
        std::uniform_int_distribution<> yDis(1, height-2);
        std::uniform_int_distribution<> typeDis(0, 100);

        int x = xDis(gen);
        int y = yDis(gen);

        if (maze[y][x] == CellState::WALL) {
            // 检查是否可以打通这堵墙创建环
            int adjacentPaths = 0;
            const int dx[] = {0, 1, 0, -1};
            const int dy[] = {-1, 0, 1, 0};

            for (int j = 0; j < 4; j++) {
                int nx = x + dx[j];
                int ny = y + dy[j];

                if (inBounds(nx, ny) && maze[ny][nx] == CellState::EMPTY) {
                    adjacentPaths++;
                }
            }

            // 如果相邻至少两个通道，打通它会创建环
            if (adjacentPaths >= 2 && typeDis(gen) < 30) { // 30%概率创建环
                maze[y][x] = CellState::EMPTY;
            }
        } else if (maze[y][x] == CellState::EMPTY) {
            // 随机决定是否创建一个死胡同
            if (typeDis(gen) < 15) { // 15%概率创建死胡同
                // 找到当前空单元格的一个方向是墙
                for (int dir = 0; dir < 4; dir++) {
                    const int dx[] = {0, 1, 0, -1};
                    const int dy[] = {-1, 0, 1, 0};

                    int nx = x + dx[dir];
                    int ny = y + dy[dir];

                    // 检查这个方向是否有墙可以打通创建死胡同
                    if (inBounds(nx, ny) && maze[ny][nx] == CellState::WALL) {
                        // 检查打通后是否只连接当前单元格
                        int connections = 0;
                        for (int d = 0; d < 4; d++) {
                            int cx = nx + dx[d];
                            int cy = ny + dy[d];
                            if (inBounds(cx, cy) && maze[cy][cx] == CellState::EMPTY) {
                                connections++;
                            }
                        }

                        if (connections == 1) { // 只连接到当前单元格，创建死胡同
                            maze[ny][nx] = CellState::EMPTY;
                            break;
                        }
                    }
                }
            }
        }
    }
}

QPoint MazeGenerator::findEmptyCell(int startX, int startY) {
    // 使用BFS搜索寻找空单元格
    std::vector<std::vector<bool>> visited(height, std::vector<bool>(width, false));
    std::queue<QPoint> q;

    q.push(QPoint(startX, startY));
    visited[startY][startX] = true;

    const int dx[] = {0, 1, 0, -1};
    const int dy[] = {-1, 0, 1, 0};

    while (!q.empty()) {
        QPoint current = q.front();
        q.pop();

        // 如果是空单元格，返回
        if (maze[current.y()][current.x()] == CellState::EMPTY) {
            return current;
        }

        // 继续搜索邻居
        for (int i = 0; i < 4; i++) {
            int nx = current.x() + dx[i];
            int ny = current.y() + dy[i];

            if (inBounds(nx, ny) && !visited[ny][nx]) {
                visited[ny][nx] = true;
                q.push(QPoint(nx, ny));
            }
        }
    }

    // 如果找不到，返回边界内的随机位置
    std::uniform_int_distribution<> disX(0, width-1);
    std::uniform_int_distribution<> disY(0, height-1);
    return QPoint(disX(gen), disY(gen));
}

bool MazeGenerator::inBounds(int x, int y) const {
    return x >= 0 && x < width && y >= 0 && y < height;
}

bool MazeGenerator::isValidPosition(int x, int y) const {
    return inBounds(x, y);
}

bool MazeGenerator::isWalkable(int x, int y) const {
    if (!inBounds(x, y)) return false;

    CellState state = maze[y][x];
    return state == CellState::EMPTY ||
           state == CellState::START ||
           state == CellState::END ||
           state == CellState::PATH;
}

void MazeGenerator::setStartPoint(const QPoint& start) {
    if (inBounds(start.x(), start.y())) {
        // 清除原来的起点
        if (inBounds(startPoint.x(), startPoint.y())) {
            maze[startPoint.y()][startPoint.x()] = CellState::EMPTY;
        }

        startPoint = start;
        maze[start.y()][start.x()] = CellState::START;
    }
}

void MazeGenerator::setEndPoint(const QPoint& end) {
    if (inBounds(end.x(), end.y())) {
        // 清除原来的终点
        if (inBounds(endPoint.x(), endPoint.y())) {
            maze[endPoint.y()][endPoint.x()] = CellState::EMPTY;
        }

        endPoint = end;
        maze[end.y()][end.x()] = CellState::END;
    }
}

void MazeGenerator::resetMaze() {
    // 重置所有非墙壁的单元格为空
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            if (maze[y][x] != CellState::WALL) {
                maze[y][x] = CellState::EMPTY;
            }
        }
    }

    // 重新设置起点和终点
    maze[startPoint.y()][startPoint.x()] = CellState::START;
    maze[endPoint.y()][endPoint.x()] = CellState::END;
}

void MazeGenerator::saveMaze(const QString& filename) const {
    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        std::cerr << "无法保存迷宫到文件: " << filename.toStdString() << std::endl;
        return;
    }

    QTextStream out(&file);

    // 写入尺寸
    out << width << " " << height << "\n";

    // 写入起点和终点
    out << startPoint.x() << " " << startPoint.y() << "\n";
    out << endPoint.x() << " " << endPoint.y() << "\n";

    // 写入迷宫数据
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            out << static_cast<int>(maze[y][x]) << " ";
        }
        out << "\n";
    }

    file.close();
}

bool MazeGenerator::loadMaze(const QString& filename) {
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        std::cerr << "无法从文件加载迷宫: " << filename.toStdString() << std::endl;
        return false;
    }

    QTextStream in(&file);

    // 读取尺寸
    int w, h;
    in >> w >> h;

    if (w != width || h != height) {
        // 尺寸不匹配，调整迷宫大小
        width = w;
        height = h;
        maze.resize(height, std::vector<CellState>(width, CellState::WALL));
    }

    // 读取起点和终点
    int startX, startY, endX, endY;
    in >> startX >> startY;
    in >> endX >> endY;

    startPoint = QPoint(startX, startY);
    endPoint = QPoint(endX, endY);

    // 读取迷宫数据
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            int state;
            in >> state;
            maze[y][x] = static_cast<CellState>(state);
        }
    }

    file.close();
    return true;
}

void MazeGenerator::resetAndGenerate(int newWidth, int newHeight, Difficulty difficulty) {
    width = newWidth;
    height = newHeight;

    // 确保尺寸是奇数
    if (width % 2 == 0) width++;
    if (height % 2 == 0) height++;

    // 清空现有迷宫数据
    maze.clear();
    maze.resize(height, std::vector<CellState>(width, CellState::WALL));

    // 重新生成迷宫（使用指定的难度）
    generate(difficulty);
}

void MazeGenerator::startAStarPathfinding(const QPoint& start, const QPoint& end) {
    // 清除之前的寻路结果
    clearPathfinding();

    // 初始化A*算法
    std::vector<std::vector<bool>> visited(height, std::vector<bool>(width, false));
    std::vector<std::vector<int>> gScore(height, std::vector<int>(width, INT_MAX));
    std::vector<std::vector<QPoint>> cameFrom(height, std::vector<QPoint>(width, QPoint(-1, -1)));

    // 使用优先队列（最小堆）
    std::priority_queue<AStarNode, std::vector<AStarNode>, std::greater<AStarNode>> openSet;

    // 初始化起点
    int hStart = manhattanDistance(start, end);
    openSet.push(AStarNode(start, 0, hStart));
    gScore[start.y()][start.x()] = 0;

    const int dx[] = {0, 1, 0, -1};
    const int dy[] = {-1, 0, 1, 0};

    bool found = false;

    while (!openSet.empty()) {
        // 取出f值最小的节点
        AStarNode current = openSet.top();
        openSet.pop();

        // 如果已经访问过，跳过
        if (visited[current.position.y()][current.position.x()]) {
            continue;
        }

        // 标记为已访问
        visited[current.position.y()][current.position.x()] = true;

        // 记录访问的节点（用于可视化）
        if (current.position != start) {
            visitedNodes.push_back(current.position);
        }

        // 找到终点
        if (current.position == end) {
            found = true;
            break;
        }

        // 检查四个方向
        for (int i = 0; i < 4; i++) {
            int nx = current.position.x() + dx[i];
            int ny = current.position.y() + dy[i];

            if (inBounds(nx, ny) && !visited[ny][nx] && isWalkable(nx, ny)) {
                // 计算新的g值
                int tentativeG = gScore[current.position.y()][current.position.x()] + 1;

                // 如果找到更好的路径
                if (tentativeG < gScore[ny][nx]) {
                    cameFrom[ny][nx] = current.position;
                    gScore[ny][nx] = tentativeG;
                    int h = manhattanDistance(QPoint(nx, ny), end);
                    // f值在AStarNode构造函数中自动计算
                    openSet.push(AStarNode(QPoint(nx, ny), tentativeG, h));
                }
            }
        }
    }

    if (found) {
        // 回溯路径
        QPoint current = end;
        while (current != start) {
            path.push_back(current);
            current = cameFrom[current.y()][current.x()];
        }
        path.push_back(start);
        std::reverse(path.begin(), path.end());

        pathfindingState = PathfindingState::PATH_FOUND;
    } else {
        pathfindingState = PathfindingState::SEARCHING;
    }

    currentPathfindingIndex = 0;
}

int MazeGenerator::manhattanDistance(const QPoint& a, const QPoint& b) const {
    return abs(a.x() - b.x()) + abs(a.y() - b.y());
}

void MazeGenerator::clearPathfinding() {
    visitedNodes.clear();
    path.clear();
    pathfindingState = PathfindingState::NONE;
    currentPathfindingIndex = 0;
    parentMap.clear();
}

void MazeGenerator::advancePathfinding() {
    if (pathfindingState == PathfindingState::NONE) {
        return;
    }

    if (currentPathfindingIndex < static_cast<int>(visitedNodes.size())) {
        currentPathfindingIndex++;
    }
}

bool MazeGenerator::isPathfindingComplete() const {
    if (pathfindingState == PathfindingState::NONE) {
        return false;
    }

    return currentPathfindingIndex >= static_cast<int>(visitedNodes.size());
}

// 验证起点到终点是否有路径 - 使用BFS算法检查连通性
bool MazeGenerator::validatePath() const {
    // 使用BFS验证起点到终点是否有路径
    std::vector<std::vector<bool>> visited(height, std::vector<bool>(width, false));
    std::queue<QPoint> q;

    // 从起点开始搜索
    q.push(startPoint);
    visited[startPoint.y()][startPoint.x()] = true;

    // 四个移动方向
    const int dx[] = {0, 1, 0, -1};
    const int dy[] = {-1, 0, 1, 0};

    // BFS主循环
    while (!q.empty()) {
        QPoint current = q.front();
        q.pop();

        // 如果到达终点，返回true
        if (current == endPoint) {
            return true;
        }

        // 检查四个方向的邻居
        for (int i = 0; i < 4; i++) {
            int nx = current.x() + dx[i];
            int ny = current.y() + dy[i];

            // 如果新位置在范围内、未访问且不是墙壁，则加入队列
            if (inBounds(nx, ny) && !visited[ny][nx] && maze[ny][nx] != CellState::WALL) {
                visited[ny][nx] = true;
                q.push(QPoint(nx, ny));
            }
        }
    }

    // 无法到达终点
    return false;
}

// 确保迷宫有路径 - 如果没有路径则修复
void MazeGenerator::ensurePathExists() {
    // 检查是否有路径
    if (validatePath()) {
        return; // 已经有路径，无需修复
    }

    // 如果没有路径，使用A*算法找到一条路径并打通墙壁
    std::vector<std::vector<bool>> visited(height, std::vector<bool>(width, false));
    std::vector<std::vector<QPoint>> cameFrom(height, std::vector<QPoint>(width, QPoint(-1, -1)));
    std::vector<std::vector<int>> gScore(height, std::vector<int>(width, INT_MAX));

    // 使用优先队列（最小堆）
    std::priority_queue<AStarNode, std::vector<AStarNode>, std::greater<AStarNode>> openSet;

    // 初始化起点
    int hStart = manhattanDistance(startPoint, endPoint);
    openSet.push(AStarNode(startPoint, 0, hStart));
    gScore[startPoint.y()][startPoint.x()] = 0;

    // 四个移动方向
    const int dx[] = {0, 1, 0, -1};
    const int dy[] = {-1, 0, 1, 0};

    // A*算法主循环
    while (!openSet.empty()) {
        AStarNode current = openSet.top();
        openSet.pop();

        // 如果已访问，跳过
        if (visited[current.position.y()][current.position.x()]) {
            continue;
        }

        // 标记为已访问
        visited[current.position.y()][current.position.x()] = true;

        // 如果到达终点，重建路径并打通墙壁
        if (current.position == endPoint) {
            // 从终点回溯到起点
            QPoint currentPos = endPoint;
            while (currentPos != startPoint && cameFrom[currentPos.y()][currentPos.x()] != QPoint(-1, -1)) {
                // 如果当前节点是墙壁，打通它
                if (maze[currentPos.y()][currentPos.x()] == CellState::WALL) {
                    maze[currentPos.y()][currentPos.x()] = CellState::EMPTY;
                }
                currentPos = cameFrom[currentPos.y()][currentPos.x()];
            }
            return;
        }

        // 检查四个方向的邻居
        for (int i = 0; i < 4; i++) {
            int nx = current.position.x() + dx[i];
            int ny = current.position.y() + dy[i];

            if (inBounds(nx, ny)) {
                // 计算新的g值
                int newGScore = gScore[current.position.y()][current.position.x()] + 1;

                // 如果找到更优路径
                if (newGScore < gScore[ny][nx]) {
                    gScore[ny][nx] = newGScore;
                    cameFrom[ny][nx] = current.position;

                    // 计算启发式值
                    int h = manhattanDistance(QPoint(nx, ny), endPoint);

                    // 添加到优先队列
                    openSet.push(AStarNode(QPoint(nx, ny), newGScore, h));
                }
            }
        }
    }

    // 如果A*算法也无法找到路径（理论上不应该发生），使用简单的直线修复
    // 从起点到终点画一条直线，打通中间的墙壁
    int x = startPoint.x();
    int y = startPoint.y();

    while (x != endPoint.x() || y != endPoint.y()) {
        if (x < endPoint.x()) x++;
        else if (x > endPoint.x()) x--;

        if (y < endPoint.y()) y++;
        else if (y > endPoint.y()) y--;

        if (inBounds(x, y) && maze[y][x] == CellState::WALL) {
            maze[y][x] = CellState::EMPTY;
        }
    }
}

