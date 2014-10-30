#include "i2c-helper.h"

#include <string>
#include <sstream>

#include <sys/types.h>
#include <sys/stat.h>

namespace fr { namespace agent {

    namespace {

        std::string bus2path( unsigned bus_id )
        {
            if( bus_id < 10 ) {
                std::string res = "/dev/i2c-?";
                res[9] = (char)(bus_id + '0');
                return res;
            } else {
                static const std::string prefix_path("/dev/i2c-");
                std::ostringstream oss;
                oss << prefix_path << bus_id;
                return oss.str( );
            }
        }
    }

    namespace i2c {

        bool available( unsigned bus_id )
        {
            struct stat ss = { 0 };
            int res = stat( bus2path(bus_id).c_str( ), &ss );
            return res != -1;
        }
    }
}}
