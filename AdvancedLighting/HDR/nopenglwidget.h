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
#include <QString>
#include <map>
#include "camera.h"
#include "model.h"

class NOpenGLWidget : public QOpenGLWidget, QOpenGLFunctions_4_5_Core
{
    Q_OBJECT
public:
    // 允许加载多个模型，保存多个模型的信息
    // model的信息可能其他部件也要用
    struct ModelInfo {
        Model *model;
        QVector3D worldPos;
        float pitch;
        float yaw;
        float roll;
        bool isSelected;
        QString name;
    };

    explicit NOpenGLWidget(QWidget *parent = nullptr);
    ~NOpenGLWidget();

    void loadModel(string path);

    void renderQuad();
protected:
    virtual void initializeGL();
    virtual void resizeGL(int w, int h);
    virtual void paintGL();

    void wheelEvent(QWheelEvent *event);
    void keyPressEvent(QKeyEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void mouseDoubleClickEvent(QMouseEvent *event);  // 双击选中模型
signals:
    // 点击鼠标后发出信号
    // 注意signals不需要实现
    void mousePickingPos(QVector3D worldPos);
public slots:
    void on_timeout();   // 循环，为paintGL定时刷新
    void setHDR(bool checked);
    void setExposure(double exposure);
private:
    float m_exposure = 0.1f;
    float m_HDR = false;

    QElapsedTimer m_time;
    QTimer m_timer;

    QOpenGLShaderProgram m_ShaderProgram;
    QOpenGLShaderProgram m_LightingProgram;

    Camera m_camera;

    QOpenGLTexture *m_PlaneDiffuseTex;
    QOpenGLTexture *m_PlaneNormalTex;
    QOpenGLTexture *m_PlaneHeightTex;
    QOpenGLTexture *m_WallTex;

    Mesh *m_WindowMesh;
    Mesh *m_PlaneMesh;
    Mesh *m_CubeMesh;

    QMap<QString, ModelInfo> m_Models;

    vector<QVector3D> windows;
    map<float, QVector3D> sorted;

    QVector3D cameraPosInit(float maxY, float minY);
    Mesh *advProcessMesh(float *vertices, int size, unsigned int textureld);
    QVector4D worldPositionFromViewPort(int posX, int posY);

    bool m_modelMoving = false;
};

#endif // NOPENGLWIDGET_H
