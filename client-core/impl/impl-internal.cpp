#include <iostream>
#include "client-core/interfaces/IInternal.h"

#include "protocol/ferro.pb.h"

#include "vtrc-common/vtrc-stub-wrapper.h"
#include "vtrc-common/vtrc-rpc-channel.h"

#include "client-core/fr-client.h"

#include "vtrc-chrono.h"

namespace fr { namespace client { namespace interfaces {

    namespace {

        namespace vcomm  = vtrc::common;
        namespace chrono = vtrc::chrono;

        typedef chrono::high_resolution_clock     high_resolution_clock;
        typedef high_resolution_clock::time_point time_point;

        typedef vcomm::rpc_channel                           channel_type;
        typedef proto::internal::Stub                        stub_type;
        typedef vcomm::stub_wrapper<stub_type, channel_type> client_type;

        const unsigned no_wait_flags = vcomm::rpc_channel::DISABLE_WAIT;

        size_t get_micro( const time_point &f, const time_point &l )
        {
            return chrono::duration_cast<
                        chrono::microseconds
                   >( l - f ).count( );
        }

        struct internal_impl: public internal::iface {

            mutable client_type client_;

            internal_impl( core::client_core &cl )
                :client_(cl.create_channel( ), true)
            { }

            vtrc::common::rpc_channel *channel( ) override
            {
                return client_.channel( );
            }

            const vtrc::common::rpc_channel *channel( ) const override
            {
                return client_.channel( );
            }

            void exit_process( ) const override
            {
                client_.channel( )->set_flags( no_wait_flags );
                client_.call( &stub_type::exit_process );
            }

            internal::agent_info info( ) const override
            {
                proto::info_res res;
                client_.call_response( &stub_type::info, &res );

                std::cout << res.DebugString( ) << "\n";

                internal::agent_info result;

                result.name.swap(*res.mutable_name( ));
                result.now          = res.tick_now( );
                result.tick_count   = res.tick_count( );

                return result;
            }

            size_t ping( ) const override
            {
                time_point start = high_resolution_clock::now( );
                client_.call( &stub_type::ping );
                time_point stop = high_resolution_clock::now( );
                return get_micro( start, stop );
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
