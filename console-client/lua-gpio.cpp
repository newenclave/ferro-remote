
#include "lua-interface.h"

#if FR_WITH_LUA

#include <iostream>
#include <mutex>
#include <map>
#include <functional>

#include "interfaces/IGPIO.h"

#include "fr-lua/lua-wrapper.hpp"

namespace fr { namespace lua {

    namespace {


        using namespace client;
        typedef interfaces::gpio::iface  iface;
        typedef std::shared_ptr<iface>   iface_sptr;

        const std::string gpio_table_path =
                std::string(lua::names::client_table)
                + '.'
                + gpio::table_name;

        int lcall_gpio_new( lua_State *L );
        int lcall_gpio_info( lua_State *L );
        int lcall_gpio_unexport( lua_State *L );
        int lcall_gpio_set_value( lua_State *L );
        int lcall_gpio_value( lua_State *L );
        int lcall_gpio_set_edge( lua_State *L );
        int lcall_gpio_edge( lua_State *L );
        int lcall_gpio_close( lua_State *L );

        struct data;

        data * get_iface( lua_State *L )
        {
            void * p = get_component_iface( L, gpio_table_path.c_str( ) );
            return reinterpret_cast<data *>( p );
        }

        struct data: public base_data {

            client::core::client_core    &cc_;
            std::map<void *, iface_sptr>  devices_;
            std::mutex                    devices_lock_;

            data( lua_State */*ls*/, client::core::client_core &cc )
                :cc_(cc)
            { }

#define ADD_GPIO_VALUE( f ) \
    objects::new_string( #f ), objects::new_integer( interfaces::gpio::f )

            lua::objects::table_sptr init_table( )
            {
                objects::table_sptr t( objects::new_table( ) );

                using objects::new_string;
                using objects::new_function;
                using objects::new_light_userdata;

                t->add( new_string( names::inst_field ),
                        new_light_userdata( this ));

                t->add( ADD_GPIO_VALUE( DIRECT_IN ) );
                t->add( ADD_GPIO_VALUE( DIRECT_OUT ) );

                t->add( ADD_GPIO_VALUE( EDGE_NONE ) );
                t->add( ADD_GPIO_VALUE( EDGE_RISING ) );
                t->add( ADD_GPIO_VALUE( EDGE_FALLING ) );
                t->add( ADD_GPIO_VALUE( EDGE_BOTH ) );

                t->add( new_string( "export" ),
                        new_function( &lcall_gpio_new ) );
                t->add( new_string( "info" ),
                        new_function( &lcall_gpio_info ) );
                t->add( new_string( "value" ),
                        new_function( &lcall_gpio_value ) );
                t->add( new_string( "set_value" ),
                        new_function( &lcall_gpio_set_value ) );
                t->add( new_string( "edge" ),
                        new_function( &lcall_gpio_edge ) );
                t->add( new_string( "set_edge" ),
                        new_function( &lcall_gpio_set_edge ) );
                t->add( new_string( "unexport" ),
                        new_function( &lcall_gpio_unexport ) );
                t->add( new_string( "close" ),
                        new_function( &lcall_gpio_close ) );

                return t;
            }
#undef ADD_GPIO_VALUE

        };

        iface_sptr get_gpio_dev( lua_State *L, int id = -1 )
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


        int lcall_gpio_new( lua_State *L )
        {
            lua::state ls( L );
            data * i = get_iface( L );
            int n = ls.get_top( );
            unsigned gid    = ls.get<unsigned>( 1 );
            unsigned direct = 0xFF;
            if( n > 1 && ( ls.get_type( 2 ) == LUA_TNUMBER ) ) {
                direct = ls.get<unsigned>( 2 );
            }
            ls.pop( n );
            iface_sptr new_dev;
            if( direct == interfaces::gpio::DIRECT_IN ) {
                new_dev.reset(interfaces::gpio::create_input( i->cc_, gid ));
            } else if( direct == interfaces::gpio::DIRECT_OUT ) {
                new_dev.reset(interfaces::gpio::create_output( i->cc_, gid ));
            } else {
                new_dev.reset(interfaces::gpio::create( i->cc_, gid ));
                new_dev->export_device( );
            }
            do {
                std::lock_guard<std::mutex> lck( i->devices_lock_ );
                i->devices_.insert( std::make_pair( new_dev.get( ), new_dev ) );
            } while( 0 );

            ls.push( new_dev.get( ) );

            return 1;
        }

#define ADD_GPIO_INFO_FLD( info, fld ) \
    objects::new_string( #fld ), objects::new_integer( info.fld )

        int lcall_gpio_info( lua_State *L )
        {
            lua::state ls( L );
            data * i   = get_iface( L );
            iface_sptr dev = get_gpio_dev( L );
            ls.clean( );
            interfaces::gpio::info inf( dev->get_info( ) );

            objects::table_sptr t(objects::new_table( ));

            t->add( ADD_GPIO_INFO_FLD( inf, id ) );
            t->add( ADD_GPIO_INFO_FLD( inf, value ) );
            t->add( ADD_GPIO_INFO_FLD( inf, edge ) );
            t->add( ADD_GPIO_INFO_FLD( inf, direction ) );
            t->add( ADD_GPIO_INFO_FLD( inf, active_low ) );

            t->push( L );

            return 1;
        }

        int lcall_gpio_unexport( lua_State *L )
        {
            lua::state ls( L );
            data * i   = get_iface( L );
            iface_sptr dev = get_gpio_dev( L );
            ls.clean( );
            if( dev.get( ) ) {
                dev->unexport_device( );
            }
            return 0;
        }

        int lcall_gpio_set_value( lua_State *L )
        {
            lua::state ls( L );
            int        r = ls.get_top( );
            unsigned val = 0;
            iface_sptr dev = get_gpio_dev( L, 1 );
            if( r > 1 ) {
                val = ls.get<unsigned>( 2 );
            }
            ls.clean( );
            dev->set_value( val );
            return 0;
        }

        int lcall_gpio_value( lua_State *L )
        {
            lua::state ls( L );
            iface_sptr dev = get_gpio_dev( L );
            ls.clean( );
            ls.push( dev->value( ) );
            return 1;
        }

        int lcall_gpio_set_edge( lua_State *L )
        {
            lua::state ls( L );
            int        r = ls.get_top( );
            unsigned val = 0;
            iface_sptr dev = get_gpio_dev( L, 1 );
            if( r > 1 ) {
                val = ls.get<unsigned>( 2 );
            }
            ls.clean( );
            dev->set_edge( interfaces::gpio::edge_val2enum( val ) );
            return 0;
        }

        int lcall_gpio_edge( lua_State *L )
        {
            lua::state ls( L );
            iface_sptr dev = get_gpio_dev( L );
            ls.clean( );
            ls.push<unsigned>( dev->edge( ) );
            return 1;
        }

        int lcall_gpio_close( lua_State *L )
        {
            lua::state ls( L );
            data * i   = get_iface( L );
            void * dev = ls.get<void *>(  );
            ls.clean( );
            do {
                std::lock_guard<std::mutex> lck( i->devices_lock_ );
                i->devices_.erase( dev );
            } while ( 0 );
            return 0;
        }
    }

    namespace gpio {
        data_sptr init( lua_State *ls, client::core::client_core &cc )
        {
            return data_sptr( new data(ls, cc) );
        }
    }

}}

#endif
