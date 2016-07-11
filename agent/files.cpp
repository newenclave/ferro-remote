
#include "files.h"

#include <sstream>
#include <stdexcept>

#include <errno.h>
#include <stdio.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <sys/ioctl.h>

#include "errno-check.h"

namespace fr { namespace agent {

    namespace {

        int open_wrapper( const char *path, int flags )
        {
            int fd = open( path, flags );
            errno_error::errno_assert( -1 != fd, "open" );
            return fd;
        }

        FILE *open_wrapper( const char *path,  const char *mode )
        {
            FILE * fd = fopen( path, mode );
            errno_error::errno_assert( fd != NULL, "fopen" );
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
                                 seek_whence whence ) override
            {
                off_t res = lseek( fd_, static_cast<off_t>(offset), whence );
                return lseek_ret( res );
            }

            vtrc::int64_t tell( ) const override
            {
                off_t res = lseek( fd_, 0, SEEK_CUR );
                return lseek_ret( res );
            }

            size_t write( const void *data, size_t length ) override
            {
                ssize_t res = ::write( fd_, data, length );
                errno_error::errno_assert( -1 != res, "write" );
                return static_cast<size_t>(res);
            }

            void ioctl( int code, unsigned long data ) override
            {
                int res = ::ioctl( fd_, code, data );
                errno_error::errno_assert( res != -1, "ioctl" );
            }

            void ioctl( int code, void *data ) override
            {
                int res = ::ioctl( fd_, code, data );
                errno_error::errno_assert( res != -1, "ioctl" );
            }

            size_t read( void *data, size_t length ) override
            {
                ssize_t res = ::read( fd_, data, length );
                errno_error::errno_assert( -1 != res, "read" );
                return static_cast<size_t>(res);
            }

            void flush( ) override
            {
#ifdef __USE_GNU
                syncfs( fd_ );
#endif
            }

            int handle( ) const override
            {
                return fd_;
            }

        };

        struct file2_impl: public file_iface {

            FILE *fid_;
            int   fd_;

            file2_impl( std::string const &path, const std::string &mode )
                :fid_(open_wrapper(path.c_str( ), mode.c_str( )))
                ,fd_(fileno(fid_))
            { }

            ~file2_impl( )
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
                                 seek_whence whence ) override
            {
                off_t res = lseek( fd_, static_cast<off_t>(offset), whence );
                return lseek_ret( res );
            }

            vtrc::int64_t tell( ) const override
            {
                off_t res = lseek( fd_, 0, SEEK_CUR );
                return lseek_ret( res );
            }

            size_t write( const void *data, size_t length ) override
            {
                ssize_t res = ::write( fd_, data, length );
                errno_error::errno_assert( -1 != res, "write" );
                return static_cast<size_t>(res);
            }

            void ioctl( int code, unsigned long data ) override
            {
                int res = ::ioctl( fd_, code, data );
                errno_error::errno_assert( res != -1, "ioctl" );
            }

            void ioctl( int code, void *data ) override
            {
                int res = ::ioctl( fd_, code, data );
                errno_error::errno_assert( res != -1, "ioctl" );
            }

            size_t read( void *data, size_t length ) override
            {
                ssize_t res = ::read( fd_, data, length );
                errno_error::errno_assert( -1 != res, "read" );
                return static_cast<size_t>(res);
            }

            void flush( ) override
            {
                syncfs( fd_ );
            }

            int handle( ) const override
            {
                return fd_;
            }

        };

        struct device_file_impl: public file_iface {

            int fd_;

            device_file_impl( std::string const &path, vtrc::uint32_t flags )
                :fd_(open_wrapper(path.c_str( ), flags))
            { }

            device_file_impl( std::string const &path,
                              vtrc::uint32_t flags, mode_t mode )
                :fd_(open_wrapper(path.c_str( ), flags, mode))
            { }

            ~device_file_impl( )
            {
                close( fd_ );
            }

            vtrc::int64_t seek( vtrc::int64_t /*offset*/,
                                seek_whence   /*whence*/ ) override
            {
                return 0;
            }

            vtrc::int64_t tell( ) const
            {
                return 0;
            }

            void ioctl( int code, unsigned long data ) override
            {
                int res = ::ioctl( fd_, code, data );
                errno_error::errno_assert( res != -1, "ioctl" );
            }

            void ioctl( int code, void *data ) override
            {
                int res = ::ioctl( fd_, code, data );
                errno_error::errno_assert( res != -1, "ioctl" );
            }

            size_t write( const void *data, size_t length ) override
            {
                ssize_t res = ::write( fd_, data, length );
                lseek( fd_, 0, SEEK_SET );
                errno_error::errno_assert( -1 != res, "write" );
                return static_cast<size_t>(res);
            }

            size_t read( void *data, size_t length ) override
            {
                ssize_t res = ::read( fd_, data, length );
                lseek( fd_, 0, SEEK_SET );
                errno_error::errno_assert( -1 != res, "read" );
                return static_cast<size_t>(res);
            }

            void flush( ) override
            {
                syncfs( fd_ );
            }

            int handle( ) const override
            {
                return fd_;
            }

        };

        struct device2_file_impl: public file_iface {

            FILE *fid_;
            int fd_;

            device2_file_impl( std::string const &path,
                               std::string const &mode )
                :fid_(open_wrapper(path.c_str( ), mode.c_str( )))
                ,fd_(fileno(fid_))
            { }

            ~device2_file_impl( )
            {
                close( fd_ );
            }

            vtrc::int64_t seek( vtrc::int64_t /*offset*/,
                                seek_whence   /*whence*/ ) override
            {
                return 0;
            }

            vtrc::int64_t tell( ) const override
            {
                return 0;
            }

            void ioctl( int code, unsigned long data ) override
            {
                int res = ::ioctl( fd_, code, data );
                errno_error::errno_assert( res != -1, "ioctl" );
            }

            void ioctl( int code, void *data ) override
            {
                int res = ::ioctl( fd_, code, data );
                errno_error::errno_assert( res != -1, "ioctl" );
            }

            size_t write( const void *data, size_t length ) override
            {
                ssize_t res = ::write( fd_, data, length );
                lseek( fd_, 0, SEEK_SET );
                errno_error::errno_assert( -1 != res, "write" );
                return static_cast<size_t>(res);
            }

            size_t read( void *data, size_t length ) override
            {
                ssize_t res = ::read( fd_, data, length );
                lseek( fd_, 0, SEEK_SET );
                errno_error::errno_assert( -1 != res, "read" );
                return static_cast<size_t>(res);
            }

            void flush( ) override
            {
                syncfs( fd_ );
            }

            int handle( ) const override
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

        file_ptr create( std::string const &path, const std::string &mode )
        {
            return new file2_impl( path, mode );
        }

    }

    namespace device {
        file_ptr create( std::string const &path, int flags )
        {
            return new device_file_impl( path, flags );
        }

        file_ptr create( std::string const &path, int flags, mode_t mode )
        {
            return new device_file_impl( path, flags, mode );
        }

        file_ptr create( std::string const &path, const std::string &mode )
        {
            return new device2_file_impl( path, mode );
        }

    }

}}
