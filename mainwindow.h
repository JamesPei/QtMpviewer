#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMessageBox>
#include <QFileDialog>
#include <QGridLayout>
#include <FileParsers/FileParsers.h>
#include <GraphMol/ROMol.h>

#include "config.h"
#include "molviewer.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

signals:
    void file_selected(QString);

private slots:
    void on_actionopen_triggered();

private:
    Ui::MainWindow *ui;
    QGridLayout* mainLayout;
    QMessageBox messagebox;
    QString fileName;
    MolViewer* viewer;
};
#endif // MAINWINDOW_H
