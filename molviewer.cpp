﻿#include "molviewer.h"

#include <QDebug>
#include <QTimer>
#include <QKeyEvent>
#include <QDateTime>

//glm convert to QMatrix:https://stackoverflow.com/questions/36249982/opengl-and-qt-5-5-glmperspective-doesnt-work

// lighting
static QVector3D lightPos(5.0f, 5.0f, 5.0f);

QVector3D system_center(0.0f, 0.0f, -1.0f);

vector<Sphere* > balls_copy;

int total_indexcount = 0;

MolViewer::MolViewer(QWidget *parent, string molfile) :
    QOpenGLWidget(parent), MolFilePath(molfile){
    camera = make_unique<Camera>(QVector3D(camera_oginin_x, camera_oginin_y, camera_oginin_z), QVector3D(0.0f, 0.0f, -1.0f));
    m_bLeftPressed = false;

    glm::mat4 proj = glm::perspective(glm::radians(camera->zoom), WIDTH/HEIGHT, 0.1f, 100.0f);
    global_projection = glm2QMatrix(proj);

    // 必须先设置聚焦策略，否则无法响应键盘事件
    setFocusPolicy(Qt::ClickFocus);
}

void MolViewer::setMolFilePath(string mol_file_path){
    MolFilePath = mol_file_path;

}

QVector4D MolViewer::ScreenCoordinate2_WorldCoordinate(int xpos, int ypos){
    // 3d 正则化（normalised）坐标
    float x = (2.0*xpos)/this->width() - 1.0f;
    float y = 1.0f - (2.0f*ypos)/this->height();
    float z = 1.0f;

    // 4d homogeneous clip coordinate
    QVector4D ray_clip = QVector4D(x, y, -1.0, 1.0);
//    printMatrx(global_projection);
    // 4d eye coordinate
    QVector4D ray_eye = global_projection.inverted()*ray_clip;
    ray_eye = QVector4D(ray_eye.x(), ray_eye.y(), -1.0, 0.0);
//    cout<< "ray_eye:" << ray_eye.x() << "," << ray_eye.y() << endl;

    // 4d world coordinates
    QMatrix4x4 view = camera->getViewMatrix();
    QVector4D ray_eye_2 = view.inverted()*ray_eye;
    QVector4D ray_wor = QVector4D(ray_eye_2.x(), ray_eye_2.y(), ray_eye_2.z(), 1.0);
    return ray_wor;
}

void MolViewer::ray_cating(int xpos, int ypos){
    /*
     *用于3D拾取(3D-picking)的ray-cating方法
     *ray_wor为从摄像机视角出发到被点击的屏幕坐标的方向向量，通过判断该向量延长后是否与物体相交(例如通过与物体中心坐标的距离判断)，可以拾取物体
     */
    QVector4D ray_wor = ScreenCoordinate2_WorldCoordinate(xpos,ypos);
    // camera.Front = glm::vec3(ray_wor.x, ray_wor.y, ray_wor.z);

    float shortest_distance = 10000.0;
    int selected_object = -1;
    float selected_radius;
    int selected_SectorCount;
    int selected_StackCount;

    glm::vec3 selected_position;

    for(auto ball = balls_copy.begin(); ball != balls_copy.end(); ++ball){
        glm::vec3 core = (*ball)->getPosition();
        glm::vec3 pointer_vector = glm::normalize(glm::vec3(core.x-camera->position.x(), core.y-camera->position.y(), core.z-camera->position.z()));
        glm::vec3 ray_vector = glm::normalize(glm::vec3(ray_wor.x(), ray_wor.y(), ray_wor.z()));

        float distance = qSqrt(qPow((core.x-camera->position.x()), 2)+qPow(core.y-camera->position.y(), 2)+qPow((core.z-camera->position.z()), 2));
        float radius = (*ball)->getRadius();
        float angle = qTan(radius/distance);

        float angle2 = glm::angle(pointer_vector, ray_vector);  //ray_casting与物体中点的夹角
        if(angle2 <= angle || (PI-angle2) <=angle){
            int object_no = (*ball)->getNo();
            if(distance < shortest_distance){
                selected_object = object_no;
                selected_radius = (*ball)->getRadius();
                selected_SectorCount = (*ball)->getSectorCount();
                selected_StackCount = (*ball)->getStackCount();
                selected_position = (*ball)->getPosition();
                shortest_distance = distance;
            }
        }
    }

    if(selected_object != -1){
        Sphere* sphere = new Sphere(selected_object, selected_radius, selected_SectorCount, selected_StackCount, selected_position, WHITE);
        cout << "select " << selected_object << endl;
        objects.erase(objects.begin()+selected_object);
        objects.insert(objects.begin()+selected_object, sphere);
    }
    update();
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
    molShader.bind();
}

void MolViewer::resizeGL(int w, int h){
    glViewport(0, 0, w, h);
}

void MolViewer::paintGL(){
    if(!MolFilePath.empty() && MolFilePath != recentFile){
        clear_all();

        string suffix = MolFilePath.substr(MolFilePath.size()-4);

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
                // cout << (*j).x << " ," << (*j).y << " ," << (*j).z << "\n"
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
        vector<Cylinder* > cylinders;

        // 构建原子信息
        Sphere hydrogen(0, 0.1f, 8, 4, glm::vec3(0.0f, 0.0f, 0.0f), GREEN);
        Sphere carbon(0, 0.2f, 16, 8, glm::vec3(0.0f, 0.0f, 0.0f), RED);
        Sphere nitrogen(0, 0.24f, 16, 8, glm::vec3(0.0f, 0.0f, 0.0f), GOLD1);
        Sphere oxygen(0, 0.28f, 16, 8, glm::vec3(0.0f, 0.0f, 0.0f), BLUE);
        Sphere fluorine(0, 0.32f, 16, 8, glm::vec3(0.0f, 0.0f, 0.0f), CYAN);
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
        // 构建键信息
        for(auto bond = mol->beginBonds(); bond!=mol->endBonds(); ++bond){
            // cout << (*bond)->getBeginAtomIdx() << "," << (*bond)->getEndAtomIdx() << "\n";
            int start_atom_idx = (*bond)->getBeginAtomIdx();
            int end_atom_idx = (*bond)->getEndAtomIdx();
            glm::vec3 start_point = glm::vec3(positions[start_atom_idx*4],  positions[start_atom_idx*4+1],  positions[start_atom_idx*4+2]);
            glm::vec3 end_point = glm::vec3(positions[end_atom_idx*4],  positions[end_atom_idx*4+1],  positions[end_atom_idx*4+2]);
            BondType bond_type = (*bond)->getBondType();

            if(bond_type == MiniRDKit::Bond::DOUBLE){
                build_Keys(MiniRDKit::Bond::DOUBLE, end_point, start_point, cylinders);

                // }else if(bond_type == MiniRDKit::Bond::TRIPLE){

            }else if (bond_type == MiniRDKit::Bond::AROMATIC){
                auto start_atom = aromatic_map.find(start_atom_idx);
                auto end_atom = aromatic_map.find(end_atom_idx);

                if(start_atom == aromatic_map.end() && end_atom == aromatic_map.end()){
                    build_Keys(MiniRDKit::Bond::SINGLE, end_point, start_point, cylinders);
                    aromatic_map.insert({start_atom_idx, false});
                    aromatic_map.insert({end_atom_idx, false});
                }else if(start_atom == aromatic_map.end() && end_atom->second){
                    build_Keys(MiniRDKit::Bond::SINGLE, end_point, start_point, cylinders);
                    aromatic_map.insert({start_atom_idx, false});
                }else if(start_atom == aromatic_map.end() && !end_atom->second){
                    build_Keys(MiniRDKit::Bond::DOUBLE, end_point, start_point, cylinders);
                    aromatic_map.insert({start_atom_idx, true});
                    aromatic_map[end_atom_idx] = true;
                }else if(end_atom == aromatic_map.end() && start_atom->second){
                    build_Keys(MiniRDKit::Bond::SINGLE, end_point, start_point, cylinders);
                    aromatic_map.insert({end_atom_idx, false});
                }else if(end_atom == aromatic_map.end() && !start_atom->second){
                    build_Keys(MiniRDKit::Bond::DOUBLE, end_point, start_point, cylinders);
                    aromatic_map.insert({end_atom_idx, true});
                    aromatic_map[start_atom_idx] = true;
                }else if(start_atom != aromatic_map.end() && end_atom != aromatic_map.end() && (start_atom->second || end_atom->second)){
                    build_Keys(MiniRDKit::Bond::SINGLE, end_point, start_point, cylinders);
                    aromatic_map[start_atom_idx] = true;
                    aromatic_map[end_atom_idx] = true;
                }else{
                    build_Keys(MiniRDKit::Bond::DOUBLE, end_point, start_point, cylinders);
                    aromatic_map[start_atom_idx] = true;
                    aromatic_map[end_atom_idx] = true;
                }
            }else{
                build_Keys(MiniRDKit::Bond::SINGLE, end_point, start_point, cylinders);
            }
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

        // 构建键
        for(Cylinder* cylinder:cylinders){
            build_GLobject(cylinder, total_vertexcount, total_indexcount);
            objects.push_back(cylinder);
        }
        recentFile = MolFilePath;
        balls_copy = balls;
    }

    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // view/projection transformations
    projection = glm2QMatrix(glm::perspective(glm::radians(camera->zoom), WIDTH/HEIGHT, 0.1f, 100.0f));
    global_projection = projection;

    QMatrix4x4 view = camera->getViewMatrix();
    molShader.setUniformValue("projection", projection);
    molShader.setUniformValue("view", view);

    int obj_index = 0;
    for(auto vao:vaos){
        glBindVertexArray(vao);

        // world transformation
        glm::mat4 tmp_model = glm::mat4(1.0f);
        float angle = 0.0f;
        tmp_model = glm::rotate(tmp_model, glm::radians(angle), glm::vec3(1.0f, 0.0f, 0.0f));
        model = QMatrix4x4(glm::value_ptr(tmp_model)).transposed();
        molShader.setUniformValue("model", model);

        GraphicObject* obj = objects[obj_index];
        QVector3D object_color = glm2Qvector(obj->getColor());

        molShader.setUniformValue("objectColor", object_color);
        molShader.setUniformValue("lightColor", lightColor);
        // light properties
        molShader.setUniformValue("lightPos", lightPos);
        molShader.setUniformValue("viewPos", camera->position);

        glDrawElements(GL_TRIANGLES, total_indexcount*3, GL_UNSIGNED_INT, (void*)0);
        obj_index+=1;
    }
    create_CoordinateSystem();
}

void MolViewer::keyPressEvent(QKeyEvent *event){
    int key = event->key();
    if (key >= 0 && key < 1024){
        camera->keys[key] = true;
        camera->processInput(step_length);
    }
    update();
}

void MolViewer::keyReleaseEvent(QKeyEvent *event){
    int key = event->key();
    if (key >= 0 && key < 1024)
        camera->keys[key] = false;
}

void MolViewer::mousePressEvent(QMouseEvent *event){
    if(event->button() == Qt::LeftButton){
        m_bLeftPressed = true;
        m_lastPos = event->pos();                  // 2d viewport 坐标
        last_LeftButton_click_time = QDateTime::currentMSecsSinceEpoch();
        ray_cating(m_lastPos.x(), m_lastPos.y());
    }else if(event->button() == Qt::RightButton){

    }else if(event->button() == Qt::MidButton){

    }
}

void MolViewer::mouseDoubleClickEvent(QMouseEvent *event){
    cout << "Double Clicked!" << endl;
    if(all_selected){
        for(Sphere* ball: balls_copy){
            Sphere* sphere = new Sphere(*ball);
            objects.erase(objects.begin()+ball->getNo());
            objects.insert(objects.begin()+ball->getNo(), sphere);
        }
        all_selected = false;
    }else{
        all_selected = true;
        for(Sphere* ball: balls_copy){
            Sphere* sphere = new Sphere(ball->getNo(), ball->getRadius(), ball->getSectorCount(), ball->getStackCount(), ball->getPosition(), WHITE);
            objects.erase(objects.begin()+ball->getNo());
            objects.insert(objects.begin()+ball->getNo(), sphere);
        }
    }

    update();
}

void MolViewer::mouseReleaseEvent(QMouseEvent *event){
    Q_UNUSED(event);
    if(event->button() == Qt::LeftButton){
        m_bLeftPressed = false;
    }
}

void MolViewer::mouseMoveEvent(QMouseEvent *event){
    int xpos = event->pos().x();
    int ypos = event->pos().y();

    if(firstMouse){
        m_lastPos = event->pos();
        firstMouse = false;
    }

    int xoffset = xpos - m_lastPos.x();
    int yoffset = ypos - m_lastPos.y();
    m_lastPos = event->pos();

    float radius = qSqrt(qPow(camera_oginin_x-system_center.x(), 2)+pow(camera_oginin_y-system_center.y(), 2)+pow((camera_oginin_z-system_center.z()), 2));

    float xz_angle = (2*PI)*xoffset/WIDTH/2;
    float xy_angle = (2*PI)*yoffset/HEIGHT/2;

    origin_xy_angle += xy_angle;
    origin_xz_angle += xz_angle;

    camera->position.setX(radius*qCos(origin_xy_angle)*qCos(origin_xz_angle));
    camera->position.setY(radius*qSin(origin_xy_angle));
    camera->position.setZ(radius*qCos(origin_xy_angle)*qSin(origin_xz_angle));

    camera->front = QVector3D(system_center.x()-camera->position.x(), system_center.y()-camera->position.y(), system_center.z()-camera->position.z());
    update();
}

void MolViewer::wheelEvent(QWheelEvent *event){
    cout << "wheelEvent" << endl;
    QPoint offset = event->angleDelta();
    camera->processMouseScroll(offset.y()/20.0f);
    update();
}

QVector3D MolViewer::glm2Qvector(glm::vec3 vec){
    QVector3D vector(vec.x, vec.y, vec.z);
    return vector;
}

QMatrix4x4 MolViewer::glm2QMatrix(glm::mat4 matrix){
    return QMatrix4x4(glm::value_ptr(matrix)).transposed();
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

void MolViewer::clear_all(){
    for(const unsigned int vao:vaos){
        glDeleteVertexArrays(1, &vao);
    }
    vaos.clear();
    for(const unsigned int vbo:vbos){
        glDeleteBuffers(1, &vbo);
    }
    vbos.clear();
    glDeleteBuffers(1, &EBO);

    mol = nullptr;
    objects.clear();
    total_vertexcount = 0;
    total_indexcount = 0;

    aromatic_map.clear();

    firstMouse = true;
    all_selected = false;
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

void MolViewer::build_Keys(BondType bondtype, const glm::vec3 end_point, const glm::vec3 start_point, vector<Cylinder* >& cylinders){
    glm::vec3 key_vector = end_point - start_point;
    glm::vec3 up_vector = glm::vec3(0.0f, 1.0f, 0.0f);
    glm::vec3 move_vector = glm::normalize(glm::cross(key_vector, up_vector));
    move_vector *= 0.05;
    if(bondtype == MiniRDKit::Bond::DOUBLE){
        Cylinder* cylinder1 = new Cylinder(end_point+move_vector, start_point+move_vector, 0.025,0.025,16,1, RED);
        Cylinder* cylinder2 = new Cylinder(end_point-move_vector, start_point-move_vector, 0.025,0.025,16,1, BLUE);
        cylinders.push_back(cylinder1);
        cylinders.push_back(cylinder2);
    }else{
        Cylinder* cylinder = new Cylinder(end_point, start_point, 0.05,0.05,16,1, WRITE);
        cylinders.push_back(cylinder);
    }
}

void MolViewer::create_CoordinateSystem(){
    float line[(21/5+1)*(21/5+1)*2*2*6];

    int line_no = 0;
    for(int i=-10.0; i<11.0; i+=5){
        for(int j=-10.0; j<11.0; j+=5){
            line[line_no*6] = i;
            line[line_no*6+1] = j;
            line[line_no*6+2] = -10.0;
            line[line_no*6+3] = i;
            line[line_no*6+4] = j;
            line[line_no*6+5] = 10.0;
            line_no++;
        }
        for(int k=-10.0; k<11.0; k+=5){
            line[line_no*6] = i;
            line[line_no*6+1] = -10.0;
            line[line_no*6+2] = k;
            line[line_no*6+3] = i;
            line[line_no*6+4] = 10.0;
            line[line_no*6+5] = k;
            line_no++;
        }
    }

    for(int k=-10.0; k<11.0; k+=5){
        for(int i=-10.0; i<11.0; i+=5){
            line[line_no*6] = i;
            line[line_no*6+1] = -10.0;
            line[line_no*6+2] = k;
            line[line_no*6+3] = i;
            line[line_no*6+4] = 10.0;
            line[line_no*6+5] = k;
            line_no++;
        }
        for(int j=-10.0; j<11.0; j+=5){
            line[line_no*6] = -10.0;
            line[line_no*6+1] = j;
            line[line_no*6+2] = k;
            line[line_no*6+3] = 10.0;
            line[line_no*6+4] = j;
            line[line_no*6+5] = k;
            line_no++;
        }
    }

    unsigned int coordinateVAO, coordinateVBO;
    glGenVertexArrays(1, &coordinateVAO);
    glGenBuffers(1, &coordinateVBO);

    glBindBuffer(GL_ARRAY_BUFFER, coordinateVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(line), line, GL_STATIC_DRAW);
    glBindVertexArray(coordinateVAO);

    glVertexAttribPointer(0, 3, GL_FLOAT, false, 3*sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    molShader.setUniformValue("objectColor", glm2Qvector(WHITE));
    glLineWidth(1.0);
    glDrawArrays(GL_LINES, 0, sizeof(line)/sizeof(float));
}
