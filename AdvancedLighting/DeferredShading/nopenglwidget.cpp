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

QVector3D viewInitPos = QVector3D(0.0f, 2.0f, 10.0f);

QMatrix4x4 model;
QMatrix4x4 view;
QMatrix4x4 projection;
QVector3D lightPos(0.0f, 0.0f, 0.0f);

QPoint lastPos;  // 在点击的时候赋值

std::vector<QVector3D> objectPositions;
unsigned int gBuffer;
unsigned int gPosition, gNormal, gAlbedoSpec;
const unsigned int NR_LIGHTS = 32;
std::vector<QVector3D> lightPositions;
std::vector<QVector3D> lightColors;

NOpenGLWidget::NOpenGLWidget(QWidget *parent) : QOpenGLWidget(parent)
{
    // 定时刷新的条件
    connect(&m_timer, SIGNAL(timeout()), this, SLOT(on_timeout()));
    m_timer.start(timeOutmSec);
    m_time.start();
    m_camera.Position = viewInitPos;

    setFocusPolicy(Qt::StrongFocus);  // 键盘响应
    setMouseTracking(true);
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
    // renderQuad() renders a 1x1 XY quad in NDC
    // -----------------------------------------
    unsigned int quadVAO = 0;
    unsigned int quadVBO;
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

    glEnable(GL_DEPTH_TEST);
    // configure g-buffer framebuffer
    // ------------------------------
    glGenFramebuffers(1, &gBuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);

    // position color buffer
    glGenTextures(1, &gPosition);
    glBindTexture(GL_TEXTURE_2D, gPosition);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width(), height(), 0, GL_RGB, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, gPosition, 0);
    // normal color buffer
    glGenTextures(1, &gNormal);
    glBindTexture(GL_TEXTURE_2D, gNormal);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width(), height(), 0, GL_RGB, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, gNormal, 0);
    // color + specular color buffer
    glGenTextures(1, &gAlbedoSpec);
    glBindTexture(GL_TEXTURE_2D, gAlbedoSpec);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width(), height(), 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, gAlbedoSpec, 0);
    // tell OpenGL which color attachments we'll use (of this framebuffer) for rendering
    unsigned int attachments[3] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
    glDrawBuffers(3, attachments);
    // create and attach depth buffer (renderbuffer)
    unsigned int rboDepth;
    glGenRenderbuffers(1, &rboDepth);
    glBindRenderbuffer(GL_RENDERBUFFER, rboDepth);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, width(), height());
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rboDepth);
    // finally check if framebuffer is complete
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        qDebug() << "Framebuffer not complete!" ;
    glBindFramebuffer(GL_FRAMEBUFFER, defaultFramebufferObject());

    bool success;
    m_GeometryPassShaderProgram.addShaderFromSourceFile(QOpenGLShader::Vertex,":/shaders/g_buffer.vert");
    m_GeometryPassShaderProgram.addShaderFromSourceFile(QOpenGLShader::Fragment,":/shaders/g_buffer.frag");
    success=m_GeometryPassShaderProgram.link();
    if(!success) qDebug()<<"ERR:"<<m_GeometryPassShaderProgram.log();

    m_LightingPassShaderProgram.addShaderFromSourceFile(QOpenGLShader::Vertex,":/shaders/deferred_shading.vert");
    m_LightingPassShaderProgram.addShaderFromSourceFile(QOpenGLShader::Fragment,":/shaders/deferred_shading.frag");
    success=m_LightingPassShaderProgram.link();
    if(!success) qDebug()<<"ERR:"<<m_LightingPassShaderProgram.log();

    m_LightBoxShaderProgram.addShaderFromSourceFile(QOpenGLShader::Vertex,":/shaders/deferred_light_box.vert");
    m_LightBoxShaderProgram.addShaderFromSourceFile(QOpenGLShader::Fragment,":/shaders/deferred_light_box.frag");
    success=m_LightBoxShaderProgram.link();
    if(!success) qDebug()<<"ERR:"<<m_LightBoxShaderProgram.log();


    //load lights
    m_CubeMesh = advProcessMesh(vertices,36,0);
    // load models
    // -----------
    loadModel("D:/Dev/QtProject/AdvancedOpenGL/backpack/backpack.obj");
    objectPositions.push_back(QVector3D(-3.0, -0.5, -3.0));
    objectPositions.push_back(QVector3D( 0.0, -0.5, -3.0));
    objectPositions.push_back(QVector3D( 3.0, -0.5, -3.0));
    objectPositions.push_back(QVector3D(-3.0, -0.5,  0.0));
    objectPositions.push_back(QVector3D( 0.0, -0.5,  0.0));
    objectPositions.push_back(QVector3D( 3.0, -0.5,  0.0));
    objectPositions.push_back(QVector3D(-3.0, -0.5,  3.0));
    objectPositions.push_back(QVector3D( 0.0, -0.5,  3.0));
    objectPositions.push_back(QVector3D( 3.0, -0.5,  3.0));

    // lighting info
    // -------------
    srand(13);
    for (unsigned int i = 0; i < NR_LIGHTS; i++)
    {
        // calculate slightly random offsets
        float xPos = ((rand() % 100) / 100.0) * 6.0 - 3.0;
        float yPos = ((rand() % 100) / 100.0) * 6.0 - 4.0;
        float zPos = ((rand() % 100) / 100.0) * 6.0 - 3.0;
        lightPositions.push_back(QVector3D(xPos, yPos, zPos));
        // also calculate random color
        float rColor = ((rand() % 100) / 200.0f) + 0.5; // between 0.5 and 1.0
        float gColor = ((rand() % 100) / 200.0f) + 0.5; // between 0.5 and 1.0
        float bColor = ((rand() % 100) / 200.0f) + 0.5; // between 0.5 and 1.0
        lightColors.push_back(QVector3D(rColor, gColor, bColor));
    }
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
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // 1. geometry pass: render scene's geometry/color data into gbuffer
    // -----------------------------------------------------------------
    glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    m_GeometryPassShaderProgram.bind();
    m_GeometryPassShaderProgram.setUniformValue("projection", projection);
    m_GeometryPassShaderProgram.setUniformValue("view", view);
    m_GeometryPassShaderProgram.setUniformValue("viewPos",m_camera.Position);
    for(int i=0;i<objectPositions.size();i++){
        model.setToIdentity();
        model.translate(/*modelInfo.worldPos*/objectPositions[i]);
        m_GeometryPassShaderProgram.setUniformValue("model", model);
        m_Models.first().model->Draw(m_GeometryPassShaderProgram);
    }
    glBindFramebuffer(GL_FRAMEBUFFER, defaultFramebufferObject());
    // 2. lighting pass: calculate lighting by iterating over a screen filled quad pixel-by-pixel using the gbuffer's content.
    // -----------------------------------------------------------------------------------------------------------------------
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    m_LightingPassShaderProgram.bind();
    m_LightingPassShaderProgram.setUniformValue("gPosition", 0);
    m_LightingPassShaderProgram.setUniformValue("gNormal", 1);
    m_LightingPassShaderProgram.setUniformValue("gAlbedoSpec", 2);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, gPosition);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, gNormal);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, gAlbedoSpec);
    // send light relevant uniforms
    for (unsigned int i = 0; i < lightPositions.size(); i++)
    {
        std::string str="lights.Position";
        m_LightingPassShaderProgram.setUniformValue(str.c_str(), lightPositions[i]);
        str="lights.Color";
        m_LightingPassShaderProgram.setUniformValue(str.c_str(), lightColors[i]);
        // update attenuation parameters and calculate radius
        const float constant = 1.0f; // note that we don't send this to the shader, we assume it is always 1.0 (in our case)
        const float linear = 0.7f;
        const float quadratic = 1.8f;
        str="lights.Linear";
        m_LightingPassShaderProgram.setUniformValue(str.c_str(), linear);
        str="lights.Quadratic";
        m_LightingPassShaderProgram.setUniformValue(str.c_str(), quadratic);
        // then calculate radius of light volume/sphere
        const float maxBrightness = std::fmaxf(std::fmaxf(lightColors[i].x(), lightColors[i].y()), lightColors[i].z());
        float radius = (-linear + std::sqrt(linear * linear - 4 * quadratic * (constant - (256.0f / 5.0f) * maxBrightness))) / (2.0f * quadratic);
        str="lights.Radius";
        m_LightingPassShaderProgram.setUniformValue(str.c_str(), radius);
        m_LightingPassShaderProgram.setUniformValue("projection", projection);
        m_LightingPassShaderProgram.setUniformValue("view", view);
        model.setToIdentity();
        model.translate(lightPositions[i]);
        model.scale(radius);
        m_LightingPassShaderProgram.setUniformValue("model",model);
        m_LightingPassShaderProgram.setUniformValue("viewPos", m_camera.Position);
        // finally render quad
        m_CubeMesh->Draw(m_LightingPassShaderProgram);
    }

    m_LightingPassShaderProgram.release();

    // 2.5. copy content of geometry's depth buffer to default framebuffer's depth buffer
    // ----------------------------------------------------------------------------------
    glBindFramebuffer(GL_READ_FRAMEBUFFER, gBuffer);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, defaultFramebufferObject()); // write to default framebuffer
    // blit to default framebuffer. Note that this may or may not work as the internal formats of both the FBO and default framebuffer have to match.
    // the internal formats are implementation defined. This works on all of my systems, but if it doesn't on yours you'll likely have to write to the
    // depth buffer in another shader stage (or somehow see to match the default framebuffer's internal format with the FBO's internal format).
    glBlitFramebuffer(0, 0, width(), height(), 0, 0, width(), height(), GL_DEPTH_BUFFER_BIT, GL_NEAREST);
    glBindFramebuffer(GL_FRAMEBUFFER, defaultFramebufferObject());

    // 3. render lights on top of scene
    // --------------------------------
    m_LightBoxShaderProgram.bind();
    m_LightBoxShaderProgram.setUniformValue("projection", projection);
    m_LightBoxShaderProgram.setUniformValue("view", view);
    for (unsigned int i = 0; i < lightPositions.size(); i++)
    {
        model.setToIdentity();
        model.translate(lightPositions[i]);
        model.scale(0.125f);
        m_LightBoxShaderProgram.setUniformValue("model", model);
        m_LightBoxShaderProgram.setUniformValue("lightColor", lightColors[i]);
        m_CubeMesh->Draw(m_LightBoxShaderProgram);
    }
    m_LightBoxShaderProgram.release();
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
    m_Bloom = checked;
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
