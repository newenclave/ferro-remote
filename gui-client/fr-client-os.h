#ifndef FRCLIENTOS_H
#define FRCLIENTOS_H

#include <QObject>
#include "fr-client.h"

namespace fr { namespace declarative {

    class FrClientOS: public QObject
    {
        Q_OBJECT
        Q_PROPERTY( fr::declarative::FrClient *client
                    READ client WRITE setClient NOTIFY clientChanged )

        struct impl;
        impl  *impl_;

    public:
        explicit FrClientOS( QObject *parent = 0 );
        ~FrClientOS( );

    public:

        FrClient *client( ) const;
        void setClient( FrClient *new_value );

        Q_INVOKABLE int execute( const QString &cmd ) const;

    signals:

        void clientChanged( const fr::declarative::FrClient *new_value );

    public slots:

    private slots:

        void ready( );

    };

}}

#endif // FRCLIENTOS_H
