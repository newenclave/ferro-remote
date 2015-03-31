#ifndef FR_CLIENT_MLIST_HPP
#define FR_CLIENT_MLIST_HPP

#include "iface.h"

#include <vector>

namespace fr { namespace lua { namespace client {
    struct general_info;
}}}

namespace fr { namespace lua { namespace m {

    namespace fs          { iface_sptr create( client::general_info &info ); }
    namespace os          { iface_sptr create( client::general_info &info ); }
    namespace gpio        { iface_sptr create( client::general_info &info ); }
    namespace smbus       { iface_sptr create( client::general_info &info ); }
    namespace console     { iface_sptr create( client::general_info &info ); }
    namespace event_queue { iface_sptr create( client::general_info &info ); }

    static inline
    modules_list create_all( client::general_info &info )
    {
        modules_list res;

        res.push_back( fs::create( info ) );
        res.push_back( os::create( info ) );
        res.push_back( gpio::create( info ) );
        res.push_back( smbus::create( info ) );
        res.push_back( console::create( info ) );
        res.push_back( event_queue::create( info ) );

        return res;
    }

}}}

#endif // FR_CLIENT_MLIST_HPP
