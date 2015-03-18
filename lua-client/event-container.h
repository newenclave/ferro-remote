#ifndef FR_LUA_EVENT_CONTAINER_H
#define FR_LUA_EVENT_CONTAINER_H

#include <map>
#include <memory>

#include "fr-lua.h"

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

        /// Subscribe to the events from dwservice
        ///  lua stack must contain correct information for registration
        /// Here is:
        ///     1       = event name: string; from the scripts
        ///               it may be obtained by fr.client.'modulename'.events();
        ///     2       = event handler: function
        ///     3, ...  = additional params will be pass to the event handler
        ///               with their references
        int subscribe( lua_State *L );
        void call( const std::string &name, objects::base_sptr param );

        int push_state( lua_State *L ) const;
    };

}}


#endif // EVENTCONTAINER_H
