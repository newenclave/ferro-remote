#ifndef FR_LUA_MODULE_IFACE_H
#define FR_LUA_MODULE_IFACE_H

struct lua_State;

namespace fr { namespace lua { namespace client { namespace m {

    struct iface {
        virtual ~iface( ) { }
        virtual void init( )    = 0;
        virtual void deinit( )  = 0;
    };

}}}}

#endif // FR_LUA_MODULE_IFACE_H
