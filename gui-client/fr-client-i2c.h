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

        enum BusIdInvalidValue { InvalidBusId = 0xFFFFFFFF };
        enum SlaveAddrInvalidValue { InvalidSlaveAddress = 0xFFFF };
        enum I2CFunctionCodes {
            FUNC_I2C                    = 0x00000001
           ,FUNC_10BIT_ADDR             = 0x00000002
           ,FUNC_PROTOCOL_MANGLING      = 0x00000004 /* I2C_M_IGNORE_NAK etc. */
           ,FUNC_SMBUS_PEC              = 0x00000008
           ,FUNC_NOSTART                = 0x00000010 /* I2C_M_NOSTART */
           ,FUNC_SMBUS_BLOCK_PROC_CALL  = 0x00008000 /* SMBus 2.0 */
           ,FUNC_SMBUS_QUICK            = 0x00010000
           ,FUNC_SMBUS_READ_BYTE        = 0x00020000
           ,FUNC_SMBUS_WRITE_BYTE       = 0x00040000
           ,FUNC_SMBUS_READ_BYTE_DATA   = 0x00080000
           ,FUNC_SMBUS_WRITE_BYTE_DATA  = 0x00100000
           ,FUNC_SMBUS_READ_WORD_DATA   = 0x00200000
           ,FUNC_SMBUS_WRITE_WORD_DATA  = 0x00400000
           ,FUNC_SMBUS_PROC_CALL        = 0x00800000
           ,FUNC_SMBUS_READ_BLOCK_DATA  = 0x01000000
           ,FUNC_SMBUS_WRITE_BLOCK_DATA = 0x02000000
           ,FUNC_SMBUS_READ_I2C_BLOCK   = 0x04000000 /* I2C-like block xfer  */
           ,FUNC_SMBUS_WRITE_I2C_BLOCK  = 0x08000000 /* w/ 1-byte reg. addr. */
        };

        Q_ENUMS( BusIdInvalidValue )
        Q_ENUMS( SlaveAddrInvalidValue )
        Q_ENUMS( I2CFunctionCodes )

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
