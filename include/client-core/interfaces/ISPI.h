#ifndef FR_INTERFACE_SPI_H
#define FR_INTERFACE_SPI_H

#include "IBaseIface.h"

#include <string>
#include <stdint.h>
#include <vector>
#include <map>

namespace fr { namespace client {

namespace core {
    class client_core;
}

namespace interfaces { namespace spi {

    typedef std::vector<std::string> string_vector;

    struct iface: public interfaces::base {
        virtual ~iface( ) { }
        virtual void close( ) const = 0;
        virtual void setup( unsigned speed, unsigned mode ) const = 0;

        virtual std::string read( size_t len ) const = 0;
        virtual void write( const void *data, size_t len ) const = 0;
        virtual string_vector wr( const string_vector &data ) const = 0;

        virtual std::string transfer( const unsigned char *data,
                                      size_t len ) const = 0;
        virtual string_vector transfer( const string_vector &data ) const = 0;
    };

    typedef iface* iface_ptr;

    iface_ptr open( core::client_core &cl,
                    unsigned bus, unsigned channel,
                    unsigned speed, unsigned mode );

    iface_ptr open( core::client_core &cl, unsigned channel,
                    unsigned speed, unsigned mode );

}}}}

#endif
