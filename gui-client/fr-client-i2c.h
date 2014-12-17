#ifndef FRCLIENTI2C_H
#define FRCLIENTI2C_H

#include "fr-client-component.h"

namespace fr { namespace declarative {

    class FrClientI2c: public FrClientComponent
    {
        Q_OBJECT

 //        Q_PROPERTY( quint32 busId
//                    READ busId WRITE setBusId NOTIFY busIdChanged )

        struct  impl;
        impl   *impl_;

    private:

        void on_reinit( );
        void on_ready( bool value );

        bool clientFailed( ) const;

    public:

        explicit FrClientI2c( QObject *parent = nullptr );
        ~FrClientI2c( );

    signals:

    public slots:

    };

}}

#endif // FRCLIENTI2C_H
