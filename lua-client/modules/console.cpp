#include <iostream>

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

    typedef struct position_type {

        unsigned x;
        unsigned y;

        position_type( unsigned xin, unsigned yin )
            :x(xin)
            ,y(yin)
        { }

        position_type( )
            :x(0)
            ,y(0)
        { }

    } position_type;

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

#ifdef _WIN32

    void console_clear( )
    {
        HANDLE sout = GetStdHandle(STD_OUTPUT_HANDLE);
        COORD coord = {0, 0};
        DWORD count;
        CONSOLE_SCREEN_BUFFER_INFO csbi;
        GetConsoleScreenBufferInfo( sout, &csbi );
        FillConsoleOutputCharacter( sout, ' ',
                                    csbi.dwSize.X * csbi.dwSize.Y,
                                    coord, &count);
        SetConsoleCursorPosition( sout, coord );
    }

    position_type get_console_size( )
    {
        CONSOLE_SCREEN_BUFFER_INFO csbi;

        int columns;
        int rows;

        GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);

        columns = csbi.srWindow.Right  - csbi.srWindow.Left + 1;
        rows    = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;

        return position_type( columns, rows );
    }

    void set_cursor_pos( int x, int y )
    {
        COORD coord;

        coord.X = x;
        coord.Y = y;

        SetConsoleCursorPosition( GetStdHandle(STD_OUTPUT_HANDLE), coord );
    }
#else
    position_type get_console_size( )
    {
        struct winsize w;
        ioctl( STDOUT_FILENO, TIOCGWINSZ, &w );
        return position_type( w.ws_col, w.ws_row );
    }

    void set_cursor_pos( int x, int y )
    {
        std::ostringstream oss;
        oss << "\033[" << (y + 1) << ";" << (x + 1) << "H";
        std::string s(oss.str( ));
        std::cout.write( s.c_str( ), s.size( ) );
        //write( STDIN_FILENO, s.c_str( ), s.size( ) );
    }

    void console_clear( )
    {
        auto xy = get_console_size( );
        std::string ws( xy.x, ' ' );
        for( unsigned i=0; i<xy.y; i++ ) {
            set_cursor_pos( 0, i );
            std::cout.write( ws.c_str( ), ws.size( ) );
        }
        // clear command does "write(1, "\E[H\E[2J", 7 )", we don't want
        std::cout.flush( );
    }

    //    void set_console_size( const position_type &cs )
    //    {
    //        struct winsize w = { 0, 0, 0, 0 };
    //        w.ws_col = cs.first;
    //        w.ws_row = cs.second;
    //        ioctl( 0, TIOCSWINSZ, &w);
    //    }

#endif


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

    int lcall_colors    ( lua_State *L );
    int lcall_set_color ( lua_State *L );

    int lcall_set_pos   ( lua_State *L );
    int lcall_size      ( lua_State *L );
    int lcall_clear     ( lua_State *L );

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

            res->add( "colors",       new_function( &lcall_colors ) );
            res->add( "set_color",    new_function( &lcall_set_color ) );
            res->add( "clear",        new_function( &lcall_clear ) );
            res->add( "set_position", new_function( &lcall_set_pos ) );
            res->add( "set_pos",      new_function( &lcall_set_pos ) );
            res->add( "size",         new_function( &lcall_size ) );

            res->add( "set_pattern",  new_function( &lcall_set_pattern ) );
            res->add( "events",       new_function( &lcall_events ) );
            res->add( "subscribe",    new_function( &lcall_subscribe ) );

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

    int lcall_size( lua_State *L )
    {
        auto hw = get_console_size( );
        table res;
        res.add( "width",  new_integer( hw.x ) );
        res.add( "height", new_integer( hw.y ) );
        res.push( L );
        return 1;
    }

    inline bool is_number( const base *o )
    {
        return o->type_id( ) == base::TYPE_NUMBER
            || o->type_id( ) == base::TYPE_INTEGER
            || o->type_id( ) == base::TYPE_UINTEGER
            ;
    }

    inline bool is_string( const base *o )
    {
        return o->type_id( ) == base::TYPE_STRING
            ;
    }

    position_type extract_posision( lua_State *L )
    {
        lua::state ls(L);
        int t = ls.get_type( 1 );

        unsigned x = 0;
        unsigned y = 0;

        position_type size = get_console_size( );
        position_type res;

        switch( t ) {
        case base::TYPE_NUMBER: {
            x = ls.get_opt<unsigned>( 1, 0 );
            y = ls.get_opt<unsigned>( 2, 0 );
            break;
        }
        case base::TYPE_TABLE: {
            objects::base_sptr t = ls.get_object( 1 );
            for( size_t i=0; i<t->count( ); i++ ) {
                auto f = t->at(i)->at(0);
                auto s = t->at(i)->at(1);
                if( is_number( f ) && is_number( s ) ) {
                    switch( static_cast<int>(f->num( )) ) {
                    case 1:
                        x = static_cast<unsigned>(s->num( ));
                        break;
                    case 2:
                        y = static_cast<unsigned>(s->num( ));
                        break;
                    }
                } else if ( is_string( f ) && is_number( s ) ) {
                    std::string name(f->str( ));
                    if( !name.compare( "x" ) ) {
                        x = static_cast<unsigned>(s->num( ));
                    } else if( !name.compare( "y" ) ) {
                        y = static_cast<unsigned>(s->num( ));
                    }
                }
            }
            break;
        }
        }
        res = position_type( std::min( size.x, x ),
                             std::min( size.y, y ) );
        return res;
    }

    int lcall_set_pos( lua_State *L )
    {
        lua::state ls(L);
        auto pos = extract_posision( L );
        set_cursor_pos( pos.x, pos.y );
        ls.push( true );
        return 1;
    }

    int lcall_clear( lua_State *L )
    {
        console_clear( );
        return lcall_set_pos( L );
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

