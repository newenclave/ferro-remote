#include <map>

#include "application.h"
#include "subsys-gpio.h"
#include "subsys-reactor.h"

#include "protocol/gpio.pb.h"

#include "gpio.h"

#include "vtrc-common/vtrc-closure-holder.h"

#include "vtrc-stdint.h"
#include "vtrc-atomic.h"
#include "vtrc-memory.h"

#include "vtrc-common/vtrc-mutex-typedefs.h"

#include "errno-check.h"

namespace fr { namespace server { namespace subsys {

    namespace {

        const std::string subsys_name( "gpio" );
        namespace gproto = fr::protocol::gpio;
        namespace vcomm = vtrc::common;

        typedef vtrc::shared_ptr<server::gpio_helper> gpio_sptr;

        typedef std::map<vtrc::uint32_t, gpio_sptr> gpio_map;

        server::gpio::direction_type direct_from_proto( unsigned dir )
        {
            switch ( dir ) {
            case server::gpio::DIRECT_IN:
                return server::gpio::DIRECT_IN;
            case server::gpio::DIRECT_OUT:
                return server::gpio::DIRECT_OUT;
            default:
                vcomm::throw_system_error( EINVAL, "Bad direction" );
            }
        }

        server::gpio::edge_type edge_from_proto( unsigned edge )
        {
            switch ( edge ) {
            case server::gpio::EDGE_NONE:
                return server::gpio::EDGE_NONE;
            case server::gpio::EDGE_FALLING:
                return server::gpio::EDGE_FALLING;
            case server::gpio::EDGE_RISING:
                return server::gpio::EDGE_RISING;
            case server::gpio::EDGE_BOTH:
                return server::gpio::EDGE_BOTH;
            default:
                vcomm::throw_system_error( EINVAL, "Bad edge" );
            }
        }

        class gpio_impl: public gproto::instance {

            gpio_map                     gpio_;
            vtrc::shared_mutex           gpio_lock_;
            vtrc::atomic<vtrc::uint32_t> index_;

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
                    ng->set_value( val != 0 ? 1 : 0 );
                }
            }

            void open(::google::protobuf::RpcController* /*controller*/,
                         const ::fr::protocol::gpio::open_req* request,
                         ::fr::protocol::gpio::open_res* response,
                         ::google::protobuf::Closure* done) override
            {
                vcomm::closure_holder holder(done);

                vtrc::uint32_t newid = next_index( );
                vtrc::uint32_t id = request->gpio_id( );

                gpio_sptr ng( vtrc::make_shared<server::gpio_helper>( id ) );

                if( request->exp( ) ) {
                    if( !ng->exists( ) ) {
                        ng->exp( );
                    }
                    gpio_setup( ng, request->setup( ) );
                }
                response->mutable_hdl( )->set_value( newid );
                vtrc::unique_shared_lock lck( gpio_lock_ );
                gpio_.insert( std::make_pair( newid, ng ) );
            }

            void exp(::google::protobuf::RpcController* /*controller*/,
                         const ::fr::protocol::gpio::export_req* request,
                         ::fr::protocol::gpio::empty* /*response*/,
                         ::google::protobuf::Closure* done) override
            {
                vcomm::closure_holder holder(done);
                gpio_sptr g( gpio_by_index(request->hdl( ).value( ) ) );
                if( !g->exists( ) ) {
                    g->exp( );
                }
            }

            void setup(::google::protobuf::RpcController* /*controller*/,
                         const ::fr::protocol::gpio::setup_req* request,
                         ::fr::protocol::gpio::empty* /*response*/,
                         ::google::protobuf::Closure* done) override
            {
                vcomm::closure_holder holder(done);
                gpio_sptr g( gpio_by_index(request->hdl( ).value( ) ) );
                gpio_setup( g, request->setup( ) );
            }

            void unexp(::google::protobuf::RpcController* /*controller*/,
                         const ::fr::protocol::gpio::export_req* request,
                         ::fr::protocol::gpio::empty* /*response*/,
                         ::google::protobuf::Closure* done) override
            {
                vcomm::closure_holder holder(done);
                gpio_sptr g( gpio_by_index(request->hdl( ).value( ) ) );
                g->unexp( );
            }

            void info(::google::protobuf::RpcController* controller,
                         const ::fr::protocol::gpio::info_req* request,
                         ::fr::protocol::gpio::setup_data* response,
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

            }

            void close(::google::protobuf::RpcController* /*controller*/,
                         const ::fr::protocol::gpio::handle* request,
                         ::fr::protocol::gpio::empty* response,
                         ::google::protobuf::Closure* done) override
            {
                vcomm::closure_holder holder(done);
                vtrc::upgradable_lock ulck( gpio_lock_ );
                gpio_map::iterator f( gpio_.find( request->value( ) ) );
                if( f!= gpio_.end( ) ) {
                    vtrc::upgrade_to_unique utl(ulck);
                    gpio_.erase( f );
                }
            }

        public:

            gpio_impl( fr::server::application */*app*/,
                       vtrc::common::connection_iface_wptr /*cli*/ )
                :index_(100)
            { }

            static const std::string &name( )
            {
                return gproto::instance::descriptor( )->full_name( );
            }
        };

        application::service_wrapper_sptr create_service(
                                      fr::server::application *app,
                                      vtrc::common::connection_iface_wptr cl )
        {
            vtrc::shared_ptr<gpio_impl>
                    inst(vtrc::make_shared<gpio_impl>( app, cl ));

            return application::service_wrapper_sptr(
                    vtrc::make_shared<application::service_wrapper>( inst ) );
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
        impl_->reg_creator( gpio_impl::name( ),  create_service );
    }

    void gpio::stop( )
    {
        impl_->unreg_creator( gpio_impl::name( ) );
    }

}}}

    
