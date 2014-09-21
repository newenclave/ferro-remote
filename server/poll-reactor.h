#ifndef FR_POLLREACTOR_H
#define FR_POLLREACTOR_H

#include "vtrc-function.h"

namespace fr { namespace server {

    typedef vtrc::function<void ( )> reaction_callback;

    class poll_reactor {
        struct   impl;
        impl    *impl_;

    public:

        poll_reactor( );
        ~poll_reactor( );

        void add_fd( int fd, unsigned flags, reaction_callback cb );

    };

}}

#endif // FR_POLLREACTOR_H
