#ifndef FR_INTERFACE_FILESYSTEM_H
#define FR_INTERFACE_FILESYSTEM_H

#include <string>
#include "vtrc-stdint.h"

namespace fr { namespace client {

namespace core {
    class client_core;
}

namespace interfaces { namespace filesystem {


    struct stat_data {          /*            man 2 stat            */
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

    struct info_data {
        bool is_exist;
        bool is_directory;
        bool is_empty;
        bool is_regular;
        bool is_symlink;
    };

    struct directory_iterator {
        virtual ~directory_iterator( ) { }
    };

    typedef directory_iterator * directory_iterator_ptr;

    struct iface {

        virtual ~iface( ) { }

        virtual bool exists( const std::string &path ) const = 0;

        virtual void stat( const std::string &path, stat_data &data ) const = 0;
        virtual void info( const std::string &path, info_data &data ) const = 0;

        virtual vtrc::uint64_t file_size( const std::string &path ) const = 0;

        virtual void cd( const std::string &path ) = 0;
        virtual const std::string &pwd(  ) const = 0;

        virtual void mkdir( const std::string &path ) const = 0;
        virtual void del( const std::string &path )   const = 0;

    };

    typedef iface* iface_ptr;
    iface_ptr create( core::client_core &cl, const std::string &path );

    directory_iterator_ptr directory_begin( core::client_core &cl,
                                            const std::string &path );


}}}}

#endif // FR_INTERFACE_FILESYSTEM_H
