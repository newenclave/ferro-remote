#include "application.h"

#include "vtrc-memory.h"
#include "fr-client.h"
#include "vtrc-common/vtrc-pool-pair.h"

namespace fr { namespace fuse {

    application *g_app;

    using namespace vtrc;

    struct application::impl {
        common::pool_pair           pp_;
        client::core::client_core   client_;
        impl( )
            :pp_(1, 1)
            ,client_(pp_)
        { }
    };

    application::application( )
        :impl_(new impl)
    { }

    application::~application( )
    {
        delete impl_;
    }

    void application::stopall( )
    {
        impl_->pp_.stop_all( );
        impl_->pp_.join_all( );
    }

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
