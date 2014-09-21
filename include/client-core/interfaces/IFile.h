#ifndef FR_INTERFACE_FILE_H
#define FR_INTERFACE_FILE_H

#include <string>

namespace fr { namespace client {

namespace core {
    class client_core;
}

namespace interfaces { namespace file {

    namespace flags {
        enum {
             RDONLY      =        0
            ,WRONLY      =       01
            ,RDWR        =       02
            ,CREAT       =     0100
            ,EXCL        =     0200
            ,APPEND      =    02000
            ,NONBLOCK    =    04000
            ,ASYNC       =   020000
            ,SYNC        = 04010000
        };
    }

    namespace mode {
        enum {
             IRWXU  = 00700
            ,IRUSR  = 00400
            ,IWUSR  = 00200
            ,IXUSR  = 00100
            ,IRWXG  = 00070
            ,IRGRP  = 00040
            ,IWGRP  = 00020
            ,IXGRP  = 00010
            ,IRWXO  = 00007
            ,IROTH  = 00004
            ,IWOTH  = 00002
            ,IXOTH  = 00001
        };
    }

    struct iface {
        virtual ~iface( ) { }
    };

    typedef iface* iface_ptr;

    iface_ptr create(core::client_core &cl,
                      const std::string &path, unsigned flags );

    iface_ptr create( core::client_core &cl,
                      const std::string &path, unsigned flags, unsigned mode );

}}}}

#endif // IFILE_H
