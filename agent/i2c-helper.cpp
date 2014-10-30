#include "i2c-helper.h"

#include <string>
#include <sstream>

#include <stdlib.h>
#include <fcntl.h>

#include <sys/types.h>
#include <sys/stat.h>

#include <sys/ioctl.h>

#include "errno-check.h"
#include "file-keeper.h"

namespace fr { namespace agent {

    namespace {

        std::string bus2path( unsigned bus_id, char sep )
        {
            if( bus_id < 10 ) {
                std::string res = "/dev/i2c??";
                res[8] = sep;
                res[9] = (char)(bus_id + '0');
                return res;
            } else {
                static const std::string prefix_path("/dev/i2c");
                std::ostringstream oss;
                oss << prefix_path << sep << bus_id;
                return oss.str( );
            }
        }

        char available_sep( unsigned bus_id )
        {
            struct stat ss = { 0 };
            if( stat( bus2path(bus_id, '-').c_str( ), &ss ) == 0 ) {
                return '-';
            } else if( stat( bus2path(bus_id, '/').c_str( ), &ss ) == 0 ) {
                return '/';
            }
            return '\0';
        }

        int open_bus( unsigned bus_id )
        {
            char sep = available_sep( bus_id );
            std::string bus_path( bus2path( bus_id, sep ) );

            file_keeper fk( open( bus_path.c_str( ), O_RDWR ));
            errno_error::errno_assert( fk.hdl( ) != -1, "open_bus" );

            return fk.release( );
        }

    }

    i2c_helper::i2c_helper( unsigned bus_id )
        :fd_(open_bus(bus_id))
    { }

    i2c_helper::~i2c_helper(  )
    {
        if( -1 != fd_ ) {
            close( fd_ );
        }
    }

    void i2c_helper::ioctl( int code , unsigned long data )
    {
        int res = ::ioctl( fd_, code, data );
        errno_error::errno_assert( res != -1, "ioctl" );
    }

    void i2c_helper::ioctl( unsigned long *funcs )
    {
        int res = ::ioctl( fd_, I2C_FUNCS, funcs );
        errno_error::errno_assert( res != -1, "ioctl(I2C_FUNCS)" );
    }

    void i2c_helper::ioctl( i2c_rdwr_ioctl_data *data )
    {
        int res = ::ioctl( fd_, I2C_RDWR, data );
        errno_error::errno_assert( res != -1, "ioctl(I2C_RDWR)" );
    }

    void i2c_helper::ioctl( i2c_smbus_ioctl_data *data )
    {
        int res = ::ioctl( fd_, I2C_SMBUS, data );
        errno_error::errno_assert( res != -1, "ioctl(I2C_SMBUS)" );
    }

    size_t i2c_helper::write(const void *data, size_t length)
    {
        ssize_t res = ::write( fd_, data, length );
        errno_error::errno_assert( -1 != res, "write" );
        return static_cast<size_t>(res);
    }

    size_t i2c_helper::read(void *data, size_t length)
    {
        ssize_t res = ::read( fd_, data, length );
        errno_error::errno_assert( -1 != res, "read" );
        return static_cast<size_t>(res);
    }

    namespace i2c {

        bool available( unsigned bus_id )
        {
            return available_sep( bus_id ) != '\0';
        }
    }
}}
