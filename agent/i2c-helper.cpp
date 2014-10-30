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

        std::string bus2path( unsigned bus_id )
        {
            if( bus_id < 10 ) {
                std::string res = "/dev/i2c-?";
                res[9] = (char)(bus_id + '0');
                return res;
            } else {
                static const std::string prefix_path("/dev/i2c-");
                std::ostringstream oss;
                oss << prefix_path << bus_id;
                return oss.str( );
            }
        }

        int open_bus( unsigned bus_id )
        {
            std::string bus_path( bus2path( bus_id ) );
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

    namespace i2c {

        bool available( unsigned bus_id )
        {
            struct stat ss = { 0 };
            int res = stat( bus2path(bus_id).c_str( ), &ss );
            return res != -1;
        }
    }
}}
