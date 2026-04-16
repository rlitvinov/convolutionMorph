#include "MorphKernelCrosscolorSplitColor.h"


CMyMorphKernelCrosscolorSplitColor::CMyMorphKernelCrosscolorSplitColor()
    :m_randomDistrubution(MatrixElementMin, MatrixElementMax)
    ,m_randomEngine(std::random_device()())
{
    setTrivialMatrix();
}

void CMyMorphKernelCrosscolorSplitColor::setTrivialMatrix()
{
    std::fill(  &m_rawMatrixRed[0][0],
                &m_rawMatrixRed[0][0] + sizeof(m_rawMatrixRed) / sizeof(m_rawMatrixRed[0][0]),
                TMatrixElement(0.f));

    std::fill(  &m_rawMatrixGreen[0][0],
                &m_rawMatrixGreen[0][0] + sizeof(m_rawMatrixGreen) / sizeof(m_rawMatrixGreen[0][0]),
                TMatrixElement(0.f));

    std::fill(  &m_rawMatrixBlue[0][0],
                &m_rawMatrixBlue[0][0] + sizeof(m_rawMatrixBlue) / sizeof(m_rawMatrixBlue[0][0]),
                TMatrixElement(0.f));

    std::fill(  &m_rawCrossColorMatrix[0][0][0],
                &m_rawCrossColorMatrix[0][0][0] + sizeof(m_rawCrossColorMatrix) / sizeof(m_rawCrossColorMatrix[0][0][0]),
                TMatrixElement(0.f));

    updateMatrices();
}

void CMyMorphKernelCrosscolorSplitColor::randomizeMatrix()
{
    std::generate(  &m_rawMatrixRed[0][0],
                    &m_rawMatrixRed[0][0] + sizeof(m_rawMatrixRed) / sizeof(m_rawMatrixRed[0][0]),
                    [this](){ return m_randomDistrubution(m_randomEngine); });

    std::generate(  &m_rawMatrixGreen[0][0],
                    &m_rawMatrixGreen[0][0] + sizeof(m_rawMatrixGreen) / sizeof(m_rawMatrixGreen[0][0]),
                    [this](){ return m_randomDistrubution(m_randomEngine); });

    std::generate(  &m_rawMatrixBlue[0][0],
                    &m_rawMatrixBlue[0][0] + sizeof(m_rawMatrixBlue) / sizeof(m_rawMatrixBlue[0][0]),
                    [this](){ return m_randomDistrubution(m_randomEngine); });

    std::generate(  &m_rawCrossColorMatrix[0][0][0],
                    &m_rawCrossColorMatrix[0][0][0] + sizeof(m_rawCrossColorMatrix) / sizeof(m_rawCrossColorMatrix[0][0][0]),
                    [this](){ return m_randomDistrubution(m_randomEngine); });

    updateMatrices();
}

void CMyMorphKernelCrosscolorSplitColor::mutateMatrix(TMatrixElement strength)
{
    std::uniform_real_distribution<TMatrixElement> mutationDistrubution(-strength, strength);

    std::for_each(  &m_rawMatrixRed[0][0],
                    &m_rawMatrixRed[0][0] + sizeof(m_rawMatrixRed) / sizeof(m_rawMatrixRed[0][0]),
                    [this, &mutationDistrubution](auto& elem){ elem += mutationDistrubution(m_randomEngine); });

    std::for_each(  &m_rawMatrixGreen[0][0],
                    &m_rawMatrixGreen[0][0] + sizeof(m_rawMatrixGreen) / sizeof(m_rawMatrixGreen[0][0]),
                    [this, &mutationDistrubution](auto& elem){ elem += mutationDistrubution(m_randomEngine); });

    std::for_each(  &m_rawMatrixBlue[0][0],
                    &m_rawMatrixBlue[0][0] + sizeof(m_rawMatrixBlue) / sizeof(m_rawMatrixBlue[0][0]),
                    [this, &mutationDistrubution](auto& elem){ elem += mutationDistrubution(m_randomEngine); });

    std::for_each(  &m_rawCrossColorMatrix[0][0][0],
                    &m_rawCrossColorMatrix[0][0][0] + sizeof(m_rawCrossColorMatrix) / sizeof(m_rawCrossColorMatrix[0][0][0]),
                    [this, &mutationDistrubution](auto& elem){ elem += mutationDistrubution(m_randomEngine); });

    updateMatrices();
}

CMyMorphKernelCrosscolorSplitColor::SRGBColor CMyMorphKernelCrosscolorSplitColor::apply(int x, int y, const TScanlinePointers pScanlines, const QSize imageSize)
{
    SRGBColor res{0.f, 0.f, 0.f};

    {
        const auto color = pScanlines[y][x];
        res.R += qRed(color) - ZeroColor;
        res.G += qGreen(color) - ZeroColor;
        res.B += qBlue(color) - ZeroColor;
    }

    const int MatrixCenter = MatrixSize / 2;

    for (int mY = 0; mY < MatrixSize; ++mY)
        for (int mX = 0; mX < MatrixSize; ++mX)
        {
            auto imX = x + mX - MatrixCenter;
            auto imY = y + mY - MatrixCenter;

            if (imX < 0)
                imX += imageSize.width();
            else if (imX >= imageSize.width())
                imX -= imageSize.width();

            if (imY < 0)
                imY += imageSize.height();
            else if (imY >= imageSize.height())
                imY -= imageSize.height();

            Q_ASSERT(   (imX >= 0) &&
                        (imY >= 0) &&
                        (imX < imageSize.width()) &&
                        (imY < imageSize.height()) );

            const auto color = pScanlines[imY][imX];
            const auto coeffR = m_matrixRed[mY][mX];
            const auto coeffG = m_matrixGreen[mY][mX];
            const auto coeffB = m_matrixBlue[mY][mX];
            res.R += (qRed(color) - ZeroColor) * coeffR;
            res.G += (qGreen(color) - ZeroColor) * coeffG;
            res.B += (qBlue(color) - ZeroColor) * coeffB;
        }

    const int CrossColorMatrixCenter = CrossColorMatrixSize / 2;

    for (int mY = 0; mY < CrossColorMatrixSize; ++mY)
        for (int mX = 0; mX < CrossColorMatrixSize; ++mX)
        {
            auto imX = x + mX - CrossColorMatrixCenter;
            auto imY = y + mY - CrossColorMatrixCenter;

            if (imX < 0)
                imX += imageSize.width();
            else if (imX >= imageSize.width())
                imX -= imageSize.width();

            if (imY < 0)
                imY += imageSize.height();
            else if (imY >= imageSize.height())
                imY -= imageSize.height();

            Q_ASSERT(   (imX >= 0) &&
                        (imY >= 0) &&
                        (imX < imageSize.width()) &&
                        (imY < imageSize.height()) );

            const auto color = pScanlines[imY][imX];
            const auto coeffLower = m_crossColorMatrix[mY][mX][0] * CrossColorCoupling;
            const auto coeffHigher = m_crossColorMatrix[mY][mX][1] * CrossColorCoupling;

            const auto [r, g, b] = std::make_tuple(qRed(color) - ZeroColor, qGreen(color) - ZeroColor, qBlue(color) - ZeroColor);

            res.R += g * coeffLower;
            res.R += b * coeffHigher;

            res.G += b * coeffLower;
            res.G += r * coeffHigher;

            res.B += r * coeffLower;
            res.B += g * coeffHigher;
        }

    res.R += ZeroColor;
    res.G += ZeroColor;
    res.B += ZeroColor;

    return res;
}

QRgb CMyMorphKernelCrosscolorSplitColor::applyWithClamp(int x, int y, const TScanlinePointers pScanlines, const QSize imageSize)
{
    auto [resR, resG, resB] = apply(x, y, pScanlines, imageSize);

    resR = std::clamp(resR, 0.f, 255.f);
    resG = std::clamp(resG, 0.f, 255.f);
    resB = std::clamp(resB, 0.f, 255.f);

    return qRgb(static_cast<int>(resR),
                static_cast<int>(resG),
                static_cast<int>(resB));
}

void CMyMorphKernelCrosscolorSplitColor::updateMatrices()
{
    constexpr int MatrixCenter = MatrixSize / 2;
    for (int y = 0; y < MatrixSize; ++y)
    {
        const auto yCoord = y - MatrixCenter;
        for (int x = 0; x < MatrixSize; ++x)
        {
            const auto xCoord = x - MatrixCenter;
            const auto distSquared = xCoord*xCoord + yCoord*yCoord;
            const auto coeff = std::sqrt(distSquared + 1);
            const auto adjustedValueR = m_rawMatrixRed[y][x] / coeff;
            const auto adjustedValueG = m_rawMatrixGreen[y][x] / coeff;
            const auto adjustedValueB = m_rawMatrixBlue[y][x] / coeff;
            m_matrixRed[y][x] = adjustedValueR;
            m_matrixGreen[y][x] = adjustedValueG;
            m_matrixBlue[y][x] = adjustedValueB;
        }
    }

    constexpr int CrossColorMatrixCenter = CrossColorMatrixSize / 2;
    for (int y = 0; y < CrossColorMatrixSize; ++y)
    {
        const auto yCoord = y - CrossColorMatrixCenter;
        for (int x = 0; x < CrossColorMatrixSize; ++x)
        {
            const auto xCoord = x - CrossColorMatrixCenter;
            const auto distSquared = xCoord*xCoord + yCoord*yCoord;

            const auto adjustedValue0 = m_rawCrossColorMatrix[y][x][0] / std::sqrt(distSquared + 1);
            const auto adjustedValue1 = m_rawCrossColorMatrix[y][x][1] / std::sqrt(distSquared + 1);

            m_crossColorMatrix[y][x][0] = adjustedValue0;
            m_crossColorMatrix[y][x][1] = adjustedValue1;
        }
    }
}
