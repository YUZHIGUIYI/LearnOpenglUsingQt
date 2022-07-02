#include "nopenglwidget.h"
#include "vertices.h"

unsigned int fbo;
unsigned int texture;
unsigned int rbo;
unsigned int skyboxVAO;
unsigned int skyboxVBO;

const unsigned int timeOutmSec = 50;
const float PI = 3.1415926f;
float _near = 0.1f, _far = 100.0f;

QVector3D lightColor(1.0f, 1.0f, 1.0f);

QVector3D viewInitPos = QVector3D(0.0f, 8.0f, 20.0f);

QVector3D pointLightPositions[] = {
    QVector3D(0.7f, 0.2f, 2.0f),
    QVector3D(2.3f, -3.3f, -4.0f),
    QVector3D(-4.0f, 2.0f, -12.0f),
    QVector3D(0.0f, 0.0f, -3.0f)
};
QVector3D pointLightColors[] = {
    QVector3D(1.0f, 1.0f, 0.0f),
    QVector3D(0.3f, 0.3f, 0.7f),
    QVector3D(0.0f, 0.0f, 0.3f),
    QVector3D(0.4f, 0.4f, 0.4f)
};

QMatrix4x4 model;
QMatrix4x4 view;
QMatrix4x4 projection;

QPoint lastPos;  // 在点击的时候赋值

NOpenGLWidget::NOpenGLWidget(QWidget *parent) : QOpenGLWidget(parent)
{
    // 定时刷新的条件
    connect(&m_timer, SIGNAL(timeout()), this, SLOT(on_timeout()));
    m_timer.start(timeOutmSec);
    m_time.start();
    m_camera.Position = viewInitPos;

    setFocusPolicy(Qt::StrongFocus);  // 键盘响应

    windows.push_back(QVector3D( 0.0f, 2.5f, 0.7f));

    for (auto item : windows) {
        float distance =
            m_camera.Position.distanceToPoint(item);
        sorted[distance] = item;
    }
}

NOpenGLWidget::~NOpenGLWidget()
{
    // 回收内存
    for (auto iter = m_Models.begin(); iter != m_Models.end(); iter++) {
        ModelInfo *modelInfo = &iter.value(); // 取出结构
        delete modelInfo->model;
    }
    glDeleteFramebuffers(1, &fbo);
    glDeleteTextures(1, &texture);
    glDeleteRenderbuffers(1, &rbo);
}

void NOpenGLWidget::loadModel(string path)
{
    static int model_number = 0;
    // 新的模型
    makeCurrent();
    auto funcs = QOpenGLVersionFunctionsFactory::get<QOpenGLFunctions_4_5_Core>(QOpenGLContext::currentContext());
    Model *m_model = new Model(funcs, path.c_str());

    m_camera.Position = cameraPosInit(m_model->m_maxY, m_model->m_minY);
    m_Models["ZhangThree" + QString::number(model_number++)] = ModelInfo{m_model,
    QVector3D(0, 0 - m_model->m_minY, 0), 0.0, 0.0, 0.0, false, "ZhangThree" + QString::number(model_number)};
    doneCurrent();
}

void NOpenGLWidget::initializeGL()
{
    // 只使用一次的建议放在这个函数中
    initializeOpenGLFunctions();

    // 物体着色器
    bool success;
    m_ShaderProgram.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/shaders/shapes.vert");
    m_ShaderProgram.addShaderFromSourceFile(QOpenGLShader::Geometry, ":/shaders/shapes.geom");
    m_ShaderProgram.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/shaders/shapes.frag");
    success = m_ShaderProgram.link();
    if (!success) qDebug() << "Error:" << m_ShaderProgram.log();
    // 天空盒子
    m_SkyBoxShaderProgram.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/shaders/skyCube.vert");
    m_SkyBoxShaderProgram.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/shaders/skyCube.frag");
    success = m_SkyBoxShaderProgram.link();
    if (!success) qDebug() << "Error:" << m_SkyBoxShaderProgram.log();

    m_PlaneDiffuseTex = new QOpenGLTexture(
                QImage(":/images/wall.jpg").mirrored());

    m_CubeMap = new QOpenGLTexture(QOpenGLTexture::TargetCubeMap);
    QImage _right   = QImage(":/images/right.jpg").convertToFormat(QImage::Format_RGB888);
    QImage _left    = QImage(":/images/left.jpg").convertToFormat(QImage::Format_RGB888);
    QImage _top     = QImage(":/images/top.jpg").convertToFormat(QImage::Format_RGB888);
    QImage _bottom  = QImage(":/images/bottom.jpg").convertToFormat(QImage::Format_RGB888);
    QImage _front   = QImage(":/images/front.jpg").convertToFormat(QImage::Format_RGB888);
    QImage _back    = QImage(":/images/back.jpg").convertToFormat(QImage::Format_RGB888);
    m_CubeMap->setSize(_right.width(), _right.height());
    m_CubeMap->setFormat(QOpenGLTexture::RGBFormat);
    m_CubeMap->allocateStorage(QOpenGLTexture::RGB, QOpenGLTexture::UInt8);
    m_CubeMap->setData(0, 0, QOpenGLTexture::CubeMapPositiveX,QOpenGLTexture::RGB, QOpenGLTexture::UInt8, (const void *)_right.bits());
    m_CubeMap->setData(0, 0, QOpenGLTexture::CubeMapNegativeX,QOpenGLTexture::RGB, QOpenGLTexture::UInt8, (const void *)_left.bits());
    m_CubeMap->setData(0, 0, QOpenGLTexture::CubeMapPositiveY,QOpenGLTexture::RGB, QOpenGLTexture::UInt8, (const void *)_top.bits());
    m_CubeMap->setData(0, 0, QOpenGLTexture::CubeMapNegativeY,QOpenGLTexture::RGB, QOpenGLTexture::UInt8, (const void *)_bottom.bits());
    m_CubeMap->setData(0, 0, QOpenGLTexture::CubeMapPositiveZ,QOpenGLTexture::RGB, QOpenGLTexture::UInt8, (const void *)_front.bits());
    m_CubeMap->setData(0, 0, QOpenGLTexture::CubeMapNegativeZ,QOpenGLTexture::RGB, QOpenGLTexture::UInt8, (const void *)_back.bits());

    m_CubeMap->setMinificationFilter(QOpenGLTexture::Linear);     //纹理放大或缩小时，像素的取值方法 ，线性或就近抉择
    m_CubeMap->setMagnificationFilter(QOpenGLTexture::Linear);
    m_CubeMap->setWrapMode(QOpenGLTexture::DirectionS, QOpenGLTexture::ClampToEdge);   //设置纹理边缘的扩展方法
    m_CubeMap->setWrapMode(QOpenGLTexture::DirectionT, QOpenGLTexture::ClampToEdge);
    //m_CubeMap->setWrapMode(QOpenGLTexture::DirectionR, QOpenGLTexture::ClampToEdge);

    // skybox VAO
    glGenVertexArrays(1, &skyboxVAO);
    glGenBuffers(1, &skyboxVBO);
    glBindVertexArray(skyboxVAO);
    glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);


    m_PlaneMesh = advProcessMesh(planeVertices, 6, m_PlaneDiffuseTex->textureId());
}

void NOpenGLWidget::resizeGL(int w, int h)
{
    Q_UNUSED(w);
    Q_UNUSED(h);
}

void NOpenGLWidget::paintGL()
{
    model.setToIdentity();
    view.setToIdentity();
    projection.setToIdentity();

    float time = m_time.elapsed() / 200.0;
    projection.perspective(m_camera.Zoom, (float)width()/height(), _near, _far);
    view = m_camera.GetViewMatrix();

    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    m_ShaderProgram.bind();
    // material properties
    m_ShaderProgram.setUniformValue("material.shininess", 32.0f);
    m_ShaderProgram.setUniformValue("projection", projection);
    m_ShaderProgram.setUniformValue("view", view);
    m_ShaderProgram.setUniformValue("viewPos", m_camera.Position);
    // Light
    m_ShaderProgram.setUniformValue("light.direction", -0.2f, -1.0f, -0.3f);
    m_ShaderProgram.setUniformValue("light.ambient", 0.9f, 0.9f, 0.9f);
    m_ShaderProgram.setUniformValue("light.diffuse", 0.9f, 0.9f, 0.9f);
    m_ShaderProgram.setUniformValue("light.specular", 1.0f, 1.0f, 1.0f);

    //model.rotate(time, 1.0f, 1.0f, 0.5f);  // time 为旋转角度
    m_ShaderProgram.setUniformValue("model", model);
    m_ShaderProgram.setUniformValue("time", time);

    //m_PlaneMesh->Draw(m_ShaderProgram);

    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_CUBE_MAP, m_CubeMap->textureId());
    m_ShaderProgram.setUniformValue("skybox", 3);

    for (auto& modelInfo : m_Models) {
        model.setToIdentity();
        model.translate(modelInfo.worldPos);

        model.rotate(modelInfo.pitch, QVector3D(1.0, 0.0, 0.0));
        model.rotate(modelInfo.yaw, QVector3D(0.0, 1.0, 0.0));
        model.rotate(modelInfo.roll, QVector3D(0.0, 0.0, 1.0));
        m_ShaderProgram.setUniformValue("model", model);
        modelInfo.model->Draw(m_ShaderProgram);
    }

    // draw skybox as last
    glDepthFunc(GL_LEQUAL); // 确保天空盒=1也能通过测试
    m_SkyBoxShaderProgram.bind();
    QMatrix4x4 skyboxView = view;// remove translation from the view matrix
    skyboxView.setColumn(3, QVector4D(0.0f,0.0f,0.0f,1.0f));
    m_SkyBoxShaderProgram.setUniformValue("projection", projection);
    m_SkyBoxShaderProgram.setUniformValue("view", skyboxView);
    // skybox cube
    glBindVertexArray(skyboxVAO);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, m_CubeMap->textureId());
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
    glDepthFunc(GL_LESS); // set depth function back to default
    m_SkyBoxShaderProgram.release();
}

void NOpenGLWidget::wheelEvent(QWheelEvent *event)
{
    // y轴上下波动范围，一步是120
    m_camera.ProcessMouseScroll(event->angleDelta().y()/120);
}

void NOpenGLWidget::keyPressEvent(QKeyEvent *event)
{
    float deltaTime = timeOutmSec / 1000.0f;

    switch (event->key()) {
        case Qt::Key_W: m_camera.ProcessKeyboard(FORWARD, deltaTime); break;
        case Qt::Key_S: m_camera.ProcessKeyboard(BACKWARD, deltaTime); break;
        case Qt::Key_D: m_camera.ProcessKeyboard(RIGHT, deltaTime); break;
        case Qt::Key_A: m_camera.ProcessKeyboard(LEFT, deltaTime); break;
        case Qt::Key_Q: m_camera.ProcessKeyboard(DOWN, deltaTime); break;
        case Qt::Key_E: m_camera.ProcessKeyboard(UP, deltaTime); break;
        case Qt::Key_Space: m_camera.Position = viewInitPos; break;

        default: break;
    }
}

void NOpenGLWidget::mouseMoveEvent(QMouseEvent *event)
{
    makeCurrent();
    if (m_modelMoving) {
        for (auto iter = m_Models.begin(); iter != m_Models.end(); iter++) {
            ModelInfo *modelInfo = &iter.value();
            if (!modelInfo->isSelected) continue;
            modelInfo->worldPos =
                    (QVector3D)worldPositionFromViewPort(event->pos().x(), event->pos().y());
        }
    } else
        if (event->buttons() & Qt::RightButton
                || event->buttons() & Qt::LeftButton
                || event->buttons() & Qt::MiddleButton) {
            auto currentPos = event->pos();  // 当前位置
            QPoint deltaPos = currentPos - lastPos;
            lastPos = currentPos;
            if (event->buttons() & Qt::RightButton) {
                m_camera.ProcessMouseMovement(deltaPos.x(), -deltaPos.y());
            } else {
                for (auto iter = m_Models.begin(); iter != m_Models.end(); iter++) {
                    ModelInfo *modelInfo = &iter.value();
                    if (!modelInfo->isSelected) continue;
                    if (event->buttons() & Qt::MiddleButton) {
                        modelInfo->roll += deltaPos.x();
                    }
                    else if (event->buttons() & Qt::LeftButton) {
                        modelInfo->yaw += deltaPos.x();
                        modelInfo->pitch += deltaPos.y();
                    }
                }
            }
        }
    doneCurrent();
}

void NOpenGLWidget::mousePressEvent(QMouseEvent *event)
{
    bool hasSelected = false;
    makeCurrent();
    lastPos = event->pos();
    if (event->button() & Qt::LeftButton) {
        QVector4D worldPosition = worldPositionFromViewPort(event->pos().x(), event->pos().y());
        // 调用signals
        mousePickingPos(QVector3D(worldPosition));
        // 之后由mainwindow接收 - slots

        for (QMap<QString, ModelInfo>::iterator iter = m_Models.begin(); iter != m_Models.end(); iter++) {
            ModelInfo *modelInfo = &iter.value();
            float r = (modelInfo->model->m_maxY - modelInfo->model->m_minY) / 2.0f;
            if (modelInfo->worldPos.distanceToPoint(QVector3D(worldPosition)) < r && !hasSelected) {
                modelInfo->isSelected = true;
                hasSelected = true;
            }
            else
                modelInfo->isSelected = false;
        }
    }
    doneCurrent();
}

void NOpenGLWidget::mouseDoubleClickEvent(QMouseEvent *event)
{
    // 第一次双击进入状态，再一次双击退出状态
    Q_UNUSED(event);
    makeCurrent();
    if (m_modelMoving) {
        // 再次双击取消移动
        m_modelMoving = false;
        return;
    } else
    for (auto& modelInfo : m_Models) {
        // 双击启动移动
        if (modelInfo.isSelected == true) {
            m_modelMoving = true;
        }
        qDebug() << modelInfo.name << modelInfo.isSelected;
    }
    doneCurrent();
}

void NOpenGLWidget::on_timeout()
{
    update();
}

QVector3D NOpenGLWidget::cameraPosInit(float maxY, float minY)
{
    // 其实只要y的值来判断高度即可
    float height = maxY - minY;
    QVector3D temp{0.0, 0.0, 0.0};

    temp.setZ(1.5 * height); // 摄像机与模型的距离
    if (minY >= 0) {
        temp.setY(height / 2.0);
    }

    // 更改初始位置的值
    viewInitPos = temp;

    return temp;
}

Mesh* NOpenGLWidget::advProcessMesh(float *vertices, int size, unsigned int textureld)
{
    // 这个函数在initialization中调用，事先生成地面和箱子
    vector<Vertex> _vertices;
    vector<unsigned int> _indices;
    vector<Texture> _textures;

    // memcpy()不再适用
    for (unsigned int i = 0; i < size; i++) {
        Vertex vert;
        vert.Position[0] = vertices[i*5+0];
        vert.Position[1] = vertices[i*5+1];
        vert.Position[2] = vertices[i*5+2];
        vert.TexCoords[0] = vertices[i*5+3];
        vert.TexCoords[1] = vertices[i*5+4];
        _vertices.push_back(vert);
        _indices.push_back(i);
    }
    Texture tex;
    tex.id = textureld;
    tex.type = "texture_diffuse";
    _textures.push_back(tex);

    auto funcs = QOpenGLVersionFunctionsFactory::get<QOpenGLFunctions_4_5_Core>(QOpenGLContext::currentContext());
    return new Mesh(funcs, _vertices, _indices, _textures);
}

QVector4D NOpenGLWidget::worldPositionFromViewPort(int posX, int posY)
{
    // 屏幕坐标都是int型的，所以传进来的是int型的
    makeCurrent();
    float winZ;
    glReadPixels(posX, this->height()-posY,1,1,GL_DEPTH_COMPONENT,GL_FLOAT,&winZ);
    float x = (2.0f * posX) / this->width() - 1.0f;
    float y = 1.0f - (2.0f * posY) / this->height();
    float z = winZ * 2.0 - 1.0f;
    float w = (2.0 * _near * _far) / (_far + _near - z * (_far - _near));
    QVector4D worldPosition(x, y, z, 1);
    worldPosition *= w;
    worldPosition = view.inverted() * projection.inverted() * worldPosition;
    doneCurrent();
    return worldPosition;
}
