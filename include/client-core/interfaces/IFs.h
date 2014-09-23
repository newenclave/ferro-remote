#ifndef FR_INTERFACE_FILESYSTEM_H
#define FR_INTERFACE_FILESYSTEM_H

namespace fr { namespace client {

namespace core {
    class client_core;
}

namespace interfaces { namespace filesystem {

    struct iface {
        virtual ~iface( ) { }
    };

    typedef iface* iface_ptr;
    iface_ptr create( core::client_core &cl, const std::string &path );

}}}}

#endif // FR_INTERFACE_FILESYSTEM_H
