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

                t->add( new_string( "open" ),
                        new_function( &lcall_i2c_bus_open ) );
                t->add( new_string( "close" ),
                        new_function( &lcall_i2c_bus_close ) );
                t->add( new_string( "set_address" ),
                        new_function( &lcall_i2c_set_addr ) );

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
                ls.push( );
            } catch ( const std::exception &ex ) {
                ls.push( ex.what( ) );
            }
            ls.push( res );
            return 2;
        }

        int lcall_i2c_set_addr( lua_State *L )
        {
            iface_sptr dev( get_device( L ) );
            lua::state ls( L );
            dev->set_address( ls.get<unsigned>( 1 ) );
            ls.clean_stack( );
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
