#ifndef FR_I2C_HELPER_H
#define FR_I2C_HELPER_H

#include <linux/i2c-dev.h>
#include <linux/i2c.h>
#include <unistd.h>

namespace fr { namespace agent {

    namespace i2c {
        bool available( unsigned bus_id );
    }

    class i2c_helper {

        int fd_;

    public:

        i2c_helper( unsigned bus_id );
        ~i2c_helper(  );

        void ioctl( int code, unsigned long data );
        void ioctl_funcs( unsigned long *funcs );
        void ioctl( i2c_rdwr_ioctl_data *data );
        void ioctl( i2c_smbus_ioctl_data *data );

        size_t write( const void *data, size_t length );
        size_t read(        void *data, size_t length );

        int handle( ) const { return fd_; }
    };

}}

#endif // I2CHELPER_H
