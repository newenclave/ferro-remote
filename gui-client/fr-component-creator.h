#ifndef FRCOMPONENTCREATOR_H
#define FRCOMPONENTCREATOR_H

#include <QObject>
#include "fr-client-os.h"

namespace fr { namespace declarative {


    class FrComponentCreator : public QObject
    {
        Q_OBJECT

    public:

        explicit FrComponentCreator(QObject *parent = 0);

    signals:

    public slots:

    public:

        Q_INVOKABLE QObject *newClient( QObject *parent = nullptr );
        Q_INVOKABLE QObject *newOs( FrClient *client = nullptr );

    };


}}
#endif // FRCOMPONENTCREATOR_H
