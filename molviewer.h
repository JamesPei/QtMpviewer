#ifndef MOLVIEWER_H
#define MOLVIEWER_H

#include <QOpenGLWidget>
#include <QOpenGLExtraFunctions>
#include <QOpenGLFunctions_4_2_Core>
#include <QOpenGLShaderProgram>
#include <QFileDialog>
#include <QString>

#include <memory>
#include <string>
#include <FileParsers/FileParsers.h>
#include <GraphMol/ROMol.h>

#include "camera.h"
#include "sphere.h"
#include "cylinder.h"
#include "color_table.h"

using namespace std;

class MolViewer: public QOpenGLWidget, protected QOpenGLFunctions_4_2_Core{
    Q_OBJECT
    public:
        explicit MolViewer(QWidget *parent = nullptr, string molfile = "");
        ~MolViewer() Q_DECL_OVERRIDE;

        void setMolFilePath(string mol_file_path);

        QVector3D glm2Qvector(glm::vec3 vec);

    protected:
        void initializeGL()  Q_DECL_OVERRIDE;
        void resizeGL(int w, int h) Q_DECL_OVERRIDE;
        void paintGL() Q_DECL_OVERRIDE;

        void keyPressEvent(QKeyEvent *event) Q_DECL_OVERRIDE;
        void keyReleaseEvent(QKeyEvent *event) Q_DECL_OVERRIDE;
        void mousePressEvent(QMouseEvent *event) Q_DECL_OVERRIDE;
        void mouseReleaseEvent(QMouseEvent *event) Q_DECL_OVERRIDE;
        void mouseMoveEvent(QMouseEvent *event) Q_DECL_OVERRIDE;
        void wheelEvent(QWheelEvent *event) Q_DECL_OVERRIDE;

    private:
        bool createShader();
        uint loadTexture(const QString& path);
        void build_GLobject(GraphicObject* object, int& total_vertexcount, int& total_indexcount);

    private:
        QOpenGLShaderProgram molShader;
        string MolFilePath;
        string recentFile = "";
        QFileDialog* fileOperator;
        MiniRDKit::RWMol* mol;

        QTimer* m_pTimer = nullptr;
        int     m_nTimeValue = 0;

        uint VAO, VBO, EBO;
        uint diffuseMap, specularMap;

        vector<unsigned int> vaos;
        vector<unsigned int> vbos;

        int total_vertexcount = 0;
        int total_indexcount = 0;

        vector<GraphicObject* > objects;

        // camera
        std::unique_ptr<Camera> camera;
        bool m_bLeftPressed;
        QPoint m_lastPos;

        template<class T, class... Args>
        std::unique_ptr<T> make_unique(Args&&... args){
            return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
        }
};

#endif // MOLVIEWER_H
