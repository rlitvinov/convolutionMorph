#include "MorphKernelCrosscolor.h"


CMyMorphKernelCrosscolor::CMyMorphKernelCrosscolor()
    :m_randomDistrubution(MatrixElementMin, MatrixElementMax)
    ,m_randomEngine(std::random_device()())
{
    setTrivialMatrix();
}

void CMyMorphKernelCrosscolor::setTrivialMatrix()
{
    std::fill(  &m_matrix[0][0],
                &m_matrix[0][0] + sizeof(m_matrix) / sizeof(m_matrix[0][0]),
                TMatrixElement(0.f));

    std::fill(  &m_crossColorMatrix[0][0][0],
                &m_crossColorMatrix[0][0][0] + sizeof(m_crossColorMatrix) / sizeof(m_crossColorMatrix[0][0][0]),
                TMatrixElement(0.f));
}

void CMyMorphKernelCrosscolor::randomizeMatrix()
{
    std::generate(  &m_matrix[0][0],
                    &m_matrix[0][0] + sizeof(m_matrix) / sizeof(m_matrix[0][0]),
                    [this](){ return m_randomDistrubution(m_randomEngine); });

    std::generate(  &m_crossColorMatrix[0][0][0],
                    &m_crossColorMatrix[0][0][0] + sizeof(m_crossColorMatrix) / sizeof(m_crossColorMatrix[0][0][0]),
                    [this](){ return m_randomDistrubution(m_randomEngine); });
}

void CMyMorphKernelCrosscolor::mutateMatrix(TMatrixElement strength)
{
    std::uniform_real_distribution<TMatrixElement> mutationDistrubution(-strength, strength);

    std::for_each(  &m_matrix[0][0],
                    &m_matrix[0][0] + sizeof(m_matrix) / sizeof(m_matrix[0][0]),
                    [this, &mutationDistrubution](auto& elem){ elem += mutationDistrubution(m_randomEngine); });

    std::for_each(  &m_crossColorMatrix[0][0][0],
                    &m_crossColorMatrix[0][0][0] + sizeof(m_crossColorMatrix) / sizeof(m_crossColorMatrix[0][0][0]),
                    [this, &mutationDistrubution](auto& elem){ elem += mutationDistrubution(m_randomEngine); });
}

CMyMorphKernelCrosscolor::SRGBColor CMyMorphKernelCrosscolor::apply(int x, int y, const TScanlinePointers pScanlines, const QSize imageSize)
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
            const auto coeff = m_matrix[mY][mX];
            res.R += (qRed(color) - ZeroColor) * coeff;
            res.G += (qGreen(color) - ZeroColor) * coeff;
            res.B += (qBlue(color) - ZeroColor) * coeff;
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

QRgb CMyMorphKernelCrosscolor::applyWithClamp(int x, int y, const TScanlinePointers pScanlines, const QSize imageSize)
{
    auto [resR, resG, resB] = apply(x, y, pScanlines, imageSize);

    resR = std::clamp(resR, 0.f, 255.f);
    resG = std::clamp(resG, 0.f, 255.f);
    resB = std::clamp(resB, 0.f, 255.f);

    return qRgb(static_cast<int>(resR),
                static_cast<int>(resG),
                static_cast<int>(resB));
}
