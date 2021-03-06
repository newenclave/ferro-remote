
#include <map>

#include "application.h"
#include "subsys-i2c.h"
#include "subsys-logging.h"

#include "protocol/i2c.pb.h"

#include "vtrc/common/closure-holder.h"
#include "vtrc/common/exception.h"

#include "vtrc-memory.h"
#include "vtrc-stdint.h"
#include "vtrc-atomic.h"
#include "vtrc-mutex.h"

#include "i2c-helper.h"

#include "vtrc/server/channels.h"

#define LOG(lev) log_(lev) << "[i2c] "
#define LOGINF   LOG(logger::level::info)
#define LOGDBG   LOG(logger::level::debug)
#define LOGERR   LOG(logger::level::error)
#define LOGWRN   LOG(logger::level::warning)

namespace fr { namespace agent { namespace subsys {

    namespace {

        const std::string subsys_name( "i2c" );

        namespace i2cproto = fr::proto::i2c;

        namespace vcomm  = vtrc::common;
        namespace vserv  = vtrc::server;
        namespace gpb    = google::protobuf;

        using vserv::channels::unicast::create_event_channel;
        using rpc_channel_sptr = vtrc::shared_ptr<vcomm::rpc_channel>;
        using i2c_sptr = vtrc::shared_ptr<agent::i2c_helper>;
        using i2c_wptr = vtrc::weak_ptr<agent::i2c_helper>;

        using device_map = std::map<vtrc::uint32_t, i2c_sptr>;

        class i2c_inst_impl: public fr::proto::i2c::instance {

            device_map                   devices_;
            vtrc::mutex                  devices_lock_;
            vtrc::atomic<vtrc::uint32_t> index_;

            typedef vtrc::lock_guard<vtrc::mutex> locker_type;

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
                         const ::fr::proto::empty*          /*request*/,
                         ::fr::proto::empty*                /*response*/,
                         ::google::protobuf::Closure* done) override
            {
                // does nothing
                if( done ) { done->Run( ); }
            }

            void bus_available(::google::protobuf::RpcController* /*cont*/,
                         const ::fr::proto::i2c::bus_available_req* request,
                         ::fr::proto::i2c::bus_available_res* response,
                         ::google::protobuf::Closure* done) override
            {
                vcomm::closure_holder holder( done );
                const unsigned bus_id = request->bus_id( );
                response->set_value( agent::i2c::available( bus_id ) );
            }

            i2c_sptr i2c_by_index( vtrc::uint32_t id )
            {
                locker_type slck(devices_lock_);
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

                if( request->has_setup( ) ) {
                    auto slv = static_cast<
                                    unsigned long
                                >(request->setup( ).slave_id( ));
                    bool force = request->setup( ).force_slave( );
                    new_dev->set_address( slv, force );
                }

                locker_type lck(devices_lock_);
                devices_.insert( std::make_pair( next_hdl, new_dev ) );
            }

            void setup(::google::protobuf::RpcController*   /*controller*/,
                         const ::fr::proto::i2c::setup_req* request,
                         ::fr::proto::i2c::setup_res*       /*response*/,
                         ::google::protobuf::Closure* done) override
            {
                vcomm::closure_holder holder( done );
                i2c_sptr dev(i2c_by_index( request->hdl( ).value( ) ));
                auto addr = static_cast<
                                unsigned long
                            >( request->setup( ).slave_id( ) );
                auto force = request->setup( ).force_slave( );

                dev->set_address( addr, force );
            }


            void ioctl( ::google::protobuf::RpcController*   /*controller*/,
                         const ::fr::proto::i2c::ioctl_req* request,
                         ::fr::proto::i2c::ioctl_res*       /*response*/,
                         ::google::protobuf::Closure* done ) override
            {
                vcomm::closure_holder holder( done );

                i2c_sptr dev(i2c_by_index( request->hdl( ).value( ) ));
                auto par = static_cast<unsigned long>( request->parameter( ) );

                dev->ioctl( request->code( ), par );
            }

            void func_mask(::google::protobuf::RpcController* /*controller*/,
                         const ::fr::proto::i2c::func_mask_req* request,
                         ::fr::proto::i2c::func_mask_res* response,
                         ::google::protobuf::Closure* done) override
            {
                vcomm::closure_holder holder( done );
                i2c_sptr dev(i2c_by_index( request->hdl( ).value( ) ));

                unsigned long mask = 0;
                dev->ioctl_funcs( &mask );

                response->set_mask( static_cast<gpb::uint64>( mask ) );
            }

            void read(::google::protobuf::RpcController* /*controller*/,
                         const ::fr::proto::i2c::data_block* request,
                         ::fr::proto::i2c::data_block* response,
                         ::google::protobuf::Closure* done) override
            {
                vcomm::closure_holder holder( done );
                i2c_sptr dev(i2c_by_index( request->hdl( ).value( ) ));

                size_t max_block = request->length( );

                if( max_block > 44000 ) { /// protocol violation
                    max_block = 44000;
                }

                if( 0 == request->length( ) ) {
                    return;
                }
                std::vector<char> data( max_block );
                size_t res = dev->read( &data[0], max_block );
                response->set_data( &data[0], res );
            }

            void write(::google::protobuf::RpcController* /*controller*/,
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

            void read_bytes(::google::protobuf::RpcController* controller,
                         const ::fr::proto::i2c::write_read_datas_req* request,
                         ::fr::proto::i2c::write_read_datas_res* response,
                         ::google::protobuf::Closure* done) override
            {
                vcomm::closure_holder holder( done );
                i2c_sptr dev(i2c_by_index( request->hdl( ).value( ) ));
                for( int i = 0; i<request->request( ).value_size( ); ++i ) {

                    const
                    i2cproto::code_data &next( request->request( ).value( i ) );

                    uint8_t b = dev->smbus_read_byte_data( next.code( ) );

                    i2cproto::code_data *res_new =
                        response->mutable_response( )->add_value( );

                    res_new->set_code( next.code( ) );
                    res_new->mutable_data( )->set_value( b );
                }
            }

            void write_bytes(::google::protobuf::RpcController*  /*controller*/,
                         const ::fr::proto::i2c::write_read_datas_req* request,
                         ::fr::proto::i2c::write_read_datas_res* /*response*/,
                         ::google::protobuf::Closure* done) override
            {
               vcomm::closure_holder holder( done );
               i2c_sptr dev(i2c_by_index( request->hdl( ).value( ) ));
               for( int i = 0; i<request->request( ).value_size( ); ++i ) {
                   const
                   i2cproto::code_data &next( request->request( ).value( i ) );
                   dev->smbus_write_byte_data( next.code( ),
                                               next.data( ).value( ) & 0xFF );
                }
            }

            void read_words(::google::protobuf::RpcController* /*controller*/,
                         const ::fr::proto::i2c::write_read_datas_req* request,
                         ::fr::proto::i2c::write_read_datas_res* response,
                         ::google::protobuf::Closure* done) override
            {
               vcomm::closure_holder holder( done );
               i2c_sptr dev(i2c_by_index( request->hdl( ).value( ) ));
               for( int i = 0; i<request->request( ).value_size( ); ++i ) {

                   const auto &next(request->request( ).value( i ));

                   uint8_t b = dev->smbus_read_word_data( next.code( ) );

                   i2cproto::code_data *res_new =
                       response->mutable_response( )->add_value( );

                   res_new->set_code( next.code( ) );
                   res_new->mutable_data( )->set_value( b );
               }

            }
            void write_words(::google::protobuf::RpcController* /*controller*/,
                         const ::fr::proto::i2c::write_read_datas_req* request,
                         ::fr::proto::i2c::write_read_datas_res* response,
                         ::google::protobuf::Closure* done) override
            {
                vcomm::closure_holder holder( done );
                i2c_sptr dev(i2c_by_index( request->hdl( ).value( ) ));
                for( int i = 0; i<request->request( ).value_size( ); ++i ) {
                    const auto &next(request->request( ).value( i ));
                    dev->smbus_write_word_data( next.code( ),
                                              next.data( ).value( ) & 0xFFFF );
                 }
            }

            void read_block(::google::protobuf::RpcController* /*controller*/,
                         const ::fr::proto::i2c::write_read_data_req* request,
                         ::fr::proto::i2c::write_read_data_res* response,
                         ::google::protobuf::Closure* done) override
            {
                vcomm::closure_holder holder( done );
                i2c_sptr dev(i2c_by_index( request->hdl( ).value( ) ));

                bool broken_block = request->request( ).data( ).broken_block( );

                if( broken_block ) {
                    response->mutable_data( )
                        ->set_block( dev->smbus_read_block_broken(
                                    request->request( ).code( ),
                                    request->request( ).data( ).value( ) ) );
                } else {
                    response->mutable_data( )
                        ->set_block( dev->smbus_read_block_data(
                                     request->request( ).code( ) ) );
                }
            }

            void write_block(::google::protobuf::RpcController* /*controller*/,
                         const ::fr::proto::i2c::write_read_data_req* request,
                         ::fr::proto::i2c::write_read_data_res* response,
                         ::google::protobuf::Closure* done) override
            {
                vcomm::closure_holder holder( done );
                i2c_sptr dev(i2c_by_index( request->hdl( ).value( ) ));
                bool broken_block = request->request( ).data( ).broken_block( );

                if( broken_block ) {
                    dev->smbus_write_block_broken( request->request( ).code( ),
                                       request->request( ).data( ).block( ) );
                } else {
                    dev->smbus_write_block_data( request->request( ).code( ),
                                     request->request( ).data( ).block( ) );
                }
            }

            void process_call(::google::protobuf::RpcController* /*controller*/,
                         const ::fr::proto::i2c::write_read_data_req* request,
                         ::fr::proto::i2c::write_read_data_res* response,
                         ::google::protobuf::Closure* done) override
            {
                vcomm::closure_holder holder( done );
                i2c_sptr dev(i2c_by_index( request->hdl( ).value( ) ));

                auto req = request->request( );

                uint16_t cmd = req.code( ) & 0xFFFF;

                if( req.data( ).has_value( ) ) {

                    uint16_t code = req.data( ).value( ) & 0xFFFF;

                    response->mutable_data( )
                            ->set_value( dev->smbus_process_call( cmd, code ) );

                } else if( req.data( ).has_block( ) ) {

                    const std::string &code( req.data( ).block( ) );

                    response->mutable_data( )
                            ->set_block(
                                   dev->smbus_block_process_call( cmd, code ) );

                } else {
                    throw vcomm::exception( EINVAL,
                                            "Empty data or code received." );
                }
            }

            void transfer(::google::protobuf::RpcController* /*controller*/,
                         const ::fr::proto::i2c::transfer_req* request,
                         ::fr::proto::i2c::transfer_res* response,
                         ::google::protobuf::Closure* done) override
            {
                vcomm::closure_holder holder( done );
                i2c_sptr dev(i2c_by_index( request->hdl( ).value( ) ));

                auto &data(request->info( ).data( ));
                auto txlen = data.size( );
                auto rxlen = request->info( ).rxlen( );

                response->set_data(dev->transfer( data.c_str( ),
                                                  txlen, rxlen ) );
            }

            void transfer_list(::google::protobuf::RpcController* /*control*/,
                         const ::fr::proto::i2c::transfer_list_req* request,
                         ::fr::proto::i2c::transfer_list_res* response,
                         ::google::protobuf::Closure* done) override
            {
                vcomm::closure_holder holder( done );
                i2c_sptr dev(i2c_by_index( request->hdl( ).value( ) ));

                for( auto &i: request->infos( ) ) {
                    auto &data(i.data( ));
                    auto txlen = i.data( ).size( );
                    auto rxlen = i.rxlen( );
                    response->add_data( dev->transfer( data.c_str( ), txlen,
                                                       rxlen ) );
                }
            }

            void close(::google::protobuf::RpcController* /*controller*/,
                         const ::fr::proto::handle*        request,
                         ::fr::proto::empty*              /*response*/,
                         ::google::protobuf::Closure* done) override
            {
                vcomm::closure_holder holder( done );
                locker_type uslck(devices_lock_);
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
            auto inst(vtrc::make_shared<i2c_inst_impl>( app, cl ));

            auto &log_(app->get_logger( ));

            LOGDBG << "Create i2c instance for " << cl.lock( )->name( );

            return app->wrap_service( cl, inst );
        }
    }

    struct i2c::impl {

        application     *app_;
        logger          &log_;

        impl( application *app )
            :app_(app)
            ,log_(app_->get_logger( ))
        { }

        void reg_creator( const std::string &name,
                          application::service_getter_type func )
        {
            app_->register_service_factory( name, func );
        }

        void unreg_creator( const std::string &name )
        {
            app_->unregister_service_factory( name );
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
        impl_->LOGINF << "Started.";
    }

    void i2c::stop( )
    {
        impl_->unreg_creator( i2c_inst_impl::name( ) );
        impl_->LOGINF << "Stopped.";
    }

}}}


