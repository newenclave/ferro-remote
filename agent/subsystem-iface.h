#ifndef FR_SUBSYSTEMIFACE_H
#define FR_SUBSYSTEMIFACE_H

#include <string>
#include "vtrc-memory.h"

namespace fr { namespace agent {

    template<typename T>
    using enable_sft = vtrc::enable_shared_from_this<T>;

    struct subsystem_iface: public enable_sft<subsystem_iface>
    {

        subsystem_iface( ) { }

        virtual ~subsystem_iface( ) { }
        virtual const std::string &name( ) const = 0;

        virtual void init( )  = 0;
        virtual void start( ) = 0;
        virtual void stop( )  = 0;

        subsystem_iface( const subsystem_iface& ) = delete;
        subsystem_iface & operator = ( const subsystem_iface& ) = delete;
    };

    using subsystem_sptr = vtrc::shared_ptr<subsystem_iface>;
}}

#endif // SUBSYSTEMIFACE_H
