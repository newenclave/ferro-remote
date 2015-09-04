
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
        typedef fr::proto::spi::instance proto_instance;

        typedef vtrc::shared_ptr<agent::file_iface> file_sptr;
        typedef vtrc::weak_ptr<agent::file_iface>   file_wptr;

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

        const char *s_channels[2] = { "/dev/spidev0.0", "/dev/spidev0.1" };
        const uint8_t   spi_BPW   = 8 ;
        const uint16_t  spi_delay = 0 ;

        typedef agent::index_map<spi_info>         file_map;

        // typedef proto::events::Stub stub_type;

        using vtrc::server::channels::unicast::create_event_channel;

        typedef vtrc::shared_ptr<vcomm::rpc_channel> rpc_channel_sptr;

        void setup_device( file_sptr f, vtrc::uint32_t speed,
                                        vtrc::uint32_t mode )
        {
            uint8_t bits = spi_BPW;

            /// mode SPI_MODE_0..SPI_MODE_3
            mode &= 3;

            /// set mode
            f->ioctl( SPI_IOC_WR_MODE,             &mode );
            /// set bits per words

            f->ioctl( SPI_IOC_WR_BITS_PER_WORD,    &bits );
            if( speed ) {
                // set speed
                f->ioctl( SPI_IOC_WR_MAX_SPEED_HZ, &speed );
            }
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

            void open(::google::protobuf::RpcController* /*controller*/,
                     const ::fr::proto::spi::open_req* request,
                     ::fr::proto::spi::open_res*       response,
                     ::google::protobuf::Closure* done) override
            {
                vcomm::closure_holder holder(done);
                const char *name = s_channels[request->channel( ) & 1];

                LOGDBG << "Try to open " << name << "...";
                spi_info si;
                si.file_.reset( agent::file::create(name, O_RDWR) );
                LOGDBG << name << ": success";
                if( request->has_setup( ) ) {
                    vtrc::uint32_t mode =  request->setup( ).mode( );
                    vtrc::uint32_t speed = request->setup( ).speed( );
                    LOGDBG << "Try to configure " << name
                           << "; speed: " << speed
                           << "; mode: " << mode
                               ;
                    setup_device( si.file_, speed, mode );
                }

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

                setup_device( f.file_, speed, mode );
                if( speed ) { // ok
                    f.speed_ = request->setup( ).speed( );
                }

            }

            void write_read(::google::protobuf::RpcController* /*controller*/,
                     const ::fr::proto::spi::write_read_req* request,
                     ::fr::proto::spi::write_read_res* response,
                     ::google::protobuf::Closure* done) override
            {
                vcomm::closure_holder holder(done);

                if( request->data( ).empty( ) ) {
                    return;
                }

                spi_info &si = get_file( request->hdl( ).value( ) );

                response->set_data( request->data( ) );
                char *data = &(*response->mutable_data( ))[0];

                spi_ioc_transfer spi = {0};
                spi.len           = request->data( ).size( );
                spi.tx_buf        = reinterpret_cast<uint64_t>(data);
                spi.rx_buf        = reinterpret_cast<uint64_t>(data);
                spi.speed_hz      = si.speed_;
                spi.delay_usecs   = spi_delay;
                spi.bits_per_word = spi_BPW;

                si.file_->ioctl( SPI_IOC_MESSAGE(1), &spi );
            }

            void close(::google::protobuf::RpcController* /*controller*/,
                     const ::fr::proto::handle*           request,
                     ::fr::proto::empty*                  /*response*/,
                     ::google::protobuf::Closure* done) override
            {
                vcomm::closure_holder holder(done);
                files_.del( request->value( ) );
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

    
