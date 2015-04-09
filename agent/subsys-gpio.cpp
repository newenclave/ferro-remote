#include <iostream>
#include <map>
#include <sys/epoll.h>
#include <unistd.h>
#include <time.h>

#include "application.h"
#include "subsys-gpio.h"
#include "subsys-reactor.h"

#include "protocol/ferro.pb.h"
#include "protocol/gpio.pb.h"

#include "gpio-helper.h"

#include "vtrc-common/vtrc-closure-holder.h"

#include "vtrc-stdint.h"
#include "vtrc-atomic.h"
#include "vtrc-memory.h"
#include "vtrc-function.h"
#include "vtrc-bind.h"

#include "vtrc-chrono.h"

#include "vtrc-common/vtrc-mutex-typedefs.h"

#include "vtrc-server/vtrc-channels.h"

#include "errno-check.h"

namespace fr { namespace agent { namespace subsys {

    namespace {

        const std::string subsys_name( "gpio" );

        namespace gproto = fr::proto::gpio;
        namespace sproto = fr::proto;

        namespace vcomm  = vtrc::common;
        namespace vserv  = vtrc::server;

        namespace chrono = vtrc::chrono;

        typedef sproto::events::Stub events_stub_type;

        using vserv::channels::unicast::create_event_channel;
        typedef vtrc::shared_ptr<vcomm::rpc_channel> rpc_channel_sptr;

        typedef vtrc::shared_ptr<agent::gpio_helper> gpio_sptr;
        typedef vtrc::weak_ptr<agent::gpio_helper> gpio_wptr;

        typedef std::map<vtrc::uint32_t, gpio_sptr> gpio_map;

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

            gpio_map                             gpio_;
            vtrc::shared_mutex                   gpio_lock_;
            vtrc::atomic<vtrc::uint32_t>         index_;
            vtrc::common::connection_iface_wptr  client_;
            rpc_channel_sptr                     event_channel_;
            subsys::reactor                     &reactor_;

            typedef chrono::high_resolution_clock::time_point time_point;

            time_point                           event_last_time_;


        public:

            gpio_impl( fr::agent::application *app,
                       vcomm::connection_iface_wptr cli )
                :index_(100)
                ,client_(cli)
                ,event_channel_(create_event_channel(client_.lock( )))
                ,reactor_(app->subsystem<subsys::reactor>( ))
                ,event_last_time_(chrono::high_resolution_clock::now( ))
            { }

            ~gpio_impl( )
            { try {
                for( gpio_map::iterator b(gpio_.begin( )), e(gpio_.end( ));
                     b != e; ++b )
                {
                    if( b->second->value_opened( ) ) {
                        reactor_.del_fd( b->second->value_fd( ) );
                    }
                }

            } catch( ... ) { }}

            inline vtrc::uint32_t next_index( )
            {
                return ++index_;
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

            void ping(::google::protobuf::RpcController*  /* controller */,
                         const ::fr::proto::gpio::empty*  /* request    */,
                         ::fr::proto::gpio::empty*        /* response   */,
                         ::google::protobuf::Closure* done) override
            {
                if( done ) done->Run( );
            }

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
                vtrc::unique_shared_lock lck( gpio_lock_ );
                gpio_.insert( std::make_pair( newid, ng ) );
            }

            void exp(::google::protobuf::RpcController* /*controller*/,
                         const ::fr::proto::gpio::export_req* request,
                         ::fr::proto::gpio::empty* /*response*/,
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
                         ::fr::proto::gpio::empty* /*response*/,
                         ::google::protobuf::Closure* done) override
            {
                vcomm::closure_holder holder(done);
                gpio_sptr g( gpio_by_index(request->hdl( ).value( ) ) );
                gpio_setup( g, request->setup( ) );
            }

            void unexp(::google::protobuf::RpcController* /*controller*/,
                         const ::fr::proto::gpio::export_req* request,
                         ::fr::proto::gpio::empty* /*response*/,
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

                event_last_time_ = chrono::high_resolution_clock::now( );

                int res = -1;
                while( -1 == res ) {
                    res = ::nanosleep( &init, &init );
                    if( ( -1 == res ) && ( errno != EINTR ) ) {
                        vcomm::throw_system_error( errno, "nanosleep failed." );
                    }
                }
                g->set_value( a );
            }

            void close(::google::protobuf::RpcController* /*controller*/,
                         const ::fr::proto::gpio::handle* request,
                         ::fr::proto::gpio::empty* response,
                         ::google::protobuf::Closure* done) override
            {
                vcomm::closure_holder holder(done);
                vtrc::upgradable_lock ulck( gpio_lock_ );
                gpio_map::iterator f( gpio_.find( request->value( ) ) );
                if( f != gpio_.end( ) ) {
                    if( f->second->value_opened( ) ) {
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
                                value_data &data,
                                vcomm::connection_iface_wptr cli )
            {
                try {
                    vcomm::connection_iface_sptr lck( cli.lock( ) );
                    if( !lck ) {
                        return false;
                    }

                    time_point this_event =
                                         chrono::high_resolution_clock::now( );
                    time_point::duration dur = this_event - event_last_time_;

                    event_last_time_ = this_event;

                    gpio_sptr gpio( data.weak_gpio.lock( ) );

                    bool               success = true;
                    std::string        err;
                    unsigned           error_code = 0;
                    events_stub_type   events(event_channel_.get( ));

                    sproto::async_op_data       op_data;
                    gproto::value_change_data   vdat;

                    op_data.set_id( data.op_id );


                    try {

                        vdat.set_interval( dur.count( ) );
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

                    events.async_op( NULL, &op_data, NULL, NULL );

                    return success;

                } catch( ... ) {
                    return false;
                }
            }

            void register_for_change(::google::protobuf::RpcController* ,
                         const ::fr::proto::gpio::register_req* request,
                         ::fr::proto::gpio::register_res* response,
                         ::google::protobuf::Closure* done) override
            {
                vcomm::closure_holder holder(done);
                vtrc::uint32_t hdl(request->hdl( ).value( ));
                gpio_sptr g( gpio_by_index( hdl ) );

                int fd = g->value_fd( );

                vtrc::uint32_t opid = reactor_.next_op_id( );

                agent::reaction_callback
                        cb( vtrc::bind( &gpio_impl::value_changed, this,
                                         vtrc::placeholders::_1,
                                         value_data( hdl, fd, g,
                                                     opid ),
                                         client_) );

                reactor_.add_fd( fd, EPOLLET | EPOLLPRI, cb );
                response->set_async_op_id( opid );
            }

            void unregister(::google::protobuf::RpcController* /*controller*/,
                         const ::fr::proto::gpio::register_req* request,
                         ::fr::proto::gpio::empty*             /*response*/,
                         ::google::protobuf::Closure* done) override
            {

                vcomm::closure_holder holder(done);
                gpio_sptr g( gpio_by_index( request->hdl( ).value( ) ) );
                int fd = g->value_fd( );
                reactor_.del_fd( fd );
            }

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
            vtrc::shared_ptr<gpio_impl>
                    inst( vtrc::make_shared<gpio_impl>( app, cl ) );

            return app->wrap_service( cl, inst );
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
        if( agent::gpio::available( ) ) {
            impl_->reg_creator( gpio_impl::name( ),  create_service );
        } else {
            //std::cout << "GPIO was not found\n";
        }
    }

    void gpio::stop( )
    {
        impl_->unreg_creator( gpio_impl::name( ) );
    }

}}}

    
