#include "iface.h"

namespace fr { namespace lua { namespace m { namespace event_queue {

namespace {

    struct module: public iface {
        general_info &info_;
        module( general_info &info )
            :info(info)
        { }
    };

}

    iface_sptr create( general_info &info )
    {
        std::shared_ptr<module>( std::ref( info ) );
    }

}}}}


