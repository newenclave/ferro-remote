
#include "application.h"
#include "subsys-lua.h"

//#include "vtrc-memory.h"

namespace fr { namespace agent { namespace subsys {

    namespace {
        const std::string subsys_name( "lua" );

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

    struct lua::impl {

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


    lua::lua( application *app )
        :impl_(new impl(app))
    { }

    lua::~lua( )
    {
        delete impl_;
    }

    /// static
    vtrc::shared_ptr<lua> lua::create( application *app )
    {
        vtrc::shared_ptr<lua> new_inst(new lua(app));
        return new_inst;
    }

    const std::string &lua::name( )  const
    {
        return subsys_name;
    }

    void lua::init( )
    {

    }

    void lua::start( )
    {

    }

    void lua::stop( )
    {

    }


}}}

    