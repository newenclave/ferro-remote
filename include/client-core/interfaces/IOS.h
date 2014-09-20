#ifndef FR_INTERFACE_OS_H
#define FR_INTERFACE_OS_H

#include <string>

namespace fr { namespace client {

namespace core {
    class client_core;
}

namespace interfaces { namespace os {

    struct iface {
        virtual ~iface( ) { }
        virtual void execute( const std::string &cmd ) = 0;
    };

    typedef iface* iface_ptr;

    iface_ptr create( core::client_core &cl );

}}}}

#endif
