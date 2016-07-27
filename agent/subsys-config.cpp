
#include "subsys-config.h"
#include "subsys-lua.h"
#include "subsys-logging.h"

#include "application.h"

#include "boost/program_options.hpp"
#include "vtrc-common/vtrc-hash-iface.h"

#include "ferro-remote-config.h"

#include "boost/filesystem.hpp"

#include <iostream>

#include "agent-lua.h"

#define LOG(lev) log_(lev) << "[cfg] "
#define LOGINF   LOG(logger::level::info)
#define LOGDBG   LOG(logger::level::debug)
#define LOGERR   LOG(logger::level::error)
#define LOGWRN   LOG(logger::level::warning)

namespace fr { namespace agent { namespace subsys {

    namespace po = boost::program_options;
    namespace vcomm = vtrc::common;

    namespace {

        const std::string subsys_name( "config" );

        namespace fs = boost::filesystem;

        typedef  std::pair<std::string, std::string> string_pair;

        THREAD_LOCAL config_values l_cfg;
        std::string l_script_path;

        static
        string_pair split_string( const std::string &key )
        {
            std::string::size_type pos = key.find( ':' );

            if( pos == std::string::npos ) {
                return std::make_pair( "", key );
            }

            return std::make_pair(
                        std::string( key.begin( ), key.begin( ) + pos ),
                        std::string( key.begin( ) + pos + 1, key.end( ) ) );
        }

        int lcall_ep_add( lua_State *L )
        {
            fr::lua::state st(L);
            auto s = st.get_opt<std::string>( -1, "" );
            if( !s.empty( ) ) {
                l_cfg.endpoints.push_back( s );
            } else {
                std::cerr << "lua config. Bad endpoint value\n";
            }
            return 0;
        }

        int lcall_lgr_add( lua_State *L )
        {
            fr::lua::state st(L);
            auto s = st.get_opt<std::string>( -1, "" );
            if( !s.empty( ) ) {
                l_cfg.loggers.push_back( s );
            } else {
                std::cerr << "lua config. Bad logger value\n";
            }
            return 0;
        }

        int lcall_thread_set( lua_State *L )
        {
            using namespace fr::lua;
            fr::lua::state st(L);
            auto param = st.get_object( -1 );

            auto ios =  object_by_path( L, param.get( ),  "io" );
            auto rpcs = object_by_path( L, param.get( ), "rpc" );
            auto only = object_by_path( L, param.get( ), "only" );

            if( ios ) {
                l_cfg.io_count = ios->inum( );
            }

            if( rpcs ) {
                l_cfg.rpc_count = rpcs->inum( );
            }

            if( only ) {
                l_cfg.only_pool = !!only->inum( );
            }

            return 0;
        }

        void lua_config_( lua_State *L, const std::string &path )
        {
            using namespace fr::lua;
            fr::lua::state st(L);

            std::unique_ptr<objects::table> ep(new objects::table);
            std::unique_ptr<objects::table> lgr(new objects::table);
            std::unique_ptr<objects::table> pools(new objects::table);

            ep->add( "add", objects::new_function( &lcall_ep_add ) );
            lgr->add( "add", objects::new_function( &lcall_lgr_add ) );
            pools->add( "set", objects::new_function( &lcall_thread_set ) );

            st.set_object( "endpoint", ep.get( ) );
            st.set_object( "logger", lgr.get( ) );
            st.set_object( "pools", pools.get( ) );

            if(0 != st.load_file( path.c_str( ) ) ) {
                std::ostringstream oss;
                oss << "failed to load file " << path << ": "
                    << st.error( );
                throw std::runtime_error( oss.str( ) );
            }
            //st.set
        }

        void lua_config_( const std::string &path )
        {
            using namespace fr::lua;
            fr::lua::state st;
            lua_config_( st.get_state( ), path );
        }

    }

    struct config::impl {

        application                        *app_;
        config_values                       configs_;
        subsys::logging                    *logger_;
        logger                             &log_;
        subsys::lua                        *lua_;

        fr::lua::state                      state_;

        impl( application *app, const config_values &configs )
            :app_(app)
            ,configs_(configs)
            ,logger_(nullptr)
            ,log_(app_->get_logger( ))
            ,lua_(nullptr)
        { }
    };

    void config::all_options( po::options_description &desc )
    {
        using string_list = std::vector<std::string>;
        desc.add_options( )
            ("help,?",   "help message")

            ("log,l", po::value<string_list>( ),
                    "files for log output; use '-' for stdout")

            ("log-level,L", po::value<std::string>( ),
                    "Loglevel: zero, error, warning, info[default], debug")

            ("server,s", po::value<string_list>( ),
                    "endpoint name; <tcp address>:<port> or <file name>")

            ("io-pool-size,i",  po::value<unsigned>( ),
                    "threads for io operations; default = 1")

            ("rpc-pool-size,r", po::value<unsigned>( ),
                    "threads for rpc calls; default = 1")

            ("only-pool,o", "use io pool for io operations and rpc calls")
            ("script,S",    po::value<std::string>(&l_script_path),
                            "lua script for configure server")
            ("key,k", po::value< std::vector< std::string> >( ),
                     "format is: key=id:key; "
                     "key will use for client with this id; "
                     "or key=key for key for any connections")
            ;
    }

    config::config(application *app, const config_values &vm )
        :impl_(new impl(app, vm))
    { }

    config::~config( )
    {
        delete impl_;
    }

    /// static
    vtrc::shared_ptr<config> config::create( application *app,
                                             const config_values &vm )
    {
        vtrc::shared_ptr<config> new_inst(new config(app, vm));
        return new_inst;
    }

    const std::string &config::name( )  const
    {
        return subsys_name;
    }

    void config::init( )
    {
        impl_->logger_ = &impl_->app_->subsystem<subsys::logging>( );
        impl_->lua_ = &impl_->app_->subsystem<subsys::lua>( );
    }

    void config::start( )
    {
        impl_->LOGINF << "Started.";
        impl_->LOGINF << impl_->configs_;
    }

    void config::stop( )
    {
        impl_->LOGINF << "Stopped.";
    }

    config_values config::load_config( const po::variables_map &vm )
    {
        config_values res;

        if( !l_script_path.empty( ) ) {
            lua_config_( l_script_path );
            res = l_cfg;
            l_cfg.clear( );
        }

        if( vm.count( "only-pool" ) ) {
            res.only_pool = true;
        }

        if( vm.count( "io-pool-size" ) ) {
            res.io_count = vm["io-pool-size"].as<unsigned>( );
        }

        if( vm.count( "rpc-pool-size" ) ) {
            res.rpc_count = vm["rpc-pool-size"].as<unsigned>( );
        }

        res.io_count  = res.io_count  ? res.io_count  : 1;
        res.rpc_count = res.rpc_count ? res.rpc_count : 1;

        typedef std::vector<std::string> slist;

        if( vm.count( "server" ) ) {
            res.endpoints = vm["server"].as<slist>( );
        }

        if( vm.count( "log" ) ) {
            res.loggers = vm["log"].as<slist>( );
        }

        if( vm.count( "key" ) ) {
            typedef slist::const_iterator citr;
            std::map<std::string, std::string> tmp;

            vtrc::unique_ptr<vcomm::hash_iface>
                                 s2( vcomm::hash::sha2::create256( ) );

            auto keys = vm["key"].as<slist>( );

            for( citr b(keys.begin( )), e(keys.end( )); b!=e; ++b ) {

                string_pair pair = split_string( *b );

                std::string key_info( pair.first + pair.second );

                std::string hash( s2->get_data_hash( key_info.c_str( ),
                                                     key_info.size( )));

                tmp.insert( std::make_pair( pair.first, hash ) );
            }
            res.key_map.swap( tmp );
        }
        return res;
    }

    config_values &config::cfgs( )
    {
        return impl_->configs_;
    }

    const config_values &config::cfgs( ) const
    {
        return impl_->configs_;
    }

    const std::string &config::script_path( ) const
    {
        return l_script_path;
    }

    void config_values::clear( )
    {
        endpoints.clear( );
        loggers  .clear( );
        key_map  .clear( );
    }

    std::ostream &operator << (std::ostream &o, const config_values &c )
    {
        o << "config: {\n";
        o << "  endoints: {\n";
        for( auto &e: c.endpoints ) {
            o << "    " << e << "\n";
        }
        o << "  }\n";
        o << "  io:  " << c.io_count << " threads\n";
        if( !c.only_pool ) {
            o << "  rpc: " << c.rpc_count << " threads\n";
        }
        if( !c.key_map.empty( ) ) {
            o << "  ids: {\n";
            for( auto &e: c.key_map ) {
                o << "    " << e.first << " = "
                  << (e.second.empty( ) ? "''" : "'********'" ) << "\n";
            }
            o << "  }\n";
        }
        o << "}";
        return o;
    }

}}}
