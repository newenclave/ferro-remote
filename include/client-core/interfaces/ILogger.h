#ifndef FR_ILOGGER_H
#define FR_ILOGGER_H

namespace fr { namespace client {

namespace core {
    class client_core;
}


namespace interfaces { namespace logger {

    enum log_level {
         zero       = 0
        ,error      = 1
        ,warning    = 2
        ,info       = 3
        ,debug      = 4
    };

    struct iface {
        virtual ~iface( ) { }
    };

}} // intrerface

}} // fr::client

#endif // ILOGGER_H
