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

        typedef proto::spi::register_info register_info;
        typedef register_info::register_bits register_bits;

        template <int T>
        struct type_to_enum;

        template <>
        struct type_to_enum<8> {
            typedef uint8_t type;
            typedef spi::cmd_uint8_vector res_type;
            static const register_bits value = register_info::REG_8BIT;
        };

        template <>
        struct type_to_enum<16> {
            typedef uint16_t type;
            typedef spi::cmd_uint16_vector res_type;
            static const register_bits value = register_info::REG_16BIT;
        };

        template <>
        struct type_to_enum<32> {
            typedef uint32_t type;
            typedef spi::cmd_uint32_vector res_type;
            static const register_bits value = register_info::REG_32BIT;
        };

        template <>
        struct type_to_enum<64> {
            typedef uint64_t type;
            typedef spi::cmd_uint64_vector res_type;
            static const register_bits value = register_info::REG_64BIT;
        };

//        register_bits bits_val2enum( unsigned val )
//        {
//            switch (val) {
//            case register_info::REG_8BIT:
//            case register_info::REG_16BIT:
//            case register_info::REG_32BIT:
//            case register_info::REG_64BIT:
//                return static_cast<register_bits>(val);
//            }
//            return register_info::REG_8BIT;
//        }

        vtrc::uint32_t open_instance( client_type &cl,
                                      unsigned bus, unsigned channel,
                                      unsigned speed, unsigned mode )
        {
            sproto::open_req req;
            sproto::open_res res;
            req.set_bus( bus );
            req.set_channel( channel );
            req.mutable_setup( )->set_speed( speed );
            req.mutable_setup( )->set_mode( mode );

            cl.call( &stub_type::open, &req, &res );

            return res.hdl( ).value( );
        }

        class spi_impl: public spi::iface {

            mutable client_type client_;
            vtrc::uint32_t      hdl_;
            vtrc::uint32_t      addr_;

        public:

            spi_impl( core::client_core &cl,
                      unsigned bus, unsigned channel,
                      unsigned speed, unsigned mode )
                :client_(cl.create_channel( ), true)
                ,hdl_(open_instance(client_, bus, channel, speed, mode))
                ,addr_(0)
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

            void set_address( unsigned address ) override
            {
                addr_ = address;
            }

            void setup( unsigned speed, unsigned mode ) const override
            {
                sproto::setup_req req;
                req.mutable_hdl( )->set_value( hdl_ );
                req.mutable_setup( )->set_speed( speed );
                req.mutable_setup( )->set_speed( mode );
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

            template <int T>
            typename type_to_enum<T>::res_type
                get_regs( const spi::uint8_vector &regs ) const
            {
                typedef spi::uint8_vector::const_iterator citr;
                typename type_to_enum<T>::res_type ret;

                proto::spi::get_register_list_req req;
                proto::spi::get_register_list_res res;

                req.mutable_hdl( )->set_value( hdl_ );

                for( citr b(regs.begin( )), e(regs.end( )); b!=e; ++b ) {
                    proto::spi::register_info *ni = req.add_infos( );
                    ni->set_address( addr_ );
                    ni->set_bits( type_to_enum<T>::value );
                    ni->set_reg( *b );
//                    std::cout << "Push: " << std::hex << addr_
//                                 << ":" << ni->value( ) << "\n";
                }

                client_.call( &stub_type::get_register_list, &req, &res );

                ret.reserve( res.infos_size( ) );
                for( int i=0; i<res.infos_size( ); i++ ) {
                    const proto::spi::register_info &ni(res.infos(i));
//                    std::cout << "Push: " << ni.reg( )
//                                 << ":" << ni.value( ) << "\n";
                    ret.push_back( std::make_pair( ni.reg( ), ni.value( ) ) );
                }
                return ret;
            }

            spi::cmd_uint8_vector
                read_regs8( const spi::uint8_vector &regs ) const override
            {
                return get_regs<8>( regs );
            }

            spi::cmd_uint16_vector
                read_regs16( const spi::uint8_vector &regs ) const override
            {
                return get_regs<16>( regs );
            }

            spi::cmd_uint32_vector
                read_regs32( const spi::uint8_vector &regs ) const override
            {
                return get_regs<32>( regs );
            }

            spi::cmd_uint64_vector
                read_regs64( const spi::uint8_vector &regs ) const override
            {
                return get_regs<64>( regs );
            }

            template <int T>
            void write_impl( const typename type_to_enum<T>::res_type &d ) const
            {
                typedef typename type_to_enum<T>::res_type::const_iterator citr;
                proto::spi::set_register_list_req req;
                proto::spi::set_register_list_res res;
                req.mutable_hdl( )->set_value( hdl_ );

                for( citr b(d.begin( )), e(d.end( )); b!=e; ++b ) {
                    proto::spi::register_info *ni = req.add_infos( );
                    ni->set_address( addr_ );
                    ni->set_bits( type_to_enum<T>::value );
                    ni->set_reg( b->first );
                    ni->set_value( b->second );
                }
                client_.call( &stub_type::set_register_list, &req, &res );
            }

            void write_regs8( const spi::cmd_uint8_vector &dat ) const override
            {
                write_impl<8>( dat );
            }

            void write_regs16( const spi::cmd_uint16_vector &dat) const override
            {
                write_impl<16>( dat );
            }

            void write_regs32( const spi::cmd_uint32_vector &dat) const override
            {
                write_impl<32>( dat );
            }

            void write_regs64( const spi::cmd_uint64_vector &dat) const override
            {
                write_impl<64>( dat );
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
