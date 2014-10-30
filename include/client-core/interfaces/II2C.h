#ifndef FR_II2C_H
#define FR_II2C_H

#include "vtrc-stdint.h"
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

    struct iface {
        virtual ~iface( ) { }
        virtual void   ioctl( unsigned code,    vtrc::uint64_t ) const = 0;
        virtual size_t  read( void *data,       size_t length  ) const = 0;
        virtual size_t write( const void *data, size_t length  ) const = 0;
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
