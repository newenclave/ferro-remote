
#include "interfaces/IGPIO.h"
#include "protocol/gpio.pb.h"

#include "client-core/fr-client.h"

#include "vtrc-common/vtrc-stub-wrapper.h"
#include "vtrc-common/vtrc-rpc-channel.h"

#include "vtrc-stdint.h"

namespace fr { namespace client { namespace interfaces {

    namespace {

        namespace vcomm = vtrc::common;

        typedef protocol::gpio::instance::Stub stub_type;
        typedef vcomm::stub_wrapper<stub_type> client_type;

        vtrc::uint32_t open_instance( client_type &cl )
        {

        }

        struct gpio_impl: public gpio::iface {

            unsigned    id_;
            client_type client_;

            gpio_impl( core::client_core &cl, unsigned id )
                :id_(id)
                ,client_(cl.create_channel( ), true)
            { }

            bool exists( ) const override
            {

            }

        };
    }

    namespace gpio {
        iface_ptr create( core::client_core &cl, unsigned id )
        {
            return new gpio_impl( cl, id );
        }
    }

}}}
