#ifndef FRCOMPONENTCREATOR_H
#define FRCOMPONENTCREATOR_H

#include <QObject>
#include <QPointer>

#include "fr-client.h"

namespace fr { namespace declarative {

    class FrComponentCreator : public QObject
    {
        Q_OBJECT

    public:

        explicit FrComponentCreator( QObject *parent = 0 );

    signals:

    public slots:

    public:

        Q_INVOKABLE QObject *newClient( QObject *parent = nullptr );
        Q_INVOKABLE QObject *newOs( FrClient *client = nullptr );
        Q_INVOKABLE QObject *newFs( FrClient *client = nullptr,
                                    const QString &path = QString( ));

    };


}}
#endif // FRCOMPONENTCREATOR_H
