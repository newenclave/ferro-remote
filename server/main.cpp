#include <iostream>

#include "application.h"
#include "vtrc-common/vtrc-pool-pair.h"

#include "google/protobuf/descriptor.h"

namespace vserver = vtrc::server;

int main( int argc, const char **argv )
{



    google::protobuf::ShutdownProtobufLibrary( );
    return 0;
}

