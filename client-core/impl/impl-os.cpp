
#include "client-core/interfaces/IOS.h"
#include "protocol/os.pb.h"

#include "client-core/fr-client.h"

#include "vtrc-common/vtrc-stub-wrapper.h"
#include "vtrc-common/vtrc-rpc-channel.h"

namespace fr { namespace client { namespace interfaces {

    namespace {

        namespace oproto = fr::proto::os;
        typedef oproto::instance::Stub                stub_type;
        typedef vtrc::common::stub_wrapper<stub_type> client_type;
        typedef vtrc::common::rpc_channel             rpc_channel;

        class os_impl: public os::iface {

            mutable client_type client_;

        public:

            os_impl( core::client_core &cl )
                :client_(cl.create_channel( ), true)
            { }

            void execute( const std::string &cmd ) const override
            {
                oproto::execute_req req;
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
