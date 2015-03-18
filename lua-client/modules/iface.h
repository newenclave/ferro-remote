#ifndef FR_LUA_MODULE_IFACE_H
#define FR_LUA_MODULE_IFACE_H

#include <memory>

struct lua_State;

namespace fr { namespace lua { namespace objects {
    struct  base;
    class   table;
}}}

namespace fr { namespace lua { namespace client { namespace m {

    struct iface {
        virtual ~iface( ) { }
        virtual void init( )    = 0;
        virtual void deinit( )  = 0;
    };

    typedef std::shared_ptr<iface> iface_sptr;

}}}}

#endif // FR_LUA_MODULE_IFACE_H
