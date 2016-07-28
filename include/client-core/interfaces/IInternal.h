#ifndef FR_IINTERNAL_H
#define FR_IINTERNAL_H

#include "IBaseIface.h"
#include "vtrc-stdint.h"

namespace fr { namespace client {

namespace core {
    class client_core;
}

namespace interfaces { namespace internal {

    struct agent_info {
        std::string     name;
        vtrc::uint64_t  now;
        vtrc::uint64_t  tick_count;
        agent_info( )
            :now(0)
            ,tick_count(0)
        { }
    };

    struct iface: public interfaces::base {
        virtual ~iface( ) { }
        virtual void exit_process( ) const = 0;
        virtual size_t ping( ) const = 0;
        virtual agent_info info( ) const = 0;
    };

    typedef iface * iface_ptr;

    iface_ptr create( core::client_core &cl );

}}}}

#endif // IINTERNAL_H
