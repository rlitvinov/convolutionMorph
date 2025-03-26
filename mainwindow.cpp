#include "mainwindow.h"
#include "./ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::resetButtonClicked()
{
    ui->morphWidget->reloadImage();
}

void MainWindow::newMatrixButtonClicked()
{
    ui->morphWidget->newMatrix();
}

void MainWindow::resetMatrixButtonClicked()
{
    ui->morphWidget->resetMatrix();
}

void MainWindow::mutateMatrixButtonClicked()
{
    ui->morphWidget->mutateMatrix();
}

void MainWindow::autoMutateCheckboxChanged(int state)
{
    ui->morphWidget->setAutoMutateMatrix(state == Qt::CheckState::Checked);
}

void MainWindow::stepButtonClicked()
{
    ui->morphWidget->applyMorph();
    ui->morphWidget->update();
}

void MainWindow::playButtonClicked()
{
    ui->morphWidget->startWorkerThread();
}

void MainWindow::stopButtonClicked()
{
    ui->morphWidget->stopWorkerThread();
}

void MainWindow::randomizeButtonClicked()
{
    ui->morphWidget->randomizeImage();
}


