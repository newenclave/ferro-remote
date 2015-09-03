#include "client-core/interfaces/ISPI.h"
#include "protocol/spi.pb.h"

#include "client-core/fr-client.h"

#include "vtrc-common/vtrc-stub-wrapper.h"
#include "vtrc-common/vtrc-rpc-channel.h"

namespace fr { namespace client { namespace interfaces {

    namespace {

        namespace sproto = fr::proto::spi;
        namespace vcomm = vtrc::common;
        typedef vcomm::rpc_channel                           channel_type;
        typedef sproto::instance::Stub                       stub_type;
        typedef vcomm::stub_wrapper<stub_type, channel_type> client_type;

        class spi_impl: public spi::iface {

            mutable client_type client_;

        public:

            spi_impl( core::client_core &cl )
                :client_(cl.create_channel( ), true)
            { }

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

    namespace spi {
        iface_ptr create( core::client_core &cl )
        {
            return new spi_impl( cl );
        }
    }

}}}
