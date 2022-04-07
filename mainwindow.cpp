#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow){
    ui->setupUi(this);
    mainLayout = new QGridLayout;
    ui->centralwidget->setLayout(mainLayout);
    viewer = new MolViewer();
    viewer->resize(WIDTH, HEIGHT);
    mainLayout->addWidget(viewer);
}

MainWindow::~MainWindow(){
    delete ui;
}

void MainWindow::on_actionopen_triggered(){
    QFileDialog fileOperator;
    fileName = fileOperator.getOpenFileName(this);
    if (!fileName.isEmpty()){
        messagebox.setText(fileName+" opened");
        messagebox.exec();
    }
    viewer->setMolFilePath(fileName.toStdString());
    viewer->update();
}
