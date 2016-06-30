#ifndef APPLICATION_H
#define APPLICATION_H

namespace fr { namespace fuse {

    struct application {
        application( );
        ~application( );
    };

    void *init_app( );
    void  destroy_app( void *app );

    extern application *g_app;

}}

#endif // APPLICATION_H
