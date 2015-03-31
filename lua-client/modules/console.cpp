#include "iface.h"
#include "../fr-lua.h"
#include "../lua-names.h"

#include "../general-info.h"
#include "../utils.h"
#include "../event-container.h"
#include "vtrc-common/vtrc-pool-pair.h"

#include "boost/regex.hpp"
#include "boost/asio/streambuf.hpp"
#include "boost/asio/placeholders.hpp"
#include "boost/asio/read_until.hpp"

#include <functional>

namespace fr { namespace lua { namespace m { namespace console {

namespace {

    using namespace objects;

    namespace ba = boost::asio;
    namespace bs = boost::system;

    typedef utils::console_handle           console_handle;
    typedef std::shared_ptr<console_handle> console_handle_sptr;
    typedef std::weak_ptr<console_handle>   console_handle_wptr;

    const std::string     module_name("console");
    const char *id_path = FR_CLIENT_LUA_HIDE_TABLE ".console.__i";

    struct module;

    module *get_module( lua_State *L )
    {
        lua::state ls(L);
        void *ptr = ls.get<void *>( id_path );
        return static_cast<module *>(ptr);
    }

    std::vector<std::string> events_names( )
    {
        std::vector<std::string> res;
        res.push_back( "on_read" );
        return res;
    }

    typedef std::ostream &(*func_type)( std::ostream & );

    static const struct  {
        const char *name_;
        func_type   call_;
    } color_names[ ] = {
         { "light"  ,&utils::ccout::light  }
        ,{ "red"    ,&utils::ccout::red    }
        ,{ "green"  ,&utils::ccout::green  }
        ,{ "blue"   ,&utils::ccout::blue   }
        ,{ "cyan"   ,&utils::ccout::cyan   }
        ,{ "yellow" ,&utils::ccout::yellow }
        ,{ "none"   ,&utils::ccout::none   }
        ,{ nullptr  ,&utils::ccout::none   }
    };

    int lcall_colors   ( lua_State *L );
    int lcall_set_color( lua_State *L );

    int lcall_set_pattern   ( lua_State *L );
    int lcall_events        ( lua_State *L );
    int lcall_subscribe     ( lua_State *L );

    struct module: public iface {

        client::general_info &info_;
        event_container       events_;
        console_handle_sptr   con_reader_;
        boost::regex          rex_;
        ba::streambuf         inp_;
        std::vector<char>     block_;

        module( client::general_info &info )
            :info_(info)
            ,events_(info_, events_names( ))
            ,rex_("\n")
            ,block_(1024)
        { }

        void init( )
        {
            lua::state ls( info_.main_ );
            ls.set( id_path, this );
        }

        void reinit( )
        {

        }

        void deinit( )
        {

        }

        void start_read( console_handle_sptr cr )
        {
            namespace ph = std::placeholders;
            if( cr ) {
                if( !rex_.empty( ) ) {
                    ba::async_read_until( cr->hdl( ), inp_, rex_,
                            std::bind( &module::reader_handler, this,
                                        ph::_1, ph::_2, true,
                                        console_handle_wptr( cr ) ) );
                } else {
                    cr->hdl( ).async_read_some( ba::buffer(block_),
                            std::bind( &module::reader_handler, this,
                                        ph::_1, ph::_2, false,
                                        console_handle_wptr( cr ) ) );
                }
            }
        }

        void reader_handler( const bs::error_code &err, size_t size,
                             bool pattern, console_handle_wptr cr )
        {
            FR_LUA_EVENT_PROLOGUE( "on_read", events_ );
            if( err ) {
                result->add( "error", new_string( err.message( ) ) );
            } else {
                if( pattern ) {
                    const char *head =
                                ba::buffer_cast<const char *>( inp_.data( ) );
                    result->add( "data", new_string( head, size ) );
                    inp_.consume( size );
                } else {
                    const char *head = &block_[0];
                    result->add( "data", new_string( head, size ) );
                }
                start_read( cr.lock( ) );
            }
            FR_LUA_EVENT_EPILOGUE
        }

        void set_pattern( const std::string &val )
        {
            boost::regex t(val);
            rex_.swap( t );
        }

        void subscribe( )
        {
            con_reader_ = std::make_shared<console_handle>
                        ( std::ref(info_.pp_->get_io_service( )) );
            start_read( con_reader_ );
        }

        void unsubscribe( )
        {
            con_reader_.reset( );
        }

        const std::string &name( ) const
        {
            return module_name;
        }

        std::shared_ptr<objects::table> table( ) const
        {
            objects::table_sptr res(std::make_shared<objects::table>( ));

            res->add( "colors",     new_function( &lcall_colors ) );
            res->add( "set_color",  new_function( &lcall_set_color ) );

            res->add( "set_pattern",    new_function( &lcall_set_pattern ) );
            res->add( "events",         new_function( &lcall_events ) );
            res->add( "subscribe",      new_function( &lcall_subscribe ) );

            return res;
        }

        bool connection_required( ) const
        {
            return false;
        }
    };

    int lcall_colors( lua_State *L )
    {
        table res;
        for( auto *p = color_names; p->name_; ++p ) {
            res.add( new_string( p->name_ ) );
        }
        res.push( L );
        return 1;
    }

    int lcall_set_color( lua_State *L )
    {
        lua::state ls(L);
        std::string color = ls.get_opt<std::string>( 1, "none" );
        bool found = false;
        for( auto *p = color_names; !found && p->name_; ++p ) {
            if( !color.compare( p->name_ ) ) {
                std::cout << p->call_;
                found = true;
            }
        }

        ls.push( found );

        if( !found ) {
            ls.push( "Invalid color name." );
            return 2;
        }

        return 1;
    }

    int lcall_set_pattern( lua_State *L )
    {
        lua::state ls(L);
        module *m = get_module( L );
        m->set_pattern( ls.get_opt<std::string>( 1 ) );
        ls.push( true );
        return 1;
    }

    int lcall_events( lua_State *L )
    {
        module *m = get_module( L );
        return m->events_.push_state( L );
    }

    int lcall_subscribe( lua_State *L )
    {
        module *m = get_module( L );
        event_container::subscribe_info si;
        m->events_.subscribe( L, 0, &si );
        if( !si.name_.compare( "on_read" ) ) {
            if( si.call_ )  {
                m->subscribe( );
            } else {
                m->unsubscribe( );
            }
        }
        return si.result_;
    }

}

    iface_sptr create( client::general_info &info )
    {
        return std::make_shared<module>( std::ref( info ) );
    }

}}}}

