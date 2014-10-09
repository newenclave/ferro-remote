
#include "command-iface.h"
#include "ferro-remote-config.h"

#if FR_WITH_LUA

#include "interfaces/IOS.h"

#define LUA_WRAPPER_TOP_NAMESPACE fr
#include "fr-lua/lua-wrapper.hpp"
#include "fr-lua/lua-objects.hpp"

#include "boost/program_options.hpp"

#include "lua-interface.h"

namespace fr { namespace cc { namespace cmd {

    namespace {

        const char * os_table_name      = "os";
        const char * os_iface_name      = "osinst";

        namespace po = boost::program_options;
        namespace core = client::core;
        namespace lo = fr::lua::objects;

        typedef fr::lua::state lua_state;

        const char *cmd_name = "lua";

        typedef client::interfaces::os::iface os_iface;
        typedef std::shared_ptr<os_iface> os_iface_sptr;

        void init( std::map<std::string, lua::data_sptr> &data,
                   lua_State *L,
                   core::client_core &cl )
        {
            std::map<std::string, lua::data_sptr> tmp;

            typedef std::map<std::string, lua::data_sptr>::const_iterator citr;

            lua_state lv( L );

            tmp.insert(std::make_pair( lua::os::table_name,
                                       lua::os::init( L, cl )));

            lo::table_sptr client_table( lo::new_table( ) );

            for( citr b(tmp.begin( )), e(tmp.end( )); b!=e; ++b ) {
                client_table->add(
                    lo::new_string( b->first ),
                    b->second->get_table( )
                );
            }

            lv.set_object_in_global( lua::names::main_table,
                                     lua::names::client_table,
                                    *client_table );

            data.swap( tmp );

        }

        struct impl: public command_iface {

            const char *name( ) const
            {
                return cmd_name;
            }

            void exec( po::variables_map &vm, core::client_core &cl )
            {
                lua_state lv;
                lua::set_core( lv.get_state( ), &cl );

                std::map<std::string, lua::data_sptr> datas;

                init( datas, lv.get_state( ), cl );

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
    
