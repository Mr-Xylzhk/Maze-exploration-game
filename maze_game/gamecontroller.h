#ifndef GAMECONTROLLER_H
#define GAMECONTROLLER_H

#include <QObject>
#include <QTimer>
#include "mazegenerator.h"
#include "player.h"

class GameController : public QObject {
    Q_OBJECT

public:
    explicit GameController(QObject* parent = nullptr);

    // 开始新游戏
    void startNewGame(Difficulty difficulty = Difficulty::MEDIUM);

    // 重新开始当前游戏
    void restartCurrentGame();

    // 处理键盘输入
    void handleKeyPress(QKeyEvent* event);

    // 获取迷宫生成器
    MazeGenerator* getMazeGenerator() { return &mazeGenerator; }

    // 获取玩家
    Player* getPlayer() { return &player; }

    // 检查游戏是否结束（使用不同名称避免冲突）
    bool isGameFinished() const { return m_gameFinished; }

    // 保存当前迷宫
    void saveCurrentMaze(const QString& filename);

    // 加载迷宫
    bool loadMaze(const QString& filename);

    // 获取当前难度
    Difficulty getCurrentDifficulty() const { return m_currentDifficulty; }

    // 寻路功能
    void startPathfinding();
    void clearPathfinding();
    void advancePathfinding();
    bool isPathfindingActive() const;
    bool isPathfindingComplete() const;

signals:
    // 游戏状态变化信号
    void gameStateChanged();
    // 游戏结束信号（使用不同名称）
    void gameVictory(bool victory);
    void mazeRegenerated();

private slots:
    // 处理玩家移动
    void onPlayerMoved(const QPoint& oldPos, const QPoint& newPos);

private:
    // 检查游戏是否胜利
    void checkVictory();

    // 重置游戏状态
    void resetGameState();

    // 确保起点和终点距离足够远
    void ensureDistance();

private:
    MazeGenerator mazeGenerator;
    Player player;
    bool m_gameFinished;  // 改为私有成员变量，加m_前缀
    QString currentMazeFile;
    Difficulty m_currentDifficulty;
};
#endif // GAMECONTROLLER_H
