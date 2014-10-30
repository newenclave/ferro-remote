
#include "application.h"
#include "subsys-i2c.h"

#include "protocol/i2c.pb.h"

#include "vtrc-common/vtrc-closure-holder.h"
#include "vtrc-memory.h"

#include "i2c-helper.h"

namespace fr { namespace agent { namespace subsys {

    namespace {

        namespace i2cproto = fr::proto::i2c;

        namespace vcomm  = vtrc::common;
        namespace vserv  = vtrc::server;

        const std::string subsys_name( "i2c" );

        class i2c_inst_impl: public fr::proto::i2c::instance {

            void ping(::google::protobuf::RpcController*    /*controller*/,
                         const ::fr::proto::i2c::empty*     /*request*/,
                         ::fr::proto::i2c::empty*           /*response*/,
                         ::google::protobuf::Closure* done) override
            {
                if( done ) done->Run( ); // does nothing
            }

            void bus_available(::google::protobuf::RpcController* controller,
                         const ::fr::proto::i2c::bus_available_req* request,
                         ::fr::proto::i2c::bus_available_res* response,
                         ::google::protobuf::Closure* done) override
            {
                vcomm::closure_holder holder( done );
                response->
                      set_value( agent::i2c::available( request->bus_id( ) ) );
            }
        };

        application::service_wrapper_sptr create_service(
                                      fr::agent::application * /*app*/,
                                      vtrc::common::connection_iface_wptr cl )
        {
            ///vtrc::shared_ptr<impl_type_here>
            ///        inst(vtrc::make_shared<impl_type_here>( app, cl ));

            return application::service_wrapper_sptr( );
        }
    }

    struct i2c::impl {

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


    i2c::i2c( application *app )
        :impl_(new impl(app))
    { }

    i2c::~i2c( )
    {
        delete impl_;
    }

    /// static
    vtrc::shared_ptr<i2c> i2c::create( application *app )
    {
        vtrc::shared_ptr<i2c> new_inst(new i2c(app));
        return new_inst;
    }

    const std::string &i2c::name( )  const
    {
        return subsys_name;
    }

    void i2c::init( )
    {

    }

    void i2c::start( )
    {

    }

    void i2c::stop( )
    {

    }


}}}

    
