#ifndef FR_I2C_HELPER_H
#define FR_I2C_HELPER_H

#include <string>

#include <linux/i2c-dev.h>
#include <linux/i2c.h>

#include <unistd.h>

#include <stdint.h>

namespace fr { namespace agent {

    namespace i2c {
        bool available( unsigned bus_id );
    }

    class i2c_helper {

        int fd_;
        int address_;
    public:

        i2c_helper( const i2c_helper & ) = delete;
        i2c_helper &operator = ( const i2c_helper & ) = delete;

        i2c_helper( unsigned bus_id );
        ~i2c_helper(  );

        void ioctl( int code, unsigned long data );
        void ioctl_funcs( unsigned long *funcs );

        void ioctl( i2c_rdwr_ioctl_data *data );
        void ioctl( i2c_smbus_ioctl_data *data );

        void smbus_read( uint8_t command, i2c_smbus_data *data, uint32_t len );
        void smbus_write( uint8_t command, i2c_smbus_data *data, uint32_t len );

        void smbus_write_quick( uint8_t value );

        uint8_t smbus_read_byte(  );
        void    smbus_write_byte( uint8_t value );

        uint8_t smbus_read_byte_data(  uint8_t cmd );
        void    smbus_write_byte_data( uint8_t cmd, uint8_t value );

        uint16_t smbus_read_word_data(  uint8_t cmd );
        void     smbus_write_word_data( uint8_t cmd, uint16_t value );

        uint16_t smbus_process_call( uint8_t cmd, uint16_t value );

        std::string smbus_block_process_call( uint8_t cmd,
                                              const std::string &data );
        uint8_t smbus_block_process_call( uint8_t cmd,
                                          uint8_t *data, uint8_t len );

        std::string smbus_read_block_data( uint8_t cmd );
        uint8_t smbus_read_block_data( uint8_t cmd, uint8_t *data );

        std::string smbus_read_block_broken( uint8_t cmd, uint8_t length );
        uint8_t smbus_read_block_broken( uint8_t cmd,
                                       uint8_t *data, uint8_t length );

        void smbus_write_block_data( uint8_t cmd, const std::string &data );
        void smbus_write_block_data( uint8_t cmd,
                                     const uint8_t *data, uint8_t length );

        void smbus_write_block_broken( uint8_t cmd, const std::string &data );
        void smbus_write_block_broken( uint8_t cmd,
                                       const uint8_t *data, uint8_t length );

        std::string transfer( const void *txbuf, size_t txlen, size_t rxlen );
        std::string transfer_nothrow( const void *txbuf, size_t txlen,
                                      size_t rxlen );
        ssize_t transfer_nothrow( const void *txbuf, size_t txlen,
                                  void *rxbuf, size_t rxlen );

        void set_address( unsigned long addr, bool force = false );
        /// read and write as file
        size_t write( const void *data, size_t length );
        size_t read(        void *data, size_t length );

        int handle( ) const noexcept { return fd_; }
    };

}}

#endif // I2CHELPER_H
