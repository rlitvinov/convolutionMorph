#include "MorphKernelCrosscolor.h"


CMyMorphKernelCrosscolor::CMyMorphKernelCrosscolor()
    :m_randomDistrubution(MatrixElementMin, MatrixElementMax)
    ,m_randomEngine(std::random_device()())
{
    setTrivialMatrix();
}

void CMyMorphKernelCrosscolor::setTrivialMatrix()
{
    std::fill(  &m_rawMatrix[0][0],
                &m_rawMatrix[0][0] + sizeof(m_rawMatrix) / sizeof(m_rawMatrix[0][0]),
                TMatrixElement(0.f));

    std::fill(  &m_rawCrossColorMatrix[0][0][0],
                &m_rawCrossColorMatrix[0][0][0] + sizeof(m_rawCrossColorMatrix) / sizeof(m_rawCrossColorMatrix[0][0][0]),
                TMatrixElement(0.f));

    updateMatrices();
}

void CMyMorphKernelCrosscolor::randomizeMatrix()
{
    std::generate(  &m_rawMatrix[0][0],
                    &m_rawMatrix[0][0] + sizeof(m_rawMatrix) / sizeof(m_rawMatrix[0][0]),
                    [this](){ return m_randomDistrubution(m_randomEngine); });

    std::generate(  &m_rawCrossColorMatrix[0][0][0],
                    &m_rawCrossColorMatrix[0][0][0] + sizeof(m_rawCrossColorMatrix) / sizeof(m_rawCrossColorMatrix[0][0][0]),
                    [this](){ return m_randomDistrubution(m_randomEngine); });

    updateMatrices();
}

void CMyMorphKernelCrosscolor::mutateMatrix(TMatrixElement strength)
{
    std::uniform_real_distribution<TMatrixElement> mutationDistrubution(-strength, strength);

    std::for_each(  &m_rawMatrix[0][0],
                    &m_rawMatrix[0][0] + sizeof(m_rawMatrix) / sizeof(m_rawMatrix[0][0]),
                    [this, &mutationDistrubution](auto& elem){ elem += mutationDistrubution(m_randomEngine); });

    std::for_each(  &m_rawCrossColorMatrix[0][0][0],
                    &m_rawCrossColorMatrix[0][0][0] + sizeof(m_rawCrossColorMatrix) / sizeof(m_rawCrossColorMatrix[0][0][0]),
                    [this, &mutationDistrubution](auto& elem){ elem += mutationDistrubution(m_randomEngine); });

    updateMatrices();
}

CMyMorphKernelCrosscolor::SRGBColor CMyMorphKernelCrosscolor::apply(int x, int y, const TScanlinePointers pScanlines)
{
    SRGBColor res{0.f, 0.f, 0.f};

    {
        const auto color = pScanlines[y][x];
        res.R += qRed(color) - ZeroColor;
        res.G += qGreen(color) - ZeroColor;
        res.B += qBlue(color) - ZeroColor;
    }

    for (int mY = 0; mY < MatrixSize; ++mY)
        for (int mX = 0; mX < MatrixSize; ++mX)
        {
            auto imX = x + mX - MatrixCenter;
            auto imY = y + mY - MatrixCenter;

            Q_ASSERT(   (imX >= 0) &&
                        (imY >= 0) );

            const auto color = pScanlines[imY][imX];
            const auto coeff = m_matrix[mY][mX];
            res.R += (qRed(color) - ZeroColor) * coeff;
            res.G += (qGreen(color) - ZeroColor) * coeff;
            res.B += (qBlue(color) - ZeroColor) * coeff;
        }

    for (int mY = 0; mY < CrossColorMatrixSize; ++mY)
        for (int mX = 0; mX < CrossColorMatrixSize; ++mX)
        {
            auto imX = x + mX - CrossColorMatrixCenter;
            auto imY = y + mY - CrossColorMatrixCenter;

            Q_ASSERT(   (imX >= 0) &&
                        (imY >= 0) );

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

QRgb CMyMorphKernelCrosscolor::applyWithClamp(int x, int y, const TScanlinePointers pScanlines)
{
    auto [resR, resG, resB] = apply(x, y, pScanlines);

    resR = std::clamp(resR, 0.f, 255.f);
    resG = std::clamp(resG, 0.f, 255.f);
    resB = std::clamp(resB, 0.f, 255.f);

    return qRgb(static_cast<int>(resR),
                static_cast<int>(resG),
                static_cast<int>(resB));
}

void CMyMorphKernelCrosscolor::updateMatrices()
{
    constexpr int MatrixCenter = MatrixSize / 2;
    for (int y = 0; y < MatrixSize; ++y)
    {
        const auto yCoord = y - MatrixCenter;
        for (int x = 0; x < MatrixSize; ++x)
        {
            const auto xCoord = x - MatrixCenter;
            const auto distSquared = xCoord*xCoord + yCoord*yCoord;
            const auto adjustedValue = m_rawMatrix[y][x] / std::sqrt(distSquared + 1);
            m_matrix[y][x] = adjustedValue;
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
