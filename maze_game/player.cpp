#include "player.h"
#include <QKeyEvent>

Player::Player(QObject* parent)
    : QObject(parent), steps(0) {
    position = QPoint(0, 0);
}

void Player::setPosition(const QPoint& pos) {
    position = pos;
}

void Player::setPosition(int x, int y) {
    position.setX(x);
    position.setY(y);
}

void Player::handleKeyPress(QKeyEvent* event) {
    if (!event) return;

    QPoint newPos = position;

    switch (event->key()) {
        case Qt::Key_W:
        case Qt::Key_Up:
            newPos.setY(position.y() - 1);
            break;
        case Qt::Key_S:
        case Qt::Key_Down:
            newPos.setY(position.y() + 1);
            break;
        case Qt::Key_A:
        case Qt::Key_Left:
            newPos.setX(position.x() - 1);
            break;
        case Qt::Key_D:
        case Qt::Key_Right:
            newPos.setX(position.x() + 1);
            break;
        default:
            return; // 不是移动按键，忽略
    }

    // 如果位置改变了，发出信号
    if (newPos != position) {
        QPoint oldPos = position;
        position = newPos;
        steps++;
        emit playerMoved(oldPos, newPos);
    }
}

void Player::draw(QPainter& painter, int cellSize) const {
    // 保存画笔状态
    painter.save();

    // 设置玩家颜色（蓝色）
    QColor playerColor(0, 120, 215); // 蓝色
    painter.setBrush(QBrush(playerColor));
    painter.setPen(Qt::NoPen);

    // 绘制玩家（圆形）
    int playerSize = cellSize * 0.7;
    int offset = (cellSize - playerSize) / 2;

    painter.drawEllipse(
        position.x() * cellSize + offset,
        position.y() * cellSize + offset,
        playerSize,
        playerSize
    );

    // 绘制玩家中心点（白色）
    painter.setBrush(QBrush(Qt::white));
    int centerSize = playerSize * 0.3;
    int centerOffset = (playerSize - centerSize) / 2;

    painter.drawEllipse(
        position.x() * cellSize + offset + centerOffset,
        position.y() * cellSize + offset + centerOffset,
        centerSize,
        centerSize
    );

    painter.restore();
}

void Player::reset() {
    steps = 0;
}
