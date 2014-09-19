
#include "subsys-config.h"


namespace fr { namespace server { namespace subsys {

    namespace {
        const std::string subsys_name( "config" );
    }

    config::config( application *app )
    { }

    /// static
    vtrc::shared_ptr<config> config::create(application *app,
                             const boost::program_options::variables_map &vm )
    {
        vtrc::shared_ptr<config> new_inst(new config(app));
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


}}}
