#include <iostream>

#include "vtrc-client/vtrc-client.h"
#include "vtrc-common/vtrc-thread-pool.h"
#include "vtrc-common/vtrc-stub-wrapper.h"

#include "protocol/ferro.pb.h"

#include "boost/lexical_cast.hpp"

using namespace vtrc;

int main( int argc, const char **argv )
{
    common::thread_pool tp( 1 );

    return 0;
}
