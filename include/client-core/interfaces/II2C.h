#ifndef FR_II2C_H
#define FR_II2C_H

#include <stdint.h>
#include "vtrc-memory.h"

namespace fr { namespace client {

namespace core {
    class client_core;
}

namespace interfaces { namespace i2c {

    enum control_codes {
         CODE_I2C_RETRIES     = 0x0701
        ,CODE_I2C_TIMEOUT     = 0x0702
        ,CODE_I2C_SLAVE       = 0x0703
        ,CODE_I2C_SLAVE_FORCE = 0x0706
        ,CODE_I2C_TENBIT      = 0x0704
        ,CODE_I2C_PEC         = 0x0708
    };

    enum function_codes {
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

    struct iface {

        virtual ~iface( ) { }

        virtual uint64_t function_mask( ) const = 0; // see function_codes

        virtual void   set_address( uint8_t addr ) const = 0;
        virtual void   ioctl( unsigned code,    uint64_t param ) const = 0;
        virtual size_t  read( void *data,       size_t length  ) const = 0;
        virtual size_t write( const void *data, size_t length  ) const = 0;

        virtual uint8_t read_byte( uint8_t command ) const = 0;
        virtual void write_byte( uint8_t command, uint8_t value ) const = 0;

        virtual uint16_t read_word( uint8_t command ) const = 0;
        virtual void write_word( uint8_t command, uint16_t value ) const = 0;

        virtual std::string read_block( uint8_t command ) const = 0;
        virtual void write_block( uint8_t command,
                                  const std::string &value ) const = 0;

        virtual std::string read_block_broken( uint8_t command,
                                               uint8_t len ) const = 0;
        virtual void write_block_broken( uint8_t command,
                                         const std::string &value ) const = 0;

    };

    typedef iface * iface_ptr;
    typedef vtrc::shared_ptr<iface> iface_sptr;

    iface_ptr open( core::client_core &cc, unsigned bus_id );
    iface_ptr open( core::client_core &cc,
                    unsigned bus_id, unsigned slave_addr );
    iface_ptr open( core::client_core &cc,
                    unsigned bus_id, unsigned slave_addr, bool slave_force );

    bool bus_available( core::client_core &cc, unsigned bus_id );

}}}}


#endif // II2C_H
