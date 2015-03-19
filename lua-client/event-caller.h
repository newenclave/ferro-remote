#ifndef FR_EVENTCALLER_H
#define FR_EVENTCALLER_H

#include <memory>
#include "fr-lua.h"

namespace boost { namespace asio {
    class io_service;
}}

namespace fr { namespace lua {

    class event_caller: public std::enable_shared_from_this<event_caller> {

        struct                impl;
        std::unique_ptr<impl> impl_;

    public:

        enum call_type {
             CALL_DEFAULT       = 0x0
            ,CALL_LAST_RESULT   = 0x1
            ,CALL_WAIT_RESULTS  = CALL_LAST_RESULT << 1
        };

        event_caller( lua_State *L, boost::asio::io_service &ios );
        ~event_caller( );

        size_t push_call( lua::objects::base_sptr call,
                          const std::vector<lua::objects::base_sptr> &params );

        size_t push_call( lua::objects::base_sptr call,
                          lua::objects::base_sptr fst_param,
                          const std::vector<lua::objects::base_sptr> &params );

        std::weak_ptr<event_caller> weak_from_this( )
        {
            return std::weak_ptr<event_caller>(shared_from_this( ));
        }

        std::weak_ptr<const event_caller> weak_from_this( ) const
        {
            return std::weak_ptr<const event_caller>(shared_from_this( ));
        }

        size_t next_index( );

        lua_State *state( );
        const lua_State *state( ) const;

    };

    typedef std::shared_ptr<event_caller> event_caller_sptr;
    typedef std::weak_ptr<event_caller>   event_caller_wptr;

}}

#endif // EVENTCALLER_H
