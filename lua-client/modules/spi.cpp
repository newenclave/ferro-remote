#include "iface.h"
#include "../fr-lua.h"
#include "../lua-names.h"

#include "../general-info.h"

#include "interfaces/ISPI.h"

#include "../utils.h"
#include "../event-container.h"

namespace fr { namespace lua { namespace m { namespace spi {

namespace {

    using namespace objects;
    namespace ispi = fr::client::interfaces::spi;

    typedef std::shared_ptr<ispi::iface>  dev_sptr;
    typedef std::map<size_t, dev_sptr>    dev_map;

    const std::string     module_name("spi");
    const char *id_path     = FR_CLIENT_LUA_HIDE_TABLE ".spi.__i";
    const char *meta_name   = FR_CLIENT_LUA_HIDE_TABLE ".spi.meta";

    struct module;

    module *get_module( lua_State *L )
    {
        lua::state ls(L);
        void *ptr = ls.get<void *>( id_path );
        return static_cast<module *>(ptr);
    }

    struct meta_object {
        utils::handle hdl_;
        unsigned bus_;
        unsigned channel_;
    };

    int lcall_meta_string( lua_State *L )
    {
        lua::state ls(L);
        void *ud = luaL_testudata( L, 1, meta_name );
        if( ud ) {
            std::ostringstream oss;
            oss << "spi@"
                << std::hex
                << static_cast<meta_object *>(ud)->hdl_
                << ":"
                << static_cast<meta_object *>(ud)->bus_
                << "."
                << static_cast<meta_object *>(ud)->channel_
                   ;
            ls.push( oss.str( ) );
        } else {
            ls.push( "Unknown object." );
        }
        return 1;
    }

    int lcall_open ( lua_State *L );
    int lcall_close( lua_State *L );

    const struct luaL_Reg spi_lib[ ] = {
         { "close",       &lcall_close          }
        ,{ "__gc",        &lcall_close          }
        ,{ "__tostring",  &lcall_meta_string    }
        ,{ nullptr,        nullptr }
    };

//    std::vector<std::string> events_names( )
//    {
//        std::vector<std::string> res;

//        res.push_back( "on_changed" );

//        return res;
//    }

    int lcall_register_meta( lua_State *L )
    {
        metatable mt( meta_name, spi_lib );
        mt.push( L );
        return 1;
    }

    void register_meta_tables( lua_State *L )
    {
        lua::state ls(L);
        ls.push( lcall_register_meta );
        lua_call( L, 0, 0 );
    }

    struct module: public iface {

        client::general_info            &info_;
        dev_map                          devs_;

        module( client::general_info &info )
            :info_(info)
        {
            register_meta_tables( info_.main_ );
        }

        void init( )
        {
            lua::state ls( info_.main_ );
            ls.set( id_path, this );
        }

        dev_sptr get_dev( utils::handle hdl )
        {
            auto f( devs_.find( utils::from_handle<size_t>(hdl) ) );
            if( f == devs_.end( ) ) {
                throw std::runtime_error( "Bad handle value." );
            }
            return f->second;
        }

        utils::handle get_object_hdl( lua_State *L, int id )
        {
            void *ud = luaL_testudata( L, id, meta_name );
            if( ud ) {
                return static_cast<meta_object *>(ud)->hdl_;
            } else {
                return utils::handle( );
            }
        }

        size_t next_handle(  )
        {
            return info_.eventor_->next_index( );
        }

        utils::handle new_dev( unsigned bus_id,
                               unsigned channel, unsigned speed,
                               unsigned mode)
        {
            size_t nh = next_handle( );
            dev_sptr r( ispi::open( *info_.client_core_,
                                     bus_id, channel, speed, mode ));
            devs_[nh] = r;
            return utils::to_handle( nh );
        }

        meta_object *push_object( lua_State *L, utils::handle hdl )
        {
            void *ud = lua_newuserdata( L, sizeof(meta_object) );
            meta_object *nfo = static_cast<meta_object *>(ud);
            if( nfo ) {
                luaL_getmetatable( L, meta_name );
                lua_setmetatable(L, -2);
                nfo->hdl_ = hdl;
            }
            return nfo;
        }

        void deinit( )
        {
            devs_.clear( );
        }

        const std::string &name( ) const
        {
            return module_name;
        }

        std::shared_ptr<objects::table> table( ) const
        {
            objects::table_sptr res(std::make_shared<objects::table>( ));

            res->add( "open", new_function( &lcall_open ) );

            return res;
        }
    };

    inline bool is_number( const objects::base *o )
    {
        return o->type_id( ) == objects::base::TYPE_NUMBER;
    }

    inline bool is_string( const objects::base *o )
    {
        return o->type_id( ) == objects::base::TYPE_STRING;
    }

    int lcall_open( lua_State *L )
    {
        module *m = get_module( L );
        lua::state ls(L);

        unsigned bus   = 0;
        unsigned chan  = 0;
        unsigned speed = 500000;
        unsigned mode  = 0;
        try {
            auto t = ls.get_type( 1 );
            if( t == base::TYPE_TABLE ) {
                auto tobj = ls.get_object( 1 );
                for( size_t i=0; i<tobj->count( ); i++ ) {
                    auto p(tobj->at(i));
                    auto f(p->at(0));
                    auto s(p->at(1));

                    if( is_number( s ) ) {
                        if( is_number( f ) ) {
                            switch( static_cast<unsigned>(f->num( )) ) {
                            case 1:
                                bus   = static_cast<unsigned>(s->num( ));
                                break;
                            case 2:
                                chan = static_cast<unsigned>(s->num( ));
                                break;
                            case 3:
                                speed = static_cast<unsigned>(s->num( ));
                                break;
                            case 4:
                                mode = static_cast<unsigned>(s->num( ));
                                break;
                            }
                        } else if( is_string( f ) ) {
                            std::string name(f->str( ));
                            /// TODO: fix it
                            if( !name.compare( "bus" ) ) {
                                bus   = static_cast<unsigned>(s->num( ));
                            } else if( !name.compare( "channel" ) ) {
                                chan = static_cast<unsigned>(s->num( ));
                            } else if( !name.compare( "chan" ) ) {
                                chan = static_cast<unsigned>(s->num( ));
                            } else if( !name.compare( "speed" ) ) {
                                speed = static_cast<unsigned>(s->num( ));
                            } else if( !name.compare( "mode" ) ) {
                                mode = static_cast<unsigned>(s->num( ));
                            }
                        }
                    }
                }
            } else {
                bus   = ls.get_opt<unsigned>( 1, 0 );
                chan  = ls.get_opt<unsigned>( 2, 1 );
                speed = ls.get_opt<unsigned>( 3, 500000 );
                mode  = ls.get_opt<unsigned>( 4, 0 );
            }
            m->push_object( L, m->new_dev( bus, chan, speed, mode ) );
        } catch( const std::exception &ex ) {
            ls.push( );
            ls.push( ex.what( ) );
            return 2;
        }

        return 1;
    }

    int lcall_close( lua_State *L )
    {
        module *m = get_module( L );
        if( !m ) {
            return 0;
        }

        lua::state ls(L);
        utils::handle h = m->get_object_hdl( L, 1 );

        m->devs_.erase( utils::from_handle<size_t>( h ) );

        ls.push( true );
        return 0;
    }

}

    iface_sptr create( client::general_info &info )
    {
        return std::make_shared<module>( std::ref( info ) );
    }

}}}}


