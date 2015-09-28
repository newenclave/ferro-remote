#ifndef FR_INTERFACE_SPI_H
#define FR_INTERFACE_SPI_H

#include "IBaseIface.h"

#include <string>

namespace fr { namespace client {

namespace core {
    class client_core;
}

namespace interfaces { namespace spi {

    enum reg_type {
         REG_8BIT  = 0
        ,REG_16BIT = 1
        ,REG_32BIT = 2
        ,REG_64BIT = 3
    };

    static
    inline reg_type reg_type_val2enum( unsigned val )
    {
        switch (val) {
        case REG_16BIT:
        case REG_32BIT:
        case REG_64BIT:
        case REG_8BIT:
        default:
            return static_cast<reg_type>(val);
        }
        return REG_8BIT;
    }

    struct iface: public interfaces::base {
        virtual ~iface( ) { }
        virtual void close( ) const = 0;
        virtual void setup( unsigned speed, unsigned mode ) const = 0;
        virtual std::string transfer( const unsigned char *data,
                                      size_t len ) const = 0;
    };

    typedef iface* iface_ptr;

    iface_ptr open( core::client_core &cl,
                      unsigned bus, unsigned channel,
                      unsigned speed, unsigned mode );

    iface_ptr open( core::client_core &cl, unsigned channel,
                      unsigned speed, unsigned mode );

}}}}

#endif
