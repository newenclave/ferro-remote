
#include "application.h"
#include "subsys-lua.h"

#if FR_WITH_LUA

#include "agent-lua.h"
#include "subsys-config.h"

#include "boost/filesystem.hpp"

//#include "vtrc-memory.h"

namespace fr { namespace agent { namespace subsys {

    namespace {

        const std::string subsys_name( "lua" );

        namespace frlua = ::fr::lua;
        namespace fs = boost::filesystem;

        application::service_wrapper_sptr create_service(
                                  fr::agent::application * /*app*/,
                                  vtrc::common::connection_iface_wptr /*cl*/ )
        {
            ///vtrc::shared_ptr<impl_type_here>
            ///        inst(vtrc::make_shared<impl_type_here>( app, cl ));
            ///return app->wrap_service( cl, inst );

            return application::service_wrapper_sptr( );
        }
    }

    struct lua::impl {

        application     *app_;
        config          *config_;
        frlua::state     state_;

        impl( application *app )
            :app_(app)
            ,config_(nullptr)
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
        impl_->config_ = &impl_->app_->subsystem<config>( );
        const std::string &spath(impl_->config_->script_path( ));
        try {
            if( fs::exists(spath) && fs::is_regular_file( spath ) ) {

                impl_->state_.check_call_error(
                            impl_->state_.load_file( spath.c_str( ) ) );
            } else {
                std::cerr << "[lua] invalid path " << spath
                          << " for the script \n";
            }
        } catch( const std::exception &ex ) {
            std::cerr << "[lua] load " << spath << " failed: "
                      << ex.what( ) << "\n";
        }
    }

    void lua::start( )
    {

    }

    void lua::stop( )
    {

    }


}}}

#endif
