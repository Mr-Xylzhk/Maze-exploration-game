#include "widget.h"
#include "./ui_widget.h"
#include <QMessageBox>
#include <QFileDialog>
#include <QGraphicsRectItem>
#include <QGraphicsTextItem>
#include <QPainter>
#include <QKeyEvent>
#include <QResizeEvent>
#include <QPixmap>

Widget::Widget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Widget)
    , scene(new QGraphicsScene(this))
    , gameController(new GameController(this))
    , redrawTimer(new QTimer(this))
    , pathfindingTimer(new QTimer(this))
    , scale(1.0)
    , offsetX(0.0)
    , offsetY(0.0)
    , cellSize(25)
{
    ui->setupUi(this);

    // 设置窗口标题
    setWindowTitle("随机迷宫游戏");

    // 设置图形视图
    ui->graphicsView->setScene(scene);
    ui->graphicsView->setRenderHint(QPainter::Antialiasing);
    ui->graphicsView->setBackgroundBrush(QColor(240, 240, 240));

    // 加载贴图资源
    wallPixmap.load(":/images/resources/images/wall.png");
    pathPixmap.load(":/images/resources/images/path.png");
    startPixmap.load(":/images/resources/images/start.png");
    endPixmap.load(":/images/resources/images/end.png");
    playerPixmap.load(":/images/resources/images/player.png");

    // 连接游戏控制器信号 - 注意信号名称已更改
    connect(gameController, &GameController::gameStateChanged,
            this, &Widget::onGameStateChanged);
    connect(gameController, &GameController::gameVictory,  // 使用新的信号名称
            this, &Widget::onGameVictory);                 // 使用新的槽函数
    connect(gameController, &GameController::mazeRegenerated,
            this, &Widget::onMazeRegenerated);

    // 设置重绘定时器
    connect(redrawTimer, &QTimer::timeout, this, &Widget::drawMaze);
    redrawTimer->start(16); // 大约60FPS

    // 设置寻路动画定时器
    connect(pathfindingTimer, &QTimer::timeout, this, &Widget::onPathfindingAnimation);

    // 初始化难度下拉框
    ui->difficultyComboBox->addItem("简单 (30x30)", static_cast<int>(Difficulty::EASY));
    ui->difficultyComboBox->addItem("中等 (50x50)", static_cast<int>(Difficulty::MEDIUM));
    ui->difficultyComboBox->addItem("困难 (70x70)", static_cast<int>(Difficulty::HARD));
    ui->difficultyComboBox->setCurrentIndex(0); // 默认选择简单难度

    // 开始新游戏
    on_newGameButton_clicked();
}

Widget::~Widget()
{
    delete ui;
}

void Widget::keyPressEvent(QKeyEvent *event)
{
    // 清除寻路状态（玩家移动后路径不再有效）
    if (gameController->isPathfindingActive()) {
        gameController->clearPathfinding();
    }

    // 传递按键事件给游戏控制器
    gameController->handleKeyPress(event);

    // 更新显示
    drawMaze();
}

// 窗口大小改变事件处理 - 自动调整迷宫显示
void Widget::resizeEvent(QResizeEvent *event)
{
    // 调用父类的resizeEvent
    QWidget::resizeEvent(event);

    // 重新绘制迷宫以适应新窗口大小
    drawMaze();
}

void Widget::on_newGameButton_clicked()
{
    // 清除寻路状态
    gameController->clearPathfinding();

    // 重置缩放和偏移
    scale = 1.0;
    offsetX = 0.0;
    offsetY = 0.0;

    // 获取当前选择的难度
    Difficulty difficulty = static_cast<Difficulty>(ui->difficultyComboBox->currentData().toInt());

    // 开始新游戏
    gameController->startNewGame(difficulty);

    // 更新状态
    QString difficultyText;
    switch (difficulty) {
        case Difficulty::EASY:
            difficultyText = "简单";
            break;
        case Difficulty::MEDIUM:
            difficultyText = "中等";
            break;
        case Difficulty::HARD:
            difficultyText = "困难";
            break;
    }
    ui->statusLabel->setText("新游戏已开始！难度: " + difficultyText + "。使用WASD或方向键移动。");
}

void Widget::on_restartButton_clicked()
{
    // 清除寻路状态
    gameController->clearPathfinding();

    // 重新开始当前游戏
    gameController->restartCurrentGame();

    // 更新状态
    ui->statusLabel->setText("游戏已重新开始！");
}

void Widget::on_saveMazeButton_clicked()
{
    // 打开保存文件对话框
    QString filename = QFileDialog::getSaveFileName(
        this, "保存迷宫", "", "迷宫文件 (*.maze);;所有文件 (*.*)"
    );

    if (!filename.isEmpty()) {
        // 确保文件扩展名
        if (!filename.endsWith(".maze", Qt::CaseInsensitive)) {
            filename += ".maze";
        }

        // 保存迷宫
        gameController->saveCurrentMaze(filename);

        // 更新状态
        ui->statusLabel->setText("迷宫已保存到: " + filename);
    }
}

void Widget::on_loadMazeButton_clicked()
{
    // 打开加载文件对话框
    QString filename = QFileDialog::getOpenFileName(
        this, "加载迷宫", "", "迷宫文件 (*.maze);;所有文件 (*.*)"
    );

    if (!filename.isEmpty()) {
        // 加载迷宫
        bool success = gameController->loadMaze(filename);

        if (success) {
            // 重置缩放和偏移
            scale = 1.0;
            offsetX = 0.0;
            offsetY = 0.0;

            // 更新状态
            ui->statusLabel->setText("迷宫已从文件加载: " + filename);
        } else {
            QMessageBox::warning(this, "加载失败", "无法加载迷宫文件: " + filename);
        }
    }
}

void Widget::on_helpButton_clicked()
{
    showAboutDialog();
}

void Widget::on_difficultyComboBox_currentIndexChanged(int index)
{
    Q_UNUSED(index);
    // 当难度改变时开始新游戏
    on_newGameButton_clicked();
}

void Widget::on_hintButton_clicked()
{
    // 清除之前的寻路结果
    gameController->clearPathfinding();

    // 开始新的寻路
    gameController->startPathfinding();

    // 启动寻路动画定时器（每50ms更新一次）
    pathfindingTimer->start(50);

    // 更新状态
    ui->statusLabel->setText("正在搜索路径...");
}

// 放大按钮点击 - 放大迷宫
void Widget::on_zoomInButton_clicked()
{
    // 放大10%
    scale *= 1.1;

    // 限制最大缩放比例
    if (scale > 3.0) {
        scale = 3.0;
    }

    // 更新显示
    drawMaze();

    // 更新状态
    ui->statusLabel->setText(QString("缩放: %1%").arg(static_cast<int>(scale * 100)));
}

// 缩小按钮点击 - 缩小迷宫
void Widget::on_zoomOutButton_clicked()
{
    // 缩小10%
    scale *= 0.9;

    // 限制最小缩放比例
    if (scale < 0.3) {
        scale = 0.3;
    }

    // 更新显示
    drawMaze();

    // 更新状态
    ui->statusLabel->setText(QString("缩放: %1%").arg(static_cast<int>(scale * 100)));
}

// 重置视图按钮点击 - 重置缩放和偏移
void Widget::on_resetViewButton_clicked()
{
    // 重置缩放和偏移
    scale = 1.0;
    offsetX = 0.0;
    offsetY = 0.0;

    // 更新显示
    drawMaze();

    // 更新状态
    ui->statusLabel->setText("视图已重置");
}

void Widget::onGameStateChanged()
{
    // 更新步数显示
    int steps = gameController->getPlayer()->getSteps();
    ui->stepsLabel->setText(QString("步数: %1").arg(steps));

    // 重绘迷宫
    drawMaze();
}

void Widget::onGameVictory(bool victory)  // 槽函数名称改为onGameVictory
{
    // 显示游戏完成对话框
    showGameFinishedDialog(victory);
}

void Widget::onMazeRegenerated()
{
    // 迷宫已重新生成，重绘
    drawMaze();
}

void Widget::onPathfindingAnimation()
{
    // 前进寻路动画
    gameController->advancePathfinding();

    // 重绘迷宫以显示寻路过程
    drawMaze();

    // 检查是否完成
    if (gameController->isPathfindingComplete()) {
        pathfindingTimer->stop();
        ui->statusLabel->setText("路径搜索完成！");
    }
}

void Widget::drawMaze()
{
    // 清除场景
    scene->clear();

    // 获取迷宫数据
    MazeGenerator* mazeGen = gameController->getMazeGenerator();
    const auto& maze = mazeGen->getMaze();

    int width = mazeGen->getWidth();
    int height = mazeGen->getHeight();

    // 根据迷宫尺寸动态计算基础单元格大小
    int maxMazeSize = std::max(width, height);
    int baseCellSize;

    if (maxMazeSize <= 30) {
        baseCellSize = 30;
    } else if (maxMazeSize <= 50) {
        baseCellSize = 25;
    } else if (maxMazeSize <= 70) {
        baseCellSize = 20;
    } else {
        baseCellSize = 15;  // 100x100迷宫使用15像素单元格
    }

    // 应用缩放比例
    int cellSize = static_cast<int>(baseCellSize * scale);

    // 计算迷宫的总尺寸
    int mazeWidth = width * cellSize;
    int mazeHeight = height * cellSize;

    // 获取视图尺寸
    int viewWidth = ui->graphicsView->width();
    int viewHeight = ui->graphicsView->height();

    // 如果是首次绘制或重置，计算居中偏移量
    if (offsetX == 0.0 && offsetY == 0.0) {
        offsetX = (viewWidth - mazeWidth) / 2.0;
        offsetY = (viewHeight - mazeHeight) / 2.0;
    }

    // 绘制迷宫单元格
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            CellState state = maze[y][x];

            // 计算绘制位置（应用缩放和偏移）
            int drawX = static_cast<int>(x * cellSize + offsetX);
            int drawY = static_cast<int>(y * cellSize + offsetY);

            // 根据单元格状态选择贴图或颜色
            // 注意：道路（EMPTY、PATH、VISITED）始终使用白色方块，不使用贴图
            bool useColorBlock = (state == CellState::EMPTY || 
                                  state == CellState::PATH || 
                                  state == CellState::VISITED);
            
            QPixmap* pixmap = nullptr;
            switch (state) {
                case CellState::WALL:
                    pixmap = &wallPixmap;
                    break;
                case CellState::EMPTY:
                    pixmap = nullptr;  // 道路不使用贴图
                    break;
                case CellState::START:
                    pixmap = &startPixmap;
                    break;
                case CellState::END:
                    pixmap = &endPixmap;
                    break;
                case CellState::PATH:
                    pixmap = nullptr;  // 路径不使用贴图
                    break;
                case CellState::VISITED:
                    pixmap = nullptr;  // 已访问不使用贴图
                    break;
                default:
                    pixmap = nullptr;
                    break;
            }

            // 如果贴图加载成功且不是道路类型，使用贴图；否则使用颜色块
            if (!useColorBlock && pixmap && !pixmap->isNull()) {
                QGraphicsPixmapItem* pixmapItem = scene->addPixmap(pixmap->scaled(cellSize, cellSize, Qt::KeepAspectRatio, Qt::SmoothTransformation));
                pixmapItem->setPos(drawX, drawY);
            } else {
                // 使用颜色块
                QColor color;
                switch (state) {
                    case CellState::WALL:
                        color = QColor(0, 0, 0);  // 黑色墙壁
                        break;
                    case CellState::EMPTY:
                        color = QColor(255, 255, 255);  // 白色道路
                        break;
                    case CellState::START:
                        color = QColor(0, 200, 0);  // 绿色起点
                        break;
                    case CellState::END:
                        color = QColor(255, 50, 50);  // 红色终点
                        break;
                    case CellState::PATH:
                        color = QColor(200, 230, 255);  // 淡蓝色路径
                        break;
                    case CellState::VISITED:
                        color = QColor(220, 220, 220);  // 灰色已访问
                        break;
                    default:
                        color = QColor(255, 255, 255);  // 默认白色
                        break;
                }

                QGraphicsRectItem* rect = scene->addRect(
                    drawX,
                    drawY,
                    cellSize,
                    cellSize
                );

                rect->setBrush(QBrush(color));
                rect->setPen(QPen(QColor(200, 200, 200), 1));
            }
        }
    }

    // 绘制寻路可视化
    if (gameController->isPathfindingActive()) {
        MazeGenerator* mazeGen = gameController->getMazeGenerator();
        const auto& visitedNodes = mazeGen->getVisitedNodes();
        const auto& path = mazeGen->getPath();
        int currentIndex = mazeGen->getCurrentPathfindingIndex();

        // 绘制已访问的节点（浅蓝色，80%透明度）
        for (int i = 0; i < currentIndex && i < static_cast<int>(visitedNodes.size()); ++i) {
            const QPoint& node = visitedNodes[i];

            // 计算绘制位置（应用缩放和偏移）
            int drawX = static_cast<int>(node.x() * cellSize + offsetX);
            int drawY = static_cast<int>(node.y() * cellSize + offsetY);

            QGraphicsRectItem* visitedRect = scene->addRect(
                drawX,
                drawY,
                cellSize,
                cellSize
            );

            QColor visitedColor(135, 206, 250, 204); // 浅蓝色，80%透明度
            visitedRect->setBrush(QBrush(visitedColor));
            visitedRect->setPen(Qt::NoPen);
        }

        // 如果寻路完成，绘制最短路径（亮黄色，80%透明度）
        if (gameController->isPathfindingComplete() && !path.empty()) {
            for (const QPoint& point : path) {
                // 计算绘制位置（应用缩放和偏移）
                int drawX = static_cast<int>(point.x() * cellSize + offsetX);
                int drawY = static_cast<int>(point.y() * cellSize + offsetY);

                QGraphicsRectItem* pathRect = scene->addRect(
                    drawX,
                    drawY,
                    cellSize,
                    cellSize
                );

                QColor pathColor(255, 255, 0, 204); // 亮黄色，80%透明度
                pathRect->setBrush(QBrush(pathColor));
                pathRect->setPen(Qt::NoPen);
            }
        }
    }

    // 绘制玩家
    Player* player = gameController->getPlayer();
    QPoint playerPos = player->getPosition();

    // 计算玩家绘制位置（应用缩放和偏移）
    int playerDrawX = static_cast<int>(playerPos.x() * cellSize + offsetX);
    int playerDrawY = static_cast<int>(playerPos.y() * cellSize + offsetY);

    // 如果玩家贴图加载成功，使用贴图；否则使用蓝色圆形
    if (!playerPixmap.isNull()) {
        QGraphicsPixmapItem* playerPixmapItem = scene->addPixmap(playerPixmap.scaled(cellSize, cellSize, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        playerPixmapItem->setPos(playerDrawX, playerDrawY);
    } else {
        QGraphicsEllipseItem* playerEllipse = scene->addEllipse(
            playerDrawX + 2,
            playerDrawY + 2,
            cellSize - 4,
            cellSize - 4
        );
        playerEllipse->setBrush(QBrush(QColor(0, 100, 255)));
        playerEllipse->setPen(QPen(QColor(0, 50, 150), 2));
    }

    // 绘制终点
    QPoint endPoint = mazeGen->getEndPoint();

    // 计算终点绘制位置（应用缩放和偏移）
    int endDrawX = static_cast<int>(endPoint.x() * cellSize + offsetX);
    int endDrawY = static_cast<int>(endPoint.y() * cellSize + offsetY);

    // 如果终点贴图加载成功，使用贴图；否则使用金色矩形
    if (!endPixmap.isNull()) {
        QGraphicsPixmapItem* endPixmapItem = scene->addPixmap(endPixmap.scaled(cellSize, cellSize, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        endPixmapItem->setPos(endDrawX, endDrawY);
    } else {
        QGraphicsRectItem* treasureRect = scene->addRect(
            endDrawX + 3,
            endDrawY + 3,
            cellSize - 6,
            cellSize - 6
        );
        treasureRect->setBrush(QBrush(QColor(255, 215, 0)));
        treasureRect->setPen(QPen(QColor(200, 150, 0), 2));
    }
}

void Widget::showGameFinishedDialog(bool victory)
{
    if (victory) {
        // 获取玩家步数
        int steps = gameController->getPlayer()->getSteps();

        // 创建自定义对话框
        QDialog dialog(this);
        dialog.setWindowTitle("恭喜！");

        QVBoxLayout* layout = new QVBoxLayout(&dialog);

        QLabel* message = new QLabel(
            QString("恭喜你成功走出迷宫！\n\n"
                    "所用步数: %1\n\n"
                    "请选择下一步操作:").arg(steps),
            &dialog
        );
        message->setAlignment(Qt::AlignCenter);
        layout->addWidget(message);

        QHBoxLayout* buttonLayout = new QHBoxLayout();

        QPushButton* restartButton = new QPushButton("重新开始(&R)", &dialog);
        QPushButton* newGameButton = new QPushButton("新的挑战(&N)", &dialog);
        QPushButton* quitButton = new QPushButton("退出游戏(&Q)", &dialog);

        buttonLayout->addWidget(restartButton);
        buttonLayout->addWidget(newGameButton);
        buttonLayout->addWidget(quitButton);

        layout->addLayout(buttonLayout);

        // 连接按钮信号
        connect(restartButton, &QPushButton::clicked, [&]() {
            dialog.accept();
            on_restartButton_clicked();
        });

        connect(newGameButton, &QPushButton::clicked, [&]() {
            dialog.accept();
            on_newGameButton_clicked();
        });

        connect(quitButton, &QPushButton::clicked, [&]() {
            dialog.reject();
            close();
        });

        dialog.exec();
    }
}

void Widget::showAboutDialog()
{
    QMessageBox::about(this, "关于随机迷宫游戏",
        "<h2>随机迷宫游戏</h2>"
        "<p>这是一个使用C++和Qt开发的随机生成迷宫游戏。</p>"
        "<p>旨在通过玩家自身体验而更深刻理解不同的寻路算法。</p>"
        "<p><b>游戏规则:</b></p>"
        "<ul>"
        "<li>使用 <b>WASD</b> 或 <b>方向键</b> 控制玩家(卡比)移动</li>"
        "<li>从 <span style='color:green'>起点(家)</span> 出发</li>"
        "<li>到达 <span style='color:red'>终点(星星)</span> 获胜</li>"
        "<li>避开 <span style='color:gray'>墙壁(砖墙)</span></li>"
        "</ul>"
        "<p><b>难度设置:</b></p>"
        "<ul>"
        "<li><b>简单 (30x30)</b> - 递归分割算法，适合新手</li>"
        "<li><b>中等 (50x50)</b> - Prim算法，适合有一定经验的玩家</li>"
        "<li><b>困难 (70x70)</b> - 迭代回溯算法，挑战性最高</li>"
        "</ul>"
        "<p><b>功能:</b></p>"
        "<ul>"
        "<li>三种不同难度的迷宫生成算法</li>"
        "<li>保存和加载迷宫</li>"
        "<li>放大和缩小迷宫</li>"
        "<li>多种难度选择</li>"
        "<li>重新开始游戏</li>"
        "<li><b>智能提示</b> - 使用A*算法可视化搜索最短路径</li>"
        "</ul>"
        "<p><b>智能提示说明:</b></p>"
        "<ul>"
        "<li>点击<b>智能提示</b>按钮开始路径搜索</li>"
        "<li><span style='color:skyblue'>浅蓝色</span>区域表示搜索过程</li>"
        "<li><span style='color:yellow'>亮黄色</span>路径表示从当前位置到终点的最短路径</li>"
        "<li>使用曼哈顿距离作为启发式函数，提高搜索效率</li>"
        "<li>玩家移动后路径会自动清除</li>"
        "</ul>"
        "<p>祝您游戏愉快！</p>"
    );
}
