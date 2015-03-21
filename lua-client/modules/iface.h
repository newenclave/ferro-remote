#ifndef FR_LUA_MODULE_IFACE_H
#define FR_LUA_MODULE_IFACE_H

#include <memory>
#include <vector>

struct lua_State;

namespace fr { namespace lua { namespace objects {
    struct  base;
    class   table;
}}}

namespace fr { namespace lua { namespace client {
    struct general_info;
}}}

namespace fr { namespace lua { namespace m {

    struct iface {

        virtual ~iface( ) { }

        virtual void init( )    = 0;
        virtual void deinit( )  = 0;
        virtual void reinit( ) {   }

        virtual const std::string &name( ) const = 0;
        virtual std::shared_ptr<objects::table> table( ) const = 0;
        virtual bool connection_required( ) const { return true; }
    };

    typedef std::shared_ptr<iface> iface_sptr;

    typedef std::vector<iface_sptr> modules_list;

}}}

#endif // FR_LUA_MODULE_IFACE_H
