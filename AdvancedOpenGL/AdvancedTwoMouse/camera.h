#pragma once
#ifndef CAMERA_H
#define CAMERA_H

#include <QMatrix4x4>
#include <vector>
#include <cmath>
#include <QOpenGLWidget>

// 移动方向枚举量，是一种抽象，以避开特定于窗口系统的输入方法
// 这里使用WSAD
enum Camera_Movement {
    FORWARD,
    BACKWARD,
    LEFT,
    RIGHT,
    UP,
    DOWN
};

// 默认值
const float YAW = -90.0f;
const float PITCH = 0.0f;
const float SPEED = 2.5f;
const float SENSITIVITY = 0.1F;
const float ZOOM = 45.0f;

// 一个抽象的camera类，用于处理输入并计算相应的Euler角度、向量和矩阵，以便在OpenGL中使用
class Camera
{
public:
    // camera Attributes
    QVector3D Position;
    QVector3D Front;
    QVector3D Up;
    QVector3D Right;
    QVector3D WorldUp;
    // euler Angles
    float Yaw;
    float Pitch;
    // camera options
    float MovementSpeed{SPEED};
    float MouseSensitivity{SENSITIVITY};
    float Zoom{ZOOM};

    // constructor with vectors
    Camera(QVector3D position = QVector3D(0.0f, 0.0f, 0.0f), QVector3D up = QVector3D(0.0f, 3.0f, 0.0f),
           float yaw = YAW, float pitch = PITCH)
    {
        Position = position;
        WorldUp = up;
        Yaw = yaw;
        Pitch = pitch;
        updateCameraVectors();
    }

    // constructor with scalar values
    Camera(float posX, float posY, float posZ, float upX, float upY, float upZ,
           float yaw = YAW, float pitch = PITCH)
    {
        Position = QVector3D(posX, posY, posZ);
        WorldUp = QVector3D(upX, upY, upZ);
        Yaw = yaw;
        Pitch = pitch;
        updateCameraVectors();
    }

    // returns the view matrix calculated using Euler Angles and the LookAt Matrix
    QMatrix4x4 GetViewMatrix()
    {
        QMatrix4x4 theMatrix;
        //QMatrix4x4 theMatrix = calculate_lookAt_Matrix(Position, Position + Front, Up);
        theMatrix.lookAt(Position, Position + Front, Up);
        return theMatrix;
    }

    // 处理从任何类似键盘的输入系统接收的输入，接收摄像机定义枚举形式的输入参数
    void ProcessKeyboard(Camera_Movement direction, float deltaTime)
    {
        float velocity = MovementSpeed * deltaTime;
        if (direction == FORWARD)
            Position += Front * velocity;
        if (direction == BACKWARD)
            Position -= Front * velocity;
        if (direction == LEFT)
            Position -= Right * velocity;
        if (direction == RIGHT)
            Position += Right * velocity;
        if (direction == UP)
            Position += Up * velocity;
        if (direction == DOWN)
            Position -= Up * velocity;


        // FPS摄像机，让user停留在地面上
        // Position.setY(0.0f);
    }

    // 处理从鼠标输入接收的输入，需要x和y方向上的偏移值
    void ProcessMouseMovement(float xoffset, float yoffset)
    {
        xoffset *= MouseSensitivity;
        yoffset *= MouseSensitivity;

        Yaw += xoffset;
        Pitch += yoffset;

        // 确保当投球超出边界时，屏幕不会翻转
        //if (constrainPitch) {
        //    if (Pitch > 89.0f) Pitch = 89.0f;
        //    if (Pitch < -89.0f) Pitch = -89.0f;
        //}

        if (Pitch > 89.0f) Pitch = 89.0f;
        if (Pitch < -89.0f) Pitch = -89.0f;

        // 使用更新的Euler角度更新前、右和上矢量
        updateCameraVectors();
    }

    // 处理从鼠标滚轮事件接收的输入，仅需要在垂直车轮轴上输入
    void ProcessMouseScroll(float yoffset)
    {
        Zoom -= (float)yoffset;
        if (Zoom < 1.0f) Zoom = 1.0f;
        if (Zoom > 75.0f) Zoom = 75.0f;
    }

private:
    // 根据相机的（更新的）Euler角度计算前矢量
    void updateCameraVectors()
    {
        // calculate the new Front vector
        float PI = 3.1415926f;
        QVector3D front;
        front.setX(std::cos(Yaw*PI/180.0) * std::cos(Pitch*PI/180.0));
        front.setY(std::sin(Pitch*PI/180.0));
        front.setZ(std::sin(Yaw*PI/180.0) * std::cos(Pitch*PI/180.0));
        front.normalize();
        Front = front;
        // also re-calculate the Right and Up vector
        Right = QVector3D::crossProduct(Front, WorldUp);
        // 标准化向量，因为向上或向下看得更多，向量的长度就越接近0，这会导致移动速度变慢
        Right.normalize();
        Up = QVector3D::crossProduct(Right, Front);
        Up.normalize();
    }

    // FPS摄像机，不能随意飞行，只能够呆在xz平面上，人为地将y值设为0即可。

    // 创建自己的LookAt函数
    QMatrix4x4 calculate_lookAt_Matrix(QVector3D position, QVector3D target, QVector3D worldUp)
    {
        // 1. Position = known
        // 2. Calculate cameraDirection
        QVector3D direction = QVector3D(position - target);
        direction.normalize();

        worldUp.normalize();
        QVector3D right = QVector3D::crossProduct(worldUp, direction);
        right.normalize();

        QVector3D up = QVector3D::crossProduct(direction, right);
        //Up.normalize();

        // Create translation and rotation matrix
        QMatrix4x4 translation;  // Identity matrix by default
        QVector4D theVector4D;
        theVector4D.setX(-position.x());
        theVector4D.setY(-position.y());
        theVector4D.setZ(-position.z());
        theVector4D.setW(1.0);
        translation.setColumn(3, theVector4D);

        QMatrix4x4 rotation;
        theVector4D.setX(right.x());  // First column, first row
        theVector4D.setY(right.y());
        theVector4D.setZ(right.z());
        theVector4D.setW(0.0);
        rotation.setColumn(0, theVector4D);

        theVector4D.setX(up.x());  // First column, second row
        theVector4D.setY(up.y());
        theVector4D.setZ(up.z());
        theVector4D.setW(0.0);
        rotation.setColumn(1, theVector4D);

        theVector4D.setX(direction.x());  // First column, third row
        theVector4D.setY(direction.y());
        theVector4D.setZ(direction.z());
        theVector4D.setW(0.0);
        rotation.setColumn(2, theVector4D);

        // Return lookAt matrix as combination of translation and rotation matrix
        return rotation * translation;
    }
};
#endif // CAMERA_H
