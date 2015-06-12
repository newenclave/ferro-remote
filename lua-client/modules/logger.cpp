
#include "iface.h"
#include <map>

#include "../fr-lua.h"
#include "../lua-names.h"

#include "../general-info.h"
#include "../event-container.h"

#include "interfaces/ILogger.h"

#include "../utils.h"

namespace fr { namespace lua { namespace m { namespace logger {

namespace {

    using namespace objects;

    namespace logiface = fr::client::interfaces::logger;
    typedef std::shared_ptr<logiface::iface>     logger_sptr;
    typedef std::map<utils::handle, logger_sptr> logger_map;

    const std::string       module_name("logger");
    const char *id_path   = FR_CLIENT_LUA_HIDE_TABLE ".logger.__i";
    const char *meta_name = FR_CLIENT_LUA_HIDE_TABLE ".logger.meta";


    struct module;

    struct name_level_type {
        const char          *name_;
        logiface::log_level code_;
    };

    const name_level_type levels[ ] = {
         { "ERR",   logiface::error   }
        ,{ "WRN",   logiface::warning }
        ,{ "INF",   logiface::info    }
        ,{ "DBG",   logiface::debug   }
        ,{ nullptr, logiface::info    }
    };

    logiface::log_level str2level( const std::string &name )
    {
        const name_level_type *p = levels;
        do {
            if( 0 == name.compare( p->name_ ) ) {
                return p->code_;
            }
        } while( (++p)->name_ );
        return logiface::info;
    }

    const char * level2str( unsigned code )
    {
        const name_level_type *p = levels;
        do {
            if( code == p->code_ ) {
                return p->name_;
            }
        } while( (++p)->name_ );
        return "INVALID";
    }

    module *get_module( lua_State *L )
    {
        lua::state ls(L);
        void *ptr = ls.get<void *>( id_path );
        return static_cast<module *>(ptr);
    }

    std::vector<std::string> events_names( )
    {
        std::vector<std::string> res;
        res.push_back( "on_write" );
        return res;
    }

    int lcall_add_logger ( lua_State *L );
    int lcall_close      ( lua_State *L );
    int lcall_meta_string( lua_State *L );
    int lcall_write      ( lua_State *L );

    int lcall_events     ( lua_State *L );
    int lcall_subscribe  ( lua_State *L );

    int lcall_set_level  ( lua_State *L );
    int lcall_get_level  ( lua_State *L );

    const struct luaL_Reg logger_lib[ ] = {
         { "__gc",        &lcall_close       }
        ,{ "__tostring",  &lcall_meta_string }
        ,{ "write",       &lcall_write       }
        ,{ "events",      &lcall_events      }
        ,{ "subscribe",   &lcall_subscribe   }
        ,{ nullptr,        nullptr           }
    };

    struct meta_object {
        utils::handle hdl_;
    };

    typedef event_container_sptr eventor_sptr;
    typedef event_container_wptr eventor_wptr;

    void register_metatable( lua_State *L )
    {
        objects::metatable mt( meta_name, logger_lib );
        mt.push( L );
    }

    struct module: public iface {

        client::general_info                  &info_;
        logger_map                             loggers_;
        std::map<utils::handle, eventor_sptr>  events_;
        std::vector<std::string>               events_name_;
        logger_sptr                            main_;

        module( client::general_info &info )
            :info_(info)
            ,events_name_(events_names( ))
        {
            register_metatable( info_.main_ );
        }

        void init( )
        {
            lua::state ls( info_.main_ );
            ls.set( id_path, this );
        }

        utils::handle next_id( )
        {
            return utils::to_handle( info_.eventor_->next_index( ) );
        }

        void reinit( )
        {
            main_.reset( logiface::create( *info_.client_core_ ) );
        }

        void deinit( )
        {

        }

        void on_write( logiface::log_level lvl, const std::string &data,
                       eventor_wptr evtr )
        {
            eventor_sptr le(evtr.lock( ));
            if( le ) {
                FR_LUA_EVENT_PROLOGUE( "on_write", *le );
                result->add( "level", new_integer( lvl ) );
                result->add( "text",  new_string( data ) );
                FR_LUA_EVENT_EPILOGUE;
            }
        }

        eventor_sptr new_eventor( )
        {
            eventor_sptr res = std::make_shared<
                                    lua::event_container
                               >( info_, events_name_ );
            return res;
        }

        eventor_sptr get_eventor( utils::handle hdl )
        {
            auto f(events_.find( hdl ));
            if( f != events_.end( ) ) {
                return f->second;
            } else {
                throw std::runtime_error( "Bad logger." );
            }
        }

        meta_object *add_object( lua_State *L )
        {
            void *ud = lua_newuserdata( L, sizeof(meta_object) );
            meta_object *nfo = static_cast<meta_object *>(ud);
            if( nfo ) {

                logger_sptr lgr(logiface::create( *info_.client_core_ ));
                utils::handle hdl = next_id( );
                luaL_getmetatable( L, meta_name );
                lua_setmetatable(L, -2);

                nfo->hdl_     = hdl;
                loggers_[hdl] = lgr;
                events_[hdl]  = new_eventor( );
            }
            return nfo;
        }

        logger_sptr get_object( lua_State *L, int id )
        {
            void *ud = luaL_testudata( L, id, meta_name );
            meta_object *mo = reinterpret_cast<meta_object *>( ud );
            auto f = loggers_.find( mo ? mo->hdl_ : nullptr );
            if( f != loggers_.end( ) ) {
                return f->second;
            } else {
                throw std::runtime_error( "Bad logger." );
            }
        }

        utils::handle get_object_hdl( lua_State *L, int id )
        {
            void *ud = luaL_testudata( L, id, meta_name );
            if( ud ) {
                return reinterpret_cast<meta_object *>( ud )->hdl_;
            } else {
                throw std::runtime_error( "Bad logger." );
            }
        }

        void close_object( lua_State *L, int id )
        {
            void *ud = luaL_testudata( L, id, meta_name );
            if( ud ) {
                loggers_.erase( ud );
                events_.erase( ud );
            }
        }

        const std::string &name( ) const
        {
            return module_name;
        }

        std::shared_ptr<objects::table> table( ) const
        {
            objects::table_sptr res(std::make_shared<objects::table>( ));

            res->add( "ERR", new_integer( 1 ) );
            res->add( "WRN", new_integer( 2 ) );
            res->add( "INF", new_integer( 3 ) );
            res->add( "DBG", new_integer( 4 ) );
            res->add( "add", new_function( &lcall_add_logger ) );

            res->add( "set_level", new_function( &lcall_set_level ) );
            res->add( "get_level", new_function( &lcall_get_level ) );

            return res;
        }

        bool connection_required( ) const
        {
            return true;
        }
    };

    int lcall_add_logger( lua_State *L )
    {
        lua::state ls(L);
        try {
            module *m = get_module( L );
            m->add_object( L );
        } catch( const std::exception &ex ) {
            ls.push( );
            ls.push( ex.what( ) );
            return 2;
        }
        return 1;
    }

    int lcall_close( lua_State *L )
    {
        lua::state ls(L);
        module *m = get_module( L );
        m->close_object( L, 1 );
        ls.push( true );
        return 1;
    }

    int lcall_meta_string( lua_State *L )
    {
        lua::state ls(L);
        void *ud = luaL_testudata( L, 1, meta_name );
        if( ud ) {
            std::ostringstream oss;
            oss << "logger@" << ud;
            ls.push( oss.str( ) );
        } else {
            ls.push( "Invalid logger." );
        }
        return 1;
    }

    logiface::log_level get_level( lua_State *L, int id )
    {
        lua::state ls(L);
        logiface::log_level lvl = logiface::info;
        if( ls.get_type( id ) == base::TYPE_NUMBER ) {
            lvl = logiface::level_val2enum( ls.get_opt<unsigned>( id ) );
        } else if( ls.get_type( id ) == base::TYPE_STRING ) {
            lvl = str2level( ls.get_opt<std::string>( 2, "INF" ) );
        }
        return lvl;
    }

    int lcall_write( lua_State *L )
    {
        module *m = get_module( L );
        lua::state ls(L);
        try {
            logger_sptr lgr = m->get_object( L, 1 );
            logiface::log_level lvl = get_level( L, 2 );
            std::string data = ls.get_opt<std::string>( 3 );
            lgr->write( lvl, data );
            ls.push( true );
        } catch( const std::exception &ex ) {
            ls.push( false );
            ls.push( ex.what( ) );
            return 2;
        }
        return 1;
    }

    int lcall_events( lua_State *L )
    {
        module *m = get_module( L );
        lua::state ls( L );
        utils::handle h = m->get_object_hdl( L, 1 );

        try {
            return m->get_eventor( h )->push_state( L );
        } catch( const std::exception &ex ) {
            ls.push( false );
            ls.push( ex.what( ) );
            return 2;
        }
        return 1;
    }

    int lcall_subscribe( lua_State *L )
    {
        return 0;
    }

    int lcall_set_level( lua_State *L )
    {
        module *m = get_module( L );
        lua::state ls(L);
        logiface::log_level lvl = get_level( L, 1 );
        m->main_->set_level( lvl );
        ls.push( true );
        return 1;
    }

    int lcall_get_level( lua_State *L )
    {
        module *m = get_module( L );
        lua::state ls(L);
        try {
            ls.push( m->main_->get_level( ) );
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

