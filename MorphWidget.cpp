#include "MorphWidget.h"

#include <QPainter>
#include <QPainterPath>

using namespace Qt::StringLiterals;

CMyMorphWidget::CMyMorphWidget(QWidget *parent)
    : QWidget{parent}
    , m_filename(u":/testFrame.jpeg"_s)
    , m_workerThread(this)
    , m_randomDistrubution(std::numeric_limits<unsigned char>::min(), std::numeric_limits<unsigned char>::max())
    , m_randomEngine(std::random_device()())
{
    connect(&m_workerThread, SIGNAL(frameUpdated()), this, SLOT(update()));

    loadImage();
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

    loadImage();
    update();

    if (wasRunning)
        startWorkerThread();
}

void CMyMorphWidget::applyMorph()
{
    if (m_image.isNull())
        return;

    if (m_image.format() != QImage::Format_RGB32)
    {
        QMutexLocker imageLocker(&m_imageMutex);
        m_image.convertTo(QImage::Format_RGB32);
    }

    Q_ASSERT(m_image.format() == QImage::Format_RGB32);

    constexpr const auto borderWidth = TKernel::getRequiredBorder();
    const auto imgSize = m_image.size();
    const auto processedSize = imgSize - QSize(2 * borderWidth, 2 * borderWidth);

    QRgb* scanlines[imgSize.height()];
    {
        auto pScanline = reinterpret_cast<QRgb*>(m_image.scanLine(0));
        auto increment = m_image.bytesPerLine() / sizeof(*pScanline);
        scanlines[0] = pScanline;

        std::generate(&scanlines[1], &scanlines[imgSize.height()], [&pScanline, &increment]{ return pScanline += increment; });
    }

    typedef TKernel::SRGBColor SColor;
    const auto MinColor = std::numeric_limits<decltype(SColor::R)>::min();
    const auto MaxColor = std::numeric_limits<decltype(SColor::R)>::max();

    std::unique_ptr<SColor[]> buf(new SColor[processedSize.height()*processedSize.width()]);

    const auto bufWidth = processedSize.width();

    SColor minColors{MaxColor, MaxColor, MaxColor};
    SColor maxColors{MinColor, MinColor, MinColor};

    SColor* pBufScanline = buf.get();
    for (int y = borderWidth;
            y < imgSize.height() - borderWidth;
            ++y, pBufScanline += bufWidth)
    {
        for (int x = borderWidth; x < imgSize.width() - borderWidth; ++x)
        {
            const auto color = m_kernel.apply(x, y, scanlines, imgSize);
            pBufScanline[x - borderWidth] = color;

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

    QImage newImage(imgSize, QImage::Format_RGB32);
    pBufScanline = buf.get();
    auto pNewScanline = reinterpret_cast<QRgb*>(newImage.scanLine(borderWidth));
    for (int y = borderWidth, increment = newImage.bytesPerLine() / sizeof(*pNewScanline);
         y < imgSize.height() - borderWidth;
         ++y, pNewScanline += increment, pBufScanline += bufWidth)
    {
        for (int x = borderWidth; x < imgSize.width() - borderWidth; ++x)
        {
            auto [r, g, b] = pBufScanline[x - borderWidth];

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

    QMutexLocker imageLocker(&m_imageMutex);
    m_image = std::move(newImage);
    initImageBorder();
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

        constexpr const auto borderWidth = TKernel::getRequiredBorder();
        const auto imgSize = m_image.size();

        for (int y = borderWidth; y < imgSize.height() - borderWidth; ++y)
        {
            QRgb* line = reinterpret_cast<QRgb*>(m_image.scanLine(y));
            for (int x = borderWidth; x < imgSize.width() - borderWidth; ++x)
            {
                const auto randomR = m_randomDistrubution(m_randomEngine);
                const auto randomG = m_randomDistrubution(m_randomEngine);
                const auto randomB = m_randomDistrubution(m_randomEngine);
                line[x] = qRgb(randomR, randomG, randomB);
            }
        }

        initImageBorder();

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

    {
        QMutexLocker imageLocker(&m_imageMutex);
        const auto imgSize = m_image.size();
        constexpr const auto borderWidth = TKernel::getRequiredBorder();
        const auto processedSize = imgSize - QSize(2 * borderWidth, 2 * borderWidth);
        painter.drawImage(QRect(0, 0, processedSize.width() * 2, processedSize.height() * 2),
                          m_image,
                          QRect(borderWidth, borderWidth, processedSize.width(), processedSize.height()));
    }

    {
        auto font = painter.font();
        font.setPixelSize(FPSFontSize);
        QPainterPath path;
        font.setBold(true);
        path.addText(FPSOffset, font, m_fps);
        painter.setPen(FPSOutlineColor);
        painter.setBrush(FPSTextColor);
        painter.drawPath(path);
    }

    if (const auto curTime = QTime::currentTime(); m_fpsTimeStamp.isNull())
    {
        m_fpsTimeStamp = curTime;
    }
    else
    {
        if (const auto msecsFromTimeStamp = m_fpsTimeStamp.msecsTo(curTime); msecsFromTimeStamp >= FPSRefreshIntervalMSecs)
        {
            auto fps = m_framesSinceTimeStamp * static_cast<double>(FPSRefreshIntervalMSecs) / msecsFromTimeStamp;
            m_fps = u"FPS: "_s + QString::number(fps, 'g', 3);

            m_fpsTimeStamp = curTime;
            m_framesSinceTimeStamp = 0;
        }
    }

    ++m_framesSinceTimeStamp;
}

void CMyMorphWidget::loadImage()
{
    QImage newImage;
    newImage.load(m_filename);

    constexpr auto borderWidth = TKernel::getRequiredBorder();
    const auto origSize = newImage.size();
    const auto sizeWithBorder = origSize + QSize(2 * borderWidth, 2 * borderWidth);

    // for some reason this caused around three-fold slowdown until worker thread restarted
    //auto tmpImage = m_image.copy(-borderWidth, -borderWidth, origSize.width() + 2 * borderWidth, origSize.height() + 2 * borderWidth);

    m_image = QImage(sizeWithBorder, QImage::Format_RGB32);

    {
        QPainter painter(&m_image);
        painter.drawImage(borderWidth, borderWidth, newImage);
    }

    initImageBorder();
    m_currentPixmap = QPixmap::fromImage(m_image);
}

void CMyMorphWidget::initImageBorder()
{
    constexpr const auto borderWidth = TKernel::getRequiredBorder();
    const auto imgSize = m_image.size();

    for (auto scanLine = borderWidth; scanLine < imgSize.height() - borderWidth; ++scanLine)
        for (size_t leftBorder = 0,
                    leftBorderSrc = imgSize.width() - 2 * borderWidth,
                    rightBorder = imgSize.width() - borderWidth,
                    rightBorderSrc = borderWidth;
             leftBorder < borderWidth;
             ++leftBorder, ++leftBorderSrc, ++rightBorder, ++rightBorderSrc)
        {
            const auto pScanline = reinterpret_cast<QRgb*>(m_image.scanLine(scanLine));
            pScanline[leftBorder] = pScanline[leftBorderSrc];
            pScanline[rightBorder] = pScanline[rightBorderSrc];
        }

    for (size_t topBorder = 0,
                topBorderSrc = imgSize.height() - 2 * borderWidth,
                bottomBorder = imgSize.height() - borderWidth,
                bottomBorderSrc = borderWidth;
         topBorder < borderWidth;
         ++topBorder, ++topBorderSrc, ++bottomBorder, ++bottomBorderSrc)
    {
        memcpy(m_image.scanLine(topBorder), m_image.scanLine(topBorderSrc), imgSize.width() * sizeof(QRgb));
        memcpy(m_image.scanLine(bottomBorder), m_image.scanLine(bottomBorderSrc), imgSize.width() * sizeof(QRgb));
    }
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
