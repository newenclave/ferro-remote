#ifndef FR_LUA_EVENT_CONTAINER_H
#define FR_LUA_EVENT_CONTAINER_H

#include <map>
#include <memory>

#include "fr-lua.h"

#define FR_LUA_EVENT_PROLOGUE( name, container )                               \
        static const std::string event_name( name );                           \
        lua::event_container &event_container(container);                      \
        if( event_container.exists_and_set( event_name ) ) {                   \
            objects::table_sptr result(std::make_shared<objects::table>( ))

#define FR_LUA_EVENT_EPILOGUE                                                  \
            event_container.call( event_name, result );                        \
        }

namespace fr { namespace lua {

    namespace client {
        class general_info;
    }

    struct event_info {
        objects::base_sptr              call_object_;
        std::vector<objects::base_sptr> parameters_;
        //size_t                          id_;
        event_info( )
            //:id_(0)
        { }
    };

    typedef std::shared_ptr<event_info> event_info_sptr;
    typedef std::map<std::string, event_info_sptr> event_handlers_map;

    class event_container {

        event_handlers_map      event_map_;
        client::general_info   &info_;

    public:

        template <typename ContType>
        event_container( client::general_info &inf, const ContType &cont )
            :info_(inf)
        {
            for( auto &name: cont ) {
                event_map_.insert( std::make_pair( name, event_info_sptr( ) ) );
            }
        }

        bool exists( const std::string &name ) const;
        bool exists_and_set( const std::string &name ) const;

        struct subscribe_info {
            int                result_;
            std::string        name_;
            objects::base_sptr call_;
        };
        /// Subscribe to the events
        ///  lua stack must contain correct information for registration
        /// Here is:
        ///     1 + shift      = event name: string; from the scripts
        ///                      it may be obtained by
        ///                      fr.client.'modulename'.events();
        ///     2 + shift      = event handler: function
        ///     3 + shift, ... = additional params will be pass
        ///                      to the event handler
        ///                      with their references
        int subscribe( lua_State *L, int shift = 0,
                       subscribe_info *res = nullptr );
        void call( const std::string &name, objects::base_sptr param );

        int push_state( lua_State *L ) const;
    };

}}


#endif // EVENTCONTAINER_H
