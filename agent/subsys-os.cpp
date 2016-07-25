
#include "application.h"
#include "subsys-os.h"
#include "subsys-logging.h"

#include "protocol/os.pb.h"
#include "vtrc-common/vtrc-closure-holder.h"
#include "vtrc-common/vtrc-exception.h"

#include <stdlib.h>
#include <stdint.h>


#define LOG(lev) log_(lev) << "[os] "
#define LOGINF   LOG(logger::level::info)
#define LOGDBG   LOG(logger::level::debug)
#define LOGERR   LOG(logger::level::error)
#define LOGWRN   LOG(logger::level::warning)

namespace fr { namespace agent { namespace subsys {

    namespace {

        namespace vcomm = vtrc::common;

        const std::string subsys_name( "os" );

        class os_proto_impl: public fr::proto::os::instance {

            void execute(::google::protobuf::RpcController* /*controller*/,
                         const ::fr::proto::os::execute_req* request,
                         ::fr::proto::os::execute_res* response,
                         ::google::protobuf::Closure* done) override
            {
                vcomm::closure_holder holder(done);
                int res = system( request->cmd( ).c_str( ) );
                if( -1 == res ) {
                    throw vtrc::common::exception( errno );
                }
                response->set_result( res );
            }

            void byte_order(::google::protobuf::RpcController*  /*controller*/,
                         const ::fr::proto::os::byte_order_req* /*request*/,
                         ::fr::proto::os::byte_order_res* response,
                         ::google::protobuf::Closure* done) override
            {
                vcomm::closure_holder holder(done);
                response->set_big_endian( (*(uint16_t *)"\0\xff") < 0x100 );
            }

        public:
            static const std::string &name( )
            {
                return fr::proto::os::instance::descriptor( )->full_name( );
            }
        };

        application::service_wrapper_sptr create_service(
                                      fr::agent::application *app,
                                      vtrc::common::connection_iface_wptr cl )
        {
            auto inst(vtrc::make_shared<os_proto_impl>( ));
            return app->wrap_service( cl, inst );
        }
    }

    struct os::impl {

        application  *app_;
        logger       &log_;

        impl( application *app )
            :app_(app)
            ,log_(app_->get_logger( ))
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

    }

    void os::start( )
    {
        impl_->app_->register_service_creator(
                    os_proto_impl::name( ), create_service );
        impl_->LOGINF << "Started.";
    }

    void os::stop( )
    {
        impl_->app_->unregister_service_creator( os_proto_impl::name( ) );
        impl_->LOGINF << "Stopped.";
    }


}}}

    
