
#include "client-core/interfaces/IOS.h"
#include "protocol/os.pb.h"

#include "client-core/fr-client.h"

#include "vtrc-common/vtrc-stub-wrapper.h"
#include "vtrc-common/vtrc-rpc-channel.h"

namespace fr { namespace client { namespace interfaces {

    namespace {

        namespace oproto = fr::proto::os;
        namespace vcomm = vtrc::common;
        typedef vcomm::rpc_channel                           channel_type;
        typedef oproto::instance::Stub                       stub_type;
        typedef vcomm::stub_wrapper<stub_type, channel_type> client_type;

        class os_impl: public os::iface {

            mutable client_type client_;

        public:

            os_impl( core::client_core &cl )
                :client_(cl.create_channel( ), true)
            { }

            int execute( const std::string &cmd ) const override
            {
                oproto::execute_req req;
                oproto::execute_res res;
                req.set_cmd( cmd );
                client_.call( &stub_type::execute, &req, &res );

                return res.result( );
            }

            vtrc::common::rpc_channel *channel( )
            {
                return client_.channel( );
            }

            const vtrc::common::rpc_channel *channel( ) const
            {
                return client_.channel( );
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
