#include "nopenglwidget.h"
#include "vertices.h"

unsigned int fbo;
unsigned int texture;
unsigned int rbo;

unsigned int quadVAO = 0;
unsigned int quadVBO;

const unsigned int timeOutmSec = 50;
const float PI = 3.1415926f;
float _near = 0.1f, _far = 100.0f;

QVector3D lightColor(1.0f, 1.0f, 1.0f);

QVector3D viewInitPos = QVector3D(0.0f, 0.0f, 3.0f);

QMatrix4x4 model;
QMatrix4x4 view;
QMatrix4x4 projection;
QVector3D lightPos(0.0f, 0.0f, 0.0f);

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

void NOpenGLWidget::renderQuad()
{
    if (quadVAO == 0)
    {
        // positions
        QVector3D pos1(-1.0f,  1.0f, 0.0f);
        QVector3D pos2(-1.0f, -1.0f, 0.0f);
        QVector3D pos3( 1.0f, -1.0f, 0.0f);
        QVector3D pos4( 1.0f,  1.0f, 0.0f);
        // texture coordinates
        QVector2D uv1(0.0f, 1.0f);
        QVector2D uv2(0.0f, 0.0f);
        QVector2D uv3(1.0f, 0.0f);
        QVector2D uv4(1.0f, 1.0f);
        // normal vector
        QVector3D nm(0.0f, 0.0f, 1.0f);

        // calculate tangent/bitangent vectors of both triangles
        QVector3D tangent1, bitangent1;
        QVector3D tangent2, bitangent2;
        // triangle 1
        // ----------
        QVector3D edge1 = pos2 - pos1;
        QVector3D edge2 = pos3 - pos1;
        QVector2D deltaUV1 = uv2 - uv1;
        QVector2D deltaUV2 = uv3 - uv1;

        float f = 1.0f / (deltaUV1.x() * deltaUV2.y() - deltaUV2.x() * deltaUV1.y());

        tangent1.setX(f * (deltaUV2.y() * edge1.x() - deltaUV1.y() * edge2.x()));
        tangent1.setY(f * (deltaUV2.y() * edge1.y() - deltaUV1.y() * edge2.y()));
        tangent1.setZ(f * (deltaUV2.y() * edge1.z() - deltaUV1.y() * edge2.z()));

        bitangent1.setX(f * (-deltaUV2.x() * edge1.x() + deltaUV1.x() * edge2.x()));
        bitangent1.setY(f * (-deltaUV2.x() * edge1.y() + deltaUV1.x() * edge2.y()));
        bitangent1.setZ(f * (-deltaUV2.x() * edge1.z() + deltaUV1.x() * edge2.z()));

        // triangle 2
        // ----------
        edge1 = pos3 - pos1;
        edge2 = pos4 - pos1;
        deltaUV1 = uv3 - uv1;
        deltaUV2 = uv4 - uv1;

        f = 1.0f / (deltaUV1.x() * deltaUV2.y() - deltaUV2.x() * deltaUV1.y());

        tangent2.setX(f * (deltaUV2.y() * edge1.x() - deltaUV1.y() * edge2.x()));
        tangent2.setY(f * (deltaUV2.y() * edge1.y() - deltaUV1.y() * edge2.y()));
        tangent2.setZ(f * (deltaUV2.y() * edge1.z() - deltaUV1.y() * edge2.z()));


        bitangent2.setX(f * (-deltaUV2.x() * edge1.x() + deltaUV1.x() * edge2.x()));
        bitangent2.setY(f * (-deltaUV2.x() * edge1.y() + deltaUV1.x() * edge2.y()));
        bitangent2.setZ(f * (-deltaUV2.x() * edge1.z() + deltaUV1.x() * edge2.z()));


        float quadVertices[] = {
            // positions            // normal         // texcoords  // tangent                          // bitangent
            pos1.x(), pos1.y(), pos1.z(), nm.x(), nm.y(), nm.z(), uv1.x(), uv1.y(), tangent1.x(), tangent1.y(), tangent1.z(), bitangent1.x(), bitangent1.y(), bitangent1.z(),
            pos2.x(), pos2.y(), pos2.z(), nm.x(), nm.y(), nm.z(), uv2.x(), uv2.y(), tangent1.x(), tangent1.y(), tangent1.z(), bitangent1.x(), bitangent1.y(), bitangent1.z(),
            pos3.x(), pos3.y(), pos3.z(), nm.x(), nm.y(), nm.z(), uv3.x(), uv3.y(), tangent1.x(), tangent1.y(), tangent1.z(), bitangent1.x(), bitangent1.y(), bitangent1.z(),

            pos1.x(), pos1.y(), pos1.z(), nm.x(), nm.y(), nm.z(), uv1.x(), uv1.y(), tangent2.x(), tangent2.y(), tangent2.z(), bitangent2.x(), bitangent2.y(), bitangent2.z(),
            pos3.x(), pos3.y(), pos3.z(), nm.x(), nm.y(), nm.z(), uv3.x(), uv3.y(), tangent2.x(), tangent2.y(), tangent2.z(), bitangent2.x(), bitangent2.y(), bitangent2.z(),
            pos4.x(), pos4.y(), pos4.z(), nm.x(), nm.y(), nm.z(), uv4.x(), uv4.y(), tangent2.x(), tangent2.y(), tangent2.z(), bitangent2.x(), bitangent2.y(), bitangent2.z()
        };
        // configure plane VAO
        glGenVertexArrays(1, &quadVAO);
        glGenBuffers(1, &quadVBO);
        glBindVertexArray(quadVAO);
        glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 14 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 14 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 14 * sizeof(float), (void*)(6 * sizeof(float)));
        glEnableVertexAttribArray(3);
        glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 14 * sizeof(float), (void*)(8 * sizeof(float)));
        glEnableVertexAttribArray(4);
        glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, 14 * sizeof(float), (void*)(11 * sizeof(float)));
    }
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}

void NOpenGLWidget::initializeGL()
{
    // 只使用一次的建议放在这个函数中
    initializeOpenGLFunctions();

    //创建VBO和VAO对象，并赋予ID
    bool success;
    m_ShaderProgram.addShaderFromSourceFile(QOpenGLShader::Vertex,":/shaders/shapes.vert");
    m_ShaderProgram.addShaderFromSourceFile(QOpenGLShader::Fragment,":/shaders/shapes.frag");
    success=m_ShaderProgram.link();
    if(!success) qDebug()<<"ERR:"<<m_ShaderProgram.log();

    m_PlaneDiffuseTex = new QOpenGLTexture(QImage(":/images/bricks2.jpg").mirrored());
    m_PlaneNormalTex = new QOpenGLTexture(QImage(":/images/bricks2_normal.jpg").mirrored());
    m_PlaneHeightTex = new QOpenGLTexture(QImage(":/images/bricks2_disp.jpg").mirrored());

    m_PlaneMesh = advProcessMesh(quadVertices, 6, m_PlaneDiffuseTex->textureId());
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

    projection.perspective(m_camera.Zoom, (float)width()/height(), _near, _far);
    view = m_camera.GetViewMatrix();

    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    m_ShaderProgram.bind();
    m_ShaderProgram.setUniformValue("projection", projection);
    m_ShaderProgram.setUniformValue("view", view);
    m_ShaderProgram.setUniformValue("viewPos",m_camera.Position);
    m_ShaderProgram.setUniformValue("model", model);
    m_ShaderProgram.setUniformValue("lightPos", QVector3D(0.5f, 1.0f, 0.3f));

    m_ShaderProgram.setUniformValue("normalMap", 1);
    m_ShaderProgram.setUniformValue("heightMap", 2);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_PlaneDiffuseTex->textureId());
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, m_PlaneNormalTex->textureId());
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, m_PlaneHeightTex->textureId());
    renderQuad();
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
        vert.Position[0] = vertices[i*8+0];
        vert.Position[1] = vertices[i*8+1];
        vert.Position[2] = vertices[i*8+2];
        vert.Normal[0] = vertices[i*8+3];
        vert.Normal[1] = vertices[i*8+4];
        vert.Normal[2] = vertices[i*8+5];
        vert.TexCoords[0] = vertices[i*8+6];
        vert.TexCoords[1] = vertices[i*8+7];
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
