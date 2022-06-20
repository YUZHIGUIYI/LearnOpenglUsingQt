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

QVector3D viewInitPos = QVector3D(0.0f, 0.0f, 5.0f);

QMatrix4x4 model;
QMatrix4x4 view;
QMatrix4x4 projection;
QVector3D lightPos(0.0f, 0.0f, 0.0f);

QPoint lastPos;  // 在点击的时候赋值

// lighting info
// -------------
// positions
std::vector<QVector3D> lightPositions;

// colors
std::vector<QVector3D> lightColors;

unsigned int hdrFBO;
unsigned int colorBuffer;
NOpenGLWidget::NOpenGLWidget(QWidget *parent) : QOpenGLWidget(parent)
{
    // 定时刷新的条件
    connect(&m_timer, SIGNAL(timeout()), this, SLOT(on_timeout()));
    m_timer.start(timeOutmSec);
    m_time.start();
    m_camera.Position = viewInitPos;

    setFocusPolicy(Qt::StrongFocus);  // 键盘响应
    setMouseTracking(true);

    lightPositions.push_back(QVector3D( 0.0f,  0.0f, 49.5f)); // back light
    lightPositions.push_back(QVector3D(-1.4f, -1.9f, 9.0f));
    lightPositions.push_back(QVector3D( 0.0f, -1.8f, 4.0f));
    lightPositions.push_back(QVector3D( 0.8f, -1.7f, 6.0f));

    lightColors.push_back(QVector3D(200.0f, 200.0f, 200.0f));
    lightColors.push_back(QVector3D(0.1f, 0.0f, 0.0f));
    lightColors.push_back(QVector3D(0.0f, 0.0f, 0.2f));
    lightColors.push_back(QVector3D(0.0f, 0.1f, 0.0f));
}

NOpenGLWidget::~NOpenGLWidget()
{
    // 回收内存
    for (auto iter = m_Models.begin(); iter != m_Models.end(); iter++) {
        ModelInfo *modelInfo = &iter.value(); // 取出结构
        delete modelInfo->model;
    }
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
        float quadVertices[] = {
                                // positions        // texture Coords
                                -1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
                                -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
                                1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
                                1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
                                };
        // setup plane VAO
        glGenVertexArrays(1, &quadVAO);
        glGenBuffers(1, &quadVBO);
        glBindVertexArray(quadVAO);
        glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    }
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
}

void NOpenGLWidget::initializeGL()
{
    // 只使用一次的建议放在这个函数中
    initializeOpenGLFunctions();

    glGenFramebuffers(1, &hdrFBO);
    // create floating point color buffer

    glGenTextures(1, &colorBuffer);
    glBindTexture(GL_TEXTURE_2D, colorBuffer);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width(), height(), 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    // create depth buffer (renderbuffer)
    unsigned int rboDepth;
    glGenRenderbuffers(1, &rboDepth);
    glBindRenderbuffer(GL_RENDERBUFFER, rboDepth);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, width(), height());
    // attach buffers
    glBindFramebuffer(GL_FRAMEBUFFER, hdrFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorBuffer, 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rboDepth);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        qDebug() << "Framebuffer not complete!";
    glBindFramebuffer(GL_FRAMEBUFFER, defaultFramebufferObject());

    bool success;
    m_LightingProgram.addShaderFromSourceFile(QOpenGLShader::Vertex,":/shaders/lighting.vert");
    m_LightingProgram.addShaderFromSourceFile(QOpenGLShader::Fragment,":/shaders/lighting.frag");
    success=m_LightingProgram.link();
    if(!success) qDebug()<<"ERR:"<<m_LightingProgram.log();
    m_LightingProgram.bind();
    for (unsigned int i = 0; i < lightPositions.size(); i++){
        std::string str="lights[" + std::to_string(i) + "].Position";
        m_LightingProgram.setUniformValue(str.c_str(), lightPositions[i]);
        str="lights[" + std::to_string(i) + "].Color";
        m_LightingProgram.setUniformValue(str.c_str(), lightColors[i]);
    }
    m_LightingProgram.release();
    m_ShaderProgram.addShaderFromSourceFile(QOpenGLShader::Vertex,":/shaders/shapes.vert");
    m_ShaderProgram.addShaderFromSourceFile(QOpenGLShader::Fragment,":/shaders/shapes.frag");
    success=m_ShaderProgram.link();
    if(!success) qDebug()<<"ERR:"<<m_ShaderProgram.log();

    // m_WallTex=new QOpenGLTexture(QImage(":/images/images/wall.jpg").mirrored());
    QImage wall = QImage(":/images/wood.png").convertToFormat(QImage::Format_RGB888);
    m_WallTex=new QOpenGLTexture(QOpenGLTexture::Target2D);
    glBindTexture(GL_TEXTURE_2D, m_WallTex->textureId());

    glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB, wall.width(), wall.height(),
                 0, GL_RGB , GL_UNSIGNED_BYTE, wall.bits());
    glGenerateMipmap(GL_TEXTURE_2D);

    m_CubeMesh = advProcessMesh(vertices,36,m_WallTex->textureId());
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

    glBindFramebuffer(GL_FRAMEBUFFER, hdrFBO);
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    // glDepthFunc(GL_LESS);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    m_LightingProgram.bind();
    m_LightingProgram.setUniformValue("projection", projection);
    m_LightingProgram.setUniformValue("view", view);
    m_LightingProgram.setUniformValue("viewPos",m_camera.Position);
    model.translate(QVector3D(0.0f, 0.0f, 25.0));
    model.scale(QVector3D(2.5f, 2.5f, 27.5f));
    m_LightingProgram.setUniformValue("model", model);
    //m_ShaderProgram.setUniformValue("lightPos", QVector3D(0.5f, 1.0f, 0.3f));

    m_LightingProgram.setUniformValue("inverse_normals", true);
    m_CubeMesh->Draw(m_LightingProgram);
    glBindFramebuffer(GL_FRAMEBUFFER, defaultFramebufferObject());
    m_LightingProgram.release();

    m_ShaderProgram.bind();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, colorBuffer);
    m_ShaderProgram.setUniformValue("hdrBuffer", 0);
    m_ShaderProgram.setUniformValue("hdr", m_HDR);
    m_ShaderProgram.setUniformValue("exposure", m_exposure);
    this->renderQuad();
    m_ShaderProgram.release();
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

void NOpenGLWidget::setHDR(bool checked)
{
    m_HDR = checked;
}

void NOpenGLWidget::setExposure(double exposure)
{
    m_exposure = exposure;
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
