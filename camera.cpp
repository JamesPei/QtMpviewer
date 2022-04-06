#include "camera.h"
#include <QDebug>

Camera::Camera(QVector3D position, QVector3D front, QVector3D up, float yaw, float pitch):
    position(position),
    front(front),
    worldUp(up),
    yaw(yaw),
    pitch(pitch),
    movementSpeed(SPEED),
    mouseSensitivity(SENSITIVITY),
    zoom(ZOOM){
    updateCameraVectors();

    for(uint i = 0; i != 1024; ++i)
        keys[i] = false;
}

Camera::Camera(float posX, float posY, float posZ, float upX, float upY, float upZ, float yaw, float pitch):
    front(QVector3D(0.0f, 0.0f, -1.0f)),
    movementSpeed(SPEED),
    mouseSensitivity(SENSITIVITY),
    zoom(ZOOM){
    position = QVector3D(posX, posY, posZ);
    worldUp = QVector3D(upX, upY, upZ);
    this->yaw = yaw;
    this->pitch = pitch;
    updateCameraVectors();
}

Camera::~Camera(){

}

// Returns the view matrix calculated using Euler Angles and the LookAt Matrix
QMatrix4x4 Camera::getViewMatrix(){
    QMatrix4x4 view;
    view.lookAt(this->position, this->position + this->front, this->up);
    return view;
}

// Processes input received from any keyboard-like input system. Accepts input parameter in the form of camera defined ENUM (to abstract it from windowing systems)
void Camera::processKeyboard(Camera_Movement direction, float deltaTime){
    float velocity = this->movementSpeed * deltaTime;
    if (direction == FORWARD)
        this->position += this->front * velocity;
    if (direction == BACKWARD)
        this->position -= this->front * velocity;
    if (direction == LEFT)
        this->position -= this->right * velocity;
    if (direction == RIGHT)
        this->position += this->right * velocity;
    if (direction == UP)
        this->position += this->worldUp * velocity;
    if (direction == DOWN)
        this->position -= this->worldUp * velocity;
}

// Processes input received from a mouse input system. Expects the offset value in both the x and y direction.
void Camera::processMouseMovement(float xoffset, float yoffset, bool constraintPitch){
  xoffset *= this->mouseSensitivity;
  yoffset *= this->mouseSensitivity;

  this->yaw += xoffset;
  this->pitch += yoffset;

  if (constraintPitch) {
    if (this->pitch > 89.0f)
      this->pitch = 89.0f;
    if (this->pitch < -89.0f)
      this->pitch = -89.0f;
  }

  updateCameraVectors();
}

// Processes input received from a mouse scroll-wheel event. Only requires input on the vertical wheel-axis
void Camera::processMouseScroll(float yoffset){
    zoom -= (float)yoffset;
    if (zoom < 1.0f) zoom = 1.0f;
    if (zoom > 45.0f) zoom = 45.0f;
}

void Camera::processInput(float dt){
    if (keys[Qt::Key_W])
      processKeyboard(FORWARD, dt);
    if (keys[Qt::Key_S])
      processKeyboard(BACKWARD, dt);
    if (keys[Qt::Key_A])
      processKeyboard(LEFT, dt);
    if (keys[Qt::Key_D])
      processKeyboard(RIGHT, dt);
    if (keys[Qt::Key_E])
      processKeyboard(UP, dt);
    if (keys[Qt::Key_Q])
      processKeyboard(DOWN, dt);
}

void Camera::updateCameraVectors(){
    // Calculate the new Front vector
    QVector3D front;
    front.setX(cos(qDegreesToRadians(this->yaw)) * cos(qDegreesToRadians(this->pitch)));
    front.setY(sin(qDegreesToRadians(this->pitch)));
    front.setZ(sin(qDegreesToRadians(this->yaw)) * cos(qDegreesToRadians(this->pitch)));
    this->front = front.normalized();
    this->right = QVector3D::crossProduct(this->front, this->worldUp).normalized();
    this->up = QVector3D::crossProduct(this->right, this->front).normalized();
}
