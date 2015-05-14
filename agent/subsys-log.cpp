
#include "application.h"
#include "subsys-log.h"

//#include "vtrc-memory.h"

namespace fr { namespace agent { namespace subsys {

    namespace {

        const std::string subsys_name( "log" );

        application::service_wrapper_sptr create_service(
                                      fr::agent::application * /*app*/,
                                      vtrc::common::connection_iface_wptr cl )
        {
            ///vtrc::shared_ptr<impl_type_here>
            ///        inst(vtrc::make_shared<impl_type_here>( app, cl ));
            ///return app->wrap_service( cl, inst );

            return application::service_wrapper_sptr( );
        }
    }

    struct log::impl {

        application     *app_;

        impl( application *app )
            :app_(app)
        { }

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


    log::log( application *app )
        :impl_(new impl(app))
    { }

    log::~log( )
    {
        delete impl_;
    }

    /// static
    vtrc::shared_ptr<log> log::create( application *app )
    {
        vtrc::shared_ptr<log> new_inst(new log(app));
        return new_inst;
    }

    const std::string &log::name( )  const
    {
        return subsys_name;
    }

    void log::init( )
    {

    }

    void log::start( )
    {

    }

    void log::stop( )
    {

    }


}}}

    
