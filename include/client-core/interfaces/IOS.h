#ifndef FR_INTERFACE_OS_H
#define FR_INTERFACE_OS_H

#include "IBaseIface.h"

#include <string>

namespace fr { namespace client {

namespace core {
    class client_core;
}

namespace interfaces { namespace os {

    struct iface: public interfaces::base {
        virtual ~iface( ) { }
        virtual int execute( const std::string &cmd ) const = 0;
    };

    typedef iface* iface_ptr;

    iface_ptr create( core::client_core &cl );

}}}}

#endif
