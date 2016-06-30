#include "application.h"

namespace fr { namespace fuse {

    application *g_app;

    application::application( )
    { }

    application::~application( )
    { }

    void *init_app( )
    {
        g_app = new application( );
        return static_cast<void *>(g_app);
    }

    void  destroy_app( void *app )
    {
        delete static_cast<application *>(app);
    }

} }
