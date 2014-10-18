#ifndef FR_SUBSYSTEMIFACE_H
#define FR_SUBSYSTEMIFACE_H

#include <string>
#include "vtrc-memory.h"

namespace fr { namespace agent {

    struct subsystem_iface
            :public vtrc::enable_shared_from_this<subsystem_iface> 
    {

        virtual ~subsystem_iface( ) { }
        virtual const std::string &name( )  const = 0;

        virtual void init( )  = 0;
        virtual void start( ) = 0;
        virtual void stop( )  = 0;
    };

    typedef vtrc::shared_ptr<subsystem_iface> subsystem_sptr;
}}

#endif // SUBSYSTEMIFACE_H
