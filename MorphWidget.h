#ifndef MORPHWIDGET_H
#define MORPHWIDGET_H

#include "MorphKernel.h"
#include "MorphKernelCrosscolor.h"
#include "MorphKernelCrosscolorSplitColor.h"

#include <QWidget>
#include <QImage>
#include <QPixmap>
#include <QMutex>
#include <QThread>
#include <QTime>

class CMyMorphWidget;

class CWorkerThread: public QThread
{
    Q_OBJECT
public:
    CWorkerThread(CMyMorphWidget* pParentWidget)
        : m_pParentWidget(pParentWidget)
        , m_autoMutate(false)
    {}

    void setAutoMutate(bool mutate) { m_autoMutate = mutate; }

protected:
    virtual void run() override;

signals:
    void frameUpdated();

private:
    CMyMorphWidget* const m_pParentWidget;
    std::atomic_bool m_autoMutate;
};

class CMyMorphWidget: public QWidget
{
    Q_OBJECT

public:
    typedef QWidget TBase;

    explicit CMyMorphWidget(QWidget *parent = nullptr);
    virtual ~CMyMorphWidget();

    void reloadImage();
    void applyMorph();
    void newMatrix();
    void resetMatrix();
    void mutateMatrixInternal();
    void mutateMatrix();
    void setAutoMutateMatrix(bool mutate);
    void randomizeImage();
    void startWorkerThread();
    void stopWorkerThread();

protected:
    virtual void paintEvent(QPaintEvent* event) override;

private:
    //typedef CMyMorphKernel TKernel;
    //typedef CMyMorphKernelCrosscolor TKernel;
    typedef CMyMorphKernelCrosscolorSplitColor TKernel;

    constexpr static const float MutationStrength = 0.001f;
    constexpr static const bool NormalizeFrame = false; // If true then each frame will be scaled to 0-255 range, otherwise it will be clamped to this range

    constexpr static const auto FPSFontSize = 30;
    constexpr static const QPoint FPSOffset{10, 30};
    constexpr static const auto FPSTextColor = Qt::green;
    constexpr static const auto FPSOutlineColor = Qt::red;
    constexpr static const auto FPSRefreshIntervalMSecs = 1000;

    void loadImage();
    void initImageBorder();

    QImage m_image;
    QMutex m_imageMutex;

    TKernel m_kernel;

    QPixmap m_currentPixmap;
    QMutex m_currentPixmapMutex;

    QString m_filename;

    CWorkerThread m_workerThread;

    std::mt19937 m_randomEngine;
    std::uniform_int_distribution<unsigned short> m_randomDistrubution;

    QTime m_fpsTimeStamp;
    int m_framesSinceTimeStamp = 0;
    QString m_fps;
};

#endif // MORPHWIDGET_H
