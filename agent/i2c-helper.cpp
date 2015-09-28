#include "i2c-helper.h"

#include <iostream>
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
            if( ::stat( bus2path(bus_id, '-').c_str( ), &ss ) == 0 ) {
                return '-';
            } else if( ::stat( bus2path(bus_id, '/').c_str( ), &ss ) == 0 ) {
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

        int smbus_read_write( int fd, uint8_t rw, uint8_t cmd,
                              uint32_t len, i2c_smbus_data *data )
        {
            i2c_smbus_ioctl_data params;
            params.read_write   = rw;
            params.command      = cmd;
            params.data         = data;
            params.size         = len;
            return ioctl( fd, I2C_SMBUS, &params );
        }

    }

    i2c_helper::i2c_helper( unsigned bus_id )
        :fd_(open_bus(bus_id))
        ,address_(-1)
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

    void i2c_helper::ioctl_funcs( unsigned long *funcs )
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

    void i2c_helper::smbus_read( uint8_t command,
                                 i2c_smbus_data *data, uint32_t len )
    {
        int res = smbus_read_write( fd_, I2C_SMBUS_READ, command, len, data );
        errno_error::errno_assert( -1 != res, "smbus_read" );
    }

    void i2c_helper::smbus_write( uint8_t command,
                                  i2c_smbus_data *data, uint32_t len )
    {
        int res = smbus_read_write( fd_, I2C_SMBUS_WRITE, command, len, data );
        errno_error::errno_assert( -1 != res, "smbus_write" );
    }

    void i2c_helper::smbus_write_quick( uint8_t value )
    {
        int res = smbus_read_write( fd_, value, 0, I2C_SMBUS_QUICK, NULL );
        errno_error::errno_assert( -1 != res, "smbus_write_quick" );
    }

    uint8_t i2c_helper::smbus_read_byte( )
    {
        i2c_smbus_data data;
        smbus_read( 0, &data, I2C_SMBUS_BYTE );
        return data.byte;
    }

    void i2c_helper::smbus_write_byte( uint8_t value )
    {
        smbus_write( value, NULL, I2C_SMBUS_BYTE );
    }

    uint8_t i2c_helper::smbus_read_byte_data( uint8_t cmd )
    {
        i2c_smbus_data data;
        smbus_read( cmd, &data, I2C_SMBUS_BYTE_DATA );
        return data.byte;
    }

    void i2c_helper::smbus_write_byte_data( uint8_t cmd, uint8_t value )
    {
        i2c_smbus_data data;
        data.byte = value;
        smbus_write( cmd, &data, I2C_SMBUS_BYTE_DATA );
    }

    uint16_t i2c_helper::smbus_read_word_data( uint8_t cmd )
    {
        i2c_smbus_data data;
        smbus_read( cmd, &data, I2C_SMBUS_WORD_DATA );
        return data.word;
    }

    void i2c_helper::smbus_write_word_data( uint8_t cmd, uint16_t value )
    {
        i2c_smbus_data data;
        data.word = value;
        smbus_write( cmd, &data, I2C_SMBUS_WORD_DATA );
    }

    uint16_t i2c_helper::smbus_process_call( uint8_t cmd, uint16_t value )
    {
        i2c_smbus_data data;
        data.word = value;
        smbus_write( cmd, &data, I2C_SMBUS_PROC_CALL );
        return data.word;
    }

    std::string i2c_helper::smbus_block_process_call( uint8_t cmd,
                                                      const std::string &data )
    {
        i2c_smbus_data ldata;
        uint8_t len = data.size( ) > I2C_SMBUS_BLOCK_MAX
                                   ? I2C_SMBUS_BLOCK_MAX
                                   : static_cast<uint8_t>( data.size( ) );
        ldata.block[0] = len;
        memcpy( &ldata.block[1], len ? &data[0] : "", len );
        smbus_write( cmd, &ldata, I2C_SMBUS_BLOCK_PROC_CALL );
        return std::string( &ldata.block[1], &ldata.block[1] + ldata.block[0] );
    }

    uint8_t i2c_helper::smbus_block_process_call( uint8_t cmd,
                                                  uint8_t *data,
                                                  uint8_t len )
    {
        i2c_smbus_data ldata;

        len = len > I2C_SMBUS_BLOCK_MAX ? I2C_SMBUS_BLOCK_MAX : len;

        memcpy( &ldata.block[1], data, len );

        smbus_write( cmd, &ldata, I2C_SMBUS_BLOCK_PROC_CALL );

        memcpy( data, &ldata.block[1], ldata.block[0] );

        return ldata.block[0];
    }

    std::string i2c_helper::smbus_read_block_data( uint8_t cmd )
    {
        i2c_smbus_data data;
        smbus_read( cmd, &data, I2C_SMBUS_BLOCK_DATA );
        return std::string( &data.block[1], &data.block[1] + data.block[0] );
    }

    uint8_t i2c_helper::smbus_read_block_data( uint8_t cmd, uint8_t *data )
    {
        i2c_smbus_data ldata;
        smbus_read( cmd, &ldata, I2C_SMBUS_BLOCK_DATA );
        memcpy( data, &ldata.block[1], ldata.block[0] );
        return ldata.block[0];
    }

    std::string i2c_helper::smbus_read_block_broken( uint8_t cmd, uint8_t len )
    {
        i2c_smbus_data data;
        len = len > I2C_SMBUS_BLOCK_MAX ? I2C_SMBUS_BLOCK_MAX : len;
        data.block[0] = len;
        smbus_read( cmd, &data,
                    len == I2C_SMBUS_BLOCK_MAX
                         ? I2C_SMBUS_I2C_BLOCK_BROKEN
                         : I2C_SMBUS_BLOCK_DATA );
        return std::string( &data.block[1], &data.block[1] + data.block[0] );
    }

    uint8_t i2c_helper::smbus_read_block_broken( uint8_t cmd,
                                                 uint8_t *data, uint8_t length )
    {
        i2c_smbus_data ldata;
        length = length > I2C_SMBUS_BLOCK_MAX ? I2C_SMBUS_BLOCK_MAX : length;
        ldata.block[0] = length;
        smbus_read( cmd, &ldata,
                    length == I2C_SMBUS_BLOCK_MAX
                            ? I2C_SMBUS_I2C_BLOCK_BROKEN
                            : I2C_SMBUS_BLOCK_DATA );
        memcpy( data, &ldata.block[1], ldata.block[0] );
        return ldata.block[0];
    }

    void i2c_helper::smbus_write_block_data( uint8_t cmd,
                                             const std::string &data )
    {
        uint8_t len = static_cast<uint8_t>( data.size( ) );
        smbus_write_block_data( cmd,
               reinterpret_cast<const uint8_t *>(len ? &data[0] : ""), len );
    }

    void i2c_helper::smbus_write_block_data( uint8_t cmd,
                                             const uint8_t *data,
                                             uint8_t length)
    {
        i2c_smbus_data ldata;
        uint8_t len = length > I2C_SMBUS_BLOCK_MAX
                             ? I2C_SMBUS_BLOCK_MAX : length;
        ldata.block[0] = len;
        memcpy( &ldata.block[1], data, len );
        smbus_write( cmd, &ldata, I2C_SMBUS_BLOCK_DATA );
    }

    void i2c_helper::smbus_write_block_broken( uint8_t cmd,
                                               const std::string &data )
    {
        uint8_t len = static_cast<uint8_t>( data.size( ) );
        smbus_write_block_broken( cmd,
               reinterpret_cast<const uint8_t *>(len ? &data[0] : ""), len );
    }

    void i2c_helper::smbus_write_block_broken( uint8_t cmd,
                                               const uint8_t *data,
                                               uint8_t length )
    {
        i2c_smbus_data ldata;
        uint8_t len = length > I2C_SMBUS_BLOCK_MAX
                             ? I2C_SMBUS_BLOCK_MAX : length;
        ldata.block[0] = len;
        memcpy( &ldata.block[1], data, len );
        smbus_write( cmd, &ldata, I2C_SMBUS_I2C_BLOCK_BROKEN );
    }

    /// write/read as file
    size_t i2c_helper::write( const void *data, size_t length )
    {
        ssize_t res = ::write( fd_, data, length );
        errno_error::errno_assert( -1 != res, "write" );
        return static_cast<size_t>(res);
    }

    size_t i2c_helper::read( void *data, size_t length )
    {
        ssize_t res = ::read( fd_, data, length );
        errno_error::errno_assert( -1 != res, "read" );
        return static_cast<size_t>(res);
    }

    ssize_t i2c_helper::transfer( void *txbuf, size_t txlen,
                                  void *rxbuf, size_t rxlen )
    {
        auto res = transfer_nothrow( txbuf, txlen, rxbuf, rxlen );
        errno_error::errno_assert( -1 != res, "i2c_transfer" );
        return res;
    }

    ssize_t i2c_helper::transfer_nothrow( void *txbuf, size_t txlen,
                                          void *rxbuf, size_t rxlen )
    {
        auto cbuf = static_cast<unsigned char *>(txbuf);

        if( cbuf[0] != address_ ) {
           if( ::ioctl(fd_, I2C_SLAVE, cbuf[0] >> 1) < 0 ) {
               return -1;
           }
           address_ = cbuf[0];
        }

        auto res = ::write( fd_, cbuf+1, txlen-1 );
        if( res == -1 ) {
            return -1;
        }

        if( (rxlen > 0) ) {
            res = ::read( fd_, rxbuf, rxlen );
            if( res == -1 ) {
                return -1;
            }
            return res;
        } else {
            return 0;
        }
    }

    namespace i2c {

        bool available( unsigned bus_id )
        {
            return available_sep( bus_id ) != '\0';
        }
    }
}}
