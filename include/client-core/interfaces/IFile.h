#ifndef FR_INTERFACE_FILE_H
#define FR_INTERFACE_FILE_H

#include <string>
#include <stdint.h>

#include "vtrc-function.h"

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
            ,TRUNC       =    01000
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

    enum seek_whence {
         POS_SEEK_SET = 0
        ,POS_SEEK_CUR = 1
        ,POS_SEEK_END = 2
    };

    static
    inline seek_whence whence_value2enum( unsigned value )
    {
        switch ( value ) {
        case file::POS_SEEK_CUR:
        case file::POS_SEEK_SET:
        case file::POS_SEEK_END:
            return static_cast<seek_whence>(value);
        }
        return file::POS_SEEK_SET;
    }

    typedef vtrc::function<
            void (unsigned, const std::string &)
    > file_event_callback;

    struct base_file_iface {
        virtual ~base_file_iface( ) { }
        virtual size_t  read( void *data,       size_t length  ) const = 0;
        virtual size_t write( const void *data, size_t length  ) const = 0;
        virtual void   ioctl( unsigned code,    uint64_t param ) const = 0;
    };

    struct iface: public base_file_iface {
        virtual ~iface( ) { }

        virtual int64_t seek( int64_t pos, seek_whence whence ) const = 0;
        virtual int64_t tell( )  const = 0;
        virtual void    flush( ) const = 0;

        virtual void register_for_events( file_event_callback cb ) = 0;
        virtual void unregister( ) = 0;

    };

    typedef iface* iface_ptr;

    iface_ptr create( core::client_core &cl,
                      const std::string &path, const std::string &mode );

    iface_ptr create( core::client_core &cl,
                      const std::string &path, unsigned flags );

    iface_ptr create( core::client_core &cl,
                      const std::string &path, unsigned flags, unsigned mode );

    iface_ptr create_simple_device( core::client_core &cl,
                      const std::string &path, unsigned flags );

    iface_ptr create_simple_device( core::client_core &cl,
                      const std::string &path, unsigned flags, unsigned mode );

}}}}

#endif // IFILE_H
