#include "nopenglwidget.h"
#include "vertices.h"

const unsigned int timeOutmSec = 50;
const float PI = 3.1415926;

unsigned int VAO, VBO, lightVAO;

QVector3D lightPos(1.2f, 1.0f, 2.0f);
QVector3D lightColor(1.0f, 1.0f, 1.0f);
QVector3D objectColor(1.0f, 0.5f, 0.31f);

QVector3D viewInitPos = QVector3D(0.0, 0.0, 5.0);

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

NOpenGLWidget::NOpenGLWidget(QWidget *parent) : QOpenGLWidget(parent)
{
    // 定时刷新的条件
    connect(&m_timer, SIGNAL(timeout()), this, SLOT(on_timeout()));
    m_timer.start(timeOutmSec);
    m_time.start();
    m_camera.Position = viewInitPos;

    setFocusPolicy(Qt::StrongFocus);  // 键盘响应
}

NOpenGLWidget::~NOpenGLWidget()
{
    makeCurrent();
    glDeleteBuffers(1, &VBO);
    glDeleteVertexArrays(1, &VAO);
    glDeleteVertexArrays(1, &lightVAO);
    doneCurrent();
}

void NOpenGLWidget::loadModel(string path)
{
    // 需要判断是不是NULL
    // 是，删掉，置空
    if (m_model != NULL)
        delete m_model;
    m_model = NULL;

    // 新的模型
    makeCurrent();
    auto funcs = QOpenGLVersionFunctionsFactory::get<QOpenGLFunctions_4_5_Core>(QOpenGLContext::currentContext());
    m_model = new Model(funcs, path.c_str());

    m_camera.Position = cameraPosInit(m_model->m_maxY, m_model->m_minY);

    doneCurrent();
}

void NOpenGLWidget::initializeGL()
{
    initializeOpenGLFunctions();

    // 物体着色器
    bool success;
    m_ShaderProgram.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/shaders/shapes.vert");
    m_ShaderProgram.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/shaders/shapes.frag");
    success = m_ShaderProgram.link();
    if (!success) qDebug() << "Error:" << m_ShaderProgram.log();
    // 光源
    lightShaderProgram.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/shaders/light.vert");
    lightShaderProgram.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/shaders/light.frag");
    success = lightShaderProgram.link();
    if (!success) qDebug() << "Error:" << lightShaderProgram.log();

    m_BoxDiffuseTex = new QOpenGLTexture(
                QImage(":/images/container2.png").mirrored());
    m_PlaneDiffuseTex = new QOpenGLTexture(
                QImage(":/images/container2_specular.png").mirrored());

    m_BoxMesh = advProcessMesh(cubeVertices, 36, m_BoxDiffuseTex->textureId());
    m_PlaneMesh = advProcessMesh(planeVertices, 6, m_PlaneDiffuseTex->textureId());
}

void NOpenGLWidget::resizeGL(int w, int h)
{
    Q_UNUSED(w);
    Q_UNUSED(h);
}

void NOpenGLWidget::paintGL()
{
    QMatrix4x4 model;
    QMatrix4x4 view;
    QMatrix4x4 projection;
    //float time = m_time.elapsed() / 50.0;
    projection.perspective(m_camera.Zoom, (float)width()/height(), 0.1, 100.0);
    view = m_camera.GetViewMatrix();

    //lightColor.setX(std::sin(time/100 * 2.0f));
    //lightColor.setY(std::sin(time/100 * 0.7f));
    //lightColor.setZ(std::sin(time/100 * 1.3f));
    QVector3D diffuseColor = lightColor * QVector3D(0.5, 0.5, 0.5);
    QVector3D ambientColor = diffuseColor * QVector3D(0.2, 0.2, 0.2);

    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glEnable(GL_DEPTH_TEST);  // 深度缓冲，Z buffer
    //glDepthFunc(GL_ALWAYS);   // 谁后渲染谁在前面，如果改为less则等同于将这行删除
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    m_ShaderProgram.bind();
    // material properties
    m_ShaderProgram.setUniformValue("material.shininess", 48.0f);
    m_ShaderProgram.setUniformValue("projection", projection);
    m_ShaderProgram.setUniformValue("view", view);
    m_ShaderProgram.setUniformValue("viewPos", m_camera.Position);

    // spotLight
    m_ShaderProgram.setUniformValue("spotLight.position", m_camera.Position);
    m_ShaderProgram.setUniformValue("spotLight.direction", m_camera.Front);
    m_ShaderProgram.setUniformValue("spotLight.cutOff", (float)cos(12.5f*PI/180));
    m_ShaderProgram.setUniformValue("spotLight.outerCutOff", (float)cos(17.5f*PI/180));

    m_ShaderProgram.setUniformValue("spotLight.ambient", 0.7f, 0.7f, 0.7f);
    m_ShaderProgram.setUniformValue("spotLight.diffuse", 0.8f, 0.8f, 0.8f);
    m_ShaderProgram.setUniformValue("spotLight.specular", 1.0f, 1.0f, 1.0f);

    m_ShaderProgram.setUniformValue("spotLight.constant", 1.0f);
    m_ShaderProgram.setUniformValue("spotLight.linear", 0.09f);
    m_ShaderProgram.setUniformValue("spotLight.quadratic", 0.032f);
    // dirLight
    m_ShaderProgram.setUniformValue("dirLight.direction", -0.2f, -1.0f, -0.3f);
    m_ShaderProgram.setUniformValue("dirLight.ambient", 0.05f, 0.05f, 0.05f);
    m_ShaderProgram.setUniformValue("dirLight.diffuse", 0.4f, 0.4f, 0.4f);
    m_ShaderProgram.setUniformValue("dirLight.specular", 0.5f, 0.5f, 0.5f);
    // pointLights
    for(int i=0;i<4;i++){
        QString iStr="pointLights["+QString::number(i)+"].position";
        m_ShaderProgram.setUniformValue(iStr.toStdString().c_str(), pointLightPositions[i].x(), pointLightPositions[i].y(), pointLightPositions[i].z());
        iStr="pointLights["+QString::number(i)+"].ambient";
        m_ShaderProgram.setUniformValue(iStr.toStdString().c_str(), pointLightColors[i].x() * 0.1, pointLightColors[i].y() * 0.1, pointLightColors[i].z() * 0.1);
        iStr="pointLights["+QString::number(i)+"].diffuse";
        m_ShaderProgram.setUniformValue(iStr.toStdString().c_str(), pointLightColors[i].x(), pointLightColors[i].y(), pointLightColors[i].z());
        iStr="pointLights["+QString::number(i)+"].specular";
        m_ShaderProgram.setUniformValue(iStr.toStdString().c_str(), pointLightColors[i].x(), pointLightColors[i].y(), pointLightColors[i].z());
        iStr="pointLights["+QString::number(i)+"].constant"; m_ShaderProgram.setUniformValue(iStr.toStdString().c_str(), 1.0f);
        iStr="pointLights["+QString::number(i)+"].linear";
        m_ShaderProgram.setUniformValue(iStr.toStdString().c_str(), 0.09f);
        iStr="pointLights["+QString::number(i)+"].quadratic";
        m_ShaderProgram.setUniformValue(iStr.toStdString().c_str(), 0.032f);
    }

    //model.rotate(time, 1.0f, 1.0f, 0.5f);  // time 为旋转角度
    m_ShaderProgram.setUniformValue("model", model);

    m_BoxMesh->Draw(m_ShaderProgram);
    m_PlaneMesh->Draw(m_ShaderProgram);

    if (m_model == NULL) return;
    m_model->Draw(m_ShaderProgram);
}

void NOpenGLWidget::wheelEvent(QWheelEvent *event)
{
    // y轴上下波动范围，一步是120
    m_camera.ProcessMouseScroll(event->angleDelta().y()/120);
}

void NOpenGLWidget::keyPressEvent(QKeyEvent *event)
{
    float deltaTime = timeOutmSec / 1000.0;

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
    // 当按下鼠标右键时才响应
    if (event->buttons() & Qt::RightButton) {
        static QPoint lastPos(width()/2, height()/2);  // 第一次是在屏幕中间
        auto currentPos = event->pos();  // 当前位置
        QPoint deltaPos = currentPos - lastPos;
        lastPos = currentPos;

        m_camera.ProcessMouseMovement(deltaPos.x(), -deltaPos.y());
    }
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
    for (size_t i = 0; i < size; i++) {
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
