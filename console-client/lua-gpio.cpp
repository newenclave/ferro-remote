
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

        int lcall_gpio_export(   lua_State *L );
        int lcall_gpio_info(     lua_State *L );
        int lcall_gpio_unexport( lua_State *L );

        int lcall_gpio_set_value(  lua_State *L );
        int lcall_gpio_value(      lua_State *L );
        int lcall_gpio_make_pulse( lua_State *L );

        int lcall_gpio_test_edge( lua_State *L );
        int lcall_gpio_set_edge(  lua_State *L );
        int lcall_gpio_edge(      lua_State *L );

        int lcall_gpio_set_dir( lua_State *L );
        int lcall_gpio_dir(     lua_State *L );

        int lcall_gpio_reg_for_change( lua_State *L );
        int lcall_gpio_reg_for_change2( lua_State *L );
        int lcall_gpio_unreg( lua_State *L );

        int lcall_gpio_close( lua_State *L );

        struct data;

        data * get_iface( lua_State *L )
        {
            void * p = get_component_iface( L, gpio::table_path( ) );
            return reinterpret_cast<data *>( p );
        }

        template <typename T>
        std::string create_ref_table_path( T value )
        {
            static const std::string tp =
                         std::string( gpio::table_path( ) ) + ".refs.";
            std::ostringstream oss;
            oss << tp << std::hex << value;
            return oss.str( );
        }

        struct data: public base_data {

            client::core::client_core    &cc_;
            std::map<void *, iface_sptr>  devices_;
            std::mutex                    devices_lock_;

            std::weak_ptr<lua::state>     state_;

            data( std::shared_ptr<lua::state> ls,
                  client::core::client_core &cc )
                :cc_(cc)
                ,state_(ls)
            { }

#define ADD_GPIO_VALUE( f ) \
    objects::new_string( #f ), objects::new_integer( interfaces::gpio::f )

            lua::objects::table_sptr init_table( )
            {
                objects::table_sptr t( objects::new_table( ) );

                bool avail = interfaces::gpio::available( cc_ );

                t->add( objects::new_string( "available" ),
                        objects::new_boolean( avail ) );

                if( avail ) {

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

                    t->add( "export",   new_function( &lcall_gpio_export ) );
                    t->add( "info",     new_function( &lcall_gpio_info ) );

                    t->add( "value",    new_function( &lcall_gpio_value ) );
                    t->add( "set_value",new_function( &lcall_gpio_set_value ) );

                    t->add( "make_pulse",
                            new_function( &lcall_gpio_make_pulse ) );

                    t->add( "edge_supported",
                            new_function( &lcall_gpio_test_edge ) );

                    t->add( "edge", new_function( &lcall_gpio_edge ) );
                    t->add( "set_edge", new_function( &lcall_gpio_set_edge ) );

                    t->add( "direction", new_function( &lcall_gpio_dir ) );
                    t->add( "set_direction",
                            new_function( &lcall_gpio_set_dir ) );

                    t->add( "unexport", new_function( &lcall_gpio_unexport ) );
                    t->add( "close", new_function( &lcall_gpio_close ) );

                    t->add( "register_for_change",
                            new_function( &lcall_gpio_reg_for_change ) );

                    t->add( "register_for_change_int",
                            new_function( &lcall_gpio_reg_for_change2 ) );

                    t->add( "unregister",
                            new_function( &lcall_gpio_unreg ) );
                }

                return t;
            }
#undef ADD_GPIO_VALUE

        };

        /// doesn't pop stack!!!
        iface_sptr get_gpio_dev( lua_State *L, int id = 1 )
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

        int lcall_gpio_export( lua_State *L )
        {
            lua::state ls( L );
            data * i = get_iface( L );
            int n = ls.get_top( );
            unsigned gid    = ls.get<unsigned>( 1 );
            unsigned direct = 0xFF;
            if( n > 1 && ( ls.get_type( 2 ) == LUA_TNUMBER ) ) {
                direct = ls.get<unsigned>( 2 );
            }
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
            //data * i   = get_iface( L );
            iface_sptr dev = get_gpio_dev( L );
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
            //data * i   = get_iface( L );
            iface_sptr dev = get_gpio_dev( L );
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
            dev->set_value( val );
            return 0;
        }

        int lcall_gpio_value( lua_State *L )
        {
            lua::state ls( L );
            iface_sptr dev = get_gpio_dev( L );
            ls.push( dev->value( ) );
            return 1;
        }

        int lcall_gpio_make_pulse( lua_State *L )
        {
            lua::state ls( L );
            iface_sptr dev = get_gpio_dev( L );
            int n = ls.get_top( );

            vtrc::uint64_t len = ls.get<vtrc::uint64_t>( 2 );
            unsigned sv = 1;
            unsigned rv = 0;

            if( n > 2 ) {
                sv = ls.get<unsigned>( 3 );
            }

            if( n > 3 ) {
                sv = ls.get<unsigned>( 4 );
            }

            dev->make_pulse( len, sv, rv );

            return 0;
        }

        int lcall_gpio_test_edge( lua_State *L )
        {
            lua::state ls( L );
            iface_sptr dev = get_gpio_dev( L, 1 );
            ls.push<bool>( dev->edge_supported( ) );
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
            dev->set_edge( interfaces::gpio::edge_val2enum( val ) );
            return 0;
        }

        int lcall_gpio_edge( lua_State *L )
        {
            lua::state ls( L );
            iface_sptr dev = get_gpio_dev( L );
            ls.push<unsigned>( dev->edge( ) );
            return 1;
        }

        int lcall_gpio_set_dir( lua_State *L )
        {
            lua::state ls( L );
            int        r = ls.get_top( );
            unsigned val = 0;
            iface_sptr dev = get_gpio_dev( L, 1 );
            if( r > 1 ) {
                val = ls.get<unsigned>( 2 );
            }
            dev->set_direction( interfaces::gpio::direction_val2enum( val ) );
            return 0;
        }

        int lcall_gpio_dir( lua_State *L )
        {
            lua::state ls( L );
            iface_sptr dev = get_gpio_dev( L );
            ls.push<unsigned>( dev->direction( ) );
            return 1;
        }

        int lcall_gpio_close( lua_State *L )
        {
            lua::state ls( L );
            data * i   = get_iface( L );
            void * dev = ls.get<void *>(  );
            do {
                std::lock_guard<std::mutex> lck( i->devices_lock_ );
                i->devices_.erase( dev );
            } while ( 0 );
            return 0;
        }

        struct handler_params {
            std::weak_ptr<lua::state>       parent_state_;
            std::shared_ptr<lua::state>     thread_;
            std::string                     call_name_;
            std::vector<objects::base_sptr> params_;
            handler_params( std::weak_ptr<lua::state> ps,
                            std::shared_ptr<lua::state> t,
                            std::string cn,
                            std::vector<objects::base_sptr> par )
                :parent_state_(ps)
                ,thread_(t)
                ,call_name_(cn)
                ,params_(par)
            { }
        };

        void gpio_event_handler( unsigned /*error*/, unsigned value,
                                 uint64_t interval, bool inter,
                                 handler_params &p )
        try {

            std::shared_ptr<lua::state> m( p.parent_state_.lock( ) );

            if( !m ) {
                return;
            }

            if( inter ) {
                p.thread_->exec_function( p.call_name_.c_str( ), value,
                                          interval,
                                          p.params_ );
            } else {
                p.thread_->exec_function( p.call_name_.c_str( ), value,
                                          p.params_ );
            }

        } catch( ... ) {
            //std::cout << "call erro " << "\n";
        }


        int lcall_gpio_reg_for_change( lua_State *L )
        {
            lua::state ls( L );
            int n = ls.get_top( );
            data * i = get_iface( L );

            iface_sptr dev = get_gpio_dev( L, 1 );
            std::string call;

            int ct = ls.get_type( 2 );
            if( ct == LUA_TSTRING ) {
                call = ls.get<const char *>( 2 );
            } else if( ct == LUA_TFUNCTION )  {
                std::ostringstream oss;
                oss << "call@" << std::hex << dev.get( );
                call = oss.str( );
                ls.set_value( call.c_str( ), 2 );
            }

            std::vector<objects::base_sptr> params;

            if( n > 2 ) {
                params.reserve( n - 2 );
                for( int i=3; i<=n; ++i ) {
                    params.push_back( ls.get_ref( i ) );
                }
            }

            std::string tmp_path(create_ref_table_path( dev.get( ) ));

            lua::state_sptr thread(
                        std::make_shared<lua::state>(
                            ls.create_thread( tmp_path.c_str( ) ) ) );

            dev->register_for_change( std::bind( gpio_event_handler,
                                                 std::placeholders::_1,
                                                 std::placeholders::_2,
                                                 0, false,
                                                 handler_params(
                                                     i->state_, thread,
                                                     call, params
                                                 ) ) );
            return 0;
        }

        int lcall_gpio_reg_for_change2( lua_State *L )
        {
            lua::state ls( L );
            int n = ls.get_top( );
            data * i = get_iface( L );

            iface_sptr dev = get_gpio_dev( L, 1 );

            std::string call;
            int ct = ls.get_type( 2 );
            if( ct == LUA_TSTRING ) {
                call = ls.get<const char *>( 2 );
            } else if( ct == LUA_TFUNCTION )  {
                std::ostringstream oss;
                oss << "call@" << std::hex << dev.get( );
                call = oss.str( );
                ls.set_value( call.c_str( ), 2 );
            }

            std::vector<objects::base_sptr> params;

            if( n > 2 ) {
                params.reserve( n - 2 );
                for( int i=3; i<=n; ++i ) {
                    params.push_back( ls.get_ref( i ) );
                }
            }

            std::string tmp_path(create_ref_table_path( dev.get( ) ));

            lua::state_sptr thread(
                        std::make_shared<lua::state>(
                            ls.create_thread( tmp_path.c_str( ) ) ) );

            dev->register_for_change_int( std::bind( gpio_event_handler,
                                                 std::placeholders::_1,
                                                 std::placeholders::_2,
                                                 std::placeholders::_3, true,
                                                 handler_params(
                                                     i->state_, thread,
                                                     call, params
                                                 ) ) );
            return 0;
        }

        int lcall_gpio_unreg( lua_State *L )
        {
            lua::state ls( L );
            iface_sptr dev = get_gpio_dev( L );
            dev->unregister( );


            std::string tmp_path(create_ref_table_path( dev.get( ) ));
            std::ostringstream oss;
            oss << "call@" << std::hex << dev.get( );

            ls.set( tmp_path.c_str( ) );    /// clean thread reference
                                            /// thread state will be free
            ls.set( oss.str( ).c_str( ) );  /// clean call reference


            return 0;
        }
    }

    namespace gpio {
        const char *table_name( ) { return "gpio"; }
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

