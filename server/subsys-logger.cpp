
#include "subsys-logger.h"

#include "application.h"

namespace fr { namespace server { namespace subsys {

    namespace {
        const std::string subsys_name( "logger" );
    }

    struct logger::impl {
        application *app_;
    };

    logger::logger( application *app )
        :impl_(new impl)
    {
        impl_->app_ = app;
    }

    logger::~logger( )
    {
        delete impl_;
    }

    /// static
    vtrc::shared_ptr<logger> logger::create( application *app )
    {
        vtrc::shared_ptr<logger> new_inst(new logger(app));
        return new_inst;
    }

    const std::string &logger::name( )  const
    {
        return subsys_name;
    }

    void logger::init( )
    {

    }

    void logger::start( )
    {

    }

    void logger::stop( )
    {

    }

}}}

    
