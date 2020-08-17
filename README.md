# QCVimg class
### What is it?
It is a convenience wrapper class around Qt's QImage and OpenCV's cv::Mat classes.

### Why was it created?
* *Short version*: To make life easier when needing to work with Qt and OpenCV simultaneously.
* *Long version*: If you need to work on a project utilizing both Qt and OpenCV, you'll be faced with a problem almost instantaneously: the cv::Mat and QImage classes are too different to be able to seamlessly work with them in a project relying heavily on both of the aforementioned libraries, so some sort of a bridge class would come in handy. QCVimg is such a class.

### What does it do?
It provides a convenient way to work with both QImage and cv::Mat classes simultaneously by storing an easily accessible instance of both classes. The image data however is shared between these classes for memory efficiency, where allocation (and memory management in general) is always done by QImage, thus cv::Mat serves only as a different type of interface to the same underlying image data.

This class also ensures, that only compatible image formats can be used, which means, that only such in-memory image representations are allowed, which both classes can understand (correctly).

### Are there drawbacks?
Due to the vast differences between these two image classes, such an approach will naturally result in some compromises:
* A number of image formats that are supported by cv::Mat or QImage are not allowed to work with (incompatible data representation).
* Some OpenCV image manipulating functions will not work as expected, as some of them (hiddenly) rely on cv:Mat's memory management facilities (which are out of commission if cv::Mat didn't allocate the data, exactly like in this situation). In such cases a temporary deep copy of data into (then out of) a native cv::Mat instance might be the only viable solution.
* QImage's COW approach shouldn't be relied upon. In fact, any image moving or copying should be done directly with QCVimg to ensure image data consistency.

### Is it production ready?
Probably not. This class was created as part of a hobby project, which I do not have time anymore for. If you'd like to improve on it, please feel free to create a PR or just simply leave a comment.

### Can I get more info about it?
Of course. Please feel free to read the extensive Doxygen documentation in the header file. It contains detailed info on just about anything related to this class.

### Building
* Just #include the header into your project, and it should do the trick. 
* For convenience, I also provide a Qt Creator project file, so that you can add this to your existing subdirs Qt project as a library.

### License and copyright
Copyright (C) Robert Puskas
Licensed under GPLv3
