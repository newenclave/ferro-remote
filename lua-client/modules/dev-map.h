#ifndef FR_DEVMAP_H
#define FR_DEVMAP_H

#include "../utils.h"
#include "../fr-lua.h"

//////// TODO: think!
namespace fr { namespace lua {

    template <typename DevType, typename MetaType>
    class dev_map {
        const char *meta_name_;
        using storage_type = std::map<size_t, DevType>;
        storage_type devs_;
    public:
        dev_map( const char *meta_name )
            :meta_name_(meta_name)
        { }

        MetaType *push_object( lua_State *L, utils::handle hdl )
        {
            void *ud = lua_newuserdata( L, sizeof(MetaType) );
            auto *nfo = static_cast<MetaType *>(ud);
            if( nfo ) {
                luaL_getmetatable( L, meta_name_ );
                lua_setmetatable(L, -2);
                nfo->hdl_ = hdl;
            }
            return nfo;
        }

        utils::handle new_dev( utils::handle h, DevType dev )
        {
            devs_[h] = dev;
            return nh;
        }

        MetaType * get_object_ptr( lua_State *L, int id )
        {
            void *ud = luaL_testudata( L, id, meta_name_ );
            return ud ? static_cast<MetaType *>(ud) : nullptr;
        }

        DevType get_dev( utils::handle hdl )
        {
            auto f( devs_.find( utils::from_handle<size_t>(hdl) ) );
            if( f == devs_.end( ) ) {
                throw std::runtime_error( "Bad handle value." );
            }
            return f->second;
        }

    };

}}

#endif // DEVMAP_H
