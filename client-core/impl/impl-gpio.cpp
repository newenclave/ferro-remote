#include <iostream>

#include "client-core/interfaces/IGPIO.h"
#include "client-core/interfaces/IAsyncOperation.h"

#include "protocol/gpio.pb.h"

#include "client-core/fr-client.h"

#include "vtrc-common/vtrc-stub-wrapper.h"
#include "vtrc-common/vtrc-rpc-channel.h"

#include "vtrc-stdint.h"
#include "vtrc-bind.h"
#include "vtrc-memory.h"

namespace fr { namespace client { namespace interfaces {

    namespace {

        namespace vcomm = vtrc::common;
        namespace gproto = proto::gpio;

        typedef vcomm::rpc_channel                           channel_type;
        typedef gproto::instance::Stub                       stub_type;
        typedef vcomm::stub_wrapper<stub_type, channel_type> client_type;
        static const unsigned nw_flag = vcomm::rpc_channel::DISABLE_WAIT;

        struct instance_info {
            vtrc::uint32_t hdl_;
            bool edge_available_;
            instance_info( )
                :hdl_(0)
                ,edge_available_(false)
            { }

            instance_info( vtrc::uint32_t h, bool ea )
                :hdl_(h)
                ,edge_available_(ea)
            { }
        };

        instance_info open_instance( client_type &cl, unsigned gpio_id )
        {
            gproto::open_req req;
            gproto::open_res res;
            req.set_gpio_id( gpio_id );

            cl.call( &stub_type::open, &req, &res );

            return instance_info(res.hdl( ).value( ), res.edge_supported( ));
        }

        instance_info open_setup_inst( client_type &cl, unsigned gpio_id,
                                        gproto::setup_data &setup )
        {
            gproto::open_req req;
            gproto::open_res res;

            req.set_gpio_id( gpio_id );
            req.set_exp( true );
            req.mutable_setup( )->CopyFrom( setup );

            cl.call( &stub_type::open, &req, &res );

            return instance_info(res.hdl( ).value( ), res.edge_supported( ));

        }

        struct gpio_impl: public gpio::iface {

            core::client_core  &core_;
            unsigned            id_;
            mutable client_type client_;
            instance_info       ii_;

            gpio_impl( core::client_core &cl, unsigned id )
                :core_(cl)
                ,id_(id)
                ,client_(core_.create_channel( ), true)
                ,ii_(open_instance( client_, id_ ))
            { }

            gpio_impl( core::client_core &cl )
                :core_(cl)
                ,id_(0)
                ,client_(core_.create_channel( ), true)
            { }

            gpio_impl( core::client_core &cl, unsigned id,
                       gpio::direction_type dir, unsigned value )
                :core_(cl)
                ,id_(id)
                ,client_(core_.create_channel( ), true)
            {
                gproto::setup_data setup;
                setup.set_direction( dir );
                if( ( dir == gpio::DIRECT_OUT ) && ( value != 0 ) ) {
                    setup.set_value( 1 );
                }

                ii_ = open_setup_inst( client_, id_, setup );
            }


            gpio_impl( core::client_core &cl, vtrc::common::rpc_channel *chan,
                       unsigned id )
                :core_(cl)
                ,id_(id)
                ,client_(chan, true)
                ,ii_(open_instance( client_, id_ ))
            { }

            gpio_impl( core::client_core &cl, vtrc::common::rpc_channel *chan )
                :core_(cl)
                ,id_(0)
                ,client_(chan, true)
            { }

            gpio_impl( core::client_core &cl, vtrc::common::rpc_channel *chan,
                       unsigned id,
                       gpio::direction_type dir, unsigned value )
                :core_(cl)
                ,id_(id)
                ,client_(chan, true)
            {
                gproto::setup_data setup;
                setup.set_direction( dir );
                if( ( dir == gpio::DIRECT_OUT ) && ( value != 0 ) ) {
                    setup.set_value( 1 );
                }

                ii_ = open_setup_inst( client_, id_, setup );
            }

            ~gpio_impl( )
            {
                if( ii_.hdl_ != 0 ) try {
                    client_.channel( )->set_flags( nw_flag );
                    close_impl( );
                } catch( ... ) {  }
            }

            vtrc::common::rpc_channel *channel( )
            {
                return client_.channel( );
            }

            const vtrc::common::rpc_channel *channel( ) const
            {
                return client_.channel( );
            }

            void close_impl( )
            {
                gproto::handle req;
                req.set_value( ii_.hdl_ );
                client_.call_request( &stub_type::close, &req );
            }

            bool available(  ) const
            {
                try {
                    client_.call( &stub_type::ping );
                    return true;
                } catch( ... ) { }
                return false;
            }

            unsigned id( ) const
            {
                return id_;
            }

            unsigned value( ) const override
            {
                gproto::info_req    req;
                gproto::setup_data  res;
                req.mutable_hdl( )->set_value( ii_.hdl_ );

                req.set_with_value( true );
                client_.call( &stub_type::info, &req, &res );
                return res.value( );
            }

            void set_value( unsigned value ) const override
            {
                gproto::setup_req req;
                req.mutable_hdl( )->set_value( ii_.hdl_ );
                req.mutable_setup( )->set_value( value );
                client_.call_request( &stub_type::setup, &req );
            }

            void  make_pulse( vtrc::uint64_t length,
                              unsigned sv, unsigned rv ) const
            {
                gproto::pulse_req req;
                req.mutable_hdl( )->set_value( ii_.hdl_ );
                req.set_length( length );
                req.set_set_value( sv );
                req.set_reset_value( rv );
                client_.call_request( &stub_type::make_pulse, &req );
            }

            unsigned active_low( ) const
            {
                gproto::info_req    req;
                gproto::setup_data  res;
                req.mutable_hdl( )->set_value( ii_.hdl_ );

                req.set_with_active_low( true );
                client_.call( &stub_type::info, &req, &res );
                return res.active_low( );
            }

            void  set_active_low( unsigned value ) const
            {
                gproto::setup_req req;
                req.mutable_hdl( )->set_value( ii_.hdl_ );
                req.mutable_setup( )->set_active_low( value );
                client_.call_request( &stub_type::setup, &req );
            }

            void export_device( ) const override
            {
                gproto::export_req req;
                req.mutable_hdl( )->set_value( ii_.hdl_ );
                client_.call_request( &stub_type::exp, &req );
            }

            void unexport_device( ) const override
            {
                gproto::export_req req;
                req.mutable_hdl( )->set_value( ii_.hdl_ );
                client_.call_request( &stub_type::unexp, &req );
            }

            static gpio::direction_type dir_value2enum( unsigned val )
            {
                return gpio::direction_val2enum( val );
            }

            gpio::direction_type direction( ) const override
            {
                gproto::info_req    req;
                gproto::setup_data  res;

                req.mutable_hdl( )->set_value( ii_.hdl_ );
                req.set_with_direction( true );
                client_.call( &stub_type::info, &req, &res );

                return dir_value2enum(res.direction( ));
            }

            void set_direction( gpio::direction_type value ) const override
            {
                gproto::setup_req req;
                req.mutable_hdl( )->set_value( ii_.hdl_ );
                req.mutable_setup( )->set_direction( value );
                client_.call_request( &stub_type::setup, &req );
            }

            static gpio::edge_type edge_value2enum( unsigned val )
            {
                return gpio::edge_val2enum( val );
            }

            bool edge_supported( ) const
            {
                return ii_.edge_available_;
            }

            gpio::edge_type edge( ) const override
            {
                gproto::info_req    req;
                gproto::setup_data  res;

                req.mutable_hdl( )->set_value( ii_.hdl_ );
                req.set_with_edge( true );
                client_.call( &stub_type::info, &req, &res );

                return edge_value2enum( res.edge( ) );
            }

            void set_edge( gpio::edge_type value ) const override
            {
                gproto::setup_req req;
                req.mutable_hdl( )->set_value( ii_.hdl_ );
                req.mutable_setup( )->set_edge( value );
                client_.call_request( &stub_type::setup, &req );
            }

            void event_handler( unsigned err,
                                const std::string &data,
                                gpio::value_change_callback &cb ) const
            {
                gproto::value_change_data vcd;
                vcd.ParseFromString( data );
                cb( err, vcd.new_value( ) );
            }

            void event_handler2( unsigned err,
                                 const std::string &data,
                                gpio::value_change_interval_callback &cb ) const
            {
                gproto::value_change_data vcd;
                vcd.ParseFromString( data );
                cb( err, vcd.new_value( ), vcd.interval( ) );
            }

            void register_for_change( gpio
                                   ::value_change_callback cb ) const override
            {
                gproto::register_req req;
                gproto::register_res res;

                req.mutable_hdl( )->set_value( ii_.hdl_ );

                client_.call( &stub_type::register_for_change, &req, &res );

                async_op_callback_type acb(
                            vtrc::bind( &gpio_impl::event_handler, this,
                                        vtrc::placeholders::_1,
                                        vtrc::placeholders::_2,
                                        cb ) );

                core_.register_async_op( res.async_op_id( ), acb );
            }

            void register_for_change_int( gpio
                         ::value_change_interval_callback cb ) const override
            {
                gproto::register_req req;
                gproto::register_res res;

                req.mutable_hdl( )->set_value( ii_.hdl_ );

                client_.call( &stub_type::register_for_change, &req, &res );

                async_op_callback_type acb(
                            vtrc::bind( &gpio_impl::event_handler2, this,
                                        vtrc::placeholders::_1,
                                        vtrc::placeholders::_2,
                                        cb ) );

                core_.register_async_op( res.async_op_id( ), acb );
            }

            void unregister_impl( ) const
            {
                gproto::register_req req;
                req.mutable_hdl( )->set_value( ii_.hdl_ );
                client_.call_request( &stub_type::unregister, &req );
            }

            void unregister( ) const override
            {
                unregister_impl( );
            }

            void set_info( const gpio::info &inf ) const override
            {
                gproto::setup_req req;
                req.mutable_hdl( )->set_value( ii_.hdl_ );
                req.mutable_setup( )->set_direction( inf.direction );
                req.mutable_setup( )->set_value( inf.value );
                req.mutable_setup( )->set_active_low( inf.active_low );
                req.mutable_setup( )->set_edge( inf.edge );
                client_.call_request( &stub_type::setup, &req );
            }

            gpio::info get_info( ) const override
            {
                gpio::info result;
                gproto::info_req    req;
                gproto::setup_data  res;

                req.mutable_hdl( )->set_value( ii_.hdl_ );

                result.id = id_;
                req.set_with_active_low( true );
                req.set_with_value(      true );
                req.set_with_edge(       true );
                req.set_with_direction(  true );
                client_.call( &stub_type::info, &req, &res );

                result.value      = res.value( );
                result.active_low = res.active_low( );
                result.direction  = dir_value2enum( res.direction( ) );
                result.edge       = edge_value2enum( res.edge( ) );

                return result;
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

        bool available( core::client_core &cl )
        {
            vtrc::unique_ptr<gpio_impl> i( new gpio_impl( cl ) );
            return i->available( );
        }
    }

}}}
