#include "iface.h"
#include "../fr-lua.h"
#include "../lua-names.h"

#include "../general-info.h"
#include "../utils.h"

#include "vtrc-common/vtrc-delayed-call.h"
#include "vtrc-common/vtrc-pool-pair.h"

namespace fr { namespace lua { namespace m { namespace event_queue {

namespace {

    using namespace objects;
    namespace vcomm = vtrc::common;

    typedef vtrc::common::delayed_call             delayed_call;
    typedef std::shared_ptr<delayed_call>          delayed_call_sptr;
    typedef std::map<size_t, delayed_call_sptr>    timers_map;

    const std::string     module_name("event_queue");
    const char *id_path = FR_CLIENT_LUA_HIDE_TABLE ".event_queue.__i";

    struct module;

    module *get_module( lua_State *L )
    {
        lua::state ls(L);
        void *ptr = ls.get<void *>( id_path );
        return static_cast<module *>(ptr);
    }

    int lcall_post( lua_State *L );
    int lcall_timer_post( lua_State *L );
    int lcall_timer_cancel( lua_State *L );

    struct module: public iface {

        client::general_info &info_;
        bool                 working_;
        timers_map           timers_;

        module( client::general_info &info )
            :info_(info)
            ,working_(true)
        { }

        void init( )
        {
            lua::state ls( info_.main_ );
            ls.set( id_path, this );
        }

        void deinit( )
        {
            working_ = false;
            timers_.clear( );
        }

        const std::string &name( ) const
        {
            return module_name;
        }

        std::shared_ptr<objects::table> table( ) const
        {
            objects::table_sptr res(std::make_shared<objects::table>( ));
            res->add( "post", new_function( &lcall_post ) );
            res->add( "timer", new_table( )
                      ->add( "post",    new_function( &lcall_timer_post ) )
                      ->add( "cancel",  new_function( &lcall_timer_cancel ) )
                      );
            return res;
        }

        bool connection_required( ) const
        {
            return false;
        }

        delayed_call_sptr add_timer( size_t id )
        {
            delayed_call_sptr res(
                std::make_shared<delayed_call>(
                        std::ref( info_.pp_->get_io_service( ) ) ) );
            timers_[id] = res;
            return res;
        }

        void del_timer( size_t id )
        {
            timers_.erase(id);
        }

        void timer_handler( const boost::system::error_code &err, size_t id,
                            base_sptr call,
                            std::vector<objects::base_sptr> params,
                            std::weak_ptr<event_caller> k )
        {
            if( !working_ ) {
                return;
            }
            auto kl(k.lock( ));
            if( !kl ) {
                return;
            }

            if( !err ) {
                params.insert( params.begin( ),
                               base_sptr( new objects::nil( ) ) );
            } else {
                params.insert( params.begin( ),
                               base_sptr( new_string( err.message( ) ) ) );
            }

            kl->push_call( call, params );
            del_timer( id );
        }
    };

    int lcall_post( lua_State *L )
    {
        module *m = get_module( L );
        lua::state ls(L);

        int n = ls.get_top( );

        base_sptr call(new_reference( L, 1 ));

        std::vector<objects::base_sptr> params;

        if( n > 1 ) {
            params.reserve( n - 1 );
            for( int i=2; i<=n; ++i ) {
                params.push_back( base_sptr( new_reference( L, i ) ) );
            }
        }
        m->info_.eventor_->push_call( call, params );
        ls.push( true );
        return 1;
    }

    inline bool is_number( const objects::base *o )
    {
        return o->type_id( ) == objects::base::TYPE_NUMBER
//            || o->type_id( ) == objects::base::TYPE_INTEGER
//            || o->type_id( ) == objects::base::TYPE_UINTEGER
            ;
    }

    inline bool is_string( const objects::base *o )
    {
        return o->type_id( ) == objects::base::TYPE_STRING
            ;
    }

    int lcall_timer_post( lua_State *L )
    {
        module *m = get_module( L );

        lua::state ls(L);

        int n = ls.get_top( );

        unsigned sec    = 0;
        unsigned milli  = 0;
        unsigned micro  = 0;

        base_sptr call;

        if( n > 0 ) {
            call = ls.get_ref( 1 );
        }

        if( n > 1 ) {
            auto tobj(ls.get_object( 2 ));
            if( tobj->type_id( ) == base::TYPE_TABLE ) {
                for( size_t i=0; i<tobj->count( ); i++ ) {

                    auto p(tobj->at(i)); // always pair 'cuz type_table
                    auto f(p->at(0));
                    auto s(p->at(1));

                    if( is_number( s ) ) {
                        if( is_number( f ) ) {
                            switch( static_cast<unsigned>(f->num( )) ) {
                            case 1:
                                sec   = static_cast<unsigned>(s->num( ));
                                break;
                            case 2:
                                milli = static_cast<unsigned>(s->num( ));
                                break;
                            case 3:
                                micro = static_cast<unsigned>(s->num( ));
                                break;
                            }
                        } else if( is_string( f ) ) {
                            std::string name(f->str( ));
                            if( !name.compare( "seconds" ) ) {
                                sec   = static_cast<unsigned>(s->num( ));
                            } else if( !name.compare( "sec" ) ) {
                                sec   = static_cast<unsigned>(s->num( ));
                            } else if( !name.compare( "milliseconds" ) ) {
                                milli = static_cast<unsigned>(s->num( ));
                            } else if( !name.compare( "milli" ) ) {
                                milli = static_cast<unsigned>(s->num( ));
                            } else if( !name.compare( "microseconds" ) ) {
                                micro = static_cast<unsigned>(s->num( ));
                            } else if( !name.compare( "micro" ) ) {
                                micro = static_cast<unsigned>(s->num( ));
                            }
                        }
                    }
                }
            } else if( is_number( tobj.get( ) ) ) {
                sec = static_cast<unsigned>(tobj->num( ));
            }
        }

        std::vector<objects::base_sptr> params;

        if( n > 2 ) {
            for( int i=3; i<=n; i++ ) {
                params.push_back( ls.get_ref( i ) );
            }
        }

        typedef vcomm::timer::monotonic_traits::seconds      seconds;
        typedef vcomm::timer::monotonic_traits::milliseconds millisec;
        typedef vcomm::timer::monotonic_traits::microseconds microsec;

        if( call ) {
            size_t id = m->info_.eventor_->next_index( );
            auto t    = m->add_timer( id );
            t->call_from_now( std::bind( &module::timer_handler, m,
                                          std::placeholders::_1,
                                          id, call, params,
                                          m->info_.eventor_->weak_from_this( )),
                              seconds(sec) + millisec(milli) + microsec(micro));
            ls.push( utils::to_handle(id) );
            return 1;
        }

        ls.push( utils::to_handle(0) );
        return 1;
    }

    int lcall_timer_cancel( lua_State *L )
    {
        module *m = get_module( L );
        lua::state ls(L);
        size_t id = utils::from_handle<size_t>( ls.get_opt<void *>( 1 ) );
        if( id ) {
            m->del_timer( id );
        }
        ls.push( true );
        return 1;
    }

}

    iface_sptr create( client::general_info &info )
    {
        return std::make_shared<module>( std::ref( info ) );
    }

}}}}


