#ifndef FRCLIENTI2C_H
#define FRCLIENTI2C_H

#include "fr-client-component.h"

namespace fr { namespace declarative {

    class FrClientI2c: public FrClientComponent
    {
        Q_OBJECT

        Q_PROPERTY( quint32 busId
                    READ busId WRITE setBusId NOTIFY busIdChanged )

        struct  impl;
        impl   *impl_;

    private:

        void on_reinit( );
        void on_ready( bool value );

        bool clientFailed( ) const;

    public:

        explicit FrClientI2c( QObject *parent = nullptr );
        ~FrClientI2c( );

    public:

        enum BusIdInvalidValue { Invalid_Bus_Id = 0xFFFFFFFF };
        Q_ENUMS( BusIdInvalidValue )

        quint32 busId( ) const;
        void setBusId( quint32 value );

    signals:

        void busIdChanged( quint32 value ) const;

    public slots:

    };

}}

#endif // FRCLIENTI2C_H
