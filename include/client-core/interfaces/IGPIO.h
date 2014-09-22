#ifndef FR_IGPIO_H
#define FR_IGPIO_H

namespace fr { namespace client {

namespace core {
    class client_core;
}

namespace interfaces { namespace gpio {

    struct iface {
        virtual ~iface( ) { }
    };

}}}}

#endif // IGPIO_H
