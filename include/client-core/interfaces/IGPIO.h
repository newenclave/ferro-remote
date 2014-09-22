#ifndef FR_IGPIO_H
#define FR_IGPIO_H

namespace fr { namespace client {

namespace core {
    class client_core;
}

namespace interfaces { namespace gpio {

    struct iface {
        virtual ~iface( ) { }
        virtual bool exists( ) const = 0;
    };

    typedef iface* iface_ptr;

    iface_ptr create( core::client_core &cl, unsigned gpio_id );

}}}}

#endif // IGPIO_H
