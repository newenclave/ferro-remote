
#include "boost/asio.hpp"
#include "event-caller.h"

#include <atomic>
#include <iostream>

namespace fr { namespace lua {

    struct event_caller::impl {

        lua_State                       *state_;
        boost::asio::io_service::strand  dispatcher_;
        std::atomic<size_t>              index_;
        bool                             enabled_;

        impl(lua_State *L, boost::asio::io_service &ios)
            :state_(L)
            ,dispatcher_(ios)
            ,index_(100)
            ,enabled_(true)
        { }

    };

    event_caller::event_caller( lua_State *L, boost::asio::io_service &ios )
        :impl_(new impl(L, ios))
    { }

    event_caller::~event_caller( )
    { }

    lua_State * event_caller::state( )
    {
        return impl_->state_;
    }

    const lua_State * event_caller::state( ) const
    {
        return impl_->state_;
    }

    typedef std::vector<lua::objects::base_sptr> param_vector;
    typedef std::shared_ptr<
                std::vector<lua::objects::base_sptr>
            >  param_vector_sptr;

    void push_call_impl( std::weak_ptr<event_caller> k,
                         lua::objects::base_sptr call,
                         param_vector_sptr params )
    {
        auto kl(k.lock( ));
        if( !kl ) {
            return;
        }

        lua_State *L = kl->state( );

        lua::state ls(kl->state( ));

        call->push( L );

        for( auto &p: *params ) {
            p->push( L );
        }

        int res = lua_pcall( L, params->size( ), LUA_MULTRET, 0 );

        if( res != LUA_OK ) {
            std::string err = ls.pop_error( );
            std::cerr << err << "\n";
        }
    }

    size_t event_caller::next_index( )
    {
        return impl_->index_++;
    }

    void event_caller::set_enable( bool value )
    {
        impl_->enabled_ = value;
    }

    bool event_caller::get_enable( ) const
    {
        return impl_->enabled_;
    }

    size_t event_caller::push_call( lua::objects::base_sptr call,
                                    lua::objects::base_sptr fst_param,
                                    const param_vector &params )
    {
        if( !impl_->enabled_ ) {
            return 0;
        }
        std::shared_ptr<param_vector> allp(std::make_shared<param_vector>( ));

        allp->push_back( fst_param );
        allp->insert( allp->end( ), params.begin( ), params.end( ));
        impl_->dispatcher_.post( std::bind( push_call_impl,
                                 weak_from_this( ), call, allp ) );
        return 1;
    }

    size_t event_caller::push_call( lua::objects::base_sptr call,
                                    const param_vector &params )
    {
        if( !impl_->enabled_ ) {
            return 0;
        }
        impl_->dispatcher_.post( std::bind( push_call_impl,
                                 weak_from_this( ), call,
                                 std::make_shared<param_vector>(params) ) );
        return 1;
    }

}}
