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
    ui->morphWidget->requestImageReload();
}

void MainWindow::newMatrixButtonClicked()
{
    ui->morphWidget->requestMatrixRandomization();
}

void MainWindow::resetMatrixButtonClicked()
{
    ui->morphWidget->requestMatrixReset();
}

void MainWindow::mutateMatrixButtonClicked()
{
    ui->morphWidget->requestMatrixMutation();
}

void MainWindow::autoMutateCheckboxChanged(int state)
{
    ui->morphWidget->setAutoMutateMatrix(state == Qt::CheckState::Checked);
}

void MainWindow::stepButtonClicked()
{
    ui->morphWidget->singleStep();
    ui->morphWidget->update();
}

void MainWindow::playButtonClicked()
{
    ui->morphWidget->startWorkersManagerThread();
}

void MainWindow::stopButtonClicked()
{
    ui->morphWidget->stopWorkersManagerThread();
}

void MainWindow::randomizeButtonClicked()
{
    ui->morphWidget->requestImageRandomization();
}


