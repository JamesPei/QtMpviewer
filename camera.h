#ifndef CAMERA_H
#define CAMERA_H

#include <QVector3D>
#include <QMatrix4x4>
#include <QKeyEvent>
#include <QtMath>

// Defines several possible options for camera movement. Used as abstraction to stay away from window-system specific input methods
enum Camera_Movement {
  FORWARD,
  BACKWARD,
  LEFT,
  RIGHT,
  UP,
  DOWN
};

// Default camera values
const float YAW         = -90.0f;
const float PITCH       =  0.0f;
const float SPEED       =  2.5f;
const float SENSITIVITY =  0.1f;
const float ZOOM        =  45.0f;

class Camera {
public:
    QVector3D position;
    QVector3D front;
    QVector3D up;
    QVector3D right;
    QVector3D worldUp;

    //Eular Angles
    float yaw;
    float pitch;
    //Camera options
    float movementSpeed;
    float mouseSensitivity;
    float zoom;

    Camera(QVector3D position = QVector3D(0.0f, 0.0f, 0.0f),
           QVector3D front = QVector3D(0.0f, 0.0f, -1.0f),
           QVector3D up = QVector3D(0.0f, 1.0f, 0.0f),
           float yaw = YAW,
           float pitch = PITCH);

    Camera(float posX, float posY, float posZ, float upX, float upY, float upZ, float yaw, float pitch);

    ~Camera();

    QMatrix4x4 getViewMatrix();

    void processKeyboard(Camera_Movement direction, float deltaTime);
    void processMouseMovement(float xoffset, float yoffset, bool constraintPitch = true);
    void processMouseScroll(float yoffset);
    void processInput(float dt);

    //Keyboard multi-touch
    bool keys[1024];
private:
    void updateCameraVectors();

};

#endif // CAMERA_H
