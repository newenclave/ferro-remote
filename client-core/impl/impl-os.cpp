
#include "interfaces/IOS.h"
#include "protocol/os.pb.h"

#include "client-core/fr-client.h"

#include "vtrc-common/vtrc-stub-wrapper.h"
#include "vtrc-common/vtrc-rpc-channel.h"

namespace fr { namespace client { namespace interfaces {

    namespace {

        namespace os_proto = fr::protocol::os;
        typedef os_proto::instance_Stub               stub_type;
        typedef vtrc::common::stub_wrapper<stub_type> client_type;
        typedef vtrc::common::rpc_channel             rpc_channel;

        class os_impl: public os::iface {

            client_type client_;

        public:

            os_impl( core::client_core &cl )
                :client_(cl.create_channel( ))
            { }

            void execute( const std::string &cmd )
            {
                os_proto::execute_req req;
                req.set_cmd( cmd );
                client_.call_request( &stub_type::execute, &req );
            }
        };
    }

    namespace os {
        iface_ptr create( core::client_core &cl )
        {
            return new os_impl( cl );
        }
    }

}}}
