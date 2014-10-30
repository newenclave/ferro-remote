#ifndef FR_I2C_HELPER_H
#define FR_I2C_HELPER_H

#include <linux/i2c-dev.h>
#include <linux/i2c.h>

namespace fr { namespace agent {

    namespace i2c {
        bool available( unsigned bus_id );
    }

    class i2c_helper {

        int fd_;

    public:

        i2c_helper( unsigned bus_id );
        ~i2c_helper(  );

    };

}}

#endif // I2CHELPER_H
