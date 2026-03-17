#include "gamecontroller.h"
#include <QKeyEvent>
#include <QMessageBox>
#include <QFile>
#include <QDebug>

GameController::GameController(QObject* parent)
    : QObject(parent), m_gameFinished(false), m_currentDifficulty(Difficulty::MEDIUM) {

    // 连接玩家移动信号
    connect(&player, &Player::playerMoved, this, &GameController::onPlayerMoved);
}

void GameController::startNewGame(Difficulty difficulty) {
    // 根据难度确定迷宫尺寸
    int mazeSize;
    switch (difficulty) {
        case Difficulty::EASY:
            mazeSize = 30;
            break;
        case Difficulty::MEDIUM:
            mazeSize = 50;
            break;
        case Difficulty::HARD:
            mazeSize = 70;
            break;
        default:
            mazeSize = 50;
            break;
    }

    // 保存当前难度
    m_currentDifficulty = difficulty;

    // 使用现有的迷宫生成器，重置并生成新迷宫
    mazeGenerator.resetAndGenerate(mazeSize, mazeSize, difficulty);

    // 重置游戏状态
    resetGameState();

    // 设置玩家到起点
    player.setPosition(mazeGenerator.getStartPoint());
    player.reset();

    m_gameFinished = false;

    // 发出迷宫重新生成的信号
    emit mazeRegenerated();

    // 确保起点和终点距离足够远，增加难度
    ensureDistance();
}

void GameController::ensureDistance() {
    MazeGenerator* mazeGen = getMazeGenerator();
    QPoint start = mazeGen->getStartPoint();
    QPoint end = mazeGen->getEndPoint();

    // 计算曼哈顿距离
    int distance = abs(start.x() - end.x()) + abs(start.y() - end.y());

    // 根据难度动态设置最小距离
    int minDistance;
    int mazeSize = std::min(mazeGen->getWidth(), mazeGen->getHeight());

    switch (m_currentDifficulty) {
        case Difficulty::EASY:
            // 简单难度：30x30迷宫，至少20格距离
            minDistance = mazeSize * 2 / 3;
            break;
        case Difficulty::MEDIUM:
            // 中等难度：50x50迷宫，至少37格距离
            minDistance = mazeSize * 3 / 4;
            break;
        case Difficulty::HARD:
            // 困难难度：70x70迷宫，至少56格距离
            minDistance = mazeSize * 4 / 5;
            break;
        default:
            minDistance = mazeSize * 2 / 3;
            break;
    }

    // 如果距离太近，重新选择终点
    if (distance < minDistance) {
        // 寻找对角线的点作为新终点
        int newEndX = mazeGen->getWidth() - start.x() - 1;
        int newEndY = mazeGen->getHeight() - start.y() - 1;

        // 确保新终点在范围内
        newEndX = std::max(1, std::min(newEndX, mazeGen->getWidth() - 2));
        newEndY = std::max(1, std::min(newEndY, mazeGen->getHeight() - 2));

        // 如果新终点是墙，寻找最近的通道
        if (!mazeGen->isWalkable(newEndX, newEndY)) {
            for (int radius = 1; radius < std::max(mazeGen->getWidth(), mazeGen->getHeight()); radius++) {
                bool found = false;
                for (int dx = -radius; dx <= radius; dx++) {
                    for (int dy = -radius; dy <= radius; dy++) {
                        if (abs(dx) == radius || abs(dy) == radius) {
                            int x = newEndX + dx;
                            int y = newEndY + dy;

                            if (mazeGen->isValidPosition(x, y) && mazeGen->isWalkable(x, y)) {
                                mazeGen->setEndPoint(QPoint(x, y));
                                found = true;
                                break;
                            }
                        }
                    }
                    if (found) break;
                }
                if (found) break;
            }
        } else {
            mazeGen->setEndPoint(QPoint(newEndX, newEndY));
        }
    }
}

void GameController::restartCurrentGame() {
    // 重置迷宫状态（保留结构）
    mazeGenerator.resetMaze();

    // 重置游戏状态
    resetGameState();

    // 设置玩家到起点
    player.setPosition(mazeGenerator.getStartPoint());
    player.reset();

    m_gameFinished = false;

    // 发出游戏状态变化信号
    emit gameStateChanged();
}

void GameController::handleKeyPress(QKeyEvent* event) {
    if (m_gameFinished) return;

    // 获取玩家当前位置
    QPoint currentPos = player.getPosition();

    // 让玩家处理按键
    player.handleKeyPress(event);

    // 获取移动后的位置
    QPoint newPos = player.getPosition();

    // 检查移动是否有效
    if (newPos != currentPos) {
        if (!mazeGenerator.isValidPosition(newPos.x(), newPos.y()) ||
            !mazeGenerator.isWalkable(newPos.x(), newPos.y())) {
            // 移动无效，回到原位置
            player.setPosition(currentPos);
        }
    }

    // 检查是否到达终点
    checkVictory();
}

void GameController::onPlayerMoved(const QPoint& oldPos, const QPoint& newPos) {
    Q_UNUSED(oldPos);
    Q_UNUSED(newPos);

    // 发出游戏状态变化信号
    emit gameStateChanged();
}

void GameController::checkVictory() {
    QPoint playerPos = player.getPosition();
    QPoint endPoint = mazeGenerator.getEndPoint();

    if (playerPos == endPoint) {
        m_gameFinished = true;

        // 发出游戏结束信号
        emit gameVictory(true);
    }
}

void GameController::resetGameState() {
    m_gameFinished = false;
    player.reset();
}

void GameController::saveCurrentMaze(const QString& filename) {
    mazeGenerator.saveMaze(filename);
    currentMazeFile = filename;
}

bool GameController::loadMaze(const QString& filename) {
    bool success = mazeGenerator.loadMaze(filename);

    if (success) {
        // 重置游戏状态
        resetGameState();

        // 设置玩家到起点
        player.setPosition(mazeGenerator.getStartPoint());
        player.reset();

        m_gameFinished = false;
        currentMazeFile = filename;

        // 发出迷宫重新生成的信号
        emit mazeRegenerated();
    }

    return success;
}

void GameController::startPathfinding() {
    // 获取玩家当前位置和终点
    QPoint playerPos = player.getPosition();
    QPoint endPoint = mazeGenerator.getEndPoint();

    // 开始A*寻路
    mazeGenerator.startAStarPathfinding(playerPos, endPoint);
}

void GameController::clearPathfinding() {
    mazeGenerator.clearPathfinding();
}

void GameController::advancePathfinding() {
    mazeGenerator.advancePathfinding();
}

bool GameController::isPathfindingActive() const {
    return mazeGenerator.getPathfindingState() != PathfindingState::NONE;
}

bool GameController::isPathfindingComplete() const {
    return mazeGenerator.isPathfindingComplete();
}
