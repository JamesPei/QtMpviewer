#ifndef MOLVIEWER_H
#define MOLVIEWER_H

#include <QOpenGLWidget>
#include <QOpenGLExtraFunctions>
#include <QOpenGLFunctions_4_2_Core>
#include <QOpenGLShaderProgram>
#include <memory>

#include "camera.h"
#include "sphere.h"

class MolViewer: public QOpenGLWidget, protected QOpenGLFunctions_4_2_Core{
    Q_OBJECT
    public:
        explicit MolViewer(QWidget *parent = nullptr);
        ~MolViewer() Q_DECL_OVERRIDE;

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

    private:
        QOpenGLShaderProgram molShader;

        QTimer* m_pTimer = nullptr;
        int     m_nTimeValue = 0;

        uint VAO, VBO, EBO;
        uint diffuseMap, specularMap;

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
