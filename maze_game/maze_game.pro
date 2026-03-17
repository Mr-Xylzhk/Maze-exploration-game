QT       += core gui
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

SOURCES += \
    main.cpp \
    widget.cpp \
    mazegenerator.cpp \
    player.cpp \
    gamecontroller.cpp

HEADERS += \
    widget.h \
    mazegenerator.h \
    player.h \
    gamecontroller.h

FORMS += \
    widget.ui

# 默认规则
RESOURCES += resources.qrc

# 部署设置
win32:CONFIG(release, debug|release): LIBS += -lglu32 -lopengl32
else:unix:!macx: LIBS += -lGL

TARGET = maze_game
TEMPLATE = app
