#ifndef QIMGMATCH_GLOBAL_H
#define QIMGMATCH_GLOBAL_H

#include <QtCore/qglobal.h>

#ifdef QIMGMATCH_LIB
# define QIMGMATCH_EXPORT Q_DECL_EXPORT
#else
# define QIMGMATCH_EXPORT Q_DECL_IMPORT
#endif

#endif // QIMGMATCH_GLOBAL_H
