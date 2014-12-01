#ifndef FRCLIENTOS_H
#define FRCLIENTOS_H

#include <QObject>
#include "fr-base-component.h"

namespace fr { namespace declarative {

    class FrClientOS: public FrBaseComponent {

        Q_OBJECT

        struct impl;
        impl  *impl_;

    private:

        void on_reinit( );
        bool clientFailed( ) const;

    public:

        explicit FrClientOS( QObject *parent = 0 );

        ~FrClientOS( );

    public:

        Q_INVOKABLE int execute( const QString &cmd ) const;

    signals:

    public slots:

    private slots:
        void ready(  );
    };

}}

#endif // FRCLIENTOS_H
