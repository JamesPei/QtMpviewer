#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMessageBox>
#include <QFileDialog>
#include <FileParsers/FileParsers.h>
#include <GraphMol/ROMol.h>
#include "molviewer.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_actionopen_triggered();

    void on_openGLWidget_aboutToCompose();

private:
    Ui::MainWindow *ui;
    QMessageBox messagebox;
    QString fileName;
};
#endif // MAINWINDOW_H
