#include <iostream>

#include "application.h"

#include "google/protobuf/descriptor.h"

int main( int argc, const char **argv )
{
    google::protobuf::ShutdownProtobufLibrary( );
    return 0;
}

