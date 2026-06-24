#include "MorphWidget.h"

#include <QPainter>
#include <QPainterPath>

#include <barrier>
#include <thread>

using namespace Qt::StringLiterals;

CMyMorphWidget::CMyMorphWidget(QWidget *parent)
    : QWidget{parent}
    , m_filename(u":/testFrame.jpeg"_s)
    , m_workersManagerThread(std::bind(&CMyMorphWidget::computeNextFrameData, this, std::placeholders::_1))
    , m_randomDistrubution(std::numeric_limits<unsigned char>::min(), std::numeric_limits<unsigned char>::max())
    , m_randomEngine(std::random_device()())
{
    connect(&m_workersManagerThread, SIGNAL(frameUpdated()), this, SLOT(frameDataReady()));

    loadImage();
}

CMyMorphWidget::~CMyMorphWidget()
{
    stopWorkersManagerThread();
    disconnect(&m_workersManagerThread, SIGNAL(frameUpdated()), this, SLOT(frameDataReady()));
}

void CMyMorphWidget::singleStep()
{
    if (m_workersManagerThread.isRunning())
        return;

    Q_ASSERT(!m_image.isNull());
    Q_ASSERT(m_image.format() == QImage::Format_RGB32);

    for (size_t i = 0; i < m_nextFrameComputationSlots.size(); ++i)
        computeNextFrameData(i);

    applyNextFrameData();
}

void CMyMorphWidget::requestNewImage(QString filename)
{
    m_filename = filename;
    requestImageReload();
}

void CMyMorphWidget::requestImageReload()
{
    requestOperation(std::bind(&CMyMorphWidget::loadImage, this), true);
}

void CMyMorphWidget::requestMatrixRandomization()
{
    requestOperation(std::bind(&TKernel::randomizeMatrix, &m_kernel));
}

void CMyMorphWidget::requestMatrixReset()
{
    requestOperation(std::bind(&TKernel::setTrivialMatrix, &m_kernel));
}

void CMyMorphWidget::requestMatrixMutation()
{
    requestOperation(std::bind(&TKernel::mutateMatrix, &m_kernel, MutationStrength));
}

void CMyMorphWidget::requestImageRandomization()
{
    requestOperation(std::bind(&CMyMorphWidget::randomizeImage, this), true);
}

void CMyMorphWidget::setAutoMutateMatrix(bool mutate)
{
    m_autoMutate = mutate;
}

void CMyMorphWidget::randomizeImage()
{
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
}

void CMyMorphWidget::startWorkersManagerThread()
{
    if (!m_workersManagerThread.isRunning())
    {
        m_workersManagerThread.start();
        m_workersManagerThread.startWorkCycle();
    }
    Q_ASSERT(m_workersManagerThread.isRunning());
}

void CMyMorphWidget::stopWorkersManagerThread()
{
    m_workersManagerThread.requestInterruption();
    m_workersManagerThread.wait();
    Q_ASSERT(!m_workersManagerThread.isRunning());

    m_fpsTimeStamp = decltype(m_fpsTimeStamp)();
    m_fps.clear();

    processDeferredOperations();
}

void CMyMorphWidget::paintEvent(QPaintEvent* e)
{
    QPainter painter(this);

    {
        const auto imgSize = m_image.size();
        constexpr const auto borderWidth = TKernel::getRequiredBorder();
        const auto processedSize = imgSize - QSize(2 * borderWidth, 2 * borderWidth);
        const auto scale = std::min(static_cast<float>(width()) / processedSize.width(),
                                    static_cast<float>(height()) / processedSize.height());
        const auto destSize = processedSize * scale;
        const auto destOffsetSize = (size() - destSize) / 2;
        const QPoint destOffset(destOffsetSize.width(), destOffsetSize.height());

        painter.drawImage(QRect(destOffset, destSize),
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
}

void CMyMorphWidget::frameDataReady() // when this slot is called, other threads (WorkersManager and Workers) are
{                                     // waiting for new data until m_workersManagerThread.startWorkCycle() is called
    applyNextFrameData();

    if (m_autoMutate)
        m_kernel.mutateMatrix(MutationStrength);

    processDeferredOperations();

    m_workersManagerThread.startWorkCycle();

    if (const auto curTime = QTime::currentTime(); m_fpsTimeStamp.isNull())
    {
        m_fpsTimeStamp = curTime;
        m_framesSinceTimeStamp = 0;
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

    update();
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

    refreshScanlinesForNewImage();
    prepareComputationalSlots();
    initImageBorder();

    m_nextFrameDataSize = origSize;
    m_spNextFrameData = std::make_unique<TKernel::SRGBColor[]>(m_nextFrameDataSize.height() * m_nextFrameDataSize.width());
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

void CMyMorphWidget::refreshScanlinesForNewImage()
{
    m_spImageScanlines = std::make_unique<QRgb*[]>(m_image.height());

    {
        auto pScanline = reinterpret_cast<QRgb*>(m_image.scanLine(0));
        auto increment = m_image.bytesPerLine() / sizeof(*pScanline);
        m_spImageScanlines[0] = pScanline;

        std::generate(m_spImageScanlines.get() + 1, m_spImageScanlines.get() + m_image.height(), [&pScanline, &increment]{ return pScanline += increment; });
    }
}

void CMyMorphWidget::prepareComputationalSlots()
{
    constexpr const auto borderWidth = TKernel::getRequiredBorder();
    const auto imgSize = m_image.size();
    const auto processedSize = imgSize - QSize(2 * borderWidth, 2 * borderWidth);

    std::vector<QRect> rects(1, QRect({borderWidth, borderWidth}, processedSize));

    auto divideRects = [](std::vector<QRect> &rects)
    {
        std::vector<QRect> res;
        for (auto& rect: rects)
        {
            if (rect.width() > rect.height())
            {
                auto halfWidth = rect.width() / 2;
                QRect rect1(rect.x(), rect.y(), halfWidth, rect.height());
                QRect rect2(rect.x() + halfWidth, rect.y(), rect.width() - halfWidth, rect.height());
                res.push_back(rect1);
                res.push_back(rect2);
            }
            else
            {
                auto halfHeight = rect.height() / 2;
                QRect rect1(rect.x(), rect.y(), rect.width(), halfHeight);
                QRect rect2(rect.x(), rect.y() + halfHeight, rect.width(), rect.height() - halfHeight);
                res.push_back(rect1);
                res.push_back(rect2);
            }
        }
        return res;
    };

    const auto idealThreadCount = QThread::idealThreadCount();

    while (rects.size() < idealThreadCount)
    {
        rects = divideRects(rects);
    }

    m_nextFrameComputationSlots.resize(rects.size());

    std::transform(std::begin(rects),
                   std::end(rects),
                   std::begin(m_nextFrameComputationSlots),
                   [](QRect &rect)
                   {
                       SNextFrameComputationSlot res;
                       const auto topLeft = rect.topLeft();
                       res.m_source = topLeft;
                       res.m_destination = topLeft - QPoint(borderWidth, borderWidth);
                       res.m_size = rect.size();
                       return res;
                   });

    m_workersManagerThread.setThreadCount(m_nextFrameComputationSlots.size());
}

void CMyMorphWidget::computeNextFrameData(size_t slot)
{
    Q_ASSERT(!m_image.isNull());
    Q_ASSERT(m_image.format() == QImage::Format_RGB32);

    const auto MinColor = std::numeric_limits<decltype(SColor::R)>::min();
    const auto MaxColor = std::numeric_limits<decltype(SColor::R)>::max();

    SColor minColors{MaxColor, MaxColor, MaxColor};
    SColor maxColors{MinColor, MinColor, MinColor};

    const auto &bufWidth = m_nextFrameDataSize.width();
    const auto &outX = m_nextFrameComputationSlots[slot].m_destination.x();
    const auto &outY = m_nextFrameComputationSlots[slot].m_destination.y();
    const auto &sourceX = m_nextFrameComputationSlots[slot].m_source.x();
    const auto &sourceY = m_nextFrameComputationSlots[slot].m_source.y();
    const auto &sizeToProcess = m_nextFrameComputationSlots[slot].m_size;

    SColor* pBufScanline = m_spNextFrameData.get() + outX + bufWidth * outY;
    for (int y = sourceY;
         y < sourceY + sizeToProcess.height();
         ++y, pBufScanline += bufWidth)
    {
        for (int x = sourceX; x < sourceX + sizeToProcess.width(); ++x)
        {
            const auto color = m_kernel.apply(x, y, m_spImageScanlines.get());
            pBufScanline[x - sourceX] = color;

            if constexpr (NormalizeFrame)
            {
                minColors.R = std::min(minColors.R, color.R);
                maxColors.R = std::max(maxColors.R, color.R);

                minColors.G = std::min(minColors.G, color.G);
                maxColors.G = std::max(maxColors.G, color.G);

                minColors.B = std::min(minColors.B, color.B);
                maxColors.B = std::max(maxColors.B, color.B);
            }
        }
    }

    if constexpr (NormalizeFrame)
    {
        m_nextFrameComputationSlots[slot].m_minimums = minColors;
        m_nextFrameComputationSlots[slot].m_maximums = maxColors;
    }
}

void CMyMorphWidget::applyNextFrameData()
{
    const auto MinColor = std::numeric_limits<decltype(SColor::R)>::min();
    const auto MaxColor = std::numeric_limits<decltype(SColor::R)>::max();

    SColor minColors{MaxColor, MaxColor, MaxColor};
    SColor maxColors{MinColor, MinColor, MinColor};

    decltype(maxColors.R) rSpan;
    decltype(maxColors.G) gSpan;
    decltype(maxColors.B) bSpan;

    float rScale;
    float gScale;
    float bScale;

    if constexpr (NormalizeFrame)
    {
        maxColors = std::accumulate(std::begin(m_nextFrameComputationSlots),
                                    std::end(m_nextFrameComputationSlots),
                                    maxColors,
                                    [](const SColor& maxColors, const SNextFrameComputationSlot& slot)
                                    {
                                        SColor res;
                                        res.R = std::max(maxColors.R, slot.m_maximums.R);
                                        res.G = std::max(maxColors.G, slot.m_maximums.G);
                                        res.B = std::max(maxColors.B, slot.m_maximums.B);
                                        return res;
                                    });

        minColors = std::accumulate(std::begin(m_nextFrameComputationSlots),
                                    std::end(m_nextFrameComputationSlots),
                                    minColors,
                                    [](const SColor& minColors, const SNextFrameComputationSlot& slot)
                                    {
                                        SColor res;
                                        res.R = std::min(minColors.R, slot.m_minimums.R);
                                        res.G = std::min(minColors.G, slot.m_minimums.G);
                                        res.B = std::min(minColors.B, slot.m_minimums.B);
                                        return res;
                                    });

        rSpan = maxColors.R - minColors.R;
        gSpan = maxColors.G - minColors.G;
        bSpan = maxColors.B - minColors.B;

        rScale = 255.f / rSpan;
        gScale = 255.f / gSpan;
        bScale = 255.f / bSpan;
    }

    const auto imgSize = m_image.size();
    constexpr const auto borderWidth = TKernel::getRequiredBorder();

    QImage newImage(imgSize, QImage::Format_RGB32);
    SColor* pBufScanline = m_spNextFrameData.get();
    auto pNewScanline = reinterpret_cast<QRgb*>(newImage.scanLine(borderWidth));
    for (int y = borderWidth, increment = newImage.bytesPerLine() / sizeof(*pNewScanline);
         y < imgSize.height() - borderWidth;
         ++y, pNewScanline += increment, pBufScanline += m_nextFrameDataSize.width())
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

    m_image = std::move(newImage);
    refreshScanlinesForNewImage();
    initImageBorder();
}

void CMyMorphWidget::requestOperation(std::function<void ()> operation, bool updateNeeded)
{
    if (m_workersManagerThread.isRunning())
    {
        m_deferredOperationsQueue.emplace(operation);
    }
    else
    {
        operation();
        if (updateNeeded)
            update();
    }
}

void CMyMorphWidget::processDeferredOperations()
{
    for (; !m_deferredOperationsQueue.empty(); m_deferredOperationsQueue.pop())
        m_deferredOperationsQueue.front()();
}



void CWorkersManagerThread::setThreadCount(size_t count)
{
    Q_ASSERT((m_numThreads == count) || !isRunning());
    m_numThreads = count;
}

void CWorkersManagerThread::startWorkCycle()
{
    {
        std::lock_guard feedLock(m_feedMutex);
        m_feedReady = true;
    }
    m_feedCV.notify_all();
}

void CWorkersManagerThread::requestInterruption()
{
    TBase::requestInterruption();
    m_feedCV.notify_all();
}

void CWorkersManagerThread::run()
{
    std::atomic_bool interruptionRequested = false;
    std::barrier frameDataDone(m_numThreads + 1, [this, &interruptionRequested]() noexcept
                                {
                                    interruptionRequested = isInterruptionRequested();
                                    std::unique_lock feedLock(m_feedMutex);
                                    m_feedReady = false;
                                });

    auto worker = [this, &frameDataDone, &interruptionRequested](size_t slot)
                    {
                        do
                        {
                            {
                                std::unique_lock feedLock(m_feedMutex);
                                m_feedCV.wait(feedLock, [this]{ return m_feedReady || isInterruptionRequested(); });
                            }
                            if (m_feedReady)
                                m_workFunction(slot);
                            frameDataDone.arrive_and_wait();
                        } while (!interruptionRequested);
                    };

    std::vector<std::jthread> workers;
    workers.reserve(m_numThreads);
    for (size_t i = 0; i < m_numThreads; ++i)
        workers.emplace_back(worker, i);

    while (true)
    {
        {
            std::unique_lock feedLock(m_feedMutex);
            m_feedCV.wait(feedLock, [this]{ return m_feedReady || isInterruptionRequested(); });
        }
        frameDataDone.arrive_and_wait();
        if (interruptionRequested)
            break;
        emit frameUpdated();
    }
}
