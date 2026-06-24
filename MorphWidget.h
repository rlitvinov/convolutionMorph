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

#include <queue>

class CMyMorphWidget;

class CWorkersManagerThread: public QThread
{
    Q_OBJECT

    typedef QThread TBase;

public:
    typedef std::function<void(size_t)> TWorkFunction;

    CWorkersManagerThread(TWorkFunction workFunction, size_t threadCount = QThread::idealThreadCount())
        : m_workFunction(workFunction)
        , m_numThreads(threadCount)
        , m_feedReady(false)
    {}

    void setThreadCount(size_t count);
    void startWorkCycle();

    void requestInterruption();

protected:
    virtual void run() override;

signals:
    void frameUpdated();

private:
    TWorkFunction m_workFunction;
    size_t m_numThreads;

    bool m_feedReady;
    std::mutex m_feedMutex;
    std::condition_variable m_feedCV;
};


class CMyMorphWidget: public QWidget
{
    Q_OBJECT

public:
    typedef QWidget TBase;

    explicit CMyMorphWidget(QWidget *parent = nullptr);
    virtual ~CMyMorphWidget();

    void singleStep(); // displays next frame only if the WorkerMaanger thread is not running
    void requestNewImage(QString filename);
    void requestImageReload();
    void requestMatrixRandomization();
    void requestMatrixReset();
    void requestMatrixMutation();
    void requestImageRandomization();

    void setAutoMutateMatrix(bool mutate);

    void randomizeImage();

    void startWorkersManagerThread();
    void stopWorkersManagerThread();

protected:
    virtual void paintEvent(QPaintEvent* event) override;

private slots:
    void frameDataReady();

private:
    //typedef CMyMorphKernel TKernel;
    //typedef CMyMorphKernelCrosscolor TKernel;
    typedef CMyMorphKernelCrosscolorSplitColor TKernel;
    typedef TKernel::SRGBColor SColor;

    struct SNextFrameComputationSlot
    {
        // input params
        QPoint m_source;
        QPoint m_destination;
        QSize m_size;

        // output values
        SColor m_minimums;
        SColor m_maximums;
    };

    constexpr static const float MutationStrength = 0.001f;
    constexpr static const bool NormalizeFrame = false; // If true then each frame will be scaled to 0-255 range, otherwise it will be clamped to this range

    constexpr static const auto FPSFontSize = 30;
    constexpr static const QPoint FPSOffset{10, 30};
    constexpr static const auto FPSTextColor = Qt::green;
    constexpr static const auto FPSOutlineColor = Qt::red;
    constexpr static const auto FPSRefreshIntervalMSecs = 1000;

    void loadImage();
    void initImageBorder();
    void refreshScanlinesForNewImage();
    void prepareComputationalSlots();
    void computeNextFrameData(size_t slot);
    void applyNextFrameData();
    void requestOperation(std::function<void(void)> operation, bool updateNeeded = false);
    void processDeferredOperations();

    QImage m_image;
    std::unique_ptr<QRgb*[]> m_spImageScanlines;

    std::unique_ptr<SColor[]> m_spNextFrameData;
    QSize m_nextFrameDataSize;
    std::vector<SNextFrameComputationSlot> m_nextFrameComputationSlots;

    TKernel m_kernel;

    QString m_filename;

    CWorkersManagerThread m_workersManagerThread;

    std::mt19937 m_randomEngine;
    std::uniform_int_distribution<unsigned short> m_randomDistrubution;

    QTime m_fpsTimeStamp;
    int m_framesSinceTimeStamp = 0;
    QString m_fps;

    std::queue<std::function<void(void)>> m_deferredOperationsQueue;
    bool m_autoMutate = false;
};

#endif // MORPHWIDGET_H
