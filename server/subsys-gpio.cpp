
#include "application.h"
#include "subsys-gpio.h"
#include "subsys-reactor.h"

#include "protocol/gpio.pb.h"

#include "gpio.h"

namespace fr { namespace server { namespace subsys {

    namespace {
        const std::string subsys_name( "gpio" );
        namespace gproto = fr::protocol::gpio;

        class gpio_impl: public gproto::instance {

        public:
            static const std::string &name( )
            {
                return gproto::instance::descriptor( )->full_name( );
            }
        };

        application::service_wrapper_sptr create_service(
                                      fr::server::application *app,
                                      vtrc::common::connection_iface_wptr cli )
        {
            vtrc::shared_ptr<gpio_impl> inst( vtrc::make_shared<gpio_impl>( ) );

            return application::service_wrapper_sptr( );
        }
    }

    struct gpio::impl {

        application     *app_;
        subsys::reactor *reactor_;

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


    gpio::gpio( application *app )
        :impl_(new impl(app))
    { }

    gpio::~gpio( )
    {
        delete impl_;
    }

    /// static
    vtrc::shared_ptr<gpio> gpio::create( application *app )
    {
        vtrc::shared_ptr<gpio> new_inst(new gpio(app));
        return new_inst;
    }

    const std::string &gpio::name( )  const
    {
        return subsys_name;
    }

    void gpio::init( )
    {
        impl_->reactor_ = &impl_->app_->subsystem<subsys::reactor>( );
    }

    void gpio::start( )
    {
        impl_->reg_creator( gpio_impl::name( ),  create_service );
        std::cout << __LINE__ << "\n";
        gpio_inst i(74);
        std::cout << __LINE__ << "\n";
        i.exp(  );
        std::cout << __LINE__ << "\n";
        i.set_direction( server::gpio::DIRECT_OUT );
        std::cout << __LINE__ << "\n";
        i.set_value( 1 );
        std::cout << __LINE__ << "\n";
        i.unexp( );
        std::cout << __LINE__ << "\n";
    }

    void gpio::stop( )
    {
        impl_->unreg_creator( gpio_impl::name( ) );
    }

}}}

    
