#include "client-core/interfaces/IInternal.h"

#include "protocol/ferro.pb.h"

#include "vtrc-common/vtrc-stub-wrapper.h"
#include "vtrc-common/vtrc-rpc-channel.h"

#include "client-core/fr-client.h"

namespace fr { namespace client { namespace interfaces {

    namespace {
        namespace vcomm  = vtrc::common;

        typedef proto::internal::Stub           stub_type;
        typedef vcomm::stub_wrapper<stub_type>  client_type;

        static unsigned no_wait_flags = vcomm::rpc_channel::DISABLE_WAIT;

        struct internal_impl: public internal::iface {

            mutable client_type client_;
            mutable client_type nw_client_;

            internal_impl( core::client_core &cl )
                :client_(cl.create_channel( ), true)
                ,nw_client_(cl.create_channel( no_wait_flags ), true)
            { }

            void exit_process( ) const
            {
                nw_client_.call( &stub_type::exit_process );
            }
        };
    }

    namespace internal {
        iface_ptr create( core::client_core &cl )
        {
            return new internal_impl( cl );
        }
    }
}}}
