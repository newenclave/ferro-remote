#ifndef FR_INTERFACE_FILESYSTEM_H
#define FR_INTERFACE_FILESYSTEM_H

#include <string>

namespace fr { namespace client {

namespace core {
    class client_core;
}

namespace interfaces { namespace filesystem {

    struct iface {
        virtual ~iface( ) { }
        virtual bool exists( const std::string &path ) const = 0;
    };

    typedef iface* iface_ptr;
    iface_ptr create( core::client_core &cl, const std::string &path );

}}}}

#endif // FR_INTERFACE_FILESYSTEM_H
