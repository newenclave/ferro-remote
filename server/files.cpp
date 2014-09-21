
#include "files.h"

#include <sstream>
#include <stdexcept>

#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "errno-check.h"

namespace fr { namespace server {

    namespace {

        int open_wrapper( const char *path, int flags )
        {
            int fd = open( path, flags );
            errno_error::errno_assert( -1 != fd, "open" );
            return fd;
        }

        int open_wrapper( const char *path, int flags, mode_t mode )
        {
            int fd = open( path, flags, mode );
            errno_error::errno_assert( -1 != fd, "open" );
            return fd;
        }

        struct file_impl: public file_iface {

            int fd_;

            file_impl( std::string const &path, vtrc::uint32_t flags )
                :fd_(open_wrapper(path.c_str( ), flags))
            { }

            file_impl( std::string const &path,
                       vtrc::uint32_t flags, mode_t mode )
                :fd_(open_wrapper(path.c_str( ), flags, mode))
            { }

            ~file_impl( )
            {
                close( fd_ );
            }

            static vtrc::int64_t lseek_ret( off_t res )
            {
                errno_error::errno_assert( static_cast<off_t>(-1) != res,
                                           "lseek" );
                return static_cast<vtrc::int64_t>(res);
            }

            vtrc::int64_t seek( vtrc::int64_t offset,
                                 seek_whence whence )
            {
                off_t res = lseek( fd_, static_cast<off_t>(offset), whence );
                return lseek_ret( res );
            }

            vtrc::int64_t tell( ) const
            {
                off_t res = lseek( fd_, 0, SEEK_CUR );
                return lseek_ret( res );
            }

            size_t write( const void *data, size_t length )
            {
                ssize_t res = ::write( fd_, data, length );
                errno_error::errno_assert( -1 != res, "write" );
                return static_cast<size_t>(res);
            }

            size_t read( void *data, size_t length )
            {
                ssize_t res = ::read( fd_, data, length );
                errno_error::errno_assert( -1 != res, "read" );
                return static_cast<size_t>(res);
            }

            void flush( )
            {
                syncfs( fd_ );
            }

            int handle( ) const
            {
                return fd_;
            }
        };
    }


    namespace file {
        file_ptr create( std::string const &path, int flags )
        {
            return new file_impl( path, flags );
        }

        file_ptr create( std::string const &path, int flags, mode_t mode )
        {
            return new file_impl( path, flags, mode );
        }

    }

}}
