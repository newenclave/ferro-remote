#ifndef FR_ILOGGER_H
#define FR_ILOGGER_H

#include <string>

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
        virtual void set_level( log_level lvl ) const = 0;
        virtual log_level get_level( ) const = 0;
        virtual void write( log_level lvl, const std::string &text ) const = 0;
    };

    typedef iface* iface_ptr;
    iface_ptr create( core::client_core &cl );

}} // intrerface

}} // fr::client

#endif // ILOGGER_H
