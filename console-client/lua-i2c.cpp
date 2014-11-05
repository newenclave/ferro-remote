#include "lua-interface.h"

#if FR_WITH_LUA

#include "interfaces/II2C.h"

#include "fr-lua/lua-wrapper.hpp"

#include <map>
#include <iostream>
#include <mutex>

namespace fr { namespace lua {

    namespace {

        using namespace client;
        typedef interfaces::i2c::iface  iface;
        typedef std::shared_ptr<iface>  iface_sptr;

        namespace ii2c = interfaces::i2c;

        struct data;

        data * get_iface( lua_State *L )
        {
            void * p = get_component_iface( L, i2c::table_path( ) );
            return reinterpret_cast<data *>( p );
        }

        int lcall_i2c_bus_avail( lua_State *L );
        int lcall_i2c_bus_open(  lua_State *L );
        int lcall_i2c_bus_close( lua_State *L );
        int lcall_i2c_set_addr(  lua_State *L );

        int lcall_i2c_functions(  lua_State *L );

        int lcall_i2c_read(  lua_State *L );
        int lcall_i2c_write( lua_State *L );
        int lcall_i2c_ioctl( lua_State *L );

        int lcall_i2c_read_block(  lua_State *L );
        int lcall_i2c_write_block( lua_State *L );

        int lcall_i2c_read_byte(  lua_State *L );
        int lcall_i2c_write_byte( lua_State *L );

        int lcall_i2c_read_word(  lua_State *L );
        int lcall_i2c_write_word( lua_State *L );

        struct data: public base_data {

            client::core::client_core    &cc_;
            std::map<void *, iface_sptr>  devices_;
            std::mutex                    devices_lock_;

            data( std::shared_ptr<lua::state> &ls,
                  client::core::client_core &cc )
                :cc_(cc)
            { }

            virtual lua::objects::table_sptr init_table( )
            {
                objects::table_sptr t( new objects::table );

                using objects::new_string;
                using objects::new_function;
                using objects::new_light_userdata;

                t->add( new_string( names::inst_field ),
                        new_light_userdata( this ));

                t->add( new_string( "bus_available" ),
                        new_function( &lcall_i2c_bus_avail ) );

                t->add( new_string( "functions" ),
                        new_function( &lcall_i2c_functions ) );

                t->add( new_string( "open" ),
                        new_function( &lcall_i2c_bus_open ) );
                t->add( new_string( "close" ),
                        new_function( &lcall_i2c_bus_close ) );
                t->add( new_string( "set_address" ),
                        new_function( &lcall_i2c_set_addr ) );

                t->add( new_string( "read" ),
                        new_function( &lcall_i2c_read ) );
                t->add( new_string( "write" ),
                        new_function( &lcall_i2c_write ) );
                t->add( new_string( "ioctl" ),
                        new_function( &lcall_i2c_ioctl ) );

                t->add( new_string( "read_byte" ),
                        new_function( &lcall_i2c_read_byte ) );
                t->add( new_string( "write_byte" ),
                        new_function( &lcall_i2c_write_byte ) );

                t->add( new_string( "read_word" ),
                        new_function( &lcall_i2c_read_word ) );
                t->add( new_string( "write_word" ),
                        new_function( &lcall_i2c_write_word ) );

                t->add( new_string( "read_block" ),
                        new_function( &lcall_i2c_read_block ) );
                t->add( new_string( "write_block" ),
                        new_function( &lcall_i2c_write_block ) );

                return t;
            }
        };

        /// CALLS
        iface_sptr get_device( lua_State *L, int id = -1 )
        {
            lua::state ls(L);
            void *p = ls.get<void *>( id );
            iface_sptr ni;
            data *i = get_iface( L );
            {
                std::lock_guard<std::mutex> lck(i->devices_lock_);
                std::map<
                        void *,
                        iface_sptr
                >::const_iterator f( i->devices_.find( p ) );

                if( f != i->devices_.end( ) ) {
                    ni = f->second;
                }
            }
            return ni;
        }

        int lcall_i2c_bus_avail( lua_State *L )
        {
            lua::state ls( L );
            data * i = get_iface( L );
            bool res = false;
            res = ii2c::bus_available( i->cc_, ls.get<unsigned>( 1 ));
            ls.clean_stack( );
            ls.push( res );
            return 1;
        }

        int lcall_i2c_bus_open( lua_State *L )
        {
            lua::state ls( L );
            data * i = get_iface( L );

            int n = ls.get_top(  );

            unsigned bus_id     = ls.get<unsigned>( 1 );
            unsigned slave_addr = ii2c::I2C_SLAVE_INVALID_ADDRESS;
            bool slave_force    = false;

            if( n > 1 ) {
                slave_addr = ls.get<unsigned>( 2 );
            }

            if( n > 2 ) {
                slave_force = ls.get<bool>( 3 );
            }

            void *res = NULL;

            try {
                iface_sptr new_inst( ii2c::open( i->cc_, bus_id,
                                                 slave_addr, slave_force ) );
                std::lock_guard<std::mutex> lck(i->devices_lock_);
                res = new_inst.get( );
                i->devices_[res] = new_inst;
                ls.push( res );
                ls.push( );
            } catch ( const std::exception &ex ) {
                ls.push( );
                ls.push( ex.what( ) );
            }
            return 2;
        }

        int lcall_i2c_set_addr( lua_State *L )
        {
            lua::state ls( L );
            iface_sptr dev( get_device( L, 1 ) );

            dev->set_address( ls.get<unsigned>( 2 ) );
            ls.clean_stack( );

            return 0;
        }

#define I2C_ADD_FUNCTIONAL_SUPPORT( t, m, flag )        \
        t->add( objects::new_string( #flag ),           \
                objects::new_boolean( ii2c::FUNC_##flag & m ));

        objects::table_sptr table_mask( uint64_t m )
        {
            objects::table_sptr res( objects::new_table( ) );

            I2C_ADD_FUNCTIONAL_SUPPORT( res, m, I2C );
            I2C_ADD_FUNCTIONAL_SUPPORT( res, m, 10BIT_ADDR );
            I2C_ADD_FUNCTIONAL_SUPPORT( res, m, PROTOCOL_MANGLING );
            I2C_ADD_FUNCTIONAL_SUPPORT( res, m, SMBUS_PEC );
            I2C_ADD_FUNCTIONAL_SUPPORT( res, m, NOSTART );
            I2C_ADD_FUNCTIONAL_SUPPORT( res, m, SMBUS_BLOCK_PROC_CALL );
            I2C_ADD_FUNCTIONAL_SUPPORT( res, m, SMBUS_QUICK );
            I2C_ADD_FUNCTIONAL_SUPPORT( res, m, SMBUS_READ_BYTE );
            I2C_ADD_FUNCTIONAL_SUPPORT( res, m, SMBUS_WRITE_BYTE );
            I2C_ADD_FUNCTIONAL_SUPPORT( res, m, SMBUS_READ_BYTE_DATA );
            I2C_ADD_FUNCTIONAL_SUPPORT( res, m, SMBUS_WRITE_BYTE_DATA );
            I2C_ADD_FUNCTIONAL_SUPPORT( res, m, SMBUS_READ_WORD_DATA );
            I2C_ADD_FUNCTIONAL_SUPPORT( res, m, SMBUS_WRITE_WORD_DATA );
            I2C_ADD_FUNCTIONAL_SUPPORT( res, m, SMBUS_PROC_CALL );
            I2C_ADD_FUNCTIONAL_SUPPORT( res, m, SMBUS_READ_BLOCK_DATA );
            I2C_ADD_FUNCTIONAL_SUPPORT( res, m, SMBUS_WRITE_BLOCK_DATA );
            I2C_ADD_FUNCTIONAL_SUPPORT( res, m, SMBUS_READ_I2C_BLOCK );
            I2C_ADD_FUNCTIONAL_SUPPORT( res, m, SMBUS_WRITE_I2C_BLOCK );

            return res;
        }
#undef I2C_ADD_FUNCTIONAL_SUPPORT

        int lcall_i2c_functions(  lua_State *L )
        {
            lua::state ls( L );
            iface_sptr dev( get_device( L, 1 ) );
            ls.clean_stack( );

            uint64_t mask = dev->function_mask( );
            objects::table_sptr t( table_mask( mask ) );

            t->push( L );
            return 1;
        }

        int lcall_i2c_read(  lua_State *L )
        {
            lua::state ls(L);
            int n = ls.get_top( );
            unsigned maximum = 44000;
            iface_sptr f(get_device( L, 1 ));
            if( n > 1 ) {
                maximum = ls.get<unsigned>( 2 );
                if( maximum > 44000 ) maximum = 44000;
            }
            ls.pop( n );
            if( maximum ) {
                std::vector<char> data(maximum);
                size_t r = f->read( &data[0], maximum );
                ls.push( &data[0], r );
                return 1;
            }
            return 0;
        }

        int lcall_i2c_write( lua_State *L )
        {
            lua::state ls(L);

            iface_sptr f(get_device( L, 1 ));
            std::string data( ls.get<std::string>( 2 ) );

            ls.clean_stack( );

            size_t pos = 0;
            const size_t w = data.size( );
            while( pos != w ) {
                pos += f->write( &data[pos], w - pos );
            }
            ls.push( w );
            return 1;
        }

        int lcall_i2c_ioctl( lua_State *L )
        {
            lua::state ls(L);
            iface_sptr dev(get_device( L, 1 ));

            int code = -1;
            unsigned long param = 0;

            int n = ls.get_top( );

            if( n > 1 ) {
                code = ls.get<unsigned>( 2 );
            }

            if( n > 2 ) {
                param = ls.get<lua_Integer>( 3 );
            }

            try {
                dev->ioctl( code, param );
                ls.push( );
            } catch( const std::exception &ex ) {
                ls.push( ex.what( ) );
            }
            return 1;
        }

        template <typename T>
        std::vector<
            std::pair<uint8_t, T>
        > create_cmd_params( lua_State *L, int id )
        {
            lua::state ls( L );
            typedef std::vector< std::pair<uint8_t, T> > pair_vector;

            pair_vector res;

            objects::base_sptr o = ls.get_object( id );

            for( size_t i=0; i<o->count( ); ++i ) {
                const objects::base * next = o->at( i );
                if( next->count( ) >= 2 ) {
                    const objects::base * id  = next->at( 0 );
                    const objects::base * val = next->at( 1 );
                    res.push_back(
                        std::make_pair(
                            static_cast<uint8_t>( id->num( ) ),
                            static_cast<T>( val->num( ) ) ) );
                }
            }
            return res;
        }

        template <typename T>
        std::vector<T> create_cmd_from_table( lua_State *L, int id )
        {
            lua::state ls( L );


            std::vector<T> res;
            objects::base_sptr o = ls.get_object( id );

            for( size_t i=0; i<o->count( ); ++i ) {
                res.push_back( static_cast<T>( o->at( i )->at( 1 )->num( ) ) );
            }
            return res;
        }

        template <typename T, typename CallType, typename CallListType>
        int lcall_i2c_read_impl( CallType call, CallListType calllist,
                                 lua_State *L )
        {
            typedef std::vector< std::pair<uint8_t, T> > pair_vector;
            typedef std::vector<uint8_t>                 data_vector;

            iface_sptr dev( get_device( L, 1 ) );

            lua::state ls( L );
            int t = ls.get_type( 2 );

            if( t == LUA_TNUMBER ) {
                unsigned cmd = ls.get<unsigned>( 2 );
                ls.clean_stack( );
                ls.push( (dev.get( )->*call)( cmd ) );
                return 1;
            } else if( t == LUA_TTABLE ) {

                data_vector tab = create_cmd_from_table<uint8_t>( L, 2 );

                ls.clean_stack( );
                pair_vector res = (dev.get( )->*calllist)( tab );
                objects::table_sptr nt( objects::new_table( ) );

                typedef typename pair_vector::const_iterator citr;
                for( citr b(res.begin( )), e(res.end( )); b!=e; ++b ) {
                    nt->add( objects::new_integer( b->first ),
                             objects::new_integer( b->second ) );
                }
                nt->push( L );
                return 1;
            }

            return 0;
        }

        int lcall_i2c_read_byte( lua_State *L )
        {
            return lcall_i2c_read_impl<uint8_t>( &iface::read_byte,
                                                 &iface::read_bytes,
                                                 L );
//            iface_sptr dev( get_device( L, 1 ) );

//             lua::state ls( L );
//            int t = ls.get_type( 2 );

//            if( t == LUA_TNUMBER ) {
//                unsigned cmd = ls.get<unsigned>( 2 );
//                ls.clean_stack( );
//                ls.push( dev->read_byte( cmd ) );
//                return 1;
//            } else if( t == LUA_TTABLE ) {
//                ii2c::uint8_vector tab = create_from_table<uint8_t>( L, 2 );
//                ls.clean_stack( );
//                ii2c::cmd_uint8_vector res = dev->read_bytes( tab );
//                objects::table_sptr nt( objects::new_table( ) );

//                typedef ii2c::cmd_uint8_vector::const_iterator citr;
//                for( citr b(res.begin( )), e(res.end( )); b!=e; ++b ) {
//                    nt->add( objects::new_table( )
//                             ->add( objects::new_integer( b->first ) )
//                             ->add( objects::new_integer( b->second ) ) );
//                }
//                nt->push( L );
//                return 1;
//            }

//            return 0;
        }

        template <typename T, typename CallType, typename CallListType>
        int lcall_i2c_write_impl( CallType call, CallListType calllist,
                                  lua_State *L )
        {
            typedef std::vector< std::pair<uint8_t, T> > pair_vector;

            iface_sptr dev( get_device( L, 1 ) );
            lua::state ls( L );
            int t = ls.get_type( 2 );
            if( LUA_TNUMBER == t ) {
                unsigned cmd  = ls.get<unsigned>( 2 );
                unsigned data = ls.get<unsigned>( 3 );
                ls.clean_stack( );
                (dev.get( )->*call)( static_cast<uint8_t>( cmd ),
                                     static_cast<T>( data ) );
            } else if( LUA_TTABLE == t ) {
                pair_vector tab = create_cmd_params<T>( L, 2 );
                (dev.get( )->*calllist)( tab );
            }
            return 0;
        }

        int lcall_i2c_write_byte( lua_State *L )
        {
            return lcall_i2c_write_impl<uint8_t>( &iface::write_byte,
                                                  &iface::write_bytes,
                                                  L );
//            iface_sptr dev( get_device( L, 1 ) );
//            lua::state ls( L );
//            unsigned cmd  = ls.get<unsigned>( 2 );
//            unsigned data = ls.get<unsigned>( 3 );
//            ls.clean_stack( );
//            dev->write_byte( cmd, data );
//            return 0;
        }

        int lcall_i2c_read_word( lua_State *L )
        {
            return lcall_i2c_read_impl<uint16_t>( &iface::read_word,
                                                  &iface::read_words,
                                                  L );
//            iface_sptr dev( get_device( L, 1 ) );

//            lua::state ls( L );
//            int t = ls.get_type( 2 );

//            if( t == LUA_TNUMBER ) {
//                unsigned cmd = ls.get<unsigned>( 2 );
//                ls.clean_stack( );
//                ls.push( dev->read_word( cmd ) );
//                return 1;
//            } else if( t == LUA_TTABLE ) {
//                ii2c::uint16_vector tab = create_from_table<uint16_t>( L, 2 );
//                ls.clean_stack( );

//                ii2c::cmd_uint16_vector res = dev->read_words( tab );
//                objects::table_sptr nt( objects::new_table( ) );

//                typedef ii2c::cmd_uint16_vector::const_iterator citr;
//                for( citr b(res.begin( )), e(res.end( )); b!=e; ++b ) {
//                    nt->add( objects::new_table( )
//                             ->add( objects::new_integer( b->first ) )
//                             ->add( objects::new_integer( b->second ) ) );
//                }
//                nt->push( L );
//                return 1;
//            }

//            return 0;
        }

        int lcall_i2c_write_word( lua_State *L )
        {
            return lcall_i2c_write_impl<uint16_t>( &iface::write_word,
                                                   &iface::write_words,
                                                   L );
//            iface_sptr dev( get_device( L, 1 ) );
//            lua::state ls( L );
//            unsigned cmd  = ls.get<unsigned>( 2 );
//            unsigned data = ls.get<unsigned>( 3 );
//            ls.clean_stack( );
//            dev->write_word( cmd, data );
//            return 0;
        }

        int lcall_i2c_read_block(  lua_State *L )
        {
            iface_sptr dev( get_device( L ,1 ) );
            lua::state ls( L );
            unsigned cmd = ls.get<unsigned>( 2 );
            ls.clean_stack( );
            ls.push( dev->read_block( cmd ) );
            return 1;
        }

        int lcall_i2c_write_block( lua_State *L )
        {
            iface_sptr dev( get_device( L, 1 ) );
            lua::state ls( L );
            unsigned cmd     = ls.get<unsigned>( 2 );
            std::string data = ls.get<std::string>( 3 );
            dev->write_block( cmd, data );
            return 0;
        }

        int lcall_i2c_bus_close( lua_State *L )
        {
            lua::state ls( L );
            data * i   = get_iface( L );
            void * dev = ls.get<void *>(  );
            ls.clean_stack( );
            do {
                std::lock_guard<std::mutex> lck( i->devices_lock_ );
                i->devices_.erase( dev );
            } while ( 0 );
            return 0;
        }

    }

namespace i2c {

    const char *table_name( ) { return "i2c"; }
    const char *table_path( )
    {
        static const std::string path =
                std::string( names::client_table ) + '.' + table_name( );
        return path.c_str( );
    }

    data_sptr init( std::shared_ptr<lua::state> &ls,
                    client::core::client_core &cc )
    {
        return data_sptr( new data( ls, cc ) );
    }
}

}}

#endif
