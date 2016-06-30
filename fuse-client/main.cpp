#include <iostream>
#include <fuse.h>

#include "application.h"

fuse_operations operations;

int main( int argv, char **argc )
{
    operations.init         = &fr::fuse::init_app;
    operations.destroy      = &fr::fuse::destroy_app;

    fuse_main( argv, argc, &operations );
    return 0;
}


