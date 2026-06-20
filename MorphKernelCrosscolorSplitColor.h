#ifndef MORPHKERNELCROSSCOLORSPLITCOLOR_H
#define MORPHKERNELCROSSCOLORSPLITCOLOR_H

#include <QImage>
#include <cstddef>
#include <random>

class CMyMorphKernelCrosscolorSplitColor
{
    typedef float TMatrixElement;

    constexpr static const size_t MatrixSize = 7;
    constexpr static const size_t CrossColorMatrixSize = 5;

    constexpr static const TMatrixElement CrossColorCoupling = 0.3f;
    constexpr static const TMatrixElement MatrixElementMin = -0.1f;
    constexpr static const TMatrixElement MatrixElementMax = 0.1f;

    static const int ZeroColor = 127;

    constexpr static const int MatrixCenter = MatrixSize / 2;
    constexpr static const int CrossColorMatrixCenter = CrossColorMatrixSize / 2;
    constexpr static const int RequiredBorder = std::max(MatrixCenter, CrossColorMatrixCenter);

    static_assert(MatrixSize % 2 == 1, "MatrixSize must be odd");
    static_assert(CrossColorMatrixSize % 2 == 1, "CrossColorMatrixSize must be odd");

    typedef TMatrixElement TMatrix[MatrixSize][MatrixSize];
    typedef TMatrixElement TCrossColorMatrix[CrossColorMatrixSize][CrossColorMatrixSize][2];

public:
    struct SRGBColor
    {
        TMatrixElement R;
        TMatrixElement G;
        TMatrixElement B;
    };

    typedef QRgb* TScanlinePointers[];

    CMyMorphKernelCrosscolorSplitColor();

    void setTrivialMatrix(); // all zero except implicit 1 in the center - should morph image into itself
    void randomizeMatrix();
    void mutateMatrix(TMatrixElement strength);

    SRGBColor apply(int x, int y, const TScanlinePointers pScanlines);
    QRgb applyWithClamp(int x, int y, const TScanlinePointers pScanlines);

    constexpr static int getRequiredBorder() { return RequiredBorder; }

private:
    void updateMatrices();

    TMatrix m_matrixRed;
    TMatrix m_matrixGreen;
    TMatrix m_matrixBlue;

    TCrossColorMatrix m_crossColorMatrix;

    TMatrix m_rawMatrixRed;
    TMatrix m_rawMatrixGreen;
    TMatrix m_rawMatrixBlue;
    TCrossColorMatrix m_rawCrossColorMatrix;

    std::mt19937 m_randomEngine;
    std::uniform_real_distribution<TMatrixElement> m_randomDistrubution;
};

#endif // MORPHKERNELCROSSCOLORSPLITCOLOR_H
