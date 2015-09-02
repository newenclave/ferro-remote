#ifndef FR_IINTERNAL_H
#define FR_IINTERNAL_H

#include "IBaseIface.h"

namespace fr { namespace client {

namespace core {
    class client_core;
}

namespace interfaces { namespace internal {

    struct iface: public interfaces::base {
        virtual ~iface( ) { }
        virtual void exit_process( ) const = 0;
        virtual void ping( ) const = 0;
    };

    typedef iface * iface_ptr;

    iface_ptr create( core::client_core &cl );

}}}}

#endif // IINTERNAL_H
