#ifndef FR_POLLREACTOR_H
#define FR_POLLREACTOR_H

#include "vtrc-function.h"
#include <stddef.h>

namespace fr { namespace agent {

    using reaction_callback = vtrc::function<bool (unsigned, std::uint64_t)>;

    class poll_reactor {

        struct   impl;
        impl    *impl_;

    public:

        poll_reactor( const poll_reactor & ) = delete;
        poll_reactor &operator = ( const poll_reactor & ) = delete;

        poll_reactor( std::uint64_t app_start );
        ~poll_reactor( );

        void add_fd( int fd, unsigned events, reaction_callback cb );
        void del_fd( int fd );

        size_t count( ) const;

        size_t run_one( );
        size_t run( );

        void stop( );

    };

}}

#endif // FR_POLLREACTOR_H
