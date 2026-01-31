#include "MorphWidget.h"

#include <QPainter>

CMyMorphWidget::CMyMorphWidget(QWidget *parent)
    : QWidget{parent}
    , m_filename("/home/lrni/testFrame.jpeg")
    , m_workerThread(this)
    , m_randomDistrubution(std::numeric_limits<unsigned char>::min(), std::numeric_limits<unsigned char>::max())
    , m_randomEngine(std::random_device()())
{
    connect(&m_workerThread, SIGNAL(frameUpdated()), this, SLOT(update()));

    m_image.load(m_filename.c_str());
    m_currentPixmap = QPixmap::fromImage(m_image);
}

CMyMorphWidget::~CMyMorphWidget()
{
    stopWorkerThread();
    disconnect(&m_workerThread, SIGNAL(frameUpdated()), this, SLOT(update()));
}

void CMyMorphWidget::reloadImage()
{
    auto wasRunning = m_workerThread.isRunning();

    stopWorkerThread();
    {
        QMutexLocker imageLocker(&m_imageMutex);
        m_image.load(m_filename.c_str());
        QMutexLocker pixmapLocker(&m_currentPixmapMutex);
        m_currentPixmap = QPixmap::fromImage(m_image);
    }
    update();

    if (wasRunning)
        startWorkerThread();
}

void CMyMorphWidget::applyMorph()
{
    QMutexLocker imageLocker(&m_imageMutex);

    if (m_image.isNull())
        return;

    if (m_image.format() != QImage::Format_RGB32)
        m_image.convertTo(QImage::Format_RGB32);

    Q_ASSERT(m_image.format() == QImage::Format_RGB32);

    const auto imgSize = m_image.size();

    QRgb* scanlines[imgSize.height()];
    {
        auto pScanline = reinterpret_cast<QRgb*>(m_image.scanLine(0));
        auto increment = m_image.bytesPerLine() / sizeof(*pScanline);
        scanlines[0] = pScanline;

        std::generate(&scanlines[1], &scanlines[imgSize.height()], [&pScanline, &increment]{ return pScanline += increment; });
    }

    QImage newImage(imgSize, QImage::Format_RGB32);

    typedef decltype(m_kernel)::SRGBColor SColor;
    const auto MinColor = std::numeric_limits<decltype(SColor::R)>::min();
    const auto MaxColor = std::numeric_limits<decltype(SColor::R)>::max();

    SColor buf[imgSize.height()][imgSize.width()];
    const auto bufWidth = imgSize.width();

    SColor minColors{MaxColor, MaxColor, MaxColor};
    SColor maxColors{MinColor, MinColor, MinColor};

    SColor* pBufScanline = buf[0];
    for (int y = 0;
            y < imgSize.height();
            ++y, pBufScanline += bufWidth)
    {
        for (int x = 0; x < imgSize.width(); ++x)
        {
            const auto color = m_kernel.apply(x, y, scanlines, imgSize);
            pBufScanline[x] = color;

            minColors.R = std::min(minColors.R, color.R);
            maxColors.R = std::max(maxColors.R, color.R);

            minColors.G = std::min(minColors.G, color.G);
            maxColors.G = std::max(maxColors.G, color.G);

            minColors.B = std::min(minColors.B, color.B);
            maxColors.B = std::max(maxColors.B, color.B);
        }
    }

    const auto rSpan = maxColors.R - minColors.R;
    const auto gSpan = maxColors.G - minColors.G;
    const auto bSpan = maxColors.B - minColors.B;

    const auto rScale = 255.f / rSpan;
    const auto gScale = 255.f / gSpan;
    const auto bScale = 255.f / bSpan;

    pBufScanline = buf[0];
    auto pNewScanline = reinterpret_cast<QRgb*>(newImage.scanLine(0));
    for (int y = 0, increment = newImage.bytesPerLine() / sizeof(*pNewScanline);
         y < imgSize.height();
         ++y, pNewScanline += increment, pBufScanline += bufWidth)
    {
        for (int x = 0; x < imgSize.width(); ++x)
        {
            auto [r, g, b] = pBufScanline[x];

            if constexpr (NormalizeFrame)
            {
                r = (r - minColors.R) * rScale;
                g = (g - minColors.G) * gScale;
                b = (b - minColors.B) * bScale;
            }
            else
            {
                r = std::clamp(r, 0.f, 255.f);
                g = std::clamp(g, 0.f, 255.f);
                b = std::clamp(b, 0.f, 255.f);
            }

            pNewScanline[x] = qRgb(static_cast<int>(r),
                                static_cast<int>(g),
                                static_cast<int>(b));
        }
    }

    m_image = std::move(newImage);
    QMutexLocker pixmapLocker(&m_currentPixmapMutex);
    m_currentPixmap = QPixmap::fromImage(m_image);
}

void CMyMorphWidget::newMatrix()
{
    auto wasRunning = m_workerThread.isRunning();
    stopWorkerThread();

    m_kernel.randomizeMatrix();

    if (wasRunning)
        startWorkerThread();
}

void CMyMorphWidget::resetMatrix()
{
    auto wasRunning = m_workerThread.isRunning();
    stopWorkerThread();

    m_kernel.setTrivialMatrix();

    if (wasRunning)
        startWorkerThread();
}

void CMyMorphWidget::mutateMatrixInternal()
{
    m_kernel.mutateMatrix(MutationStrength);
}

void CMyMorphWidget::mutateMatrix()
{
    auto wasRunning = m_workerThread.isRunning();
    stopWorkerThread();

    mutateMatrixInternal();

    if (wasRunning)
        startWorkerThread();
}

void CMyMorphWidget::setAutoMutateMatrix(bool mutate)
{
    m_workerThread.setAutoMutate(mutate);
}

void CMyMorphWidget::randomizeImage()
{
    auto wasRunning = m_workerThread.isRunning();
    stopWorkerThread();

    {
        QMutexLocker imageLocker(&m_imageMutex);

        const auto imgSize = m_image.size();

        for (int y = 0; y < imgSize.height(); ++y)
        {
            QRgb* line = reinterpret_cast<QRgb*>(m_image.scanLine(y));
            for (int x = 0; x < imgSize.width(); ++x)
            {
                const auto randomR = m_randomDistrubution(m_randomEngine);
                const auto randomG = m_randomDistrubution(m_randomEngine);
                const auto randomB = m_randomDistrubution(m_randomEngine);
                line[x] = qRgb(randomR, randomG, randomB);
            }
        }

        QMutexLocker pixmapLocker(&m_currentPixmapMutex);
        m_currentPixmap = QPixmap::fromImage(m_image);
    }
    update();

    if (wasRunning)
        startWorkerThread();
}

void CMyMorphWidget::startWorkerThread()
{
    if (!m_workerThread.isRunning())
        m_workerThread.start();
}

void CMyMorphWidget::stopWorkerThread()
{
    m_workerThread.requestInterruption();
    m_workerThread.wait();
    Q_ASSERT(!m_workerThread.isRunning());
}

void CMyMorphWidget::paintEvent(QPaintEvent* e)
{
    QPainter painter(this);

    QMutexLocker pixmapLocker(&m_currentPixmapMutex);
    const auto imgSize = m_currentPixmap.size();
    painter.drawPixmap(0, 0, imgSize.width() * 2, imgSize.height() * 2, m_currentPixmap);
}



void CWorkerThread::run()
{
    Q_ASSERT(m_pParentWidget != nullptr);
    do
    {
        m_pParentWidget->applyMorph();
        if (m_autoMutate)
            m_pParentWidget->mutateMatrixInternal();
        emit frameUpdated();
    } while (!isInterruptionRequested());
}
