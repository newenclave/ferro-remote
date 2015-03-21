#ifndef FR_CLIENT_MLIST_HPP
#define FR_CLIENT_MLIST_HPP

#include "iface.h"

#include <vector>

namespace fr { namespace lua { namespace client {
    struct general_info;
}}}

namespace fr { namespace lua { namespace m {

    namespace event_queue { iface_sptr create( client::general_info &info ); }

    static inline
    modules_list create_all( client::general_info &info )
    {
        modules_list res;

        res.push_back( event_queue::create( info ) );

        return res;
    }

}}}

#endif // FR_CLIENT_MLIST_HPP
