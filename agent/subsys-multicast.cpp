#include "application.h"
#include "subsys-multicast.h"
#include "subsys-config.h"

//#include "vtrc-memory.h"

namespace fr { namespace agent { namespace subsys {

    namespace {
        const std::string subsys_name( "multicast" );
    }

    struct multicast::impl {

        application     *app_;
        subsys::config  *config_;

        impl( application *app )
            :app_(app)
            ,config_(NULL)
        { }

        void init( )
        {
            config_ = &app_->subsystem<config>( );
        }

        void reg_creator( const std::string &name,
                          application::service_getter_type func )
        {
            app_->register_service_creator( name, func );
        }

        void unreg_creator( const std::string &name )
        {
            app_->unregister_service_creator( name );
        }

    };


    multicast::multicast( application *app )
        :impl_(new impl(app))
    { }

    multicast::~multicast( )
    {
        delete impl_;
    }

    /// static
    vtrc::shared_ptr<multicast> multicast::create( application *app )
    {
        vtrc::shared_ptr<multicast> new_inst(new multicast(app));
        return new_inst;
    }

    const std::string &multicast::name( )  const
    {
        return subsys_name;
    }

    void multicast::init( )
    {
        impl_->init( );
    }

    void multicast::start( )
    {

    }

    void multicast::stop( )
    {

    }


}}}

    
