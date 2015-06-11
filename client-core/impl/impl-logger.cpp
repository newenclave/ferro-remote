

#include "client-core/interfaces/ILogger.h"
#include "protocol/logger.pb.h"

#include "client-core/fr-client.h"

#include "vtrc-common/vtrc-stub-wrapper.h"
#include "vtrc-common/vtrc-rpc-channel.h"

namespace fr { namespace client { namespace interfaces {

    namespace {
        class logger_impl: public logger::iface {
        public:
            logger_impl( core::client_core &cl )
            { }
        };
    }

    namespace logger {
        iface_ptr create( core::client_core &cl )
        {
            return new logger_impl( cl );
        }
    }

}}}
