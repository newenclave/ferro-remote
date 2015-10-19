#include "client-core/interfaces/ISPI.h"
#include "protocol/spi.pb.h"

#include "client-core/fr-client.h"

#include "vtrc-common/vtrc-stub-wrapper.h"
#include "vtrc-common/vtrc-rpc-channel.h"

namespace fr { namespace client { namespace interfaces {

    namespace {

        namespace sproto = fr::proto::spi;
        namespace vcomm = vtrc::common;
        typedef vcomm::rpc_channel                           channel_type;
        typedef sproto::instance::Stub                       stub_type;
        typedef vcomm::stub_wrapper<stub_type, channel_type> client_type;
        static const unsigned nw_flag = vcomm::rpc_channel::DISABLE_WAIT;

        vtrc::uint32_t open_instance( client_type &cl,
                                      unsigned bus, unsigned channel,
                                      unsigned speed, unsigned mode )
        {
            sproto::open_req req;
            sproto::open_res res;
            req.set_bus( bus );
            req.set_channel( channel );
            if( speed ) {
                req.mutable_setup( )->set_speed( speed );
                req.mutable_setup( )->set_mode( mode );
            }
            cl.call( &stub_type::open, &req, &res );

            return res.hdl( ).value( );
        }

        class spi_impl: public spi::iface {

            mutable client_type client_;
            vtrc::uint32_t      hdl_;

        public:

            spi_impl( core::client_core &cl,
                      unsigned bus, unsigned channel,
                      unsigned speed, unsigned mode )
                :client_(cl.create_channel( ), true)
                ,hdl_(open_instance(client_, bus, channel, speed, mode))
            { }

            ~spi_impl( )
            {
                if( hdl_ != 0 ) try {
                    client_.channel( )->set_flags( nw_flag );
                    close_impl( );
                } catch( ... ) {  }
            }

            vtrc::common::rpc_channel *channel( ) override
            {
                return client_.channel( );
            }

            const vtrc::common::rpc_channel *channel( ) const override
            {
                return client_.channel( );
            }

            void close_impl( ) const
            {
                proto::handle hdl;
                hdl.set_value( hdl_ );
                client_.call_request( &stub_type::close, &hdl );
            }

            void setup( unsigned speed, unsigned mode ) const override
            {
                sproto::setup_req req;
                req.mutable_hdl( )->set_value( hdl_ );
                req.mutable_setup( )->set_speed( speed );
                req.mutable_setup( )->set_mode( mode );
                client_.call_request( &stub_type::setup, &req );
            }

            std::string read( size_t len ) const override
            {
                proto::spi::read_req req;
                proto::spi::read_res res;
                req.mutable_hdl( )->set_value( hdl_ );
                req.set_len( len );
                client_.call( &stub_type::read, &req, &res );
                return res.data( );
            }

            void write( const void *data, size_t len ) const override
            {
                proto::spi::write_req req;
                req.mutable_hdl( )->set_value( hdl_ );
                req.mutable_data( )->assign( static_cast<const char *>(data),
                                             len );
                client_.call_request( &stub_type::write, &req );
            }

            spi::string_vector wr( const spi::string_vector &d ) const override
            {
                typedef spi::string_vector::const_iterator citr;
                proto::spi::wr_req req;
                proto::spi::wr_res res;
                req.mutable_hdl( )->set_value( hdl_ );

                for( citr b(d.begin( )), e(d.end( )); b!=e; ++b ) {
                    req.add_data( *b );
                }

                client_.call( &stub_type::wr, &req, &res );
                return spi::string_vector( res.data( ).begin( ),
                                           res.data( ).end( ) );
            }

            std::string transfer( const unsigned char *data,
                                  size_t len ) const override
            {
                sproto::transfer_req req;
                sproto::transfer_res res;
                req.mutable_hdl( )->set_value( hdl_ );
                req.set_data( data, len );
                client_.call( &stub_type::transfer, &req, &res );
                return res.data( );
            }

            spi::string_vector
                transfer( const spi::string_vector &data ) const override
            {
                typedef spi::string_vector::const_iterator citr;
                sproto::transfer_list_req req;
                sproto::transfer_list_res res;
                req.mutable_hdl( )->set_value( hdl_ );
                for( citr b(data.begin( )), e(data.end( )); b!=e; ++b ) {
                    req.add_datas( *b );
                }
                client_.call( &stub_type::transfer_list, &req, &res );
                return spi::string_vector( res.datas( ).begin( ),
                                           res.datas( ).end( ) );

            }

            void close( ) const override
            {
                close_impl( );
            }
        };
    }

    namespace spi {

        iface_ptr open( core::client_core &cl,
                          unsigned bus, unsigned channel,
                          unsigned speed, unsigned mode )
        {
            return new spi_impl( cl, bus, channel, speed, mode );
        }

        iface_ptr open( core::client_core &cl, unsigned channel,
                          unsigned speed, unsigned mode )
        {
            return open( cl, 0, channel, speed, mode );
        }
    }

}}}
