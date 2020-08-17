#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "qcvimg.h"

#include <QBuffer>
#include <QDebug>
#include <QImage>
#include <opencv4/opencv2/core/mat.hpp>

using namespace testing;


struct QCVimgDefaultConstruct : public Test
{
    QCVimg img;
    int originalWidth = 4, originalHeight = 7;
    int fillColor = 134;
};

TEST_F(QCVimgDefaultConstruct, MatMembersNeedsToBeEmptyToBeConsideredEmpty)
{
    img.cvMat() = cv::Mat(originalHeight, originalWidth, CV_8UC1, fillColor);

    ASSERT_FALSE(img.empty());
}

TEST_F(QCVimgDefaultConstruct, QImageMemberNeedsToBeEmptyToBeConsideredEmpty)
{
    img.qImg() = QImage(originalWidth, originalHeight, QImage::Format_Grayscale8);
    img.fill(fillColor);

    ASSERT_FALSE(img.empty());
}

TEST_F(QCVimgDefaultConstruct, IsEmptyOnCreation)
{
    ASSERT_TRUE(img.empty());
}

struct QCVimgConstructFromImage : public Test
{
    QCVimg img;
    QImage qOrigImg;
    uint32_t originalFillColor = 150;
    int originalWidth = 15, originalHeight = 10;
    int originalMatFormat = CV_8UC1;
    QImage::Format originalQImgFormat = QImage::Format_Grayscale8;
};

struct QCVimgConstructFromQImage : public QCVimgConstructFromImage
{
    void SetUp() override {
        qOrigImg = QImage(originalWidth, originalHeight, originalQImgFormat);
        qOrigImg.fill(originalFillColor);
        img = QCVimg(qOrigImg);
    }
};

TEST_F(QCVimgConstructFromQImage, QImageDataMatchesSourceImageData)
{
    int resultFillColor = qGray(img.pixelColor(1,1).rgb());

    ASSERT_THAT(resultFillColor, Eq(originalFillColor));
}

TEST_F(QCVimgConstructFromQImage, MatMemberPointsToQImageData)
{
    auto qDataPtr = img.qImg().bits();
    auto matDataPtr = img.cvMat().data;

    ASSERT_THAT(matDataPtr, Eq(qDataPtr));
}

TEST_F(QCVimgConstructFromQImage, MatMemberReturnsDataEqualToQImage)
{
    int qImgPixel = qGray(img.qImg().pixel(1,1));
    uint8_t matPixel = img.cvMat().at<uint8_t>(1,1);

    ASSERT_THAT(matPixel, Eq(qImgPixel));
}

TEST_F(QCVimgConstructFromQImage, MatMemberSizeEqualsQImageSize)
{
    ASSERT_THAT(img.cvMat().cols, img.qImg().width());
    ASSERT_THAT(img.cvMat().rows, img.qImg().height());
}

TEST_F(QCVimgConstructFromQImage, MatMemberNotBoundIfDataPtrMismatchWithQImage)
{
    img.cvMat().data = nullptr;
    ASSERT_FALSE(img.isMatBound());
}

TEST_F(QCVimgConstructFromQImage, MatMemberNotBoundIfSizeMismatchWithQImage)
{
    img.cvMat().rows = originalHeight + 1;
    ASSERT_FALSE(img.isMatBound());
}

TEST_F(QCVimgConstructFromQImage, MatMemberNotBoundIfFormatMismatchWitchQImage)
{
    img.cvMat() = cv::Mat(originalWidth, originalHeight, CV_16UC1);
    ASSERT_FALSE(img.isMatBound());
}

TEST_F(QCVimgConstructFromQImage, QImageMemberReturnsOriginalQImageFormat) {
    ASSERT_THAT(img.qFormat(), Eq(originalQImgFormat));
}

struct QImageFormatCase
{
    QImageFormatCase(QImage::Format argQFormat, int argExpectMatFormat)
        : qFormat{argQFormat}, expectedMatFormat{argExpectMatFormat} {}

    QImage::Format qFormat;
    int expectedMatFormat;
};

struct QCVimgConstructFromQImageWithFormat : public TestWithParam<QImageFormatCase>
{
    QCVimg img;
    int originalWidth = 10, originalHeight = 20;
};

struct QCVimgConstructFromQImageWithFormat1 : public QCVimgConstructFromQImageWithFormat
{
};

QImageFormatCase validQImageFormatCases[] = {
    QImageFormatCase(QImage::Format_RGB32, CV_8UC4),
    QImageFormatCase(QImage::Format_ARGB32, CV_8UC4),
    QImageFormatCase(QImage::Format_RGB888, CV_8UC3),
    QImageFormatCase(QImage::Format_Alpha8, CV_8UC1),
    QImageFormatCase(QImage::Format_Grayscale8, CV_8UC1),
//    FormatConvertCase(QImage::Format_Grayscale16, CV_16UC1) needs >= Qt 5.13
};

TEST_P(QCVimgConstructFromQImageWithFormat1, ValidQImageFormatConvertsToEquivalentMatFormat)
{
    QImageFormatCase testCase = GetParam();

    QImage qImg(originalWidth, originalHeight, testCase.qFormat);
    img = QCVimg(qImg);

    ASSERT_THAT(img.matFormat(), Eq(testCase.expectedMatFormat));
}
INSTANTIATE_TEST_SUITE_P(QCVimg_BulkTest, QCVimgConstructFromQImageWithFormat1, ValuesIn(validQImageFormatCases));

struct QCVimgConstructFromQImageWithFormat2 : public QCVimgConstructFromQImageWithFormat
{
};

QImageFormatCase invalidQImageFormatCases[] = {
    QImageFormatCase(QImage::Format_Mono, -1),
    QImageFormatCase(QImage::Format_RGB444, -1),
    QImageFormatCase(QImage::Format_RGB444, -1),
    QImageFormatCase(QImage::Format_RGB666, -1),
    QImageFormatCase(QImage::Format_RGB16, -1),
    QImageFormatCase(QImage::Format_RGBA64, -1)
};

TEST_P(QCVimgConstructFromQImageWithFormat2, InvalidQImageFormatReturnsEmptyImage)
{
    QImageFormatCase testCase = GetParam();

    QImage qImg(originalWidth, originalHeight, testCase.qFormat);
    img = QCVimg(qImg);

    ASSERT_TRUE(img.empty());
}
INSTANTIATE_TEST_SUITE_P(QCVimg_BulkTest, QCVimgConstructFromQImageWithFormat2, ValuesIn(invalidQImageFormatCases));

TEST_F(QCVimgConstructFromQImage, MovingOriginalQImageIntoQCVimgLeavesOriginalEmtpy)
{
    img = QCVimg(std::move(qOrigImg));

    ASSERT_TRUE(qOrigImg.isNull());
}

TEST_F(QCVimgConstructFromQImage, QImageMemberPointsToMovedFromData)
{
    auto originalImgDataPtr = qOrigImg.bits();

    img = QCVimg(std::move(qOrigImg));
    auto movedToImgDataPtr = img.qImg().bits();

    ASSERT_THAT(movedToImgDataPtr, Eq(originalImgDataPtr));
}

TEST_F(QCVimgConstructFromQImage, MatMemberPointsToMovedInData)
{
    auto originalImgDataPtr = qOrigImg.bits();

    img = QCVimg(std::move(qOrigImg));
    auto movedToImgDataPtr = img.cvMat().data;

    ASSERT_THAT(movedToImgDataPtr, Eq(originalImgDataPtr));
}

struct QCVimgConstructFromMat : public QCVimgConstructFromImage
{
    void SetUp() override {
        cv::Mat matOrigImg(originalHeight, originalWidth, originalMatFormat, originalFillColor);
        img = QCVimg(matOrigImg);
    }
};

TEST_F(QCVimgConstructFromMat, QImageMemberGetsCreatedWithMatchingSize)
{
    ASSERT_FALSE(img.qImg().isNull());
}

TEST_F(QCVimgConstructFromMat, QImageDataMatchesSourceImageData)
{
    int resultFillColor = qGray(img.pixelColor(1,1).rgb());

    ASSERT_THAT(resultFillColor, Eq(originalFillColor));
}

TEST_F(QCVimgConstructFromMat, MatMemberPointsToQImageData)
{
    auto qDataPtr = img.qImg().bits();
    auto matDataPtr = img.cvMat().data;

    ASSERT_THAT(matDataPtr, Eq(qDataPtr));
}


TEST_F(QCVimgConstructFromMat, MatMemberSizeEqualsQImageSize)
{
    ASSERT_THAT(img.cvMat().cols, img.qImg().width());
    ASSERT_THAT(img.cvMat().rows, img.qImg().height());
}

TEST_F(QCVimgConstructFromMat, MatMemberReturnsDataEqualToQImage)
{
    int qImgPixel = qGray(img.qImg().pixel(1,1));
    uint8_t matPixel = img.cvMat().at<uint8_t>(1,1);

    ASSERT_THAT(matPixel, Eq(qImgPixel));
}

struct QCVimgConstructFromMat2 : public QCVimgConstructFromMat
{
    void SetUp() override {
        originalFillRgbColor = qRgb(30, 50, 100);
        cv::Mat matOrigImg(originalHeight, originalWidth, CV_8UC3, QCVimg::convertQColorToScalar(originalFillRgbColor, MatColorOrder::BGR));
        img = QCVimg(matOrigImg, MatColorOrder::BGR);
    }

    QColor originalFillRgbColor;
};

TEST_F(QCVimgConstructFromMat2, SourceImageWithBGRColorOrderConvertsToRGBDuringConstruction)
{
    QColor imgPixel = img.pixelColor(1,1);

    ASSERT_THAT(imgPixel.rgb(), Eq(originalFillRgbColor.rgb()));
}

struct MatFormatCase
{
    MatFormatCase(int argMatFormat, QImage::Format argExpectedQFormat)
        : matFormat{argMatFormat}, expectedQImageFormat{argExpectedQFormat} {}

    int matFormat;
    QImage::Format expectedQImageFormat;
};

struct QCVimgConstructFromMatWithFormat : public TestWithParam<MatFormatCase>
{
    QCVimg img;
    int originalWidth = 8, originalHeight = 14;
    int originalColor = 110;
};

struct QCVimgConstructFromMatWithFormat1 : public QCVimgConstructFromMatWithFormat
{
};

MatFormatCase validMatFormatCases[] = {
    MatFormatCase(CV_8UC1, QImage::Format_Grayscale8),
    MatFormatCase(CV_8UC3, QImage::Format_RGB888),
    MatFormatCase(CV_8UC4, QImage::Format_ARGB32),
    MatFormatCase(CV_16UC1, QImage::Format_Grayscale16)
};

TEST_P(QCVimgConstructFromMatWithFormat1, ValidMatFormatConvertsToEquivalentQImageFormat)
{
    MatFormatCase testCase = GetParam();

    cv::Mat matImg(originalWidth, originalHeight, testCase.matFormat, originalColor);
    img = QCVimg(matImg);

    ASSERT_THAT(img.qFormat(), Eq(testCase.expectedQImageFormat));
}
INSTANTIATE_TEST_SUITE_P(QCVimg_BulkTest, QCVimgConstructFromMatWithFormat1, ValuesIn(validMatFormatCases));

struct QCVimgConstructFromMatWithFormat2 : public QCVimgConstructFromMatWithFormat
{
};

MatFormatCase invalidMatFormatCases[] = {
    MatFormatCase(CV_8SC1, QImage::Format_Invalid),
    MatFormatCase(CV_16UC3, QImage::Format_Invalid),
    MatFormatCase(CV_32FC1, QImage::Format_Invalid)
};

TEST_P(QCVimgConstructFromMatWithFormat2, InvalidMatFormatReturnsEmptyImage)
{
    MatFormatCase testCase = GetParam();

    cv::Mat matImg(originalWidth, originalHeight, testCase.matFormat, originalColor);
    img = QCVimg(matImg);

    ASSERT_TRUE(img.empty());
}
INSTANTIATE_TEST_SUITE_P(QCVimg_BulkTest, QCVimgConstructFromMatWithFormat2, ValuesIn(invalidMatFormatCases));

struct QCVimgConstructFromSizeAndFormatArgs : public Test
{
    void SetUp() override {
        validFormat = QImage::Format_Grayscale8;
        invalidFormat = QImage::Format_RGB555;
    }

    uint32_t originalColor = 88;
    int originalWidth = 12, originalHeight = 14;
    QImage::Format validFormat, invalidFormat;
    QCVimg img;

};

TEST_F(QCVimgConstructFromSizeAndFormatArgs, CreatesEmptyImageOnInvalidFormat)
{
    img = QCVimg(originalWidth, originalHeight, invalidFormat);

    ASSERT_TRUE(img.empty());
}

TEST_F(QCVimgConstructFromSizeAndFormatArgs, CreatesQImageMemberAccordingToSizeAndFormatArgs)
{
    img = QCVimg(originalWidth, originalHeight, validFormat);

    ASSERT_THAT(img.qImg().width(), Eq(originalWidth));
    ASSERT_THAT(img.qImg().height(), Eq(originalHeight));
    ASSERT_THAT(img.qImg().format(), Eq(validFormat));
}

TEST_F(QCVimgConstructFromSizeAndFormatArgs, MatMemberPointsToQImageData)
{
    auto qDataPtr = img.qImg().bits();
    auto matDataPtr = img.cvMat().data;

    ASSERT_THAT(matDataPtr, Eq(qDataPtr));
}

TEST_F(QCVimgConstructFromSizeAndFormatArgs, MatMemberReturnsDataEqualToQImage)
{
    img = QCVimg(originalWidth, originalHeight, validFormat);
    img.qImg().fill(originalColor);

    int qImgPixel = qGray(img.qImg().pixel(1,1));
    uint8_t matPixel= img.cvMat().at<uint8_t>(1,1);

    ASSERT_THAT(matPixel, Eq(qImgPixel));
}

struct RgbOrderCase
{
    QRgb originalColor;
    explicit RgbOrderCase(QRgb argRgb)
        : originalColor{argRgb} {}
};

struct QCVimgRgbOrderTest : public TestWithParam<RgbOrderCase>
{
    int originalWidth = 11, originalHeight = 6;
    QCVimg img;
};

struct QCVimgRgbOrderTest1 : public QCVimgRgbOrderTest
{
    void SetUp() override {
        QImage tempImg = QImage{originalWidth, originalHeight, QImage::Format_RGB888};
        img = QCVimg(tempImg);
    }
};

RgbOrderCase rgbOrderCases[] = {
    RgbOrderCase{qRgb(50, 100, 150)},
    RgbOrderCase{qRgb(250, 200, 150)},
    RgbOrderCase{qRgb(245, 50, 175)}
};

TEST_P(QCVimgRgbOrderTest1, MatRgbOrderMatchesQImageRgbOrder)
{
    RgbOrderCase testCase = GetParam();
    img.qImg().fill(testCase.originalColor);

    cv::Vec3b matRGB = img.cvMat().at<cv::Vec3b>(1,1);
    QColor qImgColor = img.qImg().pixelColor(1,1);

    EXPECT_THAT(matRGB[0], Eq(qImgColor.red()));
    EXPECT_THAT(matRGB[1], Eq(qImgColor.green()));
    ASSERT_THAT(matRGB[2], Eq(qImgColor.blue()));
}
INSTANTIATE_TEST_SUITE_P(QCVimg_BulkTest, QCVimgRgbOrderTest1, ValuesIn(rgbOrderCases));

struct QCVimgRgbOrderTest2 : public QCVimgRgbOrderTest
{
    void SetUp() override {
        QImage tempImg = QImage{originalWidth, originalHeight, QImage::Format_ARGB32};
        img = QCVimg(tempImg);
    }
};

RgbOrderCase argbOrderCases[] = {
    RgbOrderCase{qRgba(50, 100, 150, 75)},
    RgbOrderCase{qRgba(250, 200, 150, 145)},
    RgbOrderCase{qRgba(245, 50, 175, 225)}
};

TEST_P(QCVimgRgbOrderTest2, DISABLED_MatArgbOrderMatchesQImageArgbOrder)
{
    /*
     * Unfortunately using cv::cvtColor or cv::mixChannels doesn't change the order
     * of channels for unknown reasons (maybe these functions require data allocated by Mat),
     * so for the time being the order of color in Mat images is BGRA instead of ARGB.
     */
    RgbOrderCase testCase = GetParam();
    img.qImg().setPixelColor(1,1,testCase.originalColor);

    cv::Vec4b matARGB = img.cvMat().at<cv::Vec4b>(1,1);
    QColor qImgColor = img.qImg().pixelColor(1,1);

    EXPECT_THAT(matARGB[0], Eq(qImgColor.alpha()));
    EXPECT_THAT(matARGB[1], Eq(qImgColor.red()));
    EXPECT_THAT(matARGB[2], Eq(qImgColor.green()));
    ASSERT_THAT(matARGB[3], Eq(qImgColor.blue()));
}
INSTANTIATE_TEST_SUITE_P(QCVimg_BulkTest, QCVimgRgbOrderTest2, ValuesIn(argbOrderCases));

struct QCVimgConstructFromQCVimg : public Test
{
    void SetUp() override {
        originalColor = qRgb(80, 120, 140);
        modifiedColor = qRgb(210, 100, 30);

        QImage img(10, 20, QImage::Format_RGB888);
        img.fill(originalColor);
        originalImg = QCVimg(img);
    }

    QRgb originalColor, modifiedColor;
    QCVimg originalImg, copyImg;
    QCVimg& movedToImg = copyImg;
};

TEST_F(QCVimgConstructFromQCVimg, CopyConstructingMakesDeepCopyOfData)
{
    copyImg = QCVimg(originalImg);
    originalImg.fill(modifiedColor);

    auto modifiedAfterCopyOriginalQImgPixel = originalImg.qImg().pixel(1,1);
    auto copyQImgPixel = copyImg.qImg().pixel(1,1);

    ASSERT_THAT(copyQImgPixel, Ne(modifiedAfterCopyOriginalQImgPixel));
}

TEST_F(QCVimgConstructFromQCVimg, MatMemberPointsToQImageDataAfterCopyConstructing)
{
    copyImg = QCVimg(originalImg);

    auto qDataPtr = copyImg.qImg().bits();
    auto matDataPtr = copyImg.cvMat().data;

    ASSERT_THAT(matDataPtr, Eq(qDataPtr));
}

TEST_F(QCVimgConstructFromQCVimg, CopyAssignmentMakesDeepCopyOfData)
{
    copyImg = originalImg;
    originalImg.fill(modifiedColor);

    auto modifiedAfterCopyOriginalQImgPixel = originalImg.qImg().pixel(1,1);
    auto copyQImgPixel = copyImg.qImg().pixel(1,1);

    ASSERT_THAT(copyQImgPixel, Ne(modifiedAfterCopyOriginalQImgPixel));
}

TEST_F(QCVimgConstructFromQCVimg, MatMemberPointsToQImageDataAfterCopyAssignment)
{
    copyImg = originalImg;

    auto qDataPtr = copyImg.qImg().bits();
    auto matDataPtr = copyImg.cvMat().data;

    ASSERT_THAT(matDataPtr, Eq(qDataPtr));
}

TEST_F(QCVimgConstructFromQCVimg, MoveConstructorLeavesOriginalImageEmpty)
{
    QCVimg movedToImg2(std::move(originalImg));

    ASSERT_TRUE(originalImg.empty());
}

TEST_F(QCVimgConstructFromQCVimg, MovedToImageMatMemberPointsToMoveConstructedData)
{
    auto originalImgDataPtr = originalImg.cvMat().data;
    QCVimg movedToImg2(std::move(originalImg));

    auto movedToImgDataPtr = movedToImg2.cvMat().data;

    ASSERT_THAT(movedToImgDataPtr, Eq(originalImgDataPtr));
}

TEST_F(QCVimgConstructFromQCVimg, OriginalImageMatMemberDoesNotPointToMovedToImageDataAfterMoveConstruction)
{
    QCVimg movedToImg2(std::move(originalImg));

    ASSERT_TRUE(originalImg.cvMat().data == NULL);
}

TEST_F(QCVimgConstructFromQCVimg, MoveAssignmentLeavesOriginalImageEmpty)
{
    movedToImg = std::move(originalImg);

    ASSERT_TRUE(originalImg.empty());
}

TEST_F(QCVimgConstructFromQCVimg, MovedToImageImageMatMemberPointsToMoveAssignedData)
{
    auto originalImgDataPtr = originalImg.cvMat().data;
    movedToImg = std::move(originalImg);

    auto movedToImgDataPtr = movedToImg.cvMat().data;

    ASSERT_THAT(movedToImgDataPtr, Eq(originalImgDataPtr));
}

TEST_F(QCVimgConstructFromQCVimg, OriginalImageMatMemberDoesNotPointToMovedToImageDataAfterMoveAssignment)
{
    auto originalDataPtr = originalImg.cvMat().data;
    movedToImg = std::move(originalImg);

    auto originalAfterMoveDataPtr = originalImg.cvMat().data;

    ASSERT_THAT(originalAfterMoveDataPtr, Ne(originalDataPtr));
}

struct QCVimgRebindMembers : public Test
{
    void SetUp() override {
        qCompatImg = QImage(originalWidth, originalHeight, compatQImgFormat);
        qCompatImg.fill(fillColorCompat);

        qIncompatImg = QImage(originalWidth, originalHeight, incompatQImgFormat);
        qIncompatImg.fill(fillColorIncompat);

        matCompatImg = cv::Mat(originalHeight, originalWidth, compatMatFormat, fillColorCompat);
        matIncompatImg = cv::Mat(originalHeight, originalWidth, incompatMatFormat, fillColorIncompat);

        img = QCVimg(originalWidth, originalHeight, compatQImgFormat);
        img.fill(fillColorOrig);
    }

    QImage qCompatImg, qIncompatImg;
    cv::Mat matCompatImg, matIncompatImg;
    QCVimg img;
    QImage::Format compatQImgFormat = QImage::Format_RGB888;
    QImage::Format incompatQImgFormat = QImage::Format_RGB555;
    int compatMatFormat = CV_8UC1, incompatMatFormat = CV_16SC2;
    int originalWidth = 4, originalHeight = 6;
    uint8_t fillColorOrig = 76, fillColorIncompat = 29, fillColorCompat = 122;
};

struct QCVimgRebindMat : public QCVimgRebindMembers
{
};

TEST_F(QCVimgRebindMat, RebindsMatOnCompatibleQImageAndReturnsNormally)
{
    img.cvMat() = matCompatImg;

    int retVal = img.rebindMat();

    ASSERT_THAT(retVal, Eq(0));
    ASSERT_TRUE(img.isMatBound());
}

TEST_F(QCVimgRebindMat, DoesNotReturnNormallyOnIncompatibleQImageFormat)
{
    img.qImg() = qIncompatImg;

    int retVal = img.rebindMat(DataPrio::Hi);

    ASSERT_THAT(retVal, Eq(-1));
}

TEST_F(QCVimgRebindMat, MatIsNotBoundIfCalledOnIncompatibleQImageFormatAndHiDataPrio)
{
    img.qImg() = qIncompatImg;

    img.rebindMat(DataPrio::Hi);

    ASSERT_FALSE(img.isMatBound());
}

TEST_F(QCVimgRebindMat, SetsMatEmptyOnIncompatibleQImageFormatAndHiDataPrio)
{
    img.qImg() = qIncompatImg;

    img.rebindMat(DataPrio::Hi);

    ASSERT_TRUE(img.cvMat().empty());
}

TEST_F(QCVimgRebindMat, LeavesQImageMemberIntactOnIncompatibleQImageFormatAndHiDataPrio)
{
    img.qImg() = qIncompatImg;
    auto dataBeforeRebind = img.qImg().constScanLine(0);

    img.rebindMat(DataPrio::Hi);
    auto dataAfterRebind = img.qImg().constScanLine(0);

    EXPECT_THAT(*dataAfterRebind, Eq(*dataBeforeRebind));
}

TEST_F(QCVimgRebindMat, SetsMatAndQImageMemberEmptyOnIncompatibleQImageFormatAndLowDataPrio)
{
    img.qImg() = qIncompatImg;

    img.rebindMat(DataPrio::Low);

    ASSERT_TRUE(img.empty());
}

/*
 * In the following few tests rebinding of QImage means copying data over from Mat and then binding
 * the Mat to the new QImage.
 */
struct QCVimgRebindQImage : public QCVimgRebindMembers
{
};

TEST_F(QCVimgRebindQImage, CopiesDataFromCompatibleMatMemberToQImageMemberAndReturnsNormally)
{
    img.cvMat() = matCompatImg;

    int retVal = img.rebindQImg();
    int qImgPixelAfterCopy = qGray(img.pixelColor(1,1).rgb());

    ASSERT_THAT(retVal, Eq(0));
    ASSERT_THAT(qImgPixelAfterCopy, fillColorCompat);
}

TEST_F(QCVimgRebindQImage, SetsRGBColorOrderOfImageIfMatHadBGRColorOrder)
{
    cv::Vec3b bgrColor(20, 50, 110);
    cv::Vec3b rgbColor(110, 50, 20);
    img.cvMat() = cv::Mat(originalHeight, originalWidth, CV_8UC3, bgrColor);

    img.rebindQImg(DataPrio::Low, MatColorOrder::BGR);

    cv::Vec3b colorAfterRebind = img.cvMat().at<cv::Vec3b>(0,0);

    ASSERT_THAT(colorAfterRebind, Eq(rgbColor));
}

TEST_F(QCVimgRebindQImage, DoesNotReturnNormallyOnIncompatibleQImageFormat)
{
    img.cvMat() = matIncompatImg;

    int retVal = img.rebindQImg(DataPrio::Hi);

    ASSERT_THAT(retVal, Eq(-1));
}

TEST_F(QCVimgRebindQImage, MatIsNotBoundIfCalledOnIncompatibleQImageFormatAndHiDataPrio)
{
    img.cvMat() = matIncompatImg;

    img.rebindQImg(DataPrio::Hi);

    ASSERT_FALSE(img.isMatBound());
}

TEST_F(QCVimgRebindQImage, SetsQImgEmptyOnIncompatibleMatFormatAndHiDataPrio)
{
    img.cvMat() = matIncompatImg;

    img.rebindQImg(DataPrio::Hi);

    ASSERT_TRUE(img.qImg().isNull());
}

TEST_F(QCVimgRebindQImage, LeavesMatMemberIntactOnIncompatibleMatFormatAndHiDataPrio)
{
    img.cvMat() = matIncompatImg;
    auto dataBeforeRebind = img.cvMat().at<uint32_t>(0,0);

    img.rebindQImg(DataPrio::Hi);
    auto dataAfterRebind = img.cvMat().at<uint32_t>(0,0);

    EXPECT_THAT(dataAfterRebind, Eq(dataBeforeRebind));
}

TEST_F(QCVimgRebindQImage, SetsMatAndQImageMemberEmptyOnIncompatibleMatFormatAndLowDataPrio)
{
    img.cvMat() = matIncompatImg;

    img.rebindQImg(DataPrio::Low);

    ASSERT_TRUE(img.empty());
}

struct QCVimgCompare : public Test
{
    void SetUp() override
    {
        copyImg = img = QCVimg(originalWidth, originalHeight, originalFormat);
        otherImg = QCVimg(otherWidth, otherHeight, otherFormat);
    }

    QCVimg img;
    QCVimg copyImg;
    QCVimg otherImg;
    int originalWidth = 10, originalHeight = 8;
    int otherWidth = 4, otherHeight = 9;
    QImage::Format originalFormat = QImage::Format_Grayscale8;
    QImage::Format otherFormat = QImage::Format_RGB888;
};

TEST_F(QCVimgCompare, QCVimgsNotEqualIfQImageMembersNotEqual)
{
    bool qcvimgCompare = img == otherImg;
    bool qimgMemberCompare = img.qImg() == otherImg.qImg();

    ASSERT_THAT(qcvimgCompare, Eq(qimgMemberCompare));
}

TEST_F(QCVimgCompare, QCVimgsNotEqualIfMatMembersAreNotBound)
{
    copyImg.cvMat() = cv::Mat();

    bool qcvimgCompare = img == copyImg;
    bool matBoundCompare = img.isMatBound() == copyImg.isMatBound();

    ASSERT_THAT(qcvimgCompare, Eq(matBoundCompare));
}

struct QCVimgResizeImage : public Test
{
    void SetUp() override {
        QImage qOrigImg = QImage(originalWidth, originalHeight, originalFormat);
        qOrigImg.fill(originalFillColor);
        img = QCVimg(qOrigImg);
    }

    QCVimg img;
    int originalWidth = 15, originalHeight = 10;
    int resizedWidth = 30, resizedHeight = 40;
    uint32_t originalFillColor = 150;
    QImage::Format originalFormat = QImage::Format_Grayscale8;
};

TEST_F(QCVimgResizeImage, ResizedQImageMemberReturnsNewSize) {
    QCVimg resizedImg = img.resize(resizedWidth, resizedHeight);

    ASSERT_THAT(resizedImg.qImg().width(), resizedWidth);
    ASSERT_THAT(resizedImg.qImg().height(), resizedHeight);
}

TEST_F(QCVimgResizeImage, ResizedMatMemberReturnsNewSize)
{
    QCVimg resizedImg = img.resize(resizedWidth, resizedHeight);

    ASSERT_THAT(resizedImg.cvMat().cols, Eq(resizedWidth));
    ASSERT_THAT(resizedImg.cvMat().rows, Eq(resizedHeight));
}

struct QCVimgSwap : public Test
{
    QCVimg firstImg, secondImg;
    QColor firstFillColor, secondFillColor;
};

struct QCVimgSwap1 : public QCVimgSwap
{
    void SetUp() override {
        firstFillColor = qRgb(30, 50, 70);
        secondFillColor = qRgb(230, 210, 190);

        QImage img(10, 15, QImage::Format_RGB888);

        img.fill(firstFillColor);
        firstImg = QCVimg(img);

        img.fill(secondFillColor);
        secondImg = QCVimg(img);
    }
};

TEST_F(QCVimgSwap1, ImageDataIsSwappedForEquallyFormattedImages)
{
    auto firstImgDataBeforeSwap = firstImg.qImg().bits();
    auto secondImgDataBeforeSwap = secondImg.qImg().bits();

    firstImg.swap(secondImg);

    auto firstImgDataAfterSwap = firstImg.qImg().bits();
    auto secondImgDataAfterSwap = secondImg.qImg().bits();

    ASSERT_THAT(secondImgDataAfterSwap, firstImgDataBeforeSwap);
    ASSERT_THAT(firstImgDataAfterSwap, secondImgDataBeforeSwap);
}

TEST_F(QCVimgSwap1, MatMembersPointToSwappedDataForEquallyFormattedImages)
{
    firstImg.swap(secondImg);

    ASSERT_TRUE(firstImg.isMatBound());
    ASSERT_TRUE(secondImg.isMatBound());
}

struct QCVimgSwap2 : public QCVimgSwap
{
    void SetUp() override {
        firstFillColor = qRgb(30, 50, 70);
        secondFillColor = QColor(Qt::lightGray);

        QImage img1(10, 15, QImage::Format_RGB888);
        img1.fill(firstFillColor);
        firstImg = QCVimg(img1);

        QImage img2(10, 15, QImage::Format_Grayscale8);
        img2.fill(secondFillColor);
        secondImg = QCVimg(img2);
    }
};

TEST_F(QCVimgSwap2, ImageDataIsSwappedForDifferentlyFormattedImages)
{
    auto firstImgDataBeforeSwap = firstImg.qImg().bits();
    auto secondImgDataBeforeSwap = secondImg.qImg().bits();

    firstImg.swap(secondImg);

    auto firstImgDataAfterSwap = firstImg.qImg().bits();
    auto secondImgDataAfterSwap = secondImg.qImg().bits();

    ASSERT_THAT(secondImgDataAfterSwap, firstImgDataBeforeSwap);
    ASSERT_THAT(firstImgDataAfterSwap, secondImgDataBeforeSwap);
}

TEST_F(QCVimgSwap2, MatMembersPointToSwappedDataForDifferentlyFormattedImages)
{
    firstImg.swap(secondImg);

    ASSERT_TRUE(firstImg.isMatBound());
    ASSERT_TRUE(secondImg.isMatBound());
}

TEST_F(QCVimgSwap2, SwappedInMatMemberFormatMatchesSwappedInDataFormat)
{
    firstImg.swap(secondImg);

    uint8_t firstImgColorAfterSwap = firstImg.cvMat().at<uint8_t>(1,1);
    cv::Vec3b secondImgColorAfterSwap = secondImg.cvMat().at<cv::Vec3b>(1,1);

    ASSERT_THAT(firstImgColorAfterSwap, Eq(qGray(secondFillColor.rgb())));

    EXPECT_THAT(secondImgColorAfterSwap[0], Eq(firstFillColor.red()));
    EXPECT_THAT(secondImgColorAfterSwap[1], Eq(firstFillColor.green()));
    ASSERT_THAT(secondImgColorAfterSwap[2], Eq(firstFillColor.blue()));
}

struct QCVimgChangeFormatCase
{
    QCVimgChangeFormatCase(QImage::Format argOrigFormat, QImage::Format argExpectFormat)
        : originalFormat(argOrigFormat), expectedFormat(argExpectFormat) {}

    QImage::Format originalFormat, expectedFormat;
};

struct QCVimgChangeFormat : public TestWithParam<QCVimgChangeFormatCase>
{
    QImage qImg;
    QCVimg origImg, convertedImg;
    int originalWidth = 10, originalHeight = 15;
};

struct QCVimgChangeFormat1 : public QCVimgChangeFormat
{
};

QCVimgChangeFormatCase validFormatConversions[] = {
    QCVimgChangeFormatCase(QImage::Format_RGB888, QImage::Format_ARGB32),
    QCVimgChangeFormatCase(QImage::Format_RGB888, QImage::Format_Grayscale8),
    QCVimgChangeFormatCase(QImage::Format_Grayscale8, QImage::Format_RGB888),
    QCVimgChangeFormatCase(QImage::Format_Grayscale8, QImage::Format_ARGB32)
};

TEST_P(QCVimgChangeFormat1, ImageConvertsToCompatibleFormat)
{
    QCVimgChangeFormatCase testCase = GetParam();

    qImg = QImage(originalWidth, originalHeight, testCase.originalFormat);
    origImg = QCVimg(qImg);

    convertedImg = origImg.convertToFormat(testCase.expectedFormat);

    ASSERT_THAT(convertedImg.qFormat(), Eq(testCase.expectedFormat));
}
INSTANTIATE_TEST_SUITE_P(QCVimg_BulkTest, QCVimgChangeFormat1, ValuesIn(validFormatConversions));

struct QCVimgChangeFormat2 : public QCVimgChangeFormat
{
};

QCVimgChangeFormatCase invalidFormatConversions[] = {
    QCVimgChangeFormatCase(QImage::Format_RGB888, QImage::Format_Mono),
    QCVimgChangeFormatCase(QImage::Format_RGB888, QImage::Format_RGB555),
    QCVimgChangeFormatCase(QImage::Format_Grayscale8, QImage::Format_Indexed8),
    QCVimgChangeFormatCase(QImage::Format_Grayscale8, QImage::Format_MonoLSB)
};

TEST_P(QCVimgChangeFormat2, ReturnsEmptyImageOnIncompatibleFormat)
{
    QCVimgChangeFormatCase testCase = GetParam();

    qImg = QImage(originalWidth, originalHeight, testCase.originalFormat);
    origImg = QCVimg(qImg);

    convertedImg = origImg.convertToFormat(testCase.expectedFormat);

    ASSERT_TRUE(convertedImg.empty());
}
INSTANTIATE_TEST_SUITE_P(QCVimg_BulkTest, QCVimgChangeFormat2, ValuesIn(invalidFormatConversions));

struct QCVimgCopyFromImg : public Test
{
    void SetUp() override {
        img = QCVimg(originalWidth, originalHeight, originalFormat);
        img.fill(originalColor);
    }

    QCVimg img;
    QImage sourceQimg;
    cv::Mat sourceMat;

    int originalWidth = 6, originalHeight = 14;
    int sourceImgWidth = 9, sourceImgHeight = 12;
    QImage::Format originalFormat = QImage::Format_Grayscale8;
    QImage::Format incompatibleFormat = QImage::Format_Mono;
    QColor originalColor = qRgb(30, 70, 100), sourceImgColor = qRgb(240, 210, 190);
};

struct QCVimgCopyFromMat : public QCVimgCopyFromImg
{
};

TEST_F(QCVimgCopyFromMat, SourceImageWithEqualSizeOverwritesOriginalData)
{
    int imgColorOriginal = qGray(sourceImgColor.rgb());
    sourceMat = cv::Mat(originalHeight, originalWidth, CV_8UC1, imgColorOriginal);

    img.copy(sourceMat);
    int imgColorAfterCopy = qGray(img.pixelColor(1,1).rgb());

    ASSERT_THAT(imgColorAfterCopy, Eq(imgColorOriginal));
}

TEST_F(QCVimgCopyFromMat, MatMemberPointsToQImageDataAfterCopy)
{
    img.copy(sourceMat);

    auto qDataPtr = img.qImg().bits();
    auto matDataPtr = img.cvMat().data;

    ASSERT_THAT(matDataPtr, Eq(qDataPtr));
}

TEST_F(QCVimgCopyFromMat, SourceImageWithDifferentSizeReallocatesQImageMember)
{
    sourceMat = cv::Mat(sourceImgHeight, sourceImgWidth, CV_8UC1, qGray(sourceImgColor.rgb()));
    auto imageDataSizeBeforeCopy = img.qImg().sizeInBytes();

    int returnCode = img.copy(sourceMat);

    auto imageDataSizeAfterCopy = img.qImg().sizeInBytes();

    EXPECT_THAT(returnCode, Eq(0));
    ASSERT_THAT(imageDataSizeAfterCopy, Ne(imageDataSizeBeforeCopy));
}

TEST_F(QCVimgCopyFromMat, SourceImageWithDifferentSizeOverwritesOriginalData)
{
    int imgColorOriginal = qGray(sourceImgColor.rgb());
    sourceMat = cv::Mat(sourceImgHeight, sourceImgWidth, CV_8UC1, imgColorOriginal);
    img.copy(sourceMat);

    int imgColorAfterCopy = qGray(img.pixelColor(1,1).rgb());

    ASSERT_THAT(imgColorAfterCopy, Eq(imgColorOriginal));
}

TEST_F(QCVimgCopyFromMat, SourceImageWithDifferentFormatOverwritesOriginalData)
{
    cv::Scalar sourceImgColorScalar = QCVimg::convertQColorToScalar(sourceImgColor, MatColorOrder::RGB);
    sourceMat = cv::Mat(originalHeight, originalWidth, CV_8UC3, sourceImgColorScalar);

    int returnCode = img.copy(sourceMat);
    QColor imgPixelColorAfterCopy = img.pixelColor(1,1);

    EXPECT_THAT(returnCode, Eq(0));
    ASSERT_THAT(imgPixelColorAfterCopy.rgb(), Eq(sourceImgColor.rgb()));
}

TEST_F(QCVimgCopyFromMat, SourceImageWithBGRColorOrderConvertsToRGBDuringCopy)
{
    cv::Scalar sourceImgColorScalar = QCVimg::convertQColorToScalar(sourceImgColor, MatColorOrder::BGR);
    sourceMat = cv::Mat(originalHeight, originalWidth, CV_8UC3, sourceImgColorScalar);

    int returnCode = img.copy(sourceMat, MatColorOrder::BGR);
    QColor imgPixelColorAfterCopy = img.pixelColor(1,1);

    EXPECT_THAT(returnCode, Eq(0));
    ASSERT_THAT(imgPixelColorAfterCopy.rgb(), Eq(sourceImgColor.rgb()));
};

struct QCVimgCopyFromQImage : public QCVimgCopyFromImg
{
};

TEST_F(QCVimgCopyFromQImage, SourceImageWithAnySizeOverwritesOriginalData)
{
    sourceQimg = QImage(sourceImgWidth, sourceImgHeight, QImage::Format_RGB888);
    sourceQimg.fill(sourceImgColor);
    int returnCode = img.copy(sourceQimg);

    QColor imgColorAfterCopy = img.pixelColor(1,1);

    EXPECT_THAT(returnCode, Eq(0));
    ASSERT_THAT(imgColorAfterCopy.rgb(), Eq(sourceImgColor.rgb()));
}

TEST_F(QCVimgCopyFromQImage, MatMemberPointsToCopiedData)
{
    sourceQimg = QImage(sourceImgWidth, sourceImgHeight, originalFormat);

    img.copy(sourceQimg);

    auto qDataPtr = img.qImg().bits();
    auto matDataPtr = img.cvMat().data;

    ASSERT_THAT(matDataPtr, Eq(qDataPtr));
}

TEST_F(QCVimgCopyFromQImage, SourceImageWithIncompatibleFormatLeavesOriginalImageUnchanged)
{
    sourceQimg = QImage(sourceImgWidth, sourceImgHeight, incompatibleFormat);

    int returnCode = img.copy(sourceQimg);

    auto imgFormatAfterCopy = img.qFormat();

    EXPECT_THAT(returnCode, Ne(0));
    ASSERT_THAT(imgFormatAfterCopy, Eq(originalFormat));
}

struct QCVimgSerialization : public TestWithParam<QCVimg>
{
    void SetUp() override {
        dataStream.setDevice(&buffer);
    }

    QBuffer buffer;
    QDataStream dataStream;
};

QCVimg imageFormatCases[] = {
    {8, 4, QImage::Format_RGB888},
    {8, 4, QImage::Format_RGB32},
    {8, 4, QImage::Format_ARGB32},
    {8, 4, QImage::Format_Grayscale8},
    {8, 4, QImage::Format_Alpha8}
};

TEST_P(QCVimgSerialization, OriginalImageDataIsUnchangedAfterSerialization)
{
    QCVimg sourceImgCase = GetParam();
    sourceImgCase.fill(Qt::gray);
    QCVimg destImg;

    buffer.open(QIODevice::WriteOnly);
    dataStream << sourceImgCase;
    buffer.close();

    buffer.open(QIODevice::ReadOnly);
    dataStream >> destImg;

    ASSERT_THAT(destImg, Eq(sourceImgCase));
}
INSTANTIATE_TEST_SUITE_P(QCVimg_BulkTest, QCVimgSerialization, ValuesIn(imageFormatCases));
