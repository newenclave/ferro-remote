#include <iostream>
#include <map>
#include <sys/epoll.h>
#include <unistd.h>
#include <time.h>

#include "application.h"
#include "subsys-gpio.h"
#include "subsys-reactor.h"
#include "subsys-logging.h"
#include "subsys-multicast.h"

#include "protocol/ferro.pb.h"
#include "protocol/gpio.pb.h"

#include "gpio-helper.h"

#include "vtrc-server/vtrc-channels.h"

#include "vtrc-common/vtrc-closure-holder.h"
#include "vtrc-common/vtrc-mutex-typedefs.h"
#include "vtrc-common/vtrc-mutex-typedefs.h"

#include "vtrc-stdint.h"
#include "vtrc-atomic.h"
#include "vtrc-memory.h"
#include "vtrc-function.h"
#include "vtrc-bind.h"

#include "vtrc-chrono.h"
#include "errno-check.h"


#define LOG(lev) log_(lev) << "[gpio] "
#define LOGINF   LOG(logger::level::info)
#define LOGDBG   LOG(logger::level::debug)
#define LOGERR   LOG(logger::level::error)
#define LOGWRN   LOG(logger::level::warning)

#define DOUBLE_QUEUE 1

namespace fr { namespace agent { namespace subsys {

    namespace {

        const std::string subsys_name( "gpio" );

        namespace gproto = fr::proto::gpio;
        namespace sproto = fr::proto;

        namespace vcomm  = vtrc::common;
        namespace vserv  = vtrc::server;

        namespace chrono = vtrc::chrono;

        using events_stub_type = sproto::events::Stub;

        using vserv::channels::unicast::create_event_channel;
        using rpc_channel_sptr = vtrc::shared_ptr<vcomm::rpc_channel>;

        using gpio_sptr = vtrc::shared_ptr<agent::gpio_helper>;
        using gpio_wptr = vtrc::weak_ptr<agent::gpio_helper>;

        using gpio_map = std::map<vtrc::uint32_t, gpio_sptr>;

        using time_point = chrono::high_resolution_clock::time_point;

        agent::gpio::direction_type direct_from_proto( unsigned dir )
        {
            switch ( dir ) {
            case agent::gpio::DIRECT_IN:
            case agent::gpio::DIRECT_OUT:
                return static_cast<agent::gpio::direction_type>(dir);
            default:
                vcomm::throw_system_error( EINVAL, "Bad direction" );
            }
            return agent::gpio::DIRECT_OUT; ///
        }

        agent::gpio::edge_type edge_from_proto( unsigned edge )
        {
            switch ( edge ) {
            case agent::gpio::EDGE_NONE:
            case agent::gpio::EDGE_FALLING:
            case agent::gpio::EDGE_RISING:
            case agent::gpio::EDGE_BOTH:
                return static_cast<agent::gpio::edge_type>(edge);
            default:
                vcomm::throw_system_error( EINVAL, "Bad edge" );
            }
            return agent::gpio::EDGE_NONE;
        }

        class gpio_impl: public gproto::instance {

            fr::agent::application              *app_;
            gpio_map                             gpio_;
            vtrc::shared_mutex                   gpio_lock_;
            vtrc::common::connection_iface_wptr  client_;
            rpc_channel_sptr                     event_channel_;
            events_stub_type                     eventor_;
            subsys::reactor                     &reactor_;
            logger                              &log_;

        public:

            gpio_impl( fr::agent::application *app,
                       vcomm::connection_iface_wptr cli )
                :app_(app)
                ,client_(cli)
                ,event_channel_(create_event_channel(client_.lock( )))
                ,eventor_(event_channel_.get( ))
                ,reactor_(app->subsystem<subsys::reactor>( ))
                ,log_(app->get_logger( ))
            { }

            ~gpio_impl( )
            { try {
                for( gpio_map::iterator b(gpio_.begin( )), e(gpio_.end( ));
                     b != e; ++b )
                {
                    if( b->second->value_opened( ) ) {
#if DOUBLE_QUEUE
                        b->second->del_reactor_action( reactor_.get( ),
                                                       b->first );
#else
//                        reactor_.del_fd( b->second->value_fd( ) );
#endif
                    }
                }

            } catch( ... ) { }}

            inline vtrc::uint32_t next_index( )
            {
                return reactor_.next_op_id( );
            }

            gpio_sptr gpio_by_index( vtrc::uint32_t id )
            {
                vtrc::shared_lock slck(gpio_lock_);
                gpio_map::iterator f(gpio_.find(id));
                if( f == gpio_.end( ) ) {
                    vcomm::throw_system_error( EINVAL, "Bad GPIO handle." );
                }
                return f->second;
            }

            void gpio_setup( gpio_sptr &ng, const gproto::setup_data &setup )
            {
                if( setup.has_direction( ) ) {
                    unsigned dir = setup.direction( );
                    ng->set_direction( direct_from_proto( dir ) );
                }

                if( setup.has_edge( ) ) {
                    unsigned edge = setup.edge( );
                    ng->set_edge( edge_from_proto( edge ) );
                }

                if( setup.has_value( ) ) {
                    unsigned val = setup.value( );
                    ng->set_value( val == 0 ? 0 : 1 );
                }

                if( setup.has_active_low( ) ) {
                    unsigned act_low = setup.active_low( );
                    ng->set_active_low( act_low == 0 ? 0 : 1 );
                }
            }

            void ping( ::google::protobuf::RpcController* /* controller */,
                       const ::fr::proto::empty*          /* request    */,
                       ::fr::proto::empty*                /* response   */,
                       ::google::protobuf::Closure* done) override
            {
                if( done ) done->Run( );
            }

            void exp(::google::protobuf::RpcController* /*controller*/,
                         const ::fr::proto::gpio::export_req* request,
                         ::fr::proto::empty*            /*response*/,
                         ::google::protobuf::Closure* done) override
            {
                vcomm::closure_holder holder(done);
                gpio_sptr g( gpio_by_index(request->hdl( ).value( ) ) );
                if( !g->exists( ) ) {
                    g->exp( );
                }
            }

            void setup(::google::protobuf::RpcController* /*controller*/,
                         const ::fr::proto::gpio::setup_req* request,
                         ::fr::proto::empty*              /*response*/,
                         ::google::protobuf::Closure* done) override
            {
                vcomm::closure_holder holder(done);
                gpio_sptr g( gpio_by_index(request->hdl( ).value( ) ) );
                gpio_setup( g, request->setup( ) );
            }

            void unexp(::google::protobuf::RpcController* /*controller*/,
                         const ::fr::proto::gpio::export_req* request,
                         ::fr::proto::empty*              /*response*/,
                         ::google::protobuf::Closure* done) override
            {
                vcomm::closure_holder holder(done);
                gpio_sptr g( gpio_by_index(request->hdl( ).value( ) ) );
                g->unexp( );
            }

            void info(::google::protobuf::RpcController* controller,
                         const ::fr::proto::gpio::info_req* request,
                         ::fr::proto::gpio::setup_data* response,
                         ::google::protobuf::Closure* done) override
            {
                vcomm::closure_holder holder(done);
                gpio_sptr g( gpio_by_index(request->hdl( ).value( ) ) );

                if( request->with_value( ) ) {
                    response->set_value( g->value( ) );
                }

                if( request->with_direction( ) ) {
                    response->set_direction( g->direction( ) );
                }

                if( request->with_edge( ) ) {
                    response->set_edge( g->edge( ) );
                }

                if( request->with_active_low( ) ) {
                    response->set_active_low( g->active_low( ) );
                }

            }

            void make_pulse(::google::protobuf::RpcController* /*controller*/,
                         const ::fr::proto::gpio::pulse_req* request,
                         ::fr::proto::gpio::pulse_res*         /*response*/,
                         ::google::protobuf::Closure* done) override
            {
                vcomm::closure_holder holder(done);
                gpio_sptr g( gpio_by_index(request->hdl( ).value( ) ) );

                unsigned b = request->has_set_value( )
                           ? request->set_value( )
                           : 1;

                unsigned a = request->has_reset_value( )
                           ? request->reset_value( )
                           : 0;

                timespec init = { 0, 0 };

                const uint64_t pl = request->length( );

                init.tv_nsec = ( pl % ( 1000 * 1000 ) ) * 1000;
                init.tv_sec  =   pl / ( 1000 * 1000 );

                g->set_value( b );

                int res = -1;
                while( -1 == res ) {
                    res = ::nanosleep( &init, &init );
                    if( ( -1 == res ) && ( errno != EINTR ) ) {
                        LOGERR << "::nanosleep failed. errno = " << errno;
                        vcomm::throw_system_error( errno, "nanosleep failed." );
                    }
                }
                g->set_value( a );
            }


#if DOUBLE_QUEUE

            void open(::google::protobuf::RpcController* /*controller*/,
                         const ::fr::proto::gpio::open_req* request,
                         ::fr::proto::gpio::open_res* response,
                         ::google::protobuf::Closure* done) override
            {
                vcomm::closure_holder holder(done);

                vtrc::uint32_t newid = next_index( );
                vtrc::uint32_t id = request->gpio_id( );

                gpio_sptr ng = app_->subsystem<gpio>( ).open_pin( id );

                if( request->exp( ) ) {
                    if( !ng->exists( ) ) {
                        ng->exp( );
                    }
                    gpio_setup( ng, request->setup( ) );
                }
                response->mutable_hdl( )->set_value( newid );
                response->set_edge_supported( ng->edge_supported( ) );
                LOGDBG << "Open device with ID: " << id
                       << " fd: " << ng->value_fd( )
                          ;
                vtrc::unique_shared_lock lck( gpio_lock_ );
                gpio_.insert( std::make_pair( newid, ng ) );
            }


            void close(::google::protobuf::RpcController* /*controller*/,
                         const ::fr::proto::handle*         request,
                         ::fr::proto::empty*              /*response*/,
                         ::google::protobuf::Closure* done) override
            {
                vcomm::closure_holder holder(done);
                vtrc::upgradable_lock ulck( gpio_lock_ );
                gpio_map::iterator f( gpio_.find( request->value( ) ) );
                if( f != gpio_.end( ) ) {
                    auto g = f->second;
                    if( g->value_opened( ) ) {
                        g->del_reactor_action( reactor_.get( ), f->first );
                    }
                    {
                        vtrc::upgrade_to_unique utl(ulck);
                        gpio_.erase( f );
                    }
                    app_->subsystem<gpio>( ).close_pin( f->first );
                }
            }

            struct value_data {
                vtrc::uint32_t  hdl;
                int             fd;
                gpio_wptr       weak_gpio;
                value_data( vtrc::uint32_t h, int f, gpio_sptr &gpio )
                    :hdl(h)
                    ,fd(f)
                    ,weak_gpio(gpio)
                { }
            };

            bool value_changed( unsigned value,
                                std::uint64_t tick_count,
                                value_data &data,
                                vcomm::connection_iface_wptr cli )
            {
                try {
                    vcomm::connection_iface_sptr lck( cli.lock( ) );
                    if( !lck ) {
                        return false;
                    }

                    //LOGDBG << "Value " << value << " for " << data.hdl;

                    gpio_sptr gpio( data.weak_gpio.lock( ) );

                    bool               success = true;
                    std::string        err;
                    unsigned           error_code = 0;

                    sproto::async_op_data       op_data;
                    gproto::value_change_data   vdat;

                    op_data.set_id( data.hdl );
                    op_data.set_tick_count( tick_count );

                    vdat.set_timepoint( tick_count );
                    vdat.set_new_value( value );
                    op_data.set_data( vdat.SerializeAsString( ) );

                    if( !success ) {
                        op_data.mutable_error( )->set_code( error_code );
                        op_data.mutable_error( )->set_text( err );
                    }

                    eventor_.async_op( NULL, &op_data, NULL, NULL );

                    return success;

                } catch( std::exception &ex ) {
                    LOGERR << "Failed to send GPIO event: " << ex.what( );
                } catch( ... ) {
                    LOGERR << "Failed to send GPIO event: ...";
                }
                return false;
            }

            void register_for_change(::google::protobuf::RpcController* ,
                         const ::fr::proto::gpio::register_req* request,
                         ::fr::proto::gpio::register_res* response,
                         ::google::protobuf::Closure* done) override
            {
                namespace ph = vtrc::placeholders;

                vcomm::closure_holder holder(done);
                vtrc::uint32_t hdl(request->hdl( ).value( ));
                gpio_sptr g( gpio_by_index( hdl ) );

                agent::reaction_callback
                        cb( vtrc::bind( &gpio_impl::value_changed, this,
                                         ph::_1, ph::_2,
                                         value_data( hdl, g->value_fd( ), g ),
                                         client_ ) );

                g->add_reactor_action( reactor_.get( ), hdl, cb );
                LOGDBG << "Add device to reactor; ID: " << hdl
                       << " fd: " << g->value_fd( )
                       ;
                response->set_async_op_id( hdl );
            }

            void unregister( ::google::protobuf::RpcController* /*controller*/,
                             const ::fr::proto::gpio::register_req* request,
                             ::fr::proto::empty*                /*response*/,
                             ::google::protobuf::Closure* done) override
            {

                vcomm::closure_holder holder(done);

                gpio_sptr g( gpio_by_index( request->hdl( ).value( ) ) );
                g->del_reactor_action( reactor_.get( ),
                                       request->hdl( ).value( ) );

                LOGDBG << "remove device from reactor; ID: "
                       << request->hdl( ).value( )
                       << " fd: " << g->value_fd( )
                       ;
            }

#else
            void open(::google::protobuf::RpcController* /*controller*/,
                         const ::fr::proto::gpio::open_req* request,
                         ::fr::proto::gpio::open_res* response,
                         ::google::protobuf::Closure* done) override
            {
                vcomm::closure_holder holder(done);

                vtrc::uint32_t newid = next_index( );
                vtrc::uint32_t id = request->gpio_id( );

                gpio_sptr ng( vtrc::make_shared<agent::gpio_helper>( id ) );

                if( request->exp( ) ) {
                    if( !ng->exists( ) ) {
                        ng->exp( );
                    }
                    gpio_setup( ng, request->setup( ) );
                }
                response->mutable_hdl( )->set_value( newid );
                response->set_edge_supported( ng->edge_supported( ) );
                LOGDBG << "Open device with ID: " << id
                       << " fd: " << ng->value_fd( )
                          ;
                vtrc::unique_shared_lock lck( gpio_lock_ );
                gpio_.insert( std::make_pair( newid, ng ) );
            }


            void close(::google::protobuf::RpcController* /*controller*/,
                         const ::fr::proto::handle*         request,
                         ::fr::proto::empty*              /*response*/,
                         ::google::protobuf::Closure* done) override
            {
                vcomm::closure_holder holder(done);
                vtrc::upgradable_lock ulck( gpio_lock_ );
                gpio_map::iterator f( gpio_.find( request->value( ) ) );
                if( f != gpio_.end( ) ) {
                    auto g = f->second;
                    if( g->value_opened( ) ) {
                        //g->del_reactor_action( reactor_.get( ), f->first );
                        reactor_.del_fd( f->second->value_fd( ) );
                    }
                    vtrc::upgrade_to_unique utl(ulck);
                    gpio_.erase( f );
                }
            }

            struct value_data {
                vtrc::uint32_t  hdl;
                int             fd;
                gpio_wptr       weak_gpio;
                size_t          op_id;
                value_data( vtrc::uint32_t h, int f,
                            gpio_sptr &gpio,  size_t op )
                    :hdl(h)
                    ,fd(f)
                    ,weak_gpio(gpio)
                    ,op_id(op)
                { }
            };

            bool value_changed( unsigned /*events*/,
                                std::uint64_t tick_count,
                                value_data &data,
                                vcomm::connection_iface_wptr cli )
            {
                try {
                    vcomm::connection_iface_sptr lck( cli.lock( ) );
                    if( !lck ) {
                        return false;
                    }

                    gpio_sptr gpio( data.weak_gpio.lock( ) );

                    bool               success = true;
                    std::string        err;
                    unsigned           error_code = 0;

                    sproto::async_op_data       op_data;
                    gproto::value_change_data   vdat;

                    op_data.set_id( data.op_id );
                    op_data.set_tick_count( tick_count );

                    try {
                        vdat.set_timepoint( tick_count );
                        vdat.set_new_value( gpio->value_by_fd( data.fd ) );
                        op_data.set_data( vdat.SerializeAsString( ) );
                    } catch( const std::exception &ex ) {
                        error_code = errno;
                        err.assign( ex.what( ) );
                        success = false;
                    }

                    if( !success ) {
                        op_data.mutable_error( )->set_code( error_code );
                        op_data.mutable_error( )->set_text( err );
                    }

                    eventor_.async_op( NULL, &op_data, NULL, NULL );

                    return success;

                } catch( std::exception &ex ) {
                    LOGERR << "Failed to send GPIO event: " << ex.what( );
                } catch( ... ) {
                    LOGERR << "Failed to send GPIO event: ...";
                }
                return false;
            }

            void register_for_change(::google::protobuf::RpcController* ,
                         const ::fr::proto::gpio::register_req* request,
                         ::fr::proto::gpio::register_res* response,
                         ::google::protobuf::Closure* done) override
            {
                namespace ph = vtrc::placeholders;

                vcomm::closure_holder holder(done);
                vtrc::uint32_t hdl(request->hdl( ).value( ));
                gpio_sptr g( gpio_by_index( hdl ) );

                int fd = g->value_fd( );

                vtrc::uint32_t opid = reactor_.next_op_id( );

                agent::reaction_callback
                        cb( vtrc::bind( &gpio_impl::value_changed, this,
                                         ph::_1, ph::_2,
                                         value_data( hdl, fd, g, opid ),
                                         client_ ) );
                reactor_.add_fd( fd, EPOLLET | EPOLLPRI, cb );
                LOGDBG << "Add device to reactor; ID: " << hdl
                       << " fd: " << fd
                       ;
                response->set_async_op_id( opid );
            }

            void unregister( ::google::protobuf::RpcController* /*controller*/,
                             const ::fr::proto::gpio::register_req* request,
                             ::fr::proto::empty*                /*response*/,
                             ::google::protobuf::Closure* done) override
            {

                vcomm::closure_holder holder(done);
                gpio_sptr g( gpio_by_index( request->hdl( ).value( ) ) );
                int fd = g->value_fd( );
                reactor_.del_fd( fd );
                LOGDBG << "remove device from reactor; ID: "
                       << request->hdl( ).value( )
                       << " fd: " << fd
                       ;
            }
#endif

        public:

            static const std::string &name( )
            {
                return gproto::instance::descriptor( )->full_name( );
            }
        };

        application::service_wrapper_sptr create_service(
                                      fr::agent::application *app,
                                      vtrc::common::connection_iface_wptr cl )
        {
            auto inst = vtrc::make_shared<gpio_impl>( app, cl );
            return app->wrap_service( cl, inst );
        }
    }

    struct gpio::impl {

        using gpio_helper_wptr = std::weak_ptr<gpio_helper>;
        using gpio_map = std::map<size_t, gpio_helper_wptr>;

        application         *app_;
        logger              &log_;
        subsys::reactor     *reactor_;
        gpio_map             devices_;
        vtrc::mutex          devices_lock_;

        gpio_helper::queue_type disp_;

        impl( application *app )
            :app_(app)
            ,log_(app->get_logger( ))
            ,disp_(app->get_io_service( )) /// io!
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

        gpio_helper_sptr open_pin( std::uint32_t pin )
        {
            vtrc::lock_guard<vtrc::mutex> lck(devices_lock_);
            gpio_helper_sptr res;
            auto dev = devices_.find( pin );
            if( dev != devices_.end( ) ) {
                res = dev->second.lock( );
            }
            if( !res ) {
                res = std::make_shared<gpio_helper>( pin, std::ref(disp_) );
                devices_[pin] = gpio_helper_wptr(res);
            }
            return res;
        }

        void close_pin( std::uint32_t pin )
        {
            vtrc::lock_guard<vtrc::mutex> lck(devices_lock_);
            auto dev = devices_.find( pin );
            if( dev != devices_.end( ) ) {
                auto res = dev->second.lock( );
                if( !res ) {
                    devices_.erase( dev );
                }
            }
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

    const std::string &gpio::name( ) const
    {
        return subsys_name;
    }

    void gpio::init( )
    {
        impl_->reactor_ = &impl_->app_->subsystem<subsys::reactor>( );
        impl_->app_->subsystem<subsys::multicast>( ).on_request_connect(
            [ ]( const subsys::multicast_request & /*req*/,
                       subsys::multicast_response &res )
            {
                res.gpio_available = agent::gpio::available( );
            });
    }

    void gpio::start( )
    {
        if( agent::gpio::available( ) ) {
            impl_->reg_creator( gpio_impl::name( ),  create_service );
            impl_->LOGINF << "Started.";
        } else {
            impl_->LOGDBG << "GPIO is not available";
        }
    }

    void gpio::stop( )
    {
        impl_->unreg_creator( gpio_impl::name( ) );
        impl_->LOGINF << "Stopped.";
    }

    gpio_helper_sptr gpio::open_pin( std::uint32_t pin )
    {
        return impl_->open_pin( pin );
    }

    void gpio::close_pin( std::uint32_t pin )
    {
        impl_->close_pin( pin );
    }

}}}

    
