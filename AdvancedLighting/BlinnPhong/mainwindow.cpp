#include "mainwindow.h"
#include "./ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setCentralWidget(ui->openGLWidget);
    connect(ui->openGLWidget, SIGNAL(mousePickingPos(QVector3D)),
            this, SLOT(getMousePickingPos(QVector3D)));
}

MainWindow::~MainWindow()
{
    delete ui;
}

// 需要引入QFileDialog和QString头文件
void MainWindow::on_actLoadModel_triggered()
{
    // 通过别的类的对象取调用，必然设置成public
    // 父节点，标题，目录（这里默认），文件格式过滤
    // 所有格式都可以*.*
    QString str = QFileDialog::getOpenFileName(this, "选择模型文件", "D:/Dev/QtProject/AdvancedOpenGL",
                             "OBJ(*.obj);;FBX(*.fbx);;ALL FILES(*.*)");
    ui->openGLWidget->loadModel(str.toStdString());
}

void MainWindow::getMousePickingPos(QVector3D pos)
{
    // 需要在mainwindow的构造函数中connect mousePickingPos 和 getMousePickingPos
    // Qt状态栏显示世界坐标
    ui->statusbar->setStyleSheet("font: 14pt");
    ui->statusbar->showMessage("世界坐标 X:" + QString::number(pos.x(),'f', 2)
                               + " Y:" + QString::number(pos.y(), 'f', 2)
                               + " Z:" + QString::number(pos.z(), 'f', 2));
}

