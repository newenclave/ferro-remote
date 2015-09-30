#include "iface.h"
#include "../fr-lua.h"
#include "../lua-names.h"

#include "../general-info.h"

#include "interfaces/ISPI.h"

#include "../utils.h"
#include "../event-container.h"

#include "vtrc-common/vtrc-exception.h"
#include "vtrc-common/vtrc-rpc-channel.h"
#include "vtrc-bind.h"
#include "vtrc-common/vtrc-exception.h"

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

    int lcall_open      ( lua_State *L );
    int lcall_write     ( lua_State *L );
    int lcall_setup     ( lua_State *L );
    int lcall_set_addr  ( lua_State *L );

    int lcall_set_reg8  ( lua_State *L );
    int lcall_set_reg16 ( lua_State *L );
    int lcall_set_reg32 ( lua_State *L );
    int lcall_set_reg64 ( lua_State *L );

    int lcall_get_reg8  ( lua_State *L );
    int lcall_get_reg16 ( lua_State *L );
    int lcall_get_reg32 ( lua_State *L );
    int lcall_get_reg64 ( lua_State *L );

    int lcall_close     ( lua_State *L );

    const struct luaL_Reg spi_lib[ ] = {
         { "close",       &lcall_close       }
        ,{ "write",       &lcall_write       }
        ,{ "transfer",    &lcall_write       }
        ,{ "setup",       &lcall_setup       }
        ,{ "set_address", &lcall_set_addr    }

        ,{ "write_bytes", &lcall_set_reg8    }
        ,{ "write_words", &lcall_set_reg16   }

        ,{ "set_reg8",    &lcall_set_reg8    }
        ,{ "set_reg16",   &lcall_set_reg16   }
        ,{ "set_reg32",   &lcall_set_reg32   }
        ,{ "set_reg64",   &lcall_set_reg64   }

        ,{ "read_bytes",  &lcall_get_reg8    }
        ,{ "read_words",  &lcall_get_reg16   }

        ,{ "get_reg8",    &lcall_get_reg8    }
        ,{ "get_reg16",   &lcall_get_reg16   }
        ,{ "get_reg32",   &lcall_get_reg32   }
        ,{ "get_reg64",   &lcall_get_reg64   }

        ,{ "__gc",        &lcall_close       }
        ,{ "__tostring",  &lcall_meta_string }
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

        client::general_info &info_;
        dev_map               devs_;
        int                   last_result_;

        module( client::general_info &info )
            :info_(info)
        {
            register_meta_tables( info_.main_ );
        }
#if 0
        void on_channel_error( lua_State *L, const char *mess )
        {
            lua::state ls(L);
            ls.push(  );
            ls.push( mess );
            last_result_ = 2;
        }

        void on_proto_error( lua_State *L, unsigned code, unsigned cat,
                             const char *mess )
        {
            lua::state ls(L);
            std::ostringstream oss;
            oss << "Error "
                << code << " "
                << vtrc::common::error_code_to_string( code, cat )
                << " (" << mess << ")";
            ls.push(  );
            ls.push( oss.str( ) );
            last_result_ = 2;
        }

        template <typename CallType>
        int make_call( CallType ct )
        {
            last_result_ = 1;
            ct( );
            return last_result_;
        }
#endif
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

            res->add( "open",   new_function( &lcall_open ) );

            return res;
        }
    };

    struct setup_info {
        unsigned bus   = 0;
        unsigned chan  = 1;
        unsigned speed = 500000;
        unsigned mode  = 0;
    };

    template <typename T>
    T table2pair_vector( lua::state &ls, int id )
    {
        return utils::table2pair_vector<T>( ls, id );
    }

    ispi::uint8_vector table2vector( lua::state &ls, int id )
    {
        return utils::table2vector<ispi::uint8_vector>( ls, id );
    }

    template <typename T>
    table_sptr vector2table( const T &values )
    {
        table_sptr res( new_table( ) );

        for( auto &v: values ) {
            res->add( new_integer( v.first ), new_integer( v.second ) );
        }
        return res;
    }

    setup_info get_si_from_lua_table( lua::state &ls, int id )
    {
        setup_info si;

        auto t = ls.get_type( id );
        if( t == base::TYPE_TABLE ) {
            auto tobj = ls.get_object( id );
            for( size_t i=0; i<tobj->count( ); i++ ) {
                auto p(tobj->at(i));
                auto f(p->at(0));
                auto s(p->at(1));

                if( utils::is_number( s ) ) {
                    if( utils::is_number( f ) ) {
                        switch( static_cast<unsigned>(f->num( )) ) {
                        case 1:
                            si.bus   = static_cast<unsigned>(s->num( ));
                            break;
                        case 2:
                            si.chan = static_cast<unsigned>(s->num( ));
                            break;
                        case 3:
                            si.speed = static_cast<unsigned>(s->num( ));
                            break;
                        case 4:
                            si.mode = static_cast<unsigned>(s->num( ));
                            break;
                        }
                    } else if( utils::is_string( f ) ) {
                        std::string name(f->str( ));
                        /// TODO: fix it
                        if( !name.compare( "bus" ) ) {
                            si.bus   = static_cast<unsigned>(s->num( ));
                        } else if( !name.compare( "channel" ) ) {
                            si.chan = static_cast<unsigned>(s->num( ));
                        } else if( !name.compare( "chan" ) ) {
                            si.chan = static_cast<unsigned>(s->num( ));
                        } else if( !name.compare( "speed" ) ) {
                            si.speed = static_cast<unsigned>(s->num( ));
                        } else if( !name.compare( "mode" ) ) {
                            si.mode = static_cast<unsigned>(s->num( ));
                        }
                    }
                }
            }
        } else {
            si.bus   = ls.get_opt<unsigned>( id + 0, 0 );
            si.chan  = ls.get_opt<unsigned>( id + 1, 1 );
            si.speed = ls.get_opt<unsigned>( id + 2, 500000 );
            si.mode  = ls.get_opt<unsigned>( id + 3, 0 );
        }
        return si;
    }

    int lcall_open( lua_State *L )
    {
        module *m = get_module( L );
        lua::state ls(L);

        try {
            auto si = get_si_from_lua_table( ls, 1 );
            m->push_object( L, m->new_dev( si.bus, si.chan,
                                           si.speed, si.mode ) );
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

    int lcall_set_addr( lua_State *L )
    {
        module *m = get_module( L );
        lua::state ls(L);
        utils::handle h = m->get_object_hdl( L, 1 );

        try {
            auto d    = m->get_dev( h );
            auto addr = ls.get_opt<unsigned>( 2 );
            d->set_address( addr );
        } catch( const std::exception &ex ) {
            ls.push( );
            ls.push( ex.what( ) );
            return 2;
        }
        return 1;
    }

    int lcall_set_reg8  ( lua_State *L )
    {
        module *m = get_module( L );
        lua::state ls(L);
        utils::handle h = m->get_object_hdl( L, 1 );

        try {
            auto d    = m->get_dev( h );
            auto vals = table2pair_vector<ispi::cmd_uint8_vector>( ls, 2 );
            d->write_regs8( vals );
        } catch( const std::exception &ex ) {
            ls.push( );
            ls.push( ex.what( ) );
            return 2;
        }
        return 1;
    }

    int lcall_set_reg16 ( lua_State *L )
    {
        module *m = get_module( L );
        lua::state ls(L);
        utils::handle h = m->get_object_hdl( L, 1 );

        try {
            auto d    = m->get_dev( h );
            auto vals = table2pair_vector<ispi::cmd_uint16_vector>( ls, 2 );
            d->write_regs16( vals );
        } catch( const std::exception &ex ) {
            ls.push( );
            ls.push( ex.what( ) );
            return 2;
        }
        return 1;
    }

    int lcall_set_reg32 ( lua_State *L )
    {
        module *m = get_module( L );
        lua::state ls(L);
        utils::handle h = m->get_object_hdl( L, 1 );

        try {
            auto d    = m->get_dev( h );
            auto vals = table2pair_vector<ispi::cmd_uint32_vector>( ls, 2 );
            d->write_regs32( vals );
        } catch( const std::exception &ex ) {
            ls.push( );
            ls.push( ex.what( ) );
            return 2;
        }
        return 1;
    }

    int lcall_set_reg64 ( lua_State *L )
    {
        module *m = get_module( L );
        lua::state ls(L);
        utils::handle h = m->get_object_hdl( L, 1 );

        try {
            auto d    = m->get_dev( h );
            auto vals = table2pair_vector<ispi::cmd_uint64_vector>( ls, 2 );
            d->write_regs64( vals );
        } catch( const std::exception &ex ) {
            ls.push( );
            ls.push( ex.what( ) );
            return 2;
        }
        return 1;
    }

    int lcall_get_reg8  ( lua_State *L )
    {
        module *m = get_module( L );
        lua::state ls(L);
        utils::handle h = m->get_object_hdl( L, 1 );

        try {
            auto d    = m->get_dev( h );
            auto regs = table2vector( ls, 2 );
            auto res = d->read_regs8( regs );
            auto tbl = vector2table( res );
            tbl->push( L );
        } catch( const std::exception &ex ) {
            ls.push( );
            ls.push( ex.what( ) );
            return 2;
        }
        return 1;
    }

    int lcall_get_reg16 ( lua_State *L )
    {
        module *m = get_module( L );
        lua::state ls(L);
        utils::handle h = m->get_object_hdl( L, 1 );

        try {
            auto d    = m->get_dev( h );
            auto regs = table2vector( ls, 2 );
            auto res = d->read_regs16( regs );
            vector2table( res )->push( L );

        } catch( const std::exception &ex ) {
            ls.push( );
            ls.push( ex.what( ) );
            return 2;
        }
        return 1;
    }

    int lcall_get_reg32 ( lua_State *L )
    {
        module *m = get_module( L );
        lua::state ls(L);
        utils::handle h = m->get_object_hdl( L, 1 );

        try {
            auto d    = m->get_dev( h );
            auto regs = table2vector( ls, 2 );
            auto res = d->read_regs32( regs );
            vector2table( res )->push( L );

        } catch( const std::exception &ex ) {
            ls.push( );
            ls.push( ex.what( ) );
            return 2;
        }
        return 1;
    }

    int lcall_get_reg64 ( lua_State *L )
    {
        module *m = get_module( L );
        lua::state ls(L);
        utils::handle h = m->get_object_hdl( L, 1 );

        try {
            auto d    = m->get_dev( h );
            auto regs = table2vector( ls, 2 );
            auto res = d->read_regs64( regs );
            vector2table( res )->push( L );
        } catch( const std::exception &ex ) {
            ls.push( );
            ls.push( ex.what( ) );
            return 2;
        }
        return 1;
    }

    int lcall_write ( lua_State *L )
    {
        module *m = get_module( L );
        lua::state ls(L);
        utils::handle h = m->get_object_hdl( L, 1 );

        try {
            auto d    = m->get_dev( h );
            auto data = ls.get_opt<std::string>( 2 );
            auto ptr  = reinterpret_cast<const unsigned char *>(data.c_str( ));
            ls.push( d->transfer( ptr, data.size( ) ));
        } catch( const std::exception &ex ) {
            ls.push( );
            ls.push( ex.what( ) );
            return 2;
        }

        return 1;
    }

    int lcall_setup ( lua_State *L )
    {
        module *m = get_module( L );
        lua::state ls(L);
        utils::handle h = m->get_object_hdl( L, 1 );

        try {
            auto d  = m->get_dev( h );
            auto si = get_si_from_lua_table( ls, 2 );
            d->setup( si.speed, si.mode );
            return 1;
        } catch( const std::exception &ex ) {
            ls.push( );
            ls.push( ex.what( ) );
            return 2;
        }

        return 1;
    }

}

    iface_sptr create( client::general_info &info )
    {
        return std::make_shared<module>( std::ref( info ) );
    }

}}}}


