#ifndef FR_APPLICATION_H
#define FR_APPLICATION_H

#include "vtrc-server/vtrc-application.h"
#include "vtrc-common/vtrc-signal-declaration.h"
#include "vtrc-common/vtrc-pool-pair.h"

namespace fr { namespace server {

    class application: public vtrc::server::application {
    public:
        application( vtrc::common::pool_pair &pp );

    };

}}


#endif // FR_APPLICATION_H
