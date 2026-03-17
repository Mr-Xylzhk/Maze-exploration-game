#ifndef PLAYER_H
#define PLAYER_H

#include <QPoint>
#include <QObject>
#include <QKeyEvent>
#include <QPainter>

class Player : public QObject {
    Q_OBJECT

public:
    explicit Player(QObject* parent = nullptr);

    // 设置玩家位置
    void setPosition(const QPoint& pos);
    void setPosition(int x, int y);

    // 获取玩家位置
    QPoint getPosition() const { return position; }

    // 处理键盘输入
    void handleKeyPress(QKeyEvent* event);

    // 绘制玩家
    void draw(QPainter& painter, int cellSize) const;

    // 重置玩家状态
    void reset();

    // 获取移动步数
    int getSteps() const { return steps; }

signals:
    // 玩家移动信号
    void playerMoved(const QPoint& oldPos, const QPoint& newPos);

private:
    QPoint position;
    int steps;
};

#endif // PLAYER_H
