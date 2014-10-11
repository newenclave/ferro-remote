
#include "command-iface.h"
#include "ferro-remote-config.h"

#if FR_WITH_LUA

#include <iostream>
#include "interfaces/IOS.h"

#define LUA_WRAPPER_TOP_NAMESPACE fr
#include "fr-lua/lua-wrapper.hpp"
#include "fr-lua/lua-objects.hpp"

#include "boost/program_options.hpp"

#include "lua-interface.h"

namespace fr { namespace cc { namespace cmd {

    namespace {

        namespace po = boost::program_options;
        namespace core = client::core;
        namespace lo = fr::lua::objects;

        typedef fr::lua::state lua_state;

        const char *cmd_name = "lua";

        typedef client::interfaces::os::iface os_iface;
        typedef std::shared_ptr<os_iface> os_iface_sptr;

        lo::base_sptr get_object( lua_State *L, int idx );

        lo::base_sptr get_table( lua_State *L, int idx )
        {
            lua_pushvalue( L, idx );
            lua_pushnil( L );

            lo::table_sptr new_table( lo::new_table( ) );

            while ( lua_next( L, -2 ) ) {
                lua_pushvalue( L, -2 );
                lo::pair_sptr new_pair
                        ( lo::new_pair( get_object( L, -1 ),
                                        get_object( L, -2 ) ) );
                new_table->push_back( new_pair );
                lua_pop( L, 2 );
            }

            lua_pop( L, 1 );
            return new_table;
        }

        lo::base_sptr get_string( lua_State *L, int idx )
        {
            size_t length = 0;
            const char *ptr = lua_tolstring( L, idx, &length );
            return lo::base_sptr( new lo::string( ptr, length ) );
        }

        lo::base_sptr get_object( lua_State *L, int idx )
        {
            int t = lua_type( L, idx );
            switch( t ) {
            case LUA_TBOOLEAN:
                return lo::base_sptr(
                            new lo::boolean( lua_toboolean( L, idx ) ));
            case LUA_TLIGHTUSERDATA:
                return lo::base_sptr(
                            new lo::light_userdata( lua_touserdata( L, idx ) ));
            case LUA_TNUMBER:
                return lo::base_sptr(
                            new lo::number( lua_tonumber( L, idx ) ));
            case LUA_TSTRING:
                return get_string( L, idx );
            case LUA_TFUNCTION:
                return lo::base_sptr(
                            new lo::function( lua_tocfunction( L, idx ) ));
            case LUA_TTABLE:
                return get_table( L, idx );
        //    case LUA_TUSERDATA:
        //        return "userdata";
        //    case LUA_TTHREAD:
        //        return "thread";
            }
            return lo::base_sptr( new lo::nil );
        }

        int global_print( lua_State *L )
        {
            int n = lua_gettop( L );

            lua::state lv( L );

            if( n < 0 ) {
                n = ( (n * -1) + 1 );
            }

            for ( int b = 1; b <= n; ++b ) {
                std::cout << get_object( lv.get_state( ), b )->str( );
            }
            lv.pop( n );

            return 0;
        }

        int global_println( lua_State *L )
        {
            int res = global_print( L );
            std::cout << "\n";
            return res;
        }

#define FR_INTERFACE_PAIR( ns, L, CL )      \
    std::make_pair( lua::ns::table_name, lua::ns::init( L, cl ))

        void init( std::map<std::string, lua::data_sptr> &data,
                   lua_State *L,
                   core::client_core &cl )
        {
            std::map<std::string, lua::data_sptr> tmp;

            typedef std::map<std::string, lua::data_sptr>::const_iterator citr;

            lua_state lv( L );

            tmp.insert( FR_INTERFACE_PAIR( os, L, cl ) );
            tmp.insert( FR_INTERFACE_PAIR( fs, L, cl ) );

            lo::table_sptr client_table( lo::new_table( ) );

            for( citr b(tmp.begin( )), e(tmp.end( )); b!=e; ++b ) {
                client_table->add(
                    lo::new_string( b->first ),
                    b->second->get_table( )
                );
            }

            lv.set_object( lua::names::client_table, client_table.get( ) );
            data.swap( tmp );

        }

#undef FR_INTERFACE_PAIR

        void register_printer( lua_State *L )
        {
            lua_state lv( L );

            lv.register_call( "print",   &global_print );
            lv.register_call( "println", &global_println );
        }

        struct impl: public command_iface {

            const char *name( ) const
            {
                return cmd_name;
            }

            void exec( po::variables_map &vm, core::client_core &cl )
            {
                lua_state lv;
                register_printer( lv.get_state( ) );

                std::map<std::string, lua::data_sptr> datas;

                init( datas, lv.get_state( ), cl );
                lua::set_core( lv.get_state( ), &cl );

                if( vm.count( "exec" ) ) {
                    std::string script( vm["exec"].as<std::string>( ) );
                    lv.load_file( script.c_str( ) );
                    if( vm.count( "func" ) )  {
                        std::string func( vm["func"].as<std::string>( ) );
                        lv.exec_function( func.c_str( ) );
                    }
                    //lv.exec_function( );
                }
            }

            void add_options( po::options_description &desc )
            {
                /// reserved as common
                /// "help,h"
                /// "command,c"
                /// "server,s"
                /// "io-pool-size,i"
                /// "rpc-pool-size,r"
                desc.add_options( )
                    ( "exec,e", po::value<std::string>( ),
                                              "execute the script" )
                    ( "func,f", po::value<std::string>( ),
                                              "call function from the script" )
                    ;
            }

            std::string help( ) const
            {
                return std::string( );
            }

            std::string desc( ) const
            {
                return std::string( "lua command" );
            }

            bool need_connect( ) const
            {
                return true;
            }

        };
    }

    namespace lua {
        command_sptr create( )
        {
            return command_sptr( new impl );
        }
    }

}}}

#endif
    
