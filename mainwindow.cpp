#include "mainwindow.h"
#include "./ui_mainwindow.h"

#include <QFileDialog>
#include <QImageReader>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->morphWidget->setAutoMutateMatrix(ui->autoMutateCheckBox->checkState() == Qt::CheckState::Checked);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::openImageButtonClicked()
{
    using namespace Qt::StringLiterals;

    const auto supportedImageFormats = QImageReader::supportedImageFormats();

    QString filter(u"Image Files("_s);
    for (auto &format: supportedImageFormats)
    {
        filter += u"*."_s + format + u" "_s;
    }
    filter += u")"_s;

    auto filename = QFileDialog::getOpenFileName(this, tr("Open Image"), QString(), filter);
    if (!filename.isNull())
        ui->morphWidget->requestNewImage(filename);
}

void MainWindow::resetImageButtonClicked()
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


