#include "molviewer.h"

#include <QDebug>
#include <QTimer>
#include <QKeyEvent>
#include <QDateTime>

// lighting
static QVector3D lightPos(5.0f, 0.0f, 5.0f);

QVector3D system_center(0.0f, 0.0f, -1.0f);

int total_indexcount = 0;

MolViewer::MolViewer(QWidget *parent, string molfile) : QOpenGLWidget(parent), MolFilePath(molfile){
    camera = make_unique<Camera>(QVector3D(10.0f, 0.0f, 10.0f));
    m_bLeftPressed = false;

    m_pTimer = new QTimer(this);
    connect(m_pTimer, &QTimer::timeout, this, [=]{
        m_nTimeValue += 1;
        update();
    });
    m_pTimer->start(40);
}

MolViewer::~MolViewer(){
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
}

void MolViewer::initializeGL(){
    this->initializeOpenGLFunctions();

    createShader();
    glEnable(GL_DEPTH_TEST);
}

void MolViewer::resizeGL(int w, int h){
    glViewport(0, 0, w, h);
}

void MolViewer::paintGL(){
    if(!MolFilePath.empty()){
        string suffix = MolFilePath.substr(MolFilePath.size()-4);
        MiniRDKit::RWMol* mol(nullptr);
        if(suffix == ".mol"){
            mol = MiniRDKit::MolFileToMol(MolFilePath);
        }else if(suffix == "mol2"){
            mol = MiniRDKit::Mol2FileToMol(MolFilePath);
        }else if(suffix == ".pdb"){
            mol = MiniRDKit::PDBFileToMol(MolFilePath);
        }

        vector<float> position_radius;

        for(auto i = mol->beginConformers(); i != mol->endConformers(); ++i){   // 汇总原子位置
            MiniRDKit::POINT3D_VECT points = ((*i).get())->getPositions();
            for(auto j = points.begin(); j!=points.end(); ++j){
                // cout << (*j).x << " ," << (*j).y << " ," << (*j).z << "\n";
                position_radius.push_back((*j).x);
                position_radius.push_back((*j).y);
                position_radius.push_back((*j).z);
            }
        }

        int counter = 1;
        vector<unsigned int> all_atom_id;
        for(auto i = mol->beginAtoms(); i!=mol->endAtoms(); ++i){      // 汇总原子质量
            // cout << (*i)->getAtomicNum() <<" "<<  (*i)->getIdx() << endl;
            unsigned int atom_id = (*i)->getIdx();
            all_atom_id.push_back(atom_id);
            auto pos = position_radius.begin();
            position_radius.insert(pos+(4*counter-1), (*i)->getAtomicNum());
            ++counter;
        }

        int atom_num = position_radius.size();
        float positions[atom_num];
        copy(position_radius.begin(), position_radius.end(), positions);


        vector<Sphere* > balls;

        // 构建原子信息
        Sphere hydrogen(0, 0.1f, 4, 2, glm::vec3(0.0f, 0.0f, 0.0f), GREEN);
        Sphere carbon(0, 0.2f, 8, 4, glm::vec3(0.0f, 0.0f, 0.0f), RED);
        Sphere nitrogen(0, 0.24f, 8, 4, glm::vec3(0.0f, 0.0f, 0.0f), GOLD1);
        Sphere oxygen(0, 0.28f, 8, 4, glm::vec3(0.0f, 0.0f, 0.0f), BLUE);
        Sphere fluorine(0, 0.32f, 8, 4, glm::vec3(0.0f, 0.0f, 0.0f), CYAN);
        Sphere big_atom(0, 0.36f, 16, 8, glm::vec3(0.0f, 0.0f, 0.0f), GREY31);

        for(int i=0; i<sizeof(positions)/sizeof(float); i+=4){
            glm::vec3 pos = glm::vec3(positions[i],  positions[i+1],  positions[i+2]);
            int no = i/4;
            Sphere* ball;

            if(positions[i+3]>9){
                ball = new Sphere(big_atom);
                ball->setNo(no);
                ball->setPosition(pos);
            }else if(positions[i+3]==9){
                ball = new Sphere(fluorine);
                ball->setNo(no);
                ball->setPosition(pos);
            }else if(positions[i+3]==8){
                ball = new Sphere(oxygen);
                ball->setNo(no);
                ball->setPosition(pos);
            }else if(positions[i+3]==7){
                ball = new Sphere(nitrogen);
                ball->setNo(no);
                ball->setPosition(pos);
            }else if(positions[i+3]==6){
                ball = new Sphere(carbon);
                ball->setNo(no);
                ball->setPosition(pos);
            }else{
                ball = new Sphere(hydrogen);
                ball->setNo(no);
                ball->setPosition(pos);
            }
            balls.push_back(ball);
        }

        float system_center_x = 0.0f;
        float system_center_y = 0.0f;
        float system_center_z = 0.0f;
        // 构建原子
        for(Sphere* ball:balls){
            build_GLobject(ball, total_vertexcount, total_indexcount);
            objects.push_back(ball);
            glm::vec3 ball_position = ball->getPosition();
            system_center_x += ball_position.x;
            system_center_y += ball_position.y;
            system_center_z += ball_position.z;
        }

        system_center = QVector3D(system_center_x/balls.size(), system_center_y/balls.size(), system_center_z/balls.size());
        camera->front = QVector3D(system_center.x()-camera->position.x(), system_center.y()-camera->position.y(), system_center.z()-camera->position.z());
    }

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

void MolViewer::paintEvent(QPaintEvent *event){

}

void MolViewer::keyPressEvent(QKeyEvent *event){
    int key = event->key();
    if (key >= 0 && key < 1024)
        camera->keys[key] = true;
}

void MolViewer::keyReleaseEvent(QKeyEvent *event){
    int key = event->key();
    if (key >= 0 && key < 1024)
        camera->keys[key] = false;
}

void MolViewer::mousePressEvent(QMouseEvent *event){
    if(event->button() == Qt::LeftButton){
        m_bLeftPressed = true;
        m_lastPos = event->pos();
    }
}

void MolViewer::mouseReleaseEvent(QMouseEvent *event){
    Q_UNUSED(event);

    m_bLeftPressed = false;
}

void MolViewer::mouseMoveEvent(QMouseEvent *event){
    int xpos = event->pos().x();
    int ypos = event->pos().y();

    int xoffset = xpos - m_lastPos.x();
    int yoffset = m_lastPos.y() - ypos;
    m_lastPos = event->pos();

    camera->processMouseMovement(xoffset, yoffset);
}

void MolViewer::wheelEvent(QWheelEvent *event){
    QPoint offset = event->angleDelta();
    camera->processMouseScroll(offset.y()/20.0f);
}

bool MolViewer::createShader(){
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

uint MolViewer::loadTexture(const QString& path){
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

void MolViewer::build_GLobject(GraphicObject *object, int &total_vertexcount, int &total_indexcount){
    int vertexcount = object->getInterleavedVertexCount();
    const float* v = object->getInterleavedVertices();

    float vertices[vertexcount*8];
    for(int i=0; i<vertexcount; i++){
        for(int j=0; j<8; ++j){
            vertices[i*8+j] = v[i*8+j];
        }
    }

    int indexcount = object->getTriangleCount();
    const unsigned int* index = object->getIndices();

    unsigned int indices[indexcount*3];
    for(int j = 0; j<indexcount; j++){
        for(int k=0;k<3;k++){
            indices[j*3 + k] = index[j*3 + k];
        }
    }

    total_vertexcount += vertexcount;
    total_indexcount += indexcount;

    unsigned int VBO, EBO, VAO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    vaos.push_back(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glBindVertexArray(VAO);

    vbos.push_back(VBO);

    glGenBuffers(1, &EBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    int stride = object->getInterleavedStride();        // object的stride必须一致
    glVertexAttribPointer(0, 3, GL_FLOAT, false, stride*sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, false, stride*sizeof(float), (void*)(sizeof(float)*3));
    glEnableVertexAttribArray(1);
}
