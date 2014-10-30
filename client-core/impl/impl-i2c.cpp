
#include "client-core/interfaces/II2C.h"

#include "protocol/i2c.pb.h"

#include "client-core/fr-client.h"

#include "vtrc-common/vtrc-stub-wrapper.h"
#include "vtrc-common/vtrc-rpc-channel.h"

namespace fr { namespace client { namespace interfaces { namespace i2c {

    namespace i2cproto = proto::i2c;
    namespace vcomm    = vtrc::common;
    typedef   vcomm::rpc_channel* channel_ptr;

    typedef i2cproto::instance::Stub        stub_type;
    typedef vcomm::stub_wrapper<stub_type>  client_type;

    bool bus_available(core::client_core &cc, unsigned bus_id )
    {
        client_type calls( cc.create_channel( ), true );
        i2cproto::bus_available_req req;
        i2cproto::bus_available_res res;
        req.set_bus_id( bus_id );
        calls.call( &stub_type::bus_available, &req, &res );
        return res.value( );
    }

}

}}}
