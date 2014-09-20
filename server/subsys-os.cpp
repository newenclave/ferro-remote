
#include "application.h"
#include "subsys-os.h"
#include "subsys-logger.h"

#include "protocol/os.pb.h"
#include "vtrc-common/vtrc-closure-holder.h"
#include "vtrc-common/vtrc-exception.h"

#include <stdlib.h>

namespace fr { namespace server { namespace subsys {

    namespace {

        namespace vcomm = vtrc::common;

        const std::string subsys_name( "os" );

        class os_proto_impl: public fr::protocol::os::instance {

            void execute(::google::protobuf::RpcController* controller,
                         const ::fr::protocol::os::execute_req* request,
                         ::fr::protocol::os::execute_res* response,
                         ::google::protobuf::Closure* done)
            {
                vcomm::closure_holder holder(done);
                int res = system( request->cmd( ).c_str( ) );
                if( -1 == res ) {
                    throw vtrc::common::exception( errno );
                }
            }
        public:
            static const std::string &name( )
            {
                return fr::protocol::os::instance::descriptor( )->full_name( );
            }
        };

        application::service_wrapper_sptr create_service(
                                          fr::server::application *,
                                          vtrc::common::connection_iface_wptr)
        {
            vtrc::shared_ptr<os_proto_impl>
                                    inst(vtrc::make_shared<os_proto_impl>( ));

            return application::service_wrapper_sptr(
                        new application::service_wrapper(inst) );
        }
    }

    struct os::impl {

        application     *app_;
        subsys::logger  *logger_;

        impl( application *app )
            :app_(app)
        { }
    };


    os::os( application *app )
        :impl_(new impl(app))
    { }

    os::~os( )
    {
        delete impl_;
    }

    /// static
    vtrc::shared_ptr<os> os::create( application *app )
    {
        vtrc::shared_ptr<os> new_inst(new os(app));
        return new_inst;
    }

    const std::string &os::name( )  const
    {
        return subsys_name;
    }

    void os::init( )
    {
        impl_->logger_ = &impl_->app_->subsystem<subsys::logger>( );
    }

    void os::start( )
    {
        impl_->app_->register_service_creator(
                    os_proto_impl::name( ), create_service );
    }

    void os::stop( )
    {
        impl_->app_->unregister_service_creator( os_proto_impl::name( ) );
    }


}}}

    
