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

//    enum reg_type {
//         REG_8BIT  = 0
//        ,REG_16BIT = 1
//        ,REG_32BIT = 2
//        ,REG_64BIT = 3
//    };

//    static
//    inline reg_type reg_type_val2enum( unsigned val )
//    {
//        switch (val) {
//        case REG_16BIT:
//        case REG_32BIT:
//        case REG_64BIT:
//        case REG_8BIT:
//            return static_cast<reg_type>(val);
//        }
//        return REG_8BIT;
//    }

    typedef std::vector<uint8_t> uint8_vector; /// registry for read
    typedef std::pair<uint8_t, uint8_t>  cmd_uint8;  /// command + val8
    typedef std::pair<uint8_t, uint16_t> cmd_uint16; /// command + val16
    typedef std::pair<uint8_t, uint32_t> cmd_uint32; /// command + val32
    typedef std::pair<uint8_t, uint64_t> cmd_uint64; /// command + val64

    typedef std::vector<cmd_uint8>   cmd_uint8_vector;
    typedef std::vector<cmd_uint16>  cmd_uint16_vector;
    typedef std::vector<cmd_uint32>  cmd_uint32_vector;
    typedef std::vector<cmd_uint64>  cmd_uint64_vector;
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
