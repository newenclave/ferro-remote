
#include "client-core/interfaces/II2C.h"

#include "protocol/i2c.pb.h"

#include "client-core/fr-client.h"

#include "vtrc-common/vtrc-stub-wrapper.h"
#include "vtrc-common/vtrc-rpc-channel.h"

namespace fr { namespace client { namespace interfaces { namespace i2c {

    namespace i2cproto = proto::i2c;
    namespace vcomm    = vtrc::common;
    typedef   vcomm::rpc_channel* channel_ptr;

    typedef i2cproto::instance::Stub        stub_type;
    typedef vcomm::stub_wrapper<stub_type>  client_type;

    namespace {

        unsigned open_device( client_type &cl,
                              unsigned bus_id,
                              unsigned slave_addr,
                              bool slave_force )
        {
            i2cproto::open_req req;
            i2cproto::open_res res;
            req.set_bus_id( bus_id );
            if( slave_addr != 0xFFFFFFFF ) {
                req.set_slave_id( slave_addr );
                req.set_force_slave( slave_force );
            }
            cl.call( &stub_type::open, &req, &res );
            return res.hdl( ).value( );
        }

        struct i2s_impl: public iface {

            mutable client_type client_;
            unsigned            hdl_;

            i2s_impl( core::client_core &cc,
                      unsigned bid, unsigned sa, bool sf )
                :client_(cc.create_channel( ), true)
                ,hdl_(open_device( client_, bid, sa, sf ))
            { }

            ~i2s_impl( )
            {
                close_impl( );
            }

            void close_impl( ) try
            {
                i2cproto::handle req;
                req.set_value( hdl_ );
                client_.call_request( &stub_type::close, &req );
            } catch( ... ) { ;;; }

            uint64_t function_mask( ) const
            {
                i2cproto::func_mask_req req;
                i2cproto::func_mask_res res;
                req.mutable_hdl( )->set_value( hdl_ );
                client_.call( &stub_type::func_mask, &req, &res );
                return res.mask( );
            }

            void  set_address( uint16_t addr ) const
            {
                ioctl( i2cproto::CODE_I2C_SLAVE, addr );
            }

            void ioctl( unsigned code, uint64_t par ) const
            {
                i2cproto::ioctl_req req;
                req.mutable_hdl( )->set_value( hdl_ );
                req.set_code( code );
                req.set_parameter( par );
                client_.call_request( &stub_type::ioctl, &req );
            }

            template <typename CallType>
            uint32_t read_impl( CallType call, uint8_t cmd ) const
            {
                i2cproto::write_read_data_req req;
                i2cproto::write_read_data_res res;

                req.mutable_hdl( )->set_value( hdl_ );
                req.set_code( cmd );

                client_.call( call, &req, &res );
                return res.data( ).value( );
            }

            template <typename CallType>
            void write_impl( CallType call, uint8_t cmd, uint32_t val ) const
            {
                i2cproto::write_read_data_req req;

                req.mutable_hdl( )->set_value( hdl_ );
                req.set_code( cmd );
                req.mutable_data( )->set_value( val );

                client_.call_request( call, &req  );
            }

            uint8_t read_byte( uint8_t cmd ) const
            {
                return static_cast<uint8_t>
                        ( read_impl( &stub_type::read_byte, cmd ) );
            }

            void write_byte( uint8_t cmd, uint8_t value ) const
            {
                write_impl( &stub_type::write_byte, cmd, value );
            }

            uint16_t read_word( uint8_t cmd ) const
            {
                return static_cast<uint8_t>
                        ( read_impl( &stub_type::read_word, cmd ) );
            }

            void write_word( uint8_t cmd, uint16_t value ) const
            {
                write_impl( &stub_type::write_word, cmd, value );
            }

            std::string read_block( uint8_t cmd ) const
            {
                i2cproto::write_read_data_req req;
                i2cproto::write_read_data_res res;

                req.mutable_hdl( )->set_value( hdl_ );
                req.set_code( cmd );

                client_.call( &stub_type::read_block, &req, &res );

                return res.data( ).block( );
            }

            void write_block( uint8_t cmd, const std::string &value ) const
            {
                i2cproto::write_read_data_req req;

                req.mutable_hdl( )->set_value( hdl_ );
                req.set_code( cmd );
                req.mutable_data( )->set_block( value );

                client_.call_request( &stub_type::write_block, &req );
            }

            std::string read_block_broken( uint8_t cmd, uint8_t len ) const
            {
                i2cproto::write_read_data_req req;
                i2cproto::write_read_data_res res;

                req.mutable_hdl( )->set_value( hdl_ );
                req.set_code( cmd );
                req.mutable_data( )->set_broken_block( true );
                req.mutable_data( )->set_value( len );

                client_.call( &stub_type::read_block, &req, &res );

                return res.data( ).block( );
            }

            void write_block_broken( uint8_t cmd, const std::string &val ) const
            {
                i2cproto::write_read_data_req req;

                req.mutable_hdl( )->set_value( hdl_ );
                req.set_code( cmd );
                req.mutable_data( )->set_block( val );
                req.mutable_data( )->set_broken_block( true );

                client_.call_request( &stub_type::write_block, &req );
            }

            size_t read( void *data, size_t length ) const
            {
                i2cproto::data_block req;
                i2cproto::data_block res;

                req.set_length( length );

                client_.call( &stub_type::read, &req, &res );

                memcpy( data,
                        res.data( ).empty( ) ? "" : &res.data( )[0],
                        res.data( ).size( ) );

                return res.data( ).size( );
            }

            size_t write( const void *data, size_t length ) const
            {
                i2cproto::data_block req;
                i2cproto::data_block res;

                req.set_data( data, length );

                client_.call( &stub_type::write, &req, &res );

                return res.length( );
            }
        };
    }


    iface_ptr open( core::client_core &cc, unsigned bus_id )
    {
        return new i2s_impl( cc, bus_id, 0xFFFFFFFF, false );
    }

    iface_ptr open( core::client_core &cc,
                    unsigned bus_id, unsigned slave_addr )
    {
        return new i2s_impl( cc, bus_id, slave_addr, false );
    }

    iface_ptr open( core::client_core &cc,
                    unsigned bus_id, unsigned slave_addr, bool slave_force )
    {
        return new i2s_impl( cc, bus_id, slave_addr, slave_force );
    }

    bool bus_available(core::client_core &cc, unsigned bus_id )
    {
        client_type calls( cc.create_channel( ), true );
        i2cproto::bus_available_req req;
        i2cproto::bus_available_res res;
        req.set_bus_id( bus_id );
        calls.call( &stub_type::bus_available, &req, &res );
        return res.value( );
    }
}

}}}
