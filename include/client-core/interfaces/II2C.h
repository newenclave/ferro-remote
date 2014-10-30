#ifndef FR_II2C_H
#define FR_II2C_H

namespace fr { namespace client {

namespace core {
    class client_core;
}

namespace interfaces { namespace i2c {

    bool bus_available( core::client_core &cc, unsigned bus_id );

}}}}


#endif // II2C_H
