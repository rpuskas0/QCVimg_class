#ifndef QCVIMGLIB_DECL_H
#define QCVIMGLIB_DECL_H

#include <QtCore/qglobal.h>

#if defined(QCVIMGLIB_LIBRARY)
#  define QCVIMGLIB_EXPORT Q_DECL_EXPORT
#else
#  define QCVIMGLIB_EXPORT Q_DECL_IMPORT
#endif

#endif // QCVIMGLIB_DECL_H

