
#include "subsys-config.h"

#include "boost/program_options.hpp"
#include "vtrc-common/vtrc-hash-iface.h"

#include "ferro-remote-config.h"


namespace fr { namespace agent { namespace subsys {

    namespace po = boost::program_options;
    namespace vcomm = vtrc::common;

    namespace {

        const std::string subsys_name( "config" );

        typedef  std::pair<std::string, std::string> string_pair;

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

    }

    struct config::impl {
        application                        *app_;
        po::variables_map                   vm_;
        std::vector<std::string>            endpoints_;
        std::map<std::string, std::string>  keys_;

        impl( application *app, po::variables_map vm )
            :app_(app)
            ,vm_(vm)
        {
            init_variables( );
        }

        void init_variables( )
        {
            if( vm_.count( "server" ) ) {
                typedef std::vector<std::string> slist;
                endpoints_ = vm_["server"].as<slist>( );
            }

            if( vm_.count( "key" ) ) {
                typedef std::vector<std::string>::const_iterator citr;

                vtrc::unique_ptr<vcomm::hash_iface>
                                     s2( vcomm::hash::sha2::create256( ) );

                std::vector<std::string> keys =
                        vm_["key"].as<std::vector<std::string> >( );

                for( citr b(keys.begin( )), e(keys.end( )); b!=e; ++b ) {

                    string_pair pair = split_string( *b );

                    std::string key_info( pair.first + pair.second );

                    std::string hash( s2->get_data_hash( key_info.c_str( ),
                                                         key_info.size( )));

                    keys_.insert( std::make_pair( pair.first, hash ) );
                }
            }

        }
    };

    void config::all_options( po::options_description &desc )
    {
        desc.add_options( )
            ("help,?",   "help message")
            ("server,s", po::value< std::vector< std::string> >( ),
                    "endpoint name; <tcp address>:<port> or <file name>")

            ("io-pool-size,i",  po::value<unsigned>( ),
                    "threads for io operations; default = 1")

            ("rpc-pool-size,r", po::value<unsigned>( ),
                    "threads for rpc calls; default = 1")

            ("only-pool,o", "use io pool for io operations and rpc calls")
#if FR_WITH_LUA
            ("script,S",    po::value<std::string>(&l_script_path),
                            "lua script for configure server")
#endif
            ("key,k", po::value< std::vector< std::string> >( ),
                     "format is: key=id:key; "
                     "key will use for client with this id; "
                     "or key=key for key for any connections")
            ;
    }

    config::config( application *app, const po::variables_map &vm )
        :impl_(new impl(app, vm))
    { }

    config::~config( )
    {
        delete impl_;
    }

    /// static
    vtrc::shared_ptr<config> config::create( application *app,
                                             const po::variables_map &vm )
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

    }

    void config::start( )
    {

    }

    void config::stop( )
    {

    }

    const po::variables_map &config::variables( ) const
    {
        return impl_->vm_;
    }

    const std::vector<std::string> &config::endpoints( ) const
    {
        return impl_->endpoints_;
    }

    void  config::set_endpoints( std::vector<std::string> const &ep ) const
    {
        impl_->endpoints_.assign( ep.begin( ), ep.end( ) );
    }

    const std::map<std::string, std::string> &config::id_keys( ) const
    {
        return impl_->keys_;
    }

    const std::string &config::script_path( ) const
    {
        return l_script_path;
    }

}}}
