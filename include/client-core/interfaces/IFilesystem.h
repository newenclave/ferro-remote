#ifndef FR_INTERFACE_FILESYSTEM_H
#define FR_INTERFACE_FILESYSTEM_H

#include <string>
#include "vtrc-stdint.h"

namespace fr { namespace client {

namespace core {
    class client_core;
}

namespace interfaces { namespace filesystem {


    struct path_stat {          /*            man 2 stat            */
        vtrc::uint64_t dev;     /* ID of device containing file     */
        vtrc::uint64_t ino;     /* inode number                     */
        vtrc::uint32_t mode;    /* protection                       */
        vtrc::uint64_t nlink;   /* number of hard links             */
        vtrc::uint32_t uid;     /* user ID of owner                 */
        vtrc::uint32_t gid;     /* group ID of owner                */
        vtrc::uint64_t rdev;    /* device ID (if special file)      */
        vtrc::uint64_t size;    /* total size, in bytes             */
        vtrc::uint32_t blksize; /* blocksize for filesystem I/O     */
        vtrc::uint64_t blocks;  /* number of 512B blocks allocated  */
        vtrc::uint64_t atime;   /* time of last access              */
        vtrc::uint64_t mtime;   /* time of last modification        */
        vtrc::uint64_t ctime;   /* time of last status change       */
    };

    struct iface {
        virtual ~iface( ) { }
        virtual bool exists( const std::string &path ) const = 0;
        virtual bool stat( const std::string &path, path_stat &data ) const = 0;
    };

    typedef iface* iface_ptr;
    iface_ptr create( core::client_core &cl, const std::string &path );

}}}}

#endif // FR_INTERFACE_FILESYSTEM_H
