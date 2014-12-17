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

        Q_PROPERTY( quint32 functionsSupported READ functionsSupported )

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

        enum BusIdInvalidValue { InvalidBusId = 0xFFFFFFFF };
        enum SlaveAddrInvalidValue { InvalidSlaveAddress = 0xFFFF };
        enum I2CFunctionCodes {
            FuncI2C                  = 0x00000001
           ,Func10BitAddr            = 0x00000002
           ,FuncProtocolMangling     = 0x00000004 /* I2C_M_IGNORE_NAK etc. */
           ,FuncSmbusPec             = 0x00000008
           ,FuncNostart              = 0x00000010 /* I2C_M_NOSTART */
           ,FuncSmbusBlockProcCall   = 0x00008000 /* SMBus 2.0 */
           ,FuncSmbusQuick           = 0x00010000
           ,FuncSmbusReadByte        = 0x00020000
           ,FuncSmbusWriteByte       = 0x00040000
           ,FuncSmbusReadByteData    = 0x00080000
           ,FuncSmbusWriteByteData   = 0x00100000
           ,FuncSmbusReadWordData    = 0x00200000
           ,FuncSmbusWriteWordData   = 0x00400000
           ,FuncSmbusProcCall        = 0x00800000
           ,FuncSmbusReadBlockData   = 0x01000000
           ,FuncSmbusWriteBlockData  = 0x02000000
           ,FuncSmbusReadI2CBlock    = 0x04000000 /* I2C-like block xfer  */
           ,FuncSmbusWriteI2CBlock   = 0x08000000 /* w/ 1-byte reg. addr. */
        };

        enum I2CControlCodes {
            CodeI2CRetries     = 0x0701
           ,CodeI2CTimeout     = 0x0702
           ,CodeI2CSlave       = 0x0703
           ,CodeI2CSlaveForce  = 0x0706
           ,CodeI2CTenBit      = 0x0704
           ,CodeI2CPec         = 0x0708
        };

        Q_ENUMS( BusIdInvalidValue )
        Q_ENUMS( SlaveAddrInvalidValue )
        Q_ENUMS( I2CFunctionCodes )
        Q_ENUMS( I2CControlCodes )

        quint32 busId( ) const;
        void setBusId( quint32 value );

        quint16 slaveAddress( ) const;
        void setSlaveAddress( quint16 value );

        quint32 functionsSupported( ) const;

    signals:

        void busIdChanged( quint32 value ) const;
        void slaveAddressChanged( quint16 value ) const;

    public slots:

    };

}}

#endif // FRCLIENTI2C_H
