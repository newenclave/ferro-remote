
#include "command-iface.h"
#include "ferro-remote-config.h"

#if FR_WITH_LUA

#include "interfaces/IOS.h"

#define LUA_WRAPPER_TOP_NAMESPACE fr
#include "fr-lua/lua-wrapper.hpp"
#include "fr-lua/lua-objects.hpp"

#include "boost/program_options.hpp"

namespace fr { namespace cc { namespace cmd {

    namespace {

        const std::string main_table_name( "fr" );
        const std::string client_table_name( "client" );
        const std::string os_table_name( "os" );

        const std::string os_iface_name( "osinst" );

        namespace po = boost::program_options;
        namespace core = client::core;
        namespace lo = fr::lua::objects;

        typedef fr::lua::state lua_state;

        const char *cmd_name = "lua";

        typedef client::interfaces::os::iface os_iface;
        typedef std::shared_ptr<os_iface> os_iface_sptr;

        const os_iface *get_os_iface( lua_State *L )
        {
            lua_state lv( L );
            const void *p =
                    lv.get_from_global<const void *>( main_table_name.c_str( ),
                                                      os_iface_name.c_str( ) );
            return reinterpret_cast<const os_iface *>(p);
        }

        int lcall_os_exec( lua_State *L )
        {
            lua_state lv( L );
            std::string command( lv.get<std::string>( ) );
            const os_iface *iface( get_os_iface( L ) );
            iface->execute( command );
        }

        lo::table *os_table( )
        {
            lo::table * ot(lo::new_table( ));
            ot->add( lo::new_string( "system" ),
                     lo::new_function( lcall_os_exec ) );
            return ot;
        }

        struct impl: public command_iface {

            const char *name( ) const
            {
                return cmd_name;
            }

            void exec( po::variables_map &vm, core::client_core &cl )
            {
                lua_state lv;
                os_iface_sptr osi( client::interfaces::os::create( cl ) );
                lv.set_in_global( main_table_name.c_str( ),
                                  os_iface_name.c_str( ), osi.get( ) );

                lo::table_sptr client_tabe( lo::new_table( ) );
                client_tabe->add(
                    lo::new_string( os_table_name ),
                    os_table( )
                )
                ;

                lv.set_object_in_global( main_table_name.c_str( ),
                                         client_table_name.c_str( ),
                                        *client_tabe );

                if( vm.count( "exec" ) ) {
                    std::string script( vm["exec"].as<std::string>( ) );
                    lv.load_file( script.c_str( ) );
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
    
