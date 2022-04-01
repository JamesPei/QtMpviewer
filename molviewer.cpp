#include "molviewer.h"

#include <QDebug>
#include <QTimer>
#include <QKeyEvent>
#include <QDateTime>

// lighting
static QVector3D lightPos(5.0f, 0.0f, 5.0f);

int total_indexcount = 0;

MolViewer::MolViewer(QWidget *parent) : QOpenGLWidget(parent)
{
    camera = make_unique<Camera>(QVector3D(10.0f, 0.0f, 10.0f));
    m_bLeftPressed = false;

    m_pTimer = new QTimer(this);
    connect(m_pTimer, &QTimer::timeout, this, [=]{
        m_nTimeValue += 1;
        update();
    });
    m_pTimer->start(40);
}

MolViewer::~MolViewer()
{
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
}

void MolViewer::initializeGL(){
    this->initializeOpenGLFunctions();

    createShader();

    Sphere ball;
    int vertexcount = ball.getInterleavedVertexCount();
    const float* v = ball.getInterleavedVertices();
    float vertices[vertexcount*8];
    for(int i=0; i<vertexcount; i++){
        for(int j=0; j<8; ++j){
            vertices[i*8+j] = v[i*8+j];
        }
    }

    int indexcount = ball.getTriangleCount();
    const unsigned int* index = ball.getIndices();

    unsigned int indices[indexcount*3];
    for(int j = 0; j<indexcount; j++){
        for(int k=0;k<3;k++){
            indices[j*3 + k] = index[j*3 + k];
        }
    }

    total_indexcount += indexcount;

    // first, configure the cube's VAO (and VBO)
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glBindVertexArray(VAO);

    glGenBuffers(1, &EBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    int stride = 8;
    glVertexAttribPointer(0, 3, GL_FLOAT, false, stride*sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, false, stride*sizeof(float), (void*)(sizeof(float)*3));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, false, stride*sizeof(float), (void*)(sizeof(float)*6));
    glEnableVertexAttribArray(2);

    glEnable(GL_DEPTH_TEST);
}

void MolViewer::resizeGL(int w, int h){
    glViewport(0, 0, w, h);
}

void MolViewer::paintGL(){
    camera->processInput(0.5f);//speed

    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // be sure to activate shader when setting uniforms/drawing objects
    molShader.bind();
    molShader.setUniformValue("objectColor", QVector3D(1.0f, 0.0f, 0.0f));
    molShader.setUniformValue("lightColor", QVector3D(2.0f, 2.0f, 1.0f));

    // light properties
    molShader.setUniformValue("lightPos", lightPos);
    molShader.setUniformValue("viewPos", QVector3D(20.0f, 0.5f, 0.5f));

    // view/projection transformations
    QMatrix4x4 projection;
    projection.perspective(camera->zoom, 1.0f * width() / height(), 0.1f, 100.0f);
    QMatrix4x4 view = camera->getViewMatrix();
    molShader.setUniformValue("projection", projection);
    molShader.setUniformValue("view", view);

    // world transformation
    QMatrix4x4 model;
    molShader.setUniformValue("model", model);

    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, total_indexcount*3, GL_UNSIGNED_INT, (void*)0);
}

void MolViewer::keyPressEvent(QKeyEvent *event)
{
    int key = event->key();
    if (key >= 0 && key < 1024)
        camera->keys[key] = true;
}

void MolViewer::keyReleaseEvent(QKeyEvent *event)
{
    int key = event->key();
    if (key >= 0 && key < 1024)
        camera->keys[key] = false;
}

void MolViewer::mousePressEvent(QMouseEvent *event)
{
    if(event->button() == Qt::LeftButton){
        m_bLeftPressed = true;
        m_lastPos = event->pos();
    }
}

void MolViewer::mouseReleaseEvent(QMouseEvent *event)
{
    Q_UNUSED(event);

    m_bLeftPressed = false;
}

void MolViewer::mouseMoveEvent(QMouseEvent *event)
{
    int xpos = event->pos().x();
    int ypos = event->pos().y();

    int xoffset = xpos - m_lastPos.x();
    int yoffset = m_lastPos.y() - ypos;
    m_lastPos = event->pos();

    camera->processMouseMovement(xoffset, yoffset);
}

void MolViewer::wheelEvent(QWheelEvent *event)
{
    QPoint offset = event->angleDelta();
    camera->processMouseScroll(offset.y()/20.0f);
}

bool MolViewer::createShader()
{
    bool success = molShader.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/shaders/lightedsphere.vs");
    if (!success) {
        qDebug() << "shaderProgram addShaderFromSourceFile failed!" << molShader.log();
        return success;
    }

    success = molShader.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/shaders/lightedsphere.fs");
    if (!success) {
        qDebug() << "shaderProgram addShaderFromSourceFile failed!" << molShader.log();
        return success;
    }

    success = molShader.link();
    if(!success) {
        qDebug() << "shaderProgram link failed!" << molShader.log();
    }

    return success;
}

uint MolViewer::loadTexture(const QString& path)
{
    uint textureID;
    glGenTextures(1, &textureID);

    QImage image = QImage(path).convertToFormat(QImage::Format_RGBA8888).mirrored(true, true);
    if (!image.isNull()) {
        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image.width(), image.height(), 0, GL_RGBA, GL_UNSIGNED_BYTE, image.bits());
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }

    return textureID;
}
