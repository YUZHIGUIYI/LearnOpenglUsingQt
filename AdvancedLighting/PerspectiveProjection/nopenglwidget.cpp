#include "nopenglwidget.h"
#include "vertices.h"

unsigned int fbo;
unsigned int texture;
unsigned int rbo;
unsigned int skyboxVAO;
unsigned int skyboxVBO;
unsigned int depthMapFBO;
unsigned int depthCubeMap;

const unsigned int SHADOW_WIDTH = 2048, SHADOW_HEIGHT = 2048;
const unsigned int timeOutmSec = 50;
const float PI = 3.1415926f;
float _near = 0.1f, _far = 100.0f;

QVector3D lightColor(1.0f, 1.0f, 1.0f);

QVector3D viewInitPos = QVector3D(0.0f, 0.0f, 0.0f);

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

void NOpenGLWidget::renderScene(QOpenGLShaderProgram *shader)
{
    model.setToIdentity();
    // room cube
    model.scale(5.0f);
    shader->setUniformValue("model", model);
    glDisable(GL_CULL_FACE); // note that we disable culling here since we render 'inside' the cube instead of the usual 'outside' which throws off the normal culling methods.
    shader->setUniformValue("reverse_normals", 1); // A small little hack to invert normals when drawing cube from the inside so lighting still works.
    m_CubeMesh->Draw(*shader);
    shader->setUniformValue("reverse_normals", 0); // and of course disable it
    glEnable(GL_CULL_FACE);

    // 三个箱子
    model.setToIdentity();
    model.translate(QVector3D(4.0f, -3.5f, 0.0));
    model.scale(0.5);
    shader->setUniformValue("model", model);
    m_CubeMesh->Draw(*shader);

    model.setToIdentity();
    model.translate(QVector3D(2.0f, 3.0f, 1.0));
    model.scale(0.75);
    shader->setUniformValue("model", model);
    m_CubeMesh->Draw(*shader);

    model.setToIdentity();
    model.translate(QVector3D(-3.0f, -1.0f, 0.0));
    model.scale(0.5);
    shader->setUniformValue("model", model);
    m_CubeMesh->Draw(*shader);

    model.setToIdentity();
    model.translate(QVector3D(-1.5f, 1.0f, 1.5));
    model.scale(0.5);
    //model.rotate(60,QVector3D(1.0, 0.0, 1.0));
    shader->setUniformValue("model", model);
    m_CubeMesh->Draw(*shader);

    model.setToIdentity();
    model.translate(QVector3D(-1.5f, 2.0f, -3.0));
    model.scale(0.75);
    model.rotate(60,QVector3D(1.0, 0.0, 1.0));
    shader->setUniformValue("model", model);
    m_CubeMesh->Draw(*shader);
}

void NOpenGLWidget::initializeGL()
{
    // 只使用一次的建议放在这个函数中
    initializeOpenGLFunctions();

    // 物体着色器
    bool success;
    m_ShaderProgram.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/shaders/shapes.vert");
    m_ShaderProgram.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/shaders/shapes.frag");
    success = m_ShaderProgram.link();
    if (!success) qDebug() << "Error:" << m_ShaderProgram.log();
    //
    m_DepthMapShaderProgram.addShaderFromSourceFile(QOpenGLShader::Vertex,":/shaders/depthMap.vert");
    m_DepthMapShaderProgram.addShaderFromSourceFile(QOpenGLShader::Geometry,":/shaders/depthMap.geom");
    m_DepthMapShaderProgram.addShaderFromSourceFile(QOpenGLShader::Fragment,":/shaders/depthMap.frag");
    success=m_DepthMapShaderProgram.link();
    if(!success) qDebug()<<"ERR:"<<m_DepthMapShaderProgram.log();
    //
    m_QuadShaderProgram.addShaderFromSourceFile(QOpenGLShader::Vertex,":/shaders/quad.vert");
    m_QuadShaderProgram.addShaderFromSourceFile(QOpenGLShader::Fragment,":/shaders/quad.frag");
    success=m_QuadShaderProgram.link();
    if(!success) qDebug()<<"ERR:"<<m_QuadShaderProgram.log();

    //
    m_DiffuseTex = new QOpenGLTexture(QImage(":/images/wall.jpg").mirrored());
    // 为渲染的深度贴图创建一个帧缓冲对象
    glGenFramebuffers(1, &depthMapFBO);

    // 创建2D纹理
    glGenTextures(1, &depthCubeMap);
    glBindTexture(GL_TEXTURE_CUBE_MAP, depthCubeMap);
    for (unsigned int i = 0; i < 6; ++i) {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_DEPTH_COMPONENT,
                     SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // 将创建的纹理提供给帧缓冲作为深度附件
    glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depthCubeMap, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);  // 显式告诉OpenGL我们不使用颜色数据进行渲染

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        qDebug() << "ERROR::FRAMEBUFFER:: Framebuffer is not complete! \n";
    glBindFramebuffer(GL_FRAMEBUFFER, defaultFramebufferObject());

    m_CubeMesh = advProcessMesh(vertices, 36, m_DiffuseTex->textureId());
    glEnable(GL_CULL_FACE);
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

    //projection.perspective(m_camera.Zoom, (float)width()/height(), _near, _far);
    //view = m_camera.GetViewMatrix();

    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // create depth cubemap transformation matrices
    float near_plane = 1.0f;
    float far_plane  = 25.0f;
    QMatrix4x4 shadowProj;
    QMatrix4x4 shadowView;
    shadowProj.perspective(90.0f, (float)SHADOW_WIDTH / (float)SHADOW_HEIGHT, near_plane, far_plane);
    std::vector<QMatrix4x4> shadowTransforms;
    shadowView.lookAt(lightPos, lightPos + QVector3D( 1.0, 0.0, 0.0), QVector3D(0.0,-1.0, 0.0));
    shadowTransforms.push_back(shadowProj * shadowView);    shadowView.setToIdentity();
    shadowView.lookAt(lightPos, lightPos + QVector3D(-1.0, 0.0, 0.0), QVector3D(0.0,-1.0, 0.0));
    shadowTransforms.push_back(shadowProj * shadowView);    shadowView.setToIdentity();
    shadowView.lookAt(lightPos, lightPos + QVector3D( 0.0, 1.0, 0.0), QVector3D(0.0, 0.0, 1.0));
    shadowTransforms.push_back(shadowProj * shadowView);    shadowView.setToIdentity();
    shadowView.lookAt(lightPos, lightPos + QVector3D( 0.0,-1.0, 0.0), QVector3D(0.0, 0.0,-1.0));
    shadowTransforms.push_back(shadowProj * shadowView);    shadowView.setToIdentity();
    shadowView.lookAt(lightPos, lightPos + QVector3D( 0.0, 0.0, 1.0), QVector3D(0.0,-1.0, 0.0));
    shadowTransforms.push_back(shadowProj * shadowView);    shadowView.setToIdentity();
    shadowView.lookAt(lightPos, lightPos + QVector3D( 0.0, 0.0,-1.0), QVector3D(0.0,-1.0, 0.0));
    shadowTransforms.push_back(shadowProj * shadowView);

    // 1. render depth of scene to texture (from light's perspective)
    glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
    glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
    glClear(GL_DEPTH_BUFFER_BIT);
    m_DepthMapShaderProgram.bind();
    glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
    glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
    glClear(GL_DEPTH_BUFFER_BIT);
    for (unsigned int i = 0; i < 6; ++i){
        std::string str="shadowMatrices[" + std::to_string(i) + "]";
        m_DepthMapShaderProgram.setUniformValue(str.c_str(), shadowTransforms[i]);
    }
    m_DepthMapShaderProgram.setUniformValue("far_plane", far_plane);
    m_DepthMapShaderProgram.setUniformValue("lightPos", lightPos);
    renderScene(&m_DepthMapShaderProgram);
    glBindFramebuffer(GL_FRAMEBUFFER, defaultFramebufferObject());
    m_DepthMapShaderProgram.release();

    // 2.正常渲染并输出到屏幕
    m_ShaderProgram.bind();
    projection.perspective(m_camera.Zoom,(float)width()/(float)height(),_near,_far);
    view=m_camera.GetViewMatrix();
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glViewport(0, 0, width(), height());
    m_ShaderProgram.setUniformValue("far_plane", far_plane);
    m_ShaderProgram.setUniformValue("lightPos", lightPos);
    m_ShaderProgram.setUniformValue("model", model);
    m_ShaderProgram.setUniformValue("shadows", true);
    m_ShaderProgram.setUniformValue("projection", projection);
    m_ShaderProgram.setUniformValue("view", view);
    m_ShaderProgram.setUniformValue("viewPos",m_camera.Position);
    m_ShaderProgram.setUniformValue("depthMap",1);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_CUBE_MAP, depthCubeMap);
    renderScene(&m_ShaderProgram);
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
