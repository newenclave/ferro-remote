
#include "subsys-config.h"

#include "boost/program_options.hpp"

namespace fr { namespace agent { namespace subsys {

    namespace po = boost::program_options;

    namespace {
        const std::string subsys_name( "config" );
    }

    struct config::impl {
        application                *app_;
        po::variables_map           vm_;
        std::vector<std::string>    endpoints_;

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
        }
    };

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

}}}
