#ifndef NOPENGLWIDGET_H
#define NOPENGLWIDGET_H

#include <QOpenGLWidget>
#include <QOpenGLFunctions_4_5_Core>
#include <QElapsedTimer>
#include <QTimer>
#include <QOpenGLShaderProgram>
#include <cmath>
#include <QWheelEvent>
#include <QOpenGLTexture>
#include <QOpenGLVersionFunctionsFactory>
#include "camera.h"
#include "model.h"

class NOpenGLWidget : public QOpenGLWidget, QOpenGLFunctions_4_5_Core
{
    Q_OBJECT
public:
    explicit NOpenGLWidget(QWidget *parent = nullptr);
    ~NOpenGLWidget();

    void loadModel(string path);
protected:
    virtual void initializeGL();
    virtual void resizeGL(int w, int h);
    virtual void paintGL();

    void wheelEvent(QWheelEvent *event);
    void keyPressEvent(QKeyEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void mousePressEvent(QMouseEvent *event);
signals:
    // 点击鼠标后发出信号
    // 注意signals不需要实现
    void mousePickingPos(QVector3D worldPos);
public slots:
    void on_timeout();   // 循环，为paintGL定时刷新
private:
    QElapsedTimer m_time;
    QTimer m_timer;
    QOpenGLShaderProgram m_ShaderProgram;
    QOpenGLShaderProgram lightShaderProgram;
    Camera m_camera;

    QOpenGLTexture *m_BoxDiffuseTex;
    QOpenGLTexture *m_PlaneDiffuseTex;

    Mesh *m_BoxMesh;
    Mesh *m_PlaneMesh;
    Model *m_model = NULL;

    QVector3D cameraPosInit(float maxY, float minY);
    Mesh *advProcessMesh(float *vertices, int size, unsigned int textureld);
    QVector4D worldPositionFromViewPort(int posX, int posY);
};

#endif // NOPENGLWIDGET_H
