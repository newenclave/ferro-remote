#ifndef FR_FILEIFACE_H
#define FR_FILEIFACE_H

#include "vtrc-stdint.h"

#include <string>
#include <unistd.h>

namespace fr { namespace server {

    struct file_iface {

        enum seek_whence {
             F_SEEK_SET = SEEK_SET
            ,F_SEEK_CUR = SEEK_CUR
            ,F_SEEK_END = SEEK_END
        };

        virtual ~file_iface( ) { }

        virtual vtrc::int64_t seek( vtrc::int64_t offset,
                                     seek_whence whence ) = 0;
        virtual vtrc::int64_t tell( ) const = 0;

        virtual size_t write( const void *data, size_t length ) = 0;
        virtual size_t read(        void *data, size_t length ) = 0;

        virtual void flush( ) = 0;

        virtual int handle( ) const = 0;
    };

    typedef file_iface * file_ptr;

    namespace file {
        file_ptr create( std::string const &path, int flags );
        file_ptr create( std::string const &path, int flags, mode_t mode );
    }

}}

#endif // FR_FILEIFACE_H
