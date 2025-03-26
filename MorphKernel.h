#ifndef MORPHKERNEL_H
#define MORPHKERNEL_H

#include <QImage>
#include <cstddef>
#include <random>

class CMyMorphKernel
{
    typedef float TMatrixElement;

    static const size_t MatrixSize = 9;
    constexpr static const TMatrixElement MatrixElementMin = -0.1f;
    constexpr static const TMatrixElement MatrixElementMax = 0.1f;

    static const int ZeroColor = 0;

    static_assert(MatrixSize % 2 == 1, "MatrixSize must be odd");

    typedef TMatrixElement TMatrix[MatrixSize][MatrixSize];

public:
    typedef QRgb* TScanlinePointers[];

    CMyMorphKernel();

    void setTrivialMatrix(); // all zero except implicit 1 in the center - should morph image into itself
    void randomizeMatrix();
    void mutateMatrix(TMatrixElement strength);

    QRgb apply(int x, int y, const TScanlinePointers pScanlines, const QSize imageSize);

private:
    TMatrix m_matrix;

    std::mt19937 m_randomEngine;
    std::uniform_real_distribution<TMatrixElement> m_randomDistrubution;
};

#endif // MORPHKERNEL_H
