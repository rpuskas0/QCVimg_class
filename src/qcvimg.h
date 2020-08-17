#ifndef QCVIMG_H
#define QCVIMG_H

#include "qcvimglib_decl.h"

#include <QImage>
#include <QMap>
#include <QPixmap>
#include <opencv4/opencv2/core/mat.hpp>

using MatFormat = int;

/**
 * @brief Allows to tell functions what color order the cv::Mat argument has.
 * @see QCVimg
 * @see copyFrom
 * @see swapMatRedBlue
 * @see convertQColorToScalar
 */
enum class MatColorOrder : uint8_t {RGB, BGR};

enum class DataPrio : bool {Low=true, Hi=false};

/**
 * @brief A convenience wrapper class to allow simultaneous work with QImage
 * and cv::Mat on the same data
 *
 * The QCVimg class provides a convenient way to work with both QImage and cv::Mat
 * classes simultaneously. The image data is shared between both classes, but
 * allocation (and memory management in general) is always done by QImage, thus
 * cv::Mat serves only as a different type of interface to the same data. Because
 * QImage uses the copy-on-write (COW) technique, an explicit and immediate copy
 * of the image data is performed when creating a QCVimg instance from an lvalue
 * to prevent unexpected behavior while working with cv::Mat (e.g. pointing to
 * the wrong data).
 *
 * At this time, references to the underlying full-featured QImage and cv::Mat
 * classes are also provided, but using any of their built-in functionality
 * might easily cause unexpected or undefined behavior bacause of possible
 * reallocations or change in the data representation. Only QCVimg provided
 * functions guarantee synchronization between the two image classes. Also,
 * users should be very careful while using library functions as well, as they
 * might equally cause implicit reallocations (especially true for OpenCV
 * functions) and all the problems with them.
 *
 * Although QCVimg doesn't employ COW itself, using the QImage member through
 * the reference will. Users should also remember that cv::Mat uses shallow
 * copy, so special care must be taken if moving QCVimg after creating a cv::Mat
 * copy from the underlying cv::Mat member of QCVimg,
 *
 * Because QImage and cv::Mat are vastly different, only a small fraction of
 * common functionality is possible, which is especially true for the image
 * formats. As of now, only a basic (most commonly used) set of formats are
 * supported.
 *
 * _Supported Qt to OpenCV format conversions:_
 * QImage format                       | OpenCV format
 * ------------------------------------|--------------
 * QImage::Format_RGB32                | CV_8UC4
 * QImage::Format_ARGB32               | CV_8UC4  _See note below!_
 * QImage::Format_RGB888               | CV_8UC3
 * QImage::Format_Alpha8               | CV_8UC1
 * QImage::Format_Grayscale8           | CV_8UC1
 * QImage::Format_Grayscale16          | CV_16UC1
 *
 * _Supported OpenCV to Qt format conversions:_
 * OpenCV format | QImage format
 * --------------|--------------
 * CV_8UC1       | QImage::Format_Grayscale8
 * CV_8UC3       | QImage::Format_RGB888
 * CV_8UC4       | QImage::Format_ARGB32
 * CV_16UC1      | QImage::Format_Grayscale16
 *
 * _IMPORTANT:_ In case of any 32 bit format (CV_8UC4) there is a mismatch
 * between QImage and cv::Mat color order. cv::Mat will return colors in BGRA
 * order, which will correspond to a color order of ARGB in a QColor. This means
 * that QColor.alpha() will match to the cv::Mat Blue channel, QColor.red() to
 * cv::Mat Green channel, etc. The reason why cv::cvtColor or cv::mixChannels
 * doesn't change the order of colors in such cases is not yet known, and as such
 * providing a cv::Mat that matches the QImage color order for 32 bit images is
 * not possible at this time.
 */

class QCVIMGLIB_EXPORT QCVimg : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief Default constructor.
     *
     * For both QImage and and cv::Mat members their respective default
     * constructors are called.
     */
    QCVimg(QObject* parent = nullptr) noexcept
        : QObject(parent) {}

    /**
     * @brief Copy constructor with deep copy.
     *
     * After creating a deep copy of @p img it ties the cv::Mat member to the
     * copied data.
     * @param img Source image to copy from.
     */
    QCVimg(const QCVimg& img, QObject* parent = nullptr);

    /**
     * @brief Copy assignment operator with deep copy.
     *
     * It makes a deep copy of @p img and then ties the cv::Mat member to the
     * copied data.
     * @param img Source image to copy from.
     */
    QCVimg& operator=(const QCVimg& img);

    /**
     * @brief Move constructor.
     *
     * After moving the QImage member normally from @p img, it simply copies
     * the cv::Mat member, since cv::Mat uses shallow copy (and has no move
     * semantics). The cv::Mat member in @p img is then overwritten with a default
     * constructed (empty) Mat to decrease the reference counter and to prevent
     * unexpected data modification.
     * @param img Source image to move from.
     */
    QCVimg(QCVimg&& img) noexcept;

    /**
     * @brief Move assignment operator.
     *
     * Does the same as the move constructor.
     * @param img Source image to move from.
     */
    QCVimg& operator=(QCVimg&& img);

    /**
     * Transfers arguments directly to the appropriate QImage constructor, so size
     * checking is done there to avoid duplication. @p format on the other hand is
     * checked to allow the construction of QImages that have a compatible cv::Mat
     * format only. If an incompatible format is provided, then this constructor
     * equals in functionality to the default constructor (e.g. an empty image
     * is created).
     * @param width Number of columns in a 2D image.
     * @param height Number of rows in a 2D image.
     * @param format In-memory format of image to be created. See
     * #convertQImgFormatTag and #convertMatFormatTag for valid formats.
     */
    QCVimg(int width, int height, QImage::Format format);

    /**
     * @brief Constructs from QImage with deep copy.
     *
     * Creates a deep copy of the @p img if the image had a compatible format,
     * then ties cv::Mat member to the data. On an incompatible format an
     * empty image is created.
     * @param img Source image to copy from.
     */
    explicit QCVimg(const QImage& img);

    /**
     * @brief Constructs from QImage with move semantics.
     *
     * Moves from QImage member of @p img after checking for a compatible image
     * format, then ties cv::Mat member to the data. On an incompatible format
     * an empty image is created and the original is not touched.
     * @param img Source image to move from.
     */
    explicit QCVimg(QImage&& img);

    /**
     * @brief Constructs from cv::Mat with deep copy.
     *
     * Creates a deep copy of @p img if the image had a compatible format. Memory
     * management is performed by the QImage member by first calling the QImage
     * constructor with the size and converted format provided by @p img. After
     * the cv::Mat member is tied to the newly allocated QImage, the data is
     * finally copied into it.
     * @param img Source image to copy from.
     * @param sourceMatColorOrder Allows to inform the function whether
     * @sourceMat has RGB or BGR color ordering. If BGR is provided and the
     * image is a three channel image, the red and blue channels are swapped to
     * facilitate proper interoperability between cv::Mat and QImage. The final
     * image will always have an RGB color ordering. If source image is a single
     * or four channel image, this argument has no effect.
     * @see MatColorOrder
     */
    explicit QCVimg(const cv::Mat& img, MatColorOrder sourceColorOrder = MatColorOrder::RGB);

    /**
     * @brief Returns the size of image data as reported by QImage.
     *
     * Please refer to QImage documentation for more info.
     */
    qsizetype bytes() const;

    /**
     * @brief Copies and converts the image into the provided @p format.
     *
     * In case an invalid format is provided, an empty image is returned. The
     * original data is left unchanged.
     * @param format Target format to convert the image into.
     * @return A copy of the original image converted into the format provided
     * in @p format
     */
    QCVimg convertToFormat(QImage::Format format) const;

    /**
     * @brief Makes deep copy of @p sourceQImg and ties the cv::Mat member to
     * the copied data.
     *
     * In case @p sourceQImg has an incompatible format, the data inside QCVimg
     * is left unchanged.
     * @param sourceQImg QImage to copy from.
     * @return 0 for successful copy, -1 if invalid format was provided.
     */
    int copy(const QImage& sourceQImg);

    /**
     * @brief Makes deep copy of @p sourceMat and ties the cv::Mat member to the
     * copied data.
     *
     * Memory management is performed by the QImage member, as a new empty
     * instance is created with the size and converted format provided by
     * @p sourceMat. After the cv::Mat member has been tied to the new QImage,
     * the data from @p sourceMat is finally copied into it. If the existing
     * size and format matches that of @p sourceMat, no new allocations are
     * performed, the data is simply copied over. If @p sourceMat has
     * incompatible type, no action is performed.
     * @param sourceMat Source Mat to copy from.
     * @param sourceMatColorOrder Allows to inform the function whether
     * @p sourceMat has RGB or BGR color ordering. If BGR is provided and the
     * image is a three channel image, the red and blue channels are swapped to
     * facilitate proper interoperability between cv::Mat and QImage. The final
     * image will always have an RGB color ordering. If source image is a single
     * or four channel image, this argument has no effect.
     * @return 0 for successful copy, -1 if invalid format was provided.
     */
    int copy(const cv::Mat& sourceMat, MatColorOrder sourceMatColorOrder = MatColorOrder::RGB);

    /**
     * @brief Makes deep copy of image to @p dest.
     *
     * This function uses the cv::Mat member to copy the data from, and calls its
     * native copy function (which is also copyTo, see OpenCV documentation). The
     * user must also make sure that the Mat member is indeed bound to the QImage
     * data (by calling #isMatBound first) in order to get the desired effects.
     * @param dest OpenCV array to copy the data to. See OpenCV documentation for
     * more information on OutputArray.
     */
    void copyTo(cv::OutputArray dest) const;

    /**
     * @brief Makes deep copy of image to @p dest.
     *
     * Directly calls the copy() function of QImage. See Qt documentation for more
     * details.
     * @param dest QImage to copy the data to.
     */
    void copyTo(QImage& dest) const;

    /**
     * @brief Returns a non-const reference to the cv::Mat member.
     *
     * This is provided to facilitate work with the vast OpenCV library, although
     * extreme caution must be taken. A lot of OpenCV functions will try to
     * reallocate data or modify the image metadata implicitly in a way that
     * would break compatibility between the QImage and cv::Mat members.
     * Unfortunately, it is not really well documented how OpenCV functions work
     * when not the cv::Mat is responsible for memory management, so a lot of
     * unexpected results or crashes are to be expected if not careful. The best
     * way to avoid such undesired behavior is to make sure that a temporary
     * cv::Mat is used for functions known to modifiy aforementioned properties,
     * and only the end result is copied into QCVimg with one of the provided
     * methods. It is also extremely important to remember, that cv::Mat uses
     * shallow copy, so a simple assignment will not prevent the problems from
     * happening. Any modification done on the cv::Mat metadata outside the
     * boundaries of QCVimg will not get the QImage member updated! Using
     * QCVimg's #copyFrom , #copyTo and #isMatBound is highly advised.
     */
    cv::Mat& cvMat();

    /**
     * @brief Returns a const reference to the cv::Mat member.
     *
     * An overload of the previous function, some of the same warnings apply.
     */
    const cv::Mat& cvMat() const;

    /**
     * @brief Tells if image is empty.
     *
     * Checks both QImage and cv::Mat members for emptiness to make sure the
     * instance truly has no data in it. This will only be true if both QImage
     * and cv::Mat members point to null pointers and report zero size (height
     * and width) at the same time.
     *
     * It is important to know, that if the user accidentally used the cv::Mat
     * member separately from the QImage and memory was allocated through it,
     * reducing the cv::Mat size to zero does not necessarily mean that memory
     * will be deallocated as well, so this function will report false in these
     * cases. Calling the release() method of the cv::Mat member in such cases
     * probably will fix the problem. See OpenCV documentation for more
     * information.
     * @return Only true if both QImage and cv::Mat members are empty, otherwise
     * false. See description for more information.
     */
    bool empty() const;

    /**
     * @brief Fill image with color.
     *
     * Convenience function. Directly calls QImage's appropriate fill() function.
     * See Qt documentation for more info.
     * @param pixelValue color to be used to fill image.
     */
    void fill(uint pixelValue);

    /**
     * @brief Fill image with color.
     *
     * Convenience function. Directly calls QImage's appropriate fill() function.
     * See Qt documentation for more info.
     * @param pixelValue color to be used to fill image.
     */
    void fill(const QColor& color);

    /**
     * @brief Fill image with color.
     *
     * Convenience function. Directly calls QImage's appropriate fill() function.
     * See Qt documentation for more info.
     * @param pixelValue color to be used to fill image.
     */
    void fill(const Qt::GlobalColor color);

    /**
     * @brief Returns height of the image.
     * @return Height of the image.
     */
    int height() const;

    /**
     * @brief Tells if the cv::Mat member is properly bound to the QImage member.
     *
     * All three of the following must be true, in order to consider the Mat
     * member to be properly bound:
     * 1. Both Mat and QImage point to the same data in memory.
     * 2. Both Mat and QImage report the same width and height.
     * 3. Both Mat and QImage report equivalent image formats.
     *
     * Furthermore, there are two possibilities when the Mat member is considered
     * to point to the QImage data:
     * 1. both members return the same pointer.
     * 2. both members point to nullptr (NULL is also accepted in case of cv::Mat,
     * see OpenCV docs for reasons).
     *
     * @return true if Mat member is bound, otherwise false. See description for
     * details.
     */
    bool isMatBound() const;

    /**
     * @brief Returns the image's format in OpenCV notation.
     *
     * For more information about what these numbers mean, check the OpenCV
     * documentation.
     * @return Image format in OpenCV notation.
     */
    int matFormat() const;

    /**
     * @brief Returns the image's format in Qt notation.
     *
     * For more information on what the formats mean, check the Qt documentation.
     * @return Image format in Qt notation.
     */
    QImage::Format qFormat() const;

    /**
     * @brief Compares two images for equality.
     *
     * Internally there is only one true comparison performed, and it is done
     * through calling QImage's operator==(). To make the two images truly
     * equal the cv::Mat members have to be bound to their respective QImages
     * as well.
     * @return True if QImage members are equal and cv::Mat members are bound
     * in both images, otherwise false.
     * @see operator!=
     * @see isMatBound
     */
    bool operator==(const QCVimg& other) const;

    /**
     * @brief Compares two images for equality.
     *
     * @return True if QImage members are not equal or any of the cv::Mat members
     * are not bound, otherwise false.
     * @see operator==
     */
    bool operator!=(const QCVimg& other) const;

    /**
     * @brief Allows serialization of class using QDataStream (output)
     */
    friend QDataStream& operator<<(QDataStream& ds, const QCVimg& img);

    /**
     * @brief Allows serialization of class using QDataStream (input)
     */
    friend QDataStream& operator>>(QDataStream& ds, QCVimg& img);

    /**
     * @brief Returns non-const reference to the QImage member.
     *
     * This is provided to facilitate work with the Qt framework, although extreme
     * caution must be taken. Some Qt functions might try to reallocate data or
     * modify the image metadata implicitly in a way that would break compatibility
     * between the QImage and cv::Mat members. The best way to avoid such undesired
     * behavior is to make sure that a temporary QImage is used for functions
     * known to modifiy aforementioned properties, and only the end result is
     * copied into QCVimg with one of the provided methods. Any modification
     * done on the QImage metadata outside the boundaries of QCVimg will not
     * get the cv::Mat member updated! Using QCVimg's #copy , #copyTo and
     * #isMatBound is highly advised.
     */
    QImage& qImg();

    /**
     * @brief Returns a const reference to the QImage member.
     *
     * An overload of the previous function, some of the same warnings apply.
     */
    const QImage& qImg() const;

    /**
     * @brief Returns a QPixmap generated from the internal QImage
     *
     * This is a convenience function, which simply calls QPixmap's built-in
     * function to create a QPixmap.
     */
    QPixmap qPix(Qt::ImageConversionFlags flags = Qt::AutoColor) const;

    /**
     * @brief Returns the color of a pixel in QColor format
     * @param x column number of the pixel
     * @param y row number of the pixel
     * @return Pixel color in QColor format.
     */
    QColor pixelColor(int x, int y) const;
    /**
     * @brief Rebinds the cv::Mat member to the QImage member data.
     *
     * This function can be called if the cv::Mat member somehow got out of sync
     * from the QImage member, and QCVimg consistency needs to be regained by
     * binding the Mat member to the QImage data. This is a relatively cheap
     * operation, as no image data is reallocated or copied during the process.
     * Compatible formats are checked before binding the cv::Mat format to the
     * QImage member. See @p priority for more information.
     * @param priority Determines what should be done in case of incompatible
     * QImage format. If it is Hi, and the cv::Mat member could not be bound
     * the QImage member because of an incompatible type, the data in the QImage
     * member will remain unchanged and the cv::Mat member will be set to empty.
     * If @p priority is low, and the cv::Mat member could not be bound the
     * QImage member because of an incompatible type, both members will be set
     * to an empty state.
     * On compatible QImage formats @p priority has no effect. The default is Low.
     * @return Returns 0 on successful binding of the cv::Mat member to the
     * QImage data. -1 if incompatible QImage formats were found.
     * See @p priority how these cases are handled.
     */
    int rebindMat(DataPrio priority = DataPrio::Low);

    /**
     * @brief Copies the data from the internal cv::Mat member into the QImage
     * member, then binds the Mat member to it.
     *
     * This function is the opposite of #rebindMat, as this time the data in the
     * cv::Mat member is used to restore QCVimg consistency. This is an expensive
     * operation, as the data is first copied into a new QImage instance and the
     * cv::Mat member is bound to it only after that. The overall operation is
     * almost identical to the cv::Mat overload of #copyFrom, but this time the
     * source is the internal cv::Mat member. Compatible formats are checked
     * before binding the format together. See @p priority for more information.
     * @param priority Determines what should be done in case of incompatible
     * QImage format. If it is Hi, and the data could not be copied over to
     * QImage member because of an incompatible type, the data in the cv::Mat
     * member will remain unchanged and the QImage member will be set to empty.
     * If @p priority is low, and the data could not be copied over to QImage
     * member because of an incompatible type, both members will be set to an
     * empty state. On compatible QImage formats @p priority has no effect.
     * The default is Low.
     * @param matColorOrder
     * @return On successful binding of the cv::Mat member to the QImage data
     * it returns 0. If incompatible QImage formats were found, it returns -1.
     * See @p priority how these cases are handled.
     */
    int rebindQImg(DataPrio priority = DataPrio::Low,
                   MatColorOrder matColorOrder = MatColorOrder::RGB);

    /**
     * @brief Resizes the image with the give new size
     *
     * This function directly calls the QImage::scaled() function on the QImage
     * member, then creates a new QCVimg instance. See the Qt documentation for
     * more information and parameter descriptions.
     * @return A new QCVimg instance with the new size.
     */
    QCVimg resize(int width, int height,
                  Qt::AspectRatioMode aspectRatioMode = Qt::IgnoreAspectRatio,
                  Qt::TransformationMode transformMode = Qt::FastTransformation) const;
    /**
     * @brief Swaps image with @p other
     *
     * Calls QImage's swap function, then swaps the cv::Mat members.
     * @param other Image to swap the current one with.
     */
    void swap(QCVimg& other);

    /**
     * @brief Returns true if @p x and @p y coordinates define a valid coordinate
     * within an image, otherwise false.
     *
     * Directly calls QImage's function with the same name.
     * @param x column number of the pixel
     * @param y row number of the pixel
     */
    bool valid(int x, int y) const;

    /**
     * @brief Returns the width of the image.
     * @return Width of the image.
     */
    int width() const;

    /**
     * @brief Converts a QImage format to a compatible cv::Mat format.
     *
     * In case an invalid format is provided, -1 is returned to indicate error.
     * DETAILS ABOUT VALID FORMATS
     *
     * @param qFormat A QImage::Format type format. See Qt documentation for more
     * information.
     * @return A format used for cv::Mat. See OpenCV documentation for more
     * information.
     */
    static int convertQImgFormatTag(QImage::Format qFormat);

    /**
     * @brief Converts cv::Mat format to a compatible QImage format.
     *
     * In case an invalid format is provided, QImage::Format_Invalid is returned.
     * DETAILS ABOUT VALID FORMATS
     *
     * @param matFormat A format used for cv::Mat. See OpenCV documentation for
     * more information.
     * @return A QImage::Format type format. See Qt documentation for more
     * information.
     */
    static QImage::Format convertMatFormatTag(int matFormat);

    /**
     * @brief A convenience function to check if provided image format is an
     * acceptable format.
     * @param qFormat A QImage::Format type format. See Qt documentation for
     * more information.
     * @return true if QCVimg can use provided format, otherwise false.
     */
    static bool isValidQImgFormat(QImage::Format qFormat);

    /**
     * @brief A convenience function to check if provided image format is an
     * acceptable format.
     * @param matFormat A format used for cv::Mat. See OpenCV documentation for
     * more information.
     * @return true if QCVimg can use provided format, otherwise false.
     */
    static bool isValidMatFormat(int matFormat);

    /**
     * @brief Swaps the red and blue channels in a three channel cv::Mat.
     *
     * This is a convenience function to help work with the default BGR color
     * order of OpenCV images. This is not to be used on the internal cv::Mat
     * members of the QCVimg class, as more likely then not, it will cause
     * problems. Internally it calls cv::cvtColor, thus more information on
     * the exact workings can be found in the OpenCV documentation.
     * @param sourceMat Source image in which the red and blue channels need
     * swapping. The data in it will remain unchanged.
     * @param destMat Destination image with the result. Not to be used on the
     * cv::Mat member of QCVimg. See description for more info.
     * @param sourceMatColorOrder See a list of acceptable codes in
     * #MatColorOrder enum.
     * @return returns 0 if a three channel image was provided, otherwise
     * returns -1.
     */
    static int swapMatRedBlue(const cv::Mat& sourceMat, cv::Mat& destMat,
                              MatColorOrder sourceMatColorOrder = MatColorOrder::RGB);

    /**
     * @brief Extracts red, green and blue channels of a QColor class instance
     * and puts them into a cv::Scalar.
     *
     * This is a convenience function to help initialize cv::Mat images with a
     * color.
     * @param color A QColor instance.
     * @param destScalarColorOrder Determines in what order the colors should
     * be in the resulting cv::Scalar. See #MatColorOrder for available color
     * orders.
     * @return Extracted colors in a cv::Scalar format. The values are always
     * of integer type.
     */
    static cv::Scalar convertQColorToScalar(const QColor& color, MatColorOrder destScalarColorOrder);

    /**
     * @brief Returns a list of human readable Qt image formats supprted by this
     * class
     */
    static QStringList supportedQImgFormats();

    /**
     * @brief Converts the human readable Qt image format text to QImage::Format
     * enum
     * @return returns the appropriate QImage::Format if text is valid, otherwise
     * returns QImage::Format_Invalid
     */
    static QImage::Format convertFormatTextToQImgFormat(const QString& formatText);

private:
    QImage mQImg;
    cv::Mat mMImg;
    static const QMap<QImage::Format, MatFormat> scmQtToCvFormatMap;
    static const QMap<MatFormat, QImage::Format> scmCvToQtFormatMap;
    static const QMap<QString, QImage::Format> scmTextToQImgFormat;

    static QMap<QImage::Format, MatFormat> fillWithCompatibleQtToCvFormats();
    static QMap<MatFormat, QImage::Format> fillWithCompatibleCvToQtFormats();
    static QMap<QString, QImage::Format> fillQImgFormatToStringMap();

    void createMatFromQImage(QImage& sourceQImg, cv::Mat& targetMat) const;
    void createQImageFromMat(const cv::Mat& sourceMat, QImage& targetQImg, QImage::Format qFormat) const;
    void copyFrom(const cv::Mat& sourceMat, QImage::Format qFormat);
    void copyFrom(const QImage& sourceQImg);
    void getRgbMat(const cv::Mat& sourceMat, cv::Mat& rgbMat, MatColorOrder sourceColorOrder) const;
    void setMembersEmpty();
    bool pointersMatch() const;
    bool sizesMatch() const;
    bool sizesMatch(const cv::Mat& first, const cv::Mat& second) const;
    bool formatsMatch() const;
    bool formatsMatch(const cv::Mat& first, const cv::Mat& second) const;
    bool typesMatch(const cv::Mat& first, const cv::Mat& second) const;
    bool matIsNull() const;
};


#endif // QCVIMG_H
