#ifndef FR_POLLREACTOR_H
#define FR_POLLREACTOR_H

namespace fr { namespace server {

    class poll_reactor {
        struct   impl;
        impl    *impl_;

    public:

        poll_reactor( );
        ~poll_reactor( );
    };

}}

#endif // FR_POLLREACTOR_H
