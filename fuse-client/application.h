#ifndef APPLICATION_H
#define APPLICATION_H

#include <fuse.h>

namespace boost { namespace program_options {
    class variables_map;
}}

namespace fr { namespace fuse {

    class application {
        struct impl;
        impl *impl_;
    public:
        application( );
        ~application( );
        void stopall( );

    public:

        static fuse_operations set_operations( );

    };

    extern application *g_app;
    extern boost::program_options::variables_map g_opts;
}}

#endif // APPLICATION_H
