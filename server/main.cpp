#include <iostream>

#include "vtrc-server/vtrc-application.h"
#include "vtrc-server/vtrc-listener-tcp.h"

#include "vtrc-common/vtrc-connection-iface.h"
#include "vtrc-common/vtrc-closure-holder.h"
#include "vtrc-common/vtrc-thread-pool.h"

#include "protocol/ferro.pb.h"
#include "protocol/fs.pb.h"

#include "google/protobuf/descriptor.h"
#include "boost/lexical_cast.hpp"

using namespace vtrc;

int main( int argc, const char **argv )
{
    google::protobuf::ShutdownProtobufLibrary( );
    return 0;
}
