#ifndef APPLICATION_H
#define APPLICATION_H

#include "interfaces/IFilesystem.h"
#include "interfaces/IFile.h"

namespace fr { namespace fuse {

    class application {
        struct impl;
        impl *impl_;
    public:
        application( );
        ~application( );
        void stopall( );
    };

    void *init_app( );
    void  destroy_app( void *app );

    extern application *g_app;

}}

#endif // APPLICATION_H
