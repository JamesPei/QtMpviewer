#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow){
    ui->setupUi(this);
}

MainWindow::~MainWindow(){
    delete ui;
}


void MainWindow::on_actionopen_triggered(){
    fileName = QFileDialog::getOpenFileName(this);
    if (!fileName.isEmpty()){
        messagebox.setText(fileName+" opened");
        messagebox.exec();
    }


}

void MainWindow::on_openGLWidget_aboutToCompose(){

}

