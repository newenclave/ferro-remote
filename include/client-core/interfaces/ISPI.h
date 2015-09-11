#ifndef FR_INTERFACE_SPI_H
#define FR_INTERFACE_SPI_H

#include "IBaseIface.h"

#include <string>

namespace fr { namespace client {

namespace core {
    class client_core;
}

namespace interfaces { namespace spi {

    struct iface: public interfaces::base {
        virtual ~iface( ) { }
        virtual void close( ) const = 0;
        virtual void setup( unsigned speed, unsigned mode ) const = 0;
        virtual void write_read( unsigned char *data, size_t len ) const = 0;
    };

    typedef iface* iface_ptr;

    iface_ptr open( core::client_core &cl,
                      unsigned bus, unsigned channel,
                      unsigned speed, unsigned mode );

    iface_ptr open( core::client_core &cl, unsigned channel,
                      unsigned speed, unsigned mode );

}}}}

#endif
