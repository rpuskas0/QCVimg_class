#include "qcvimg.h"
#include <opencv2/imgproc.hpp>


const QMap<QImage::Format, MatFormat> QCVimg::scmQtToCvFormatMap = fillWithCompatibleQtToCvFormats();
const QMap<MatFormat, QImage::Format> QCVimg::scmCvToQtFormatMap = fillWithCompatibleCvToQtFormats();
const QMap<QString, QImage::Format> QCVimg::scmTextToQImgFormat = fillQImgFormatToStringMap();


QCVimg::QCVimg(const QCVimg& img, QObject* parent)
    : QObject(parent), mQImg(img.mQImg.copy())
{
    createMatFromQImage(mQImg, mMImg);
}

QCVimg& QCVimg::operator=(const QCVimg& img)
{
    setParent(img.parent());
    copyFrom(img.qImg());

    return *this;
}

QCVimg::QCVimg(QCVimg&& img) noexcept
    : mQImg(std::move(img.mQImg)), mMImg(img.mMImg)
{
    setParent(img.parent());
    img.mMImg = cv::Mat();
}

QCVimg& QCVimg::operator=(QCVimg&& img)
{
    setParent(img.parent());
    mQImg = std::move(img.mQImg);
    mMImg = img.mMImg;
    img.mMImg = cv::Mat();

    return *this;
}

QCVimg::QCVimg(int width, int height, QImage::Format format)
{
    if (isValidQImgFormat(format)) {
        mQImg = QImage(width, height, format);
        createMatFromQImage(mQImg, mMImg);
    }
}

QCVimg::QCVimg(const QImage& img)
{
    if (isValidQImgFormat(img.format())) {
        copyFrom(img);
    }
}

QCVimg::QCVimg(QImage&& img)
{
    if (isValidQImgFormat(img.format())) {
        mQImg = std::move(img);
        createMatFromQImage(mQImg, mMImg);
    }
}

QCVimg::QCVimg(const cv::Mat& img, MatColorOrder sourceColorOrder)
{
    if (isValidMatFormat(img.type())) {
        auto qImgFormat = convertMatFormatTag(img.type());
        cv::Mat rgbMat;
        getRgbMat(img, rgbMat, sourceColorOrder);
        copyFrom(rgbMat, qImgFormat);
    }
}

qsizetype QCVimg::bytes() const
{
    return mQImg.sizeInBytes();
}

QCVimg QCVimg::convertToFormat(QImage::Format format) const
{
    if (isValidQImgFormat(format)) {
        return QCVimg(mQImg.convertToFormat(format));
    } else {
        return QCVimg();
    }
}

int QCVimg::copy(const QImage& sourceQImg)
{
    if (isValidQImgFormat(sourceQImg.format())) {
        copyFrom(sourceQImg);
        return 0;
    } else {
        return -1;
    }
}

int QCVimg::copy(const cv::Mat& sourceMat, MatColorOrder sourceColorOrder)
{
    auto qImgFormat = convertMatFormatTag(sourceMat.type());

    cv::Mat rgbMat;
    getRgbMat(sourceMat, rgbMat, sourceColorOrder);

    if (sizesMatch(rgbMat, mMImg) && typesMatch(rgbMat, mMImg)) {
        rgbMat.copyTo(mMImg);
        return 0;
    } else if (qImgFormat != QImage::Format_Invalid) {
        copyFrom(rgbMat, qImgFormat);
        return 0;
    } else {
        return -1;
    }

}

void QCVimg::copyTo(cv::OutputArray dest) const
{
    mMImg.copyTo(dest);
}

void QCVimg::copyTo(QImage& dest) const
{
    dest = mQImg.copy();
}

cv::Mat& QCVimg::cvMat()
{
    return mMImg;
}

const cv::Mat& QCVimg::cvMat() const
{
    return mMImg;
}

bool QCVimg::empty() const
{
    return mQImg.isNull() && matIsNull();
}

void QCVimg::fill(uint pixelValue)
{
    mQImg.fill(pixelValue);
}

void QCVimg::fill(const QColor& color)
{
    mQImg.fill(color);
}

void QCVimg::fill(const Qt::GlobalColor color)
{
    mQImg.fill(color);
}

int QCVimg::height() const
{
    return mQImg.height();
}

bool QCVimg::isMatBound() const
{
     return pointersMatch() && sizesMatch() && formatsMatch();
}

int QCVimg::matFormat() const
{
    return mMImg.type();
}

QImage::Format QCVimg::qFormat() const
{
    return mQImg.format();
}

bool QCVimg::operator==(const QCVimg& other) const
{
    return mQImg == other.mQImg && isMatBound() == other.isMatBound();
}

bool QCVimg::operator!=(const QCVimg& other) const
{
    return !(*this == other);
}

QImage& QCVimg::qImg()
{
    return mQImg;
}

const QImage& QCVimg::qImg() const
{
    return mQImg;
}

QPixmap QCVimg::qPix(Qt::ImageConversionFlags flags) const
{
    return QPixmap::fromImage(mQImg, flags);
}

QColor QCVimg::pixelColor(int x, int y) const
{
    return mQImg.pixelColor(x, y);
}

int QCVimg::rebindMat(DataPrio priority)
{
    bool qImgFormatValid = isValidQImgFormat(mQImg.format());

    if (!qImgFormatValid && priority == DataPrio::Low) {
        setMembersEmpty();
        return -1;
    } else if (!qImgFormatValid && priority == DataPrio::Hi) {
        mMImg = cv::Mat();
        return -1;
    } else {
        createMatFromQImage(mQImg, mMImg);
        return 0;
    }
}

int QCVimg::rebindQImg(DataPrio priority, MatColorOrder matColorOrder)
{
    QImage::Format qImgFormat = convertMatFormatTag(mMImg.type());
    bool matFormatValid = !(qImgFormat == QImage::Format_Invalid);
    cv::Mat rgbMat = mMImg;

    if (!matFormatValid && priority == DataPrio::Low) {
        setMembersEmpty();
        return -1;
    } else if (!matFormatValid && priority == DataPrio::Hi) {
        mQImg = QImage();
        return -1;
    } else {
        getRgbMat(mMImg, rgbMat, matColorOrder);
        copyFrom(rgbMat, qImgFormat);
        return 0;
    }
}

QCVimg QCVimg::resize(int width, int height, Qt::AspectRatioMode aspectRatioMode, Qt::TransformationMode transformMode) const
{
    return QCVimg(mQImg.scaled(width, height, aspectRatioMode, transformMode));
}

void QCVimg::swap(QCVimg &other)
{
    mQImg.swap(other.mQImg);
    cv::swap(mMImg, other.mMImg);
}

bool QCVimg::valid(int x, int y) const
{
    return mQImg.valid(x, y);
}

int QCVimg::width() const
{
    return mQImg.width();
}

int QCVimg::convertQImgFormatTag(QImage::Format qFormat)
{
    auto it = scmQtToCvFormatMap.find(qFormat);

    return it == scmQtToCvFormatMap.end() ? -1 : it.value();
}

QImage::Format QCVimg::convertMatFormatTag(int matFormat)
{
    auto it = scmCvToQtFormatMap.find(matFormat);

    return it == scmCvToQtFormatMap.end() ? QImage::Format_Invalid : it.value();
}

bool QCVimg::isValidQImgFormat(QImage::Format qFormat)
{
    return !(convertQImgFormatTag(qFormat) == -1);
}

bool QCVimg::isValidMatFormat(int matFormat)
{
    return !(convertMatFormatTag(matFormat) == QImage::Format_Invalid);
}

int QCVimg::swapMatRedBlue(const cv::Mat& sourceMat, cv::Mat& destMat, MatColorOrder sourceMatColorOrder)
{
    if (sourceMat.type() != CV_8UC3) {
        return -1;
    } else {
        if (sourceMatColorOrder == MatColorOrder::BGR) {
            cv::cvtColor(sourceMat, destMat, cv::COLOR_BGR2RGB);
        } else if (sourceMatColorOrder == MatColorOrder::RGB) {
            cv::cvtColor(sourceMat, destMat, cv::COLOR_RGB2BGR);
        }

        return 0;
    }
}

cv::Scalar QCVimg::convertQColorToScalar(const QColor& color, MatColorOrder destScalarColorOrder)
{
    int red = color.red();
    int green = color.green();
    int blue = color.blue();

    if (destScalarColorOrder == MatColorOrder::RGB) {
        return cv::Scalar_<int>(red, green, blue);
    } else {
        return cv::Scalar_<int>(blue, green, red);
    }
}

QStringList QCVimg::supportedQImgFormats()
{
    return scmTextToQImgFormat.uniqueKeys();
}

QImage::Format QCVimg::convertFormatTextToQImgFormat(const QString &formatText)
{
    auto it = scmTextToQImgFormat.find(formatText);

    if (it == scmTextToQImgFormat.end()) {
        return QImage::Format_Invalid;
    } else {
        return *it;
    }
}

QMap<QImage::Format, MatFormat> QCVimg::fillWithCompatibleQtToCvFormats()
{
    return
    {
        {QImage::Format_RGB32, CV_8UC4},
        {QImage::Format_ARGB32, CV_8UC4},
        {QImage::Format_RGB888, CV_8UC3},
        {QImage::Format_Alpha8, CV_8UC1},
        {QImage::Format_Grayscale8, CV_8UC1},
        {QImage::Format_Grayscale16, CV_16UC1}
    };
}

QMap<MatFormat, QImage::Format> QCVimg::fillWithCompatibleCvToQtFormats()
{
    return
    {
        {CV_8UC1, QImage::Format_Grayscale8},
        {CV_8UC3, QImage::Format_RGB888},
        {CV_8UC4, QImage::Format_ARGB32},
        {CV_16UC1, QImage::Format_Grayscale16}
    };
}

QMap<QString, QImage::Format> QCVimg::fillQImgFormatToStringMap()
{
    return
    {
        {"Alpha 8 bit", QImage::Format_Alpha8},
        {"ARGB 32 bit", QImage::Format_ARGB32},
        {"Grayscale 8 bit", QImage::Format_Grayscale8},
        {"Grayscale 16 bit", QImage::Format_Grayscale16},
        {"RGB 32 bit", QImage::Format_RGB32},
        {"RGB 24 bit", QImage::Format_RGB888}
    };
}

void QCVimg::createMatFromQImage(QImage& sourceQImg, cv::Mat& targetMat) const
{
    targetMat = cv::Mat(sourceQImg.height(),
                sourceQImg.width(),
                *scmQtToCvFormatMap.find(sourceQImg.format()),
                sourceQImg.bits(),
                static_cast<unsigned long>(sourceQImg.bytesPerLine()));
}

void QCVimg::createQImageFromMat(const cv::Mat& sourceMat, QImage& targetQImg, QImage::Format qFormat) const
{
    targetQImg = QImage(sourceMat.cols, sourceMat.rows, qFormat);
}

void QCVimg::copyFrom(const cv::Mat& sourceMat, QImage::Format qFormat)
{
    createQImageFromMat(sourceMat, mQImg, qFormat);
    createMatFromQImage(mQImg, mMImg);
    sourceMat.copyTo(mMImg);
}

void QCVimg::copyFrom(const QImage& sourceQImg)
{
    mQImg = sourceQImg.copy();
    createMatFromQImage(mQImg, mMImg);
}

void QCVimg::getRgbMat(const cv::Mat& sourceMat, cv::Mat& rgbMat, MatColorOrder sourceColorOrder) const
{
    if (sourceColorOrder == MatColorOrder::BGR && sourceMat.type() == CV_8UC3) {
        cv::cvtColor(sourceMat, rgbMat, cv::COLOR_BGR2RGB);
    } else {
        rgbMat = sourceMat;
    }
}

void QCVimg::setMembersEmpty()
{
    mMImg = cv::Mat();
    mQImg = QImage();
}

bool QCVimg::pointersMatch() const
{
    if ( (mQImg.bits() == mMImg.data) ||
         (mQImg.bits() == nullptr && (mMImg.data == NULL || mMImg.data == nullptr) ) )
        // OpenCV still uses NULL, so it needs to be checked
    {
        return true;
    } else {
        return false;
    }
}

bool QCVimg::sizesMatch() const
{
    return (mQImg.height() == mMImg.rows) && (mQImg.width() == mMImg.cols);
}

bool QCVimg::sizesMatch(const cv::Mat& first, const cv::Mat& second) const
{
    return first.cols == second.cols && first.rows == second.rows;
}

bool QCVimg::formatsMatch() const
{
    int matFrmt = convertQImgFormatTag(mQImg.format());

    return matFrmt == mMImg.type();
}

bool QCVimg::typesMatch(const cv::Mat &first, const cv::Mat &second) const
{
    return first.type() == second.type();
}

bool QCVimg::matIsNull() const
{
    return (mMImg.cols == 0 && mMImg.rows == 0) &&
           (mMImg.data == NULL || mMImg.data == nullptr); // OpenCV still uses NULL, so it needs to be checked
}

QDataStream& operator<<(QDataStream& ds, const QCVimg& img)
{
    return ds << img.qImg().format()
              << img.mMImg.rows
              << img.mMImg.cols
              << img.mMImg.type()
              << img.mQImg;
}

QDataStream& operator>>(QDataStream& ds, QCVimg& img)
{
    int origQImgFormat, matRows, matCols, matType;

    ds >> origQImgFormat
       >> matRows
       >> matCols
       >> matType
       >> img.mQImg;

    /*
     * The QImage reconstructed from the stream data doesn't always get the original format (Qt 5.13.2),
     * so we need to convert back to original format in such cases.
     */
    if (img.qImg().format() != QImage::Format(origQImgFormat)) {
        img.qImg().convertTo(QImage::Format(origQImgFormat));
    }

    img.mMImg = cv::Mat(matRows,
                        matCols,
                        matType,
                        img.mQImg.bits(),
                        static_cast<unsigned long>(img.mQImg.bytesPerLine()));

    return ds;
}
