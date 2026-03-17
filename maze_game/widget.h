#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <QGraphicsScene>
#include <QTimer>
#include <QKeyEvent>
#include <QPixmap>
#include "gamecontroller.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class Widget;
}
QT_END_NAMESPACE

class Widget : public QWidget
{
    Q_OBJECT

public:
    Widget(QWidget *parent = nullptr);
    ~Widget();

protected:
    // 键盘事件处理 - 处理玩家移动按键
    void keyPressEvent(QKeyEvent *event) override;

    // 鼠标滚轮事件处理 - 已禁用，改用按钮控制缩放
    // void wheelEvent(QWheelEvent *event) override;

    // 窗口大小改变事件处理 - 自动调整迷宫显示
    void resizeEvent(QResizeEvent *event) override;

private slots:
    // UI槽函数
    void on_newGameButton_clicked();
    void on_restartButton_clicked();
    void on_saveMazeButton_clicked();
    void on_loadMazeButton_clicked();
    void on_helpButton_clicked();
    void on_difficultyComboBox_currentIndexChanged(int index);
    void on_hintButton_clicked();
    void on_zoomInButton_clicked();
    void on_zoomOutButton_clicked();
    void on_resetViewButton_clicked();

    // 寻路动画槽函数
    void onPathfindingAnimation();

    // 游戏状态槽函数
    void onGameStateChanged();
    void onGameVictory(bool victory);  // 改为匹配gameVictory信号
    void onMazeRegenerated();

private:
    // 初始化UI
    void initUI();

    // 绘制迷宫
    void drawMaze();

    // 显示游戏完成对话框
    void showGameFinishedDialog(bool victory);

    // 显示关于对话框
    void showAboutDialog();

private:
    Ui::Widget *ui;
    QGraphicsScene* scene;
    GameController* gameController;
    QTimer* redrawTimer;
    QTimer* pathfindingTimer;

    // ========== 缩放和移动相关 ==========

    // 迷宫缩放比例
    double scale;

    // 迷宫偏移量（用于拖拽移动）
    double offsetX;
    double offsetY;

    // 迷宫单元格大小
    int cellSize;

    // 贴图资源
    QPixmap wallPixmap;
    QPixmap pathPixmap;
    QPixmap startPixmap;
    QPixmap endPixmap;
    QPixmap playerPixmap;
};
#endif // WIDGET_H
