#include <iostream>

#include "application.h"
#include "vtrc-common/vtrc-pool-pair.h"

#include "google/protobuf/descriptor.h"

namespace vserver = vtrc::server;
namespace vcommon = vtrc::common;
using namespace fr;

int main( int argc, const char **argv )
{
    vcommon::pool_pair pp( 0 );
    server::application app( pp );

    pp.stop_all( );
    pp.join_all( );
    google::protobuf::ShutdownProtobufLibrary( );
    return 0;
}

