#include "event-container.h"

#include "general-info.h"

namespace fr { namespace lua {

    using namespace lua::objects;

    bool event_container::exists( const std::string &name ) const
    {
        return event_map_.find(name) != event_map_.end( );
    }

    bool event_container::exists_and_set( const std::string &name ) const
    {
        auto f(event_map_.find(name));
        return (f != event_map_.end( )) && (f->second);
    }

    int event_container::subscribe( lua_State *L , int shift ,
                                    subscribe_info *res )
    {
        lua::state ls(L);
        int n = ls.get_top( );

        std::string name( ls.get_opt<std::string>( 1 + shift ) );

        base_sptr call;
        if( n > (1 + shift) ) {
            call = ls.get_ref( 2 + shift );
        }

        auto finfo( event_map_.find( name ) );

        if( finfo == event_map_.end( ) ) {
            ls.push( false );
            ls.push( std::string("Invalid event name '")
                   + name
                   + std::string("'."));
            return 2;
        }

        std::vector<base_sptr> params;

        if( n > (2 + shift) )  {
            for( int i=(3 + shift); i <= n; i++ ) {
                params.push_back( base_sptr( new_reference( L, i ) ) );
            }
        }

        if( call ) {
            event_info_sptr par(std::make_shared<event_info>( ));
            par->call_object_ = call;
            par->parameters_.swap( params );
            finfo->second = par;
        } else {
            finfo->second = event_info_sptr( );
        }

        if( res ) {
            res->call_      = call;
            res->name_      = name;
            res->result_    = 1;
        }

        ls.push( true );
        return 1;
    }

    int event_container::push_state( lua_State *L ) const
    {
        objects::table_sptr res(objects::new_table( ));
        for( auto &e: event_map_ ) {
            objects::table_sptr inf(objects::new_table( ));
            if( e.second ) {

                inf->add( "call",   e.second->call_object_ );

                if( !e.second->parameters_.empty( ) ) {
                    objects::table_sptr par(objects::new_table( ));
                    for( auto &p: e.second->parameters_ ) {
                        par->add( p );
                    }
                    inf->add( "argv", par );
                }
            }
            res->add( e.first, inf );
        }
        res->push( L );
        return 1;
    }

    void event_container::call( const std::string &name,
                                objects::base_sptr param )
    {
        auto f(event_map_.find(name));
        if((f != event_map_.end( )) && (f->second)) {
            auto &inf(*f->second);
            info_.eventor_->push_call( inf.call_object_,
                                       param, inf.parameters_ );
        }
    }

}}
