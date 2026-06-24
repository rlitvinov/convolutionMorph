#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    virtual ~MainWindow();

private slots:
    void openImageButtonClicked();
    void resetImageButtonClicked();
    void newMatrixButtonClicked();
    void resetMatrixButtonClicked();
    void mutateMatrixButtonClicked();
    void autoMutateCheckboxChanged(int state);
    void stepButtonClicked();
    void playButtonClicked();
    void stopButtonClicked();
    void randomizeButtonClicked();

private:
    Ui::MainWindow *ui;
};
#endif // MAINWINDOW_H
