#ifndef FRCLIENTOS_H
#define FRCLIENTOS_H

#include <QObject>
#include "fr-client.h"

namespace fr { namespace declarative {

    class FrClientOS : public QObject
    {
        Q_OBJECT

    public:
        explicit FrClientOS(QObject *parent = 0);

    signals:

    public slots:

    };

}}

#endif // FRCLIENTOS_H
