
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>

#include "application.h"
#include "subsys-spi.h"
#include "subsys-log.h"
#include "subsys-reactor.h"

#include "files.h"
#include "index-map.h"

#include "spi-helper.h"

#include "protocol/spi.pb.h"

#include "vtrc-common/vtrc-exception.h"
#include "vtrc-common/vtrc-mutex-typedefs.h"
#include "vtrc-common/vtrc-closure-holder.h"
#include "vtrc-server/vtrc-channels.h"
#include "vtrc-memory.h"
#include "vtrc-atomic.h"

#define LOG(lev) log_(lev) << "[spi] "
#define LOGINF   LOG(logger::info)
#define LOGDBG   LOG(logger::debug)
#define LOGERR   LOG(logger::error)
#define LOGWRN   LOG(logger::warning)

namespace fr { namespace agent { namespace subsys {

    // Have a tons of the copy-paste here
    // Gonna fix it later...
    namespace {

        const std::string subsys_name( "spi" );

        namespace vcomm = vtrc::common;
        namespace spiproto = fr::proto::spi;
        using proto_instance = spiproto::instance;

        using file_sptr = vtrc::shared_ptr<agent::spi_helper>;
        using file_wptr = vtrc::weak_ptr<agent::spi_helper>  ;

        struct spi_info {
            file_sptr       file_;
            vtrc::uint32_t  speed_;

            spi_info( )
                :speed_(0)
            { }

            spi_info(vtrc::uint32_t speed)
                :speed_(speed)
            { }
        };

        using file_map = agent::index_map<vtrc::uint32_t, spi_info> ;

        using vtrc::server::channels::unicast::create_event_channel;
        using rpc_channel_sptr = vtrc::shared_ptr<vcomm::rpc_channel>;

        std::string make_spi_name( unsigned bus, unsigned channel )
        {
            std::ostringstream oss;
            oss << "/dev/spidev" << bus << "." << channel;
            return oss.str( );
        }

//        void setup_device( file_sptr f, vtrc::uint32_t speed,
//                                        vtrc::uint32_t mode )
//        {
//            f->setup( mode, speed );
//        }

        void call_setter( file_sptr dev, int reg_bits,
                          uint32_t addr, uint32_t reg, uint64_t value )
        {
            switch (reg_bits) {
            case spiproto::register_info::REG_16BIT:
                dev->set_reg16( addr, reg, static_cast<uint16_t>(value) );
                break;
            case spiproto::register_info::REG_32BIT:
                dev->set_reg32( addr, reg, static_cast<uint32_t>(value) );
                break;
            case spiproto::register_info::REG_64BIT:
                dev->set_reg64( addr, reg, value );
                break;
            case spiproto::register_info::REG_8BIT: // default
            default:
                dev->set_reg8( addr, reg, static_cast<uint8_t>(value) );
                break;
            }
        }

        uint64_t call_getter( file_sptr dev, int reg_bits,
                              uint32_t addr, uint32_t reg )
        {
            uint64_t res = 0;
            switch (reg_bits) {
            case spiproto::register_info::REG_16BIT:
                res = dev->get_reg16( addr, reg);
                break;
            case spiproto::register_info::REG_32BIT:
                res = dev->get_reg32( addr, reg);
                break;
            case spiproto::register_info::REG_64BIT:
                res = dev->get_reg64( addr, reg);
                break;
            case spiproto::register_info::REG_8BIT: // default
            default:
                res = dev->get_reg8( addr, reg);
                break;
            }
            return res;
        }

        class proto_impl: public proto_instance {

            application                  *app_;
            vcomm::connection_iface_wptr  client_;
            file_map                      files_;
            logger                       &log_;
//            subsys::reactor              &reactor_;
//            rpc_channel_sptr              event_channel_;
//            stub_type                     events_;

        public:

            proto_impl( application *app, vcomm::connection_iface_wptr client )
                :app_(app)
                ,client_(client)
                ,files_(100) // begin index
                ,log_(app_->subsystem<subsys::log>( ).get_logger( ))
  //              ,reactor_(app_->subsystem<subsys::reactor>( ))
            { }

            spi_info &get_file( vtrc::uint32_t id )
            {
                return files_.get( id, EBADF, "Bad SPI file number." );
            }

            void bus_available(::google::protobuf::RpcController* /*control*/,
                     const ::fr::proto::spi::bus_available_req* request,
                     ::fr::proto::spi::bus_available_res* response,
                     ::google::protobuf::Closure* done) override
            {
                vcomm::closure_holder holder(done);
                response->set_value( agent::spi::available(
                                         request->bus_id( ),
                                         request->channel( ) ) );
            }

            void open(::google::protobuf::RpcController* /*controller*/,
                     const ::fr::proto::spi::open_req* request,
                     ::fr::proto::spi::open_res*       response,
                     ::google::protobuf::Closure* done) override
            {
                vcomm::closure_holder holder(done);

                std::string name = make_spi_name( request->bus( ),
                                                  request->channel( ) );

                LOGDBG << "Try to open " << name << "...";

                spi_info si;
                si.file_.reset( new agent::spi_helper( request->bus( ),
                                                       request->channel( ) ) );

                LOGDBG << name << ": was success opened.";

                vtrc::uint32_t mode =  request->setup( ).mode( );
                vtrc::uint32_t speed = request->setup( ).speed( );
                LOGDBG << "Try to configure " << name
                       << "; speed: " << speed
                       << "; mode: " << mode
                           ;
                si.file_->setup( speed, mode );

                /// all is success! hold the file
                response->mutable_hdl( )->set_value( files_.set( si ) );
            }

            void setup(::google::protobuf::RpcController* /*controller*/,
                     const ::fr::proto::spi::setup_req*   request,
                     ::fr::proto::spi::setup_res*         /*response*/,
                     ::google::protobuf::Closure* done) override
            {
                vcomm::closure_holder holder(done);
                spi_info &f(get_file(request->hdl( ).value( )));

                vtrc::uint32_t mode =  request->setup( ).mode( );
                vtrc::uint32_t speed = request->setup( ).speed( );

                f.file_->setup( speed, mode );
                f.speed_ = f.file_->speed( );

            }

            void close(::google::protobuf::RpcController* /*controller*/,
                     const ::fr::proto::handle*           request,
                     ::fr::proto::empty*                  /*response*/,
                     ::google::protobuf::Closure* done) override
            {
                vcomm::closure_holder holder(done);

                files_.del( request->value( ) );
            }

            void transfer(::google::protobuf::RpcController* /*controller*/,
                     const ::fr::proto::spi::transfer_req* request,
                     ::fr::proto::spi::transfer_res* response,
                     ::google::protobuf::Closure* done) override
            {
                vcomm::closure_holder holder(done);

                if( request->data( ).empty( ) ) {
                    return;
                }

                spi_info &si = get_file( request->hdl( ).value( ) );

                response->set_data( request->data( ) );
                char *data = &(*response->mutable_data( ))[0];

                si.file_->transfer( data, request->data( ).size( ) );
            }

            void transfer_list(::google::protobuf::RpcController* /*control*/,
                     const ::fr::proto::spi::transfer_list_req* request,
                     ::fr::proto::spi::transfer_list_res* response,
                     ::google::protobuf::Closure* done) override
            {
                vcomm::closure_holder holder(done);
                if( request->datas_size( ) == 0 ) {
                    return;
                }

                spi_info &si = get_file( request->hdl( ).value( ) );

                if( request->nothrow( ) ) {
                    for( auto &d: request->datas( ) ) {
                        auto nd = response->add_datas( );
                        if( !d.empty( ) ) {
                            nd->assign( d.begin( ), d.end( ) );
                            char *data = &(*nd)[0];
                            auto res = si.file_->transfer_nothrow( data,
                                                                nd->size( ) );
                            response->add_errors( -1 == res ? errno : 0 );
                        } else {
                            response->add_errors( 0 );
                        }
                    }
                } else {
                    for( auto &d: request->datas( ) ) {
                        auto nd = response->add_datas( );
                        if( !d.empty( ) ) {
                            nd->assign( d.begin( ), d.end( ) );
                            char *data = &(*nd)[0];
                            si.file_->transfer( data, nd->size( ) );
                        }
                    }
                }

            }

            void set_register(::google::protobuf::RpcController* /*controller*/,
                     const ::fr::proto::spi::set_register_req* request,
                     ::fr::proto::spi::set_register_res* /*response*/,
                     ::google::protobuf::Closure* done) override
            {
                vcomm::closure_holder holder(done);
                spi_info &si = get_file( request->hdl( ).value( ) );

                const auto &inf(request->info( ));
                call_setter( si.file_, inf.bits( ),
                             inf.address( ), inf.reg( ), inf.value( ) );
            }

            void get_register(::google::protobuf::RpcController* /*controller*/,
                     const ::fr::proto::spi::get_register_req* request,
                     ::fr::proto::spi::get_register_res* response,
                     ::google::protobuf::Closure* done) override
            {
                vcomm::closure_holder holder(done);
                spi_info &si = get_file( request->hdl( ).value( ) );

                const auto &inf(request->info( ));

                response->mutable_info( )
                        ->set_value( call_getter( si.file_,
                                                  inf.bits( ),
                                                  inf.address( ),
                                                  inf.reg( ) ) );
            }

            void set_register_list(::google::protobuf::RpcController* /*cont*/,
                     const ::fr::proto::spi::set_register_list_req* request,
                     ::fr::proto::spi::set_register_list_res* response,
                     ::google::protobuf::Closure* done) override
            {
                vcomm::closure_holder holder(done);
                spi_info &si = get_file( request->hdl( ).value( ) );

                for( auto &r: request->infos( ) ) {
                    call_setter( si.file_, r.bits( ),
                                 r.address( ), r.reg( ), r.value( ) );
                }

            }

            void get_register_list(::google::protobuf::RpcController* /*cont*/,
                     const ::fr::proto::spi::get_register_list_req* request,
                     ::fr::proto::spi::get_register_list_res* response,
                     ::google::protobuf::Closure* done)  override
            {
                vcomm::closure_holder holder(done);
                spi_info &si = get_file( request->hdl( ).value( ) );

                for( auto &r: request->infos( ) ) {
                    auto nr = response->add_infos( );
                    nr->CopyFrom( r );
                    nr->set_value( call_getter( si.file_, r.bits( ),
                                                r.address( ), r.reg( ) ) );
                }
            }

            static const std::string &name( )
            {
                return proto_instance::descriptor( )->full_name( );
            }
        };

        application::service_wrapper_sptr create_service(
                                      fr::agent::application * app,
                                      vcomm::connection_iface_wptr cl )
        {
            auto inst( vtrc::make_shared<proto_impl>( app, cl ) );
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
        impl_->app_->register_service_creator(
                     proto_impl::name( ), create_service );
        impl_->LOGINF << "Started.";
    }

    void spi::stop( )
    {
        impl_->app_->unregister_service_creator( proto_impl::name( ) );
        impl_->LOGINF << "Stopped.";
    }


}}}

    
