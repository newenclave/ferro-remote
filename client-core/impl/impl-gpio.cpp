
#include "interfaces/IGPIO.h"
#include "protocol/gpio.pb.h"

#include "client-core/fr-client.h"

#include "vtrc-common/vtrc-stub-wrapper.h"
#include "vtrc-common/vtrc-rpc-channel.h"

#include "vtrc-stdint.h"

namespace fr { namespace client { namespace interfaces {

    namespace {

        namespace vcomm = vtrc::common;
        namespace gproto = protocol::gpio;

        typedef gproto::instance::Stub stub_type;
        typedef vcomm::stub_wrapper<stub_type> client_type;

        vtrc::uint32_t open_instance( client_type &cl, unsigned gpio_id )
        {
            gproto::open_req req;
            gproto::open_res res;
            req.set_gpio_id( gpio_id );

            cl.call( &stub_type::open, &req, &res );

            return res.hdl( ).value( );
        }

        vtrc::uint32_t open_setup_inst( client_type &cl, unsigned gpio_id,
                                        gproto::setup_data &setup )
        {
            gproto::open_req req;
            gproto::open_res res;

            req.set_gpio_id( gpio_id );
            req.set_exp( true );
            req.mutable_setup( )->CopyFrom( setup );

            cl.call( &stub_type::open, &req, &res );
            return res.hdl( ).value( );

        }

        struct gpio_impl: public gpio::iface {

            unsigned            id_;
            mutable client_type client_;
            vtrc::uint32_t      hdl_;

            gpio_impl( core::client_core &cl, unsigned id )
                :id_(id)
                ,client_(cl.create_channel( ), true)
                ,hdl_(open_instance( client_, id_ ))
            { }

            gpio_impl( core::client_core &cl, unsigned id,
                       gpio::direction_type dir, unsigned value )
                :id_(id)
                ,client_(cl.create_channel( ), true)
            {
                gproto::setup_data setup;
                setup.set_direction( dir );
                if( ( dir == gpio::DIRECT_OUT ) && ( value != 0 ) ) {
                    setup.set_value( 1 );
                }
                hdl_ = open_setup_inst( client_, id_, setup );
            }

            unsigned value( ) const override
            {
                gproto::info_req    req;
                gproto::setup_data  res;
                req.mutable_hdl( )->set_value( hdl_ );

                req.set_with_value( true );
                client_.call( &stub_type::info, &req, &res );
                return res.value( );
            }

            void set_value( unsigned value ) const override
            {
                gproto::setup_req req;
                req.mutable_hdl( )->set_value( hdl_ );
                req.mutable_setup( )->set_value( value );
                client_.call_request( &stub_type::setup, &req );
            }

            void export_device( ) const override
            {
                gproto::export_req req;
                req.mutable_hdl( )->set_value( hdl_ );
                client_.call_request( &stub_type::exp, &req );
            }

            void unexport_device( ) const override
            {
                gproto::export_req req;
                req.mutable_hdl( )->set_value( hdl_ );
                client_.call_request( &stub_type::unexp, &req );
            }

            gpio::direction_type direction( ) const override
            {
                gproto::info_req    req;
                gproto::setup_data  res;

                req.mutable_hdl( )->set_value( hdl_ );
                req.set_with_direction( true );
                client_.call( &stub_type::info, &req, &res );

                return res.direction( ) == gpio::DIRECT_IN
                        ? gpio::DIRECT_IN
                        : gpio::DIRECT_OUT;
            }

            void set_direction( gpio::direction_type value ) const override
            {
                gproto::setup_req req;
                req.mutable_hdl( )->set_value( hdl_ );
                req.mutable_setup( )->set_direction( value );
                client_.call_request( &stub_type::setup, &req );
            }

            gpio::edge_type edge( ) const override
            {
                gproto::info_req    req;
                gproto::setup_data  res;

                req.mutable_hdl( )->set_value( hdl_ );
                req.set_with_edge( true );
                client_.call( &stub_type::info, &req, &res );

                switch (res.edge( )) {
                case gpio::EDGE_NONE:
                    return gpio::EDGE_NONE;
                case gpio::EDGE_FALLING:
                    return gpio::EDGE_FALLING;
                case gpio::EDGE_RISING:
                    return gpio::EDGE_RISING;
                case gpio::EDGE_BOTH:
                    return gpio::EDGE_BOTH;
                }
                return gpio::EDGE_NONE;
            }

            void set_edge( gpio::edge_type value ) const override
            {
                gproto::setup_req req;
                req.mutable_hdl( )->set_value( hdl_ );
                req.mutable_setup( )->set_edge( value );
                client_.call_request( &stub_type::setup, &req );
            }

        };
    }

    namespace gpio {
        iface_ptr create( core::client_core &cl, unsigned gpio_id )
        {
            return new gpio_impl( cl, gpio_id );
        }

        iface_ptr create_output( core::client_core &cl, unsigned gpio_id,
                                 unsigned value )
        {
            return new gpio_impl( cl, gpio_id, gpio::DIRECT_OUT, value );
        }

        iface_ptr create_input( core::client_core &cl, unsigned gpio_id )
        {
            return new gpio_impl( cl, gpio_id, gpio::DIRECT_IN, 0 );
        }
    }

}}}
