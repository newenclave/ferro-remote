
#include <map>

#include "application.h"
#include "subsys-i2c.h"

#include "protocol/i2c.pb.h"

#include "vtrc-common/vtrc-closure-holder.h"
#include "vtrc-common/vtrc-exception.h"

#include "vtrc-memory.h"
#include "vtrc-stdint.h"
#include "vtrc-atomic.h"

#include "i2c-helper.h"

#include "vtrc-server/vtrc-channels.h"
#include "vtrc-common/vtrc-mutex-typedefs.h"

namespace fr { namespace agent { namespace subsys {

    namespace {

        const std::string subsys_name( "i2c" );

        namespace i2cproto = fr::proto::i2c;

        namespace vcomm  = vtrc::common;
        namespace vserv  = vtrc::server;

        using vserv::channels::unicast::create_event_channel;
        typedef vtrc::shared_ptr<vcomm::rpc_channel> rpc_channel_sptr;

        typedef vtrc::shared_ptr<agent::i2c_helper> i2c_sptr;
        typedef vtrc::weak_ptr<agent::i2c_helper> i2c_wptr;

        typedef std::map<vtrc::uint32_t, i2c_sptr> device_map;

        class i2c_inst_impl: public fr::proto::i2c::instance {

            device_map                           devices_;
            vtrc::shared_mutex                   devices_lock_;
            vtrc::atomic<vtrc::uint32_t>         index_;

        public:

            i2c_inst_impl( fr::agent::application *     /*app*/,
                           vcomm::connection_iface_wptr /*cli*/ )
                :index_(100)
            { }

            inline vtrc::uint32_t next_index( )
            {
                return ++index_;
            }

            void ping(::google::protobuf::RpcController*    /*controller*/,
                         const ::fr::proto::i2c::empty*     /*request*/,
                         ::fr::proto::i2c::empty*           /*response*/,
                         ::google::protobuf::Closure* done) override
            {
                if( done ) done->Run( ); // does nothing
            }

            void bus_available(::google::protobuf::RpcController* /*cont*/,
                         const ::fr::proto::i2c::bus_available_req* request,
                         ::fr::proto::i2c::bus_available_res* response,
                         ::google::protobuf::Closure* done) override
            {
                vcomm::closure_holder holder( done );
                response->
                      set_value( agent::i2c::available( request->bus_id( ) ) );
            }

            i2c_sptr i2c_by_index( vtrc::uint32_t id )
            {
                vtrc::shared_lock slck(devices_lock_);
                device_map::iterator f(devices_.find(id));
                if( f == devices_.end( ) ) {
                    vcomm::throw_system_error( EINVAL, "Bad I2C handle." );
                }
                return f->second;
            }

            void open(::google::protobuf::RpcController* /*controller*/,
                         const ::fr::proto::i2c::open_req* request,
                         ::fr::proto::i2c::open_res* response,
                         ::google::protobuf::Closure* done) override
            {
                vcomm::closure_holder holder( done );
                unsigned bid = request->bus_id( );
                vtrc::uint32_t next_hdl = next_index( );

                i2c_sptr new_dev( vtrc::make_shared<i2c_helper>( bid ) );
                response->mutable_hdl( )->set_value( next_hdl );

                if( request->has_slave_id( ) ) {
                    unsigned long slv =
                            static_cast<unsigned long>(request->slave_id( ));
                    bool force = request->force_slave( );
                    new_dev->ioctl( force ? I2C_SLAVE_FORCE : I2C_SLAVE, slv );
                }

                vtrc::unique_shared_lock lck(devices_lock_);
                devices_.insert( std::make_pair( next_hdl, new_dev ) );
            }

            void ioctl( ::google::protobuf::RpcController*   /*controller*/,
                         const ::fr::proto::i2c::ioctl_req* request,
                         ::fr::proto::i2c::ioctl_res*       /*response*/,
                         ::google::protobuf::Closure* done ) override
            {
                vcomm::closure_holder holder( done );

                i2c_sptr dev(i2c_by_index( request->hdl( ).value( ) ));
                unsigned long par =
                        static_cast<unsigned long>( request->parameter( ) );

                dev->ioctl( request->code( ), par );
            }

            void read(::google::protobuf::RpcController* controller,
                         const ::fr::proto::i2c::data_block* request,
                         ::fr::proto::i2c::data_block* response,
                         ::google::protobuf::Closure* done) override
            {
                vcomm::closure_holder holder( done );
                i2c_sptr dev(i2c_by_index( request->hdl( ).value( ) ));
                if( 0 == request->length( ) ) {
                    return;
                }
                std::vector<char> data(request->length( ));
                size_t res = dev->read( &data[0], data.size( ) );
                response->set_data( &data[0], res );
            }

            void write(::google::protobuf::RpcController* controller,
                         const ::fr::proto::i2c::data_block* request,
                         ::fr::proto::i2c::data_block* response,
                         ::google::protobuf::Closure* done) override
            {
                vcomm::closure_holder holder( done );
                i2c_sptr dev(i2c_by_index( request->hdl( ).value( ) ));
                const std::string &data(request->data( ));
                size_t res = dev->write( data.empty( ) ? "" : &data[0],
                                         data.size( ));
                response->set_length( res );
            }

            void close(::google::protobuf::RpcController* /*controller*/,
                         const ::fr::proto::i2c::handle* request,
                         ::fr::proto::i2c::empty*         /*response*/,
                         ::google::protobuf::Closure* done) override
            {
                vcomm::closure_holder holder( done );
                vtrc::unique_shared_lock uslck(devices_lock_);
                devices_.erase( request->value( ) );
            }

        public:

            static const std::string &name( )
            {
                return i2cproto::instance::descriptor( )->full_name( );
            }
        };

        application::service_wrapper_sptr create_service(
                                      fr::agent::application *app,
                                      vtrc::common::connection_iface_wptr cl )
        {
            vtrc::shared_ptr<i2c_inst_impl>
                    inst(vtrc::make_shared<i2c_inst_impl>( app, cl ));
            return app->wrap_service( cl, inst );
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
        impl_->reg_creator( i2c_inst_impl::name( ), create_service );
    }

    void i2c::stop( )
    {
        impl_->unreg_creator( i2c_inst_impl::name( ) );
    }

}}}

    