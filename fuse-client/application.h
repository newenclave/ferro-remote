#ifndef APPLICATION_H
#define APPLICATION_H

#include <fuse.h>
#include "boost/program_options.hpp"

namespace fr { namespace fuse {

    class application {
        struct impl;
        impl *impl_;
    public:
        application( );
        ~application( );
        void stopall( );
        void start( );

    public:

        static fuse_operations set_operations( );

    };

    extern application *g_app;
    extern boost::program_options::variables_map g_opts;
}}

#endif // APPLICATION_H
