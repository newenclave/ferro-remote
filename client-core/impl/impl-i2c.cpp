
#include "client-core/interfaces/II2C.h"

#include "protocol/i2c.pb.h"

#include "client-core/fr-client.h"

#include "vtrc-common/vtrc-stub-wrapper.h"
#include "vtrc-common/vtrc-rpc-channel.h"

namespace fr { namespace client { namespace interfaces { namespace i2c {

    namespace i2cproto = proto::i2c;
    namespace vcomm    = vtrc::common;
    typedef   vcomm::rpc_channel* channel_ptr;

    typedef vcomm::rpc_channel                            channel_type;
    typedef i2cproto::instance::Stub                      stub_type;
    typedef vcomm::stub_wrapper<stub_type, channel_type>  client_type;
    static const unsigned nw_flag = vcomm::rpc_channel::DISABLE_WAIT;

    namespace {

        unsigned open_device( client_type &cl,
                              unsigned bus_id,
                              unsigned slave_addr,
                              bool slave_force )
        {
            i2cproto::open_req req;
            i2cproto::open_res res;
            req.set_bus_id( bus_id );
            if( slave_addr != I2C_SLAVE_INVALID_ADDRESS ) {
                req.set_slave_id( slave_addr );
                req.set_force_slave( slave_force );
            }
            cl.call( &stub_type::open, &req, &res );
            return res.hdl( ).value( );
        }

        struct i2s_impl: public iface {

            mutable client_type                  client_;
            unsigned                             hdl_;

            i2s_impl( core::client_core &cc,
                      unsigned bid, unsigned sa, bool sf )
                :client_(cc.create_channel( ), true)
                ,hdl_(open_device( client_, bid, sa, sf ))
            { }

            i2s_impl( vtrc::common::rpc_channel *chan,
                      unsigned bid, unsigned sa, bool sf )
                :client_(chan, true)
                ,hdl_(open_device( client_, bid, sa, sf ))
            { }

            ~i2s_impl( )
            {
                client_.channel( )->set_flags( nw_flag );
                close_impl( );
            }

            vtrc::common::rpc_channel *channel( ) override
            {
                return client_.channel( );
            }

            const vtrc::common::rpc_channel *channel( ) const override
            {
                return client_.channel( );
            }

            void close_impl( )
            {
                try {
                    i2cproto::handle req;
                    req.set_value( hdl_ );
                    client_.call_request( &stub_type::close, &req );
                } catch( ... ) { ;;; }
            }

            uint64_t function_mask( ) const override
            {
                i2cproto::func_mask_req req;
                i2cproto::func_mask_res res;
                req.mutable_hdl( )->set_value( hdl_ );
                client_.call( &stub_type::func_mask, &req, &res );
                return res.mask( );
            }

            void  set_address( uint16_t addr ) const override
            {
                ioctl( i2cproto::CODE_I2C_SLAVE, addr );
            }

            void ioctl( unsigned code, uint64_t par ) const override
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
                req.mutable_request( )->set_code( cmd );

                client_.call( call, &req, &res );
                return res.data( ).value( );
            }

            template <typename CallType>
            void write_impl( CallType call, uint8_t cmd, uint32_t val ) const
            {
                i2cproto::write_read_data_req req;

                req.mutable_hdl( )->set_value( hdl_ );
                req.mutable_request( )->set_code( cmd );
                req.mutable_request( )->mutable_data( )->set_value( val );

                client_.call_request( call, &req  );
            }

            uint8_t read_byte( uint8_t cmd ) const override
            {
                return static_cast<uint8_t>
                        ( read_impl( &stub_type::read_byte, cmd ) );
            }

            void write_byte( uint8_t cmd, uint8_t value ) const override
            {
                write_impl( &stub_type::write_byte, cmd, value );
            }

            template <typename CallType, typename Res, typename Par>
            void read_many_impl( CallType call, const Par &cmds, Res &o ) const
            {
                typedef typename Par::value_type value_type;

                Res result;

                i2cproto::write_read_datas_req req;
                i2cproto::write_read_datas_res res;

                req.mutable_hdl( )->set_value( hdl_ );

                typedef typename Par::const_iterator citr;
                for( citr b(cmds.begin( )), e(cmds.end( )); b!=e; ++b ) {
                    req.mutable_request( )->add_value( )->set_code( *b );
                }

                client_.call( call, &req, &res );

                for( int i=0; i<res.response( ).value_size( ); ++i ) {

                    const
                    i2cproto::code_data &next( res.response( ).value( i ) );

                    result.push_back( std::make_pair(
                        static_cast<value_type>( next.code( ) ),
                        static_cast<value_type>( next.data( ).value( ) ) ) );
                }
                o.swap( result );
            }

            cmd_uint8_vector
            read_bytes( const uint8_vector &cmds ) const override
            {
                cmd_uint8_vector result;
                read_many_impl(&stub_type::read_bytes, cmds, result);
                return result;
            }

            template <typename CallType, typename Par>
            void write_many_impl( CallType call, const Par &cmds ) const
            {
                typedef typename Par::const_iterator citr;

                i2cproto::write_read_datas_req req;
                req.mutable_hdl( )->set_value( hdl_ );

                for( citr b(cmds.begin( )), e(cmds.end( )); b!=e; ++b ) {
                    i2cproto::code_data *next
                            ( req.mutable_request( )->add_value( ) );
                    next->set_code( b->first );
                    next->mutable_data( )->set_value( b->second );
                }
                client_.call_request( call, &req );
            }

            void write_bytes( const cmd_uint8_vector &cmds ) const override
            {
                write_many_impl( &stub_type::write_bytes, cmds );
            }

            uint16_t read_word( uint8_t cmd ) const override
            {
                return static_cast<uint8_t>
                        ( read_impl( &stub_type::read_word, cmd ) );
            }

            void write_word( uint8_t cmd, uint16_t value ) const override
            {
                write_impl( &stub_type::write_word, cmd, value );
            }

            cmd_uint16_vector
            read_words( const uint8_vector &cmds ) const override
            {
                cmd_uint16_vector result;
                read_many_impl( &stub_type::read_words, cmds, result );
                return result;
            }

            void write_words( const cmd_uint16_vector &cmds ) const override
            {
                write_many_impl( &stub_type::write_words, cmds );
            }

            std::string read_block( uint8_t cmd ) const override
            {
                i2cproto::write_read_data_req req;
                i2cproto::write_read_data_res res;

                req.mutable_hdl( )->set_value( hdl_ );
                req.mutable_request( )->set_code( cmd );

                client_.call( &stub_type::read_block, &req, &res );

                return res.data( ).block( );
            }

            void write_block( uint8_t cmd,
                              const std::string &value ) const override
            {
                i2cproto::write_read_data_req req;

                req.mutable_hdl( )->set_value( hdl_ );
                req.mutable_request( )->set_code( cmd );
                req.mutable_request( )->mutable_data( )->set_block( value );

                client_.call_request( &stub_type::write_block, &req );
            }

            std::string read_block_broken( uint8_t cmd,
                                           uint8_t len ) const override
            {
                i2cproto::write_read_data_req req;
                i2cproto::write_read_data_res res;

                req.mutable_hdl( )->set_value( hdl_ );
                req.mutable_request( )->set_code( cmd );
                i2cproto::write_read_data *mdata =
                        req.mutable_request( )->mutable_data( );
                mdata->set_broken_block( true );
                mdata->set_value( len );

                client_.call( &stub_type::read_block, &req, &res );

                return res.data( ).block( );
            }

            void write_block_broken( uint8_t cmd,
                                     const std::string &val ) const override
            {
                i2cproto::write_read_data_req req;

                req.mutable_hdl( )->set_value( hdl_ );
                req.mutable_request( )->set_code( cmd );
                i2cproto::write_read_data *mdata =
                        req.mutable_request( )->mutable_data( );
                mdata->set_block( val );
                mdata->set_broken_block( true );

                client_.call_request( &stub_type::write_block, &req );
            }


            virtual uint16_t process_call( uint8_t command,
                                           uint16_t value ) const override
            {
                i2cproto::write_read_data_req req;
                i2cproto::write_read_data_res res;

                req.mutable_hdl( )->set_value( hdl_ );
                req.mutable_request( )->set_code( command );
                req.mutable_request( )->mutable_data( )->set_value( value );

                client_.call( &stub_type::process_call, &req, &res );

                return static_cast<uint16_t>(res.data( ).value( ));
            }

            std::string process_call( uint8_t command,
                                      const std::string &value ) const override
            {
                i2cproto::write_read_data_req req;
                i2cproto::write_read_data_res res;

                req.mutable_hdl( )->set_value( hdl_ );
                req.mutable_request( )->set_code( command );
                req.mutable_request( )->mutable_data( )->set_block( value );

                client_.call( &stub_type::process_call, &req, &res );

                return res.data( ).block( );
            }

            size_t read( void *data, size_t length ) const override
            {
                i2cproto::data_block req;
                i2cproto::data_block res;

                req.set_length( length );
                req.mutable_hdl( )->set_value( hdl_ );

                client_.call( &stub_type::read, &req, &res );

                memcpy( data,
                        res.data( ).empty( ) ? "" : &res.data( )[0],
                        res.data( ).size( ) );

                return res.data( ).size( );
            }

            size_t write( const void *data, size_t length ) const override
            {
                i2cproto::data_block req;
                i2cproto::data_block res;

                req.set_data( data, length );
                req.mutable_hdl( )->set_value( hdl_ );

                client_.call( &stub_type::write, &req, &res );

                return res.length( );
            }
        };
    }

    iface_ptr open( core::client_core &cc, unsigned bus_id )
    {
        return new i2s_impl( cc, bus_id, I2C_SLAVE_INVALID_ADDRESS, false );
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
