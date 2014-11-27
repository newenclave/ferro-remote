#ifndef FRBASECOMPONENT_H
#define FRBASECOMPONENT_H

#include <QObject>
#include "fr-client.h"

namespace fr { namespace declarative {

    class FrBaseComponent: public QObject {

        Q_OBJECT
        Q_PROPERTY( fr::declarative::FrClient *client
                    READ client WRITE setClient NOTIFY clientChanged )

    public:

        explicit FrBaseComponent(QObject *parent = 0);

        FrClient *client( ) const;
        void setClient( FrClient *new_value );

    signals:

        void clientChanged( const fr::declarative::FrClient *new_value );

    public slots:

    };

}}

#endif // FRBASECOMPONENT_H
