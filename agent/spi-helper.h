#ifndef SPIHELPER_H
#define SPIHELPER_H

#include <stdlib.h>
#include <stdint.h>

namespace fr { namespace agent {

    namespace spi {
        bool available( unsigned bus_id, unsigned channel );
    }

    class spi_helper {

        int      fd_;
        uint32_t address_;
        uint32_t speed_;

    public:

        spi_helper( const spi_helper & ) = delete;
        spi_helper &operator = ( const spi_helper & ) = delete;

        spi_helper( unsigned bus_id, unsigned channel );
        ~spi_helper( );

        void transfer( void *buf, size_t len );
        int  transfer_nothrow( void *buf, size_t len );

        void setup( uint32_t mode, uint32_t speed );

        void     set_reg8 ( uint32_t addr, uint32_t reg, uint8_t  value );
        void     set_reg8 (                uint32_t reg, uint8_t  value );

        void     set_reg16( uint32_t addr, uint32_t reg, uint16_t value );
        void     set_reg16(                uint32_t reg, uint16_t value );

        void     set_reg32( uint32_t addr, uint32_t reg, uint32_t value );
        void     set_reg32(                uint32_t reg, uint32_t value );

        void     set_reg64( uint32_t addr, uint32_t reg, uint64_t value );
        void     set_reg64(                uint32_t reg, uint64_t value );

        uint8_t  get_reg8 ( uint32_t addr, uint32_t reg );
        uint8_t  get_reg8 (                uint32_t reg );
        uint16_t get_reg16( uint32_t addr, uint32_t reg );
        uint16_t get_reg16(                uint32_t reg );
        uint32_t get_reg32( uint32_t addr, uint32_t reg );
        uint32_t get_reg32(                uint32_t reg );
        uint64_t get_reg64( uint32_t addr, uint32_t reg );
        uint64_t get_reg64(                uint32_t reg );

        void set_address( uint32_t val ) noexcept { address_ = val; }
        uint32_t address( ) const noexcept { return address_; }
        int handle( ) const noexcept { return fd_; }
    };

}}

#endif // SPIHELPER_H
