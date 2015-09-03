
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>

#include "application.h"
#include "subsys-spi.h"
#include "subsys-log.h"

#include "protocol/spi.pb.h"

#include "vtrc-memory.h"

#define LOG(lev) log_(lev) << "[spi] "
#define LOGINF   LOG(logger::info)
#define LOGDBG   LOG(logger::debug)
#define LOGERR   LOG(logger::error)
#define LOGWRN   LOG(logger::warning)

namespace fr { namespace agent { namespace subsys {

    namespace {

        const std::string subsys_name( "spi" );

        namespace vcomm = vtrc::common;
        typedef fr::proto::spi::instance proto_instance;

        class proto_impl: public proto_instance {

            application                  *app_;
            vcomm::connection_iface_wptr  client_;

        public:

            proto_impl( application *app, vcomm::connection_iface_wptr client )
                :app_(app)
                ,client_(client)
            { }

            static const std::string &name( )
            {
                return proto_instance::descriptor( )->full_name( );
            }
        };

        application::service_wrapper_sptr create_service(
                                      fr::agent::application * app,
                                      vcomm::connection_iface_wptr cl )
        {
            vtrc::shared_ptr<proto_impl>
                            inst( vtrc::make_shared<proto_impl>( app, cl ) );
            return app->wrap_service( cl, inst );
        }
    }

    struct spi::impl {

        application     *app_;
        logger          &log_;

        impl( application *app )
            :app_(app)
            ,log_(app_->subsystem<subsys::log>( ).get_logger( ))
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


    spi::spi( application *app )
        :impl_(new impl(app))
    { }

    spi::~spi( )
    {
        delete impl_;
    }

    /// static
    vtrc::shared_ptr<spi> spi::create( application *app )
    {
        vtrc::shared_ptr<spi> new_inst(new spi(app));
        return new_inst;
    }

    const std::string &spi::name( )  const
    {
        return subsys_name;
    }

    void spi::init( )
    {

    }

    void spi::start( )
    {
        impl_->LOGINF << "Started.";
    }

    void spi::stop( )
    {
        impl_->LOGINF << "Stopped.";
    }


}}}

    
