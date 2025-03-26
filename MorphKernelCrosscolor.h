#ifndef MORPHKERNELCROSSCOLOR_H
#define MORPHKERNELCROSSCOLOR_H

#include <QImage>
#include <cstddef>
#include <random>

class CMyMorphKernelCrosscolor
{
    typedef float TMatrixElement;

    static const size_t MatrixSize = 3;
    static const size_t CrossColorMatrixSize = 5;
    constexpr static const TMatrixElement MatrixElementMin = -0.1f;
    constexpr static const TMatrixElement MatrixElementMax = 0.1f;

    static const int ZeroColor = 127;

    static_assert(MatrixSize % 2 == 1, "MatrixSize must be odd");
    static_assert(CrossColorMatrixSize % 2 == 1, "CrossColorMatrixSize must be odd");

    typedef TMatrixElement TMatrix[MatrixSize][MatrixSize];
    typedef TMatrixElement TCrossColorMatrix[CrossColorMatrixSize][CrossColorMatrixSize][2];

public:
    typedef QRgb* TScanlinePointers[];

    CMyMorphKernelCrosscolor();

    void setTrivialMatrix(); // all zero except implicit 1 in the center - should morph image into itself
    void randomizeMatrix();
    void mutateMatrix(TMatrixElement strength);

    QRgb apply(int x, int y, const TScanlinePointers pScanlines, const QSize imageSize);

private:
    TMatrix m_matrix;
    TCrossColorMatrix m_crossColorMatrix;

    std::mt19937 m_randomEngine;
    std::uniform_real_distribution<TMatrixElement> m_randomDistrubution;
};

#endif // MORPHKERNELCROSSCOLOR_H
