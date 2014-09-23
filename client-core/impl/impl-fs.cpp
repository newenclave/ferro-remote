
#include "client-core/interfaces/IFs.h"

#include "protocol/fs.pb.h"

#include "client-core/fr-client.h"

#include "vtrc-common/vtrc-stub-wrapper.h"
#include "vtrc-common/vtrc-rpc-channel.h"

namespace fr { namespace client { namespace interfaces {

    namespace {
        struct fs_impl: public filesystem::iface {

        };
    }

    namespace filesystem {
        iface_ptr create( core::client_core &cl, const std::string &path )
        {
            return new fs_impl;
        }
    }

}}}
