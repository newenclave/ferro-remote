#ifndef FRCLIENTI2C_H
#define FRCLIENTI2C_H

#include "fr-client-component.h"

namespace fr { namespace declarative {

    class FrClientI2c: public FrClientComponent
    {
        Q_OBJECT

        Q_PROPERTY( quint32 busId
                    READ busId WRITE setBusId NOTIFY busIdChanged )

        Q_PROPERTY( quint16 slaveAddress
                    READ slaveAddress WRITE setSlaveAddress
                    NOTIFY slaveAddressChanged )

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
        enum SlaveAddrInvalidValue { Invalid_Slave_address = 0xFFFF };

        Q_ENUMS( BusIdInvalidValue )
        Q_ENUMS( SlaveAddrInvalidValue )

        quint32 busId( ) const;
        void setBusId( quint32 value );

        quint16 slaveAddress( ) const;
        void setSlaveAddress( quint16 value );

    signals:

        void busIdChanged( quint32 value ) const;
        void slaveAddressChanged( quint16 value ) const;

    public slots:

    };

}}

#endif // FRCLIENTI2C_H
