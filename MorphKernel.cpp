#include "MorphKernel.h"


CMyMorphKernel::CMyMorphKernel()
    :m_randomDistrubution(MatrixElementMin, MatrixElementMax)
    ,m_randomEngine(std::random_device()())
{
    setTrivialMatrix();
}

void CMyMorphKernel::setTrivialMatrix()
{
    std::fill(  &m_matrix[0][0],
                &m_matrix[0][0] + sizeof(m_matrix) / sizeof(m_matrix[0][0]),
                TMatrixElement(0.f));
}

void CMyMorphKernel::randomizeMatrix()
{
    std::generate(  &m_matrix[0][0],
                    &m_matrix[0][0] + sizeof(m_matrix) / sizeof(m_matrix[0][0]),
                    [this](){ return m_randomDistrubution(m_randomEngine); });
}

void CMyMorphKernel::mutateMatrix(TMatrixElement strength)
{
    std::uniform_real_distribution<TMatrixElement> mutationDistrubution(-strength, strength);

    std::for_each(  &m_matrix[0][0],
                    &m_matrix[0][0] + sizeof(m_matrix) / sizeof(m_matrix[0][0]),
                    [this, &mutationDistrubution](auto& elem){ elem += mutationDistrubution(m_randomEngine); });
}

CMyMorphKernel::SRGBColor CMyMorphKernel::apply(int x, int y, const TScanlinePointers pScanlines)
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

    res.R += ZeroColor;
    res.G += ZeroColor;
    res.B += ZeroColor;

    return res;
}

QRgb CMyMorphKernel::applyWithClamp(int x, int y, const TScanlinePointers pScanlines)
{
    auto [resR, resG, resB] = apply(x, y, pScanlines);

    resR = std::clamp(resR, 0.f, 255.f);
    resG = std::clamp(resG, 0.f, 255.f);
    resB = std::clamp(resB, 0.f, 255.f);

    return qRgb(static_cast<int>(resR),
                static_cast<int>(resG),
                static_cast<int>(resB));
}
