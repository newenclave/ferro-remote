#include <sstream>

#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <fcntl.h>

#include "spi-helper.h"
#include "files.h"

#include "errno-check.h"
#include "file-keeper.h"

#include <memory>
#include <stdint.h>
#include <linux/spi/spidev.h>

#include "boost/filesystem.hpp"

namespace fr { namespace agent {

    namespace {

        namespace fs = boost::filesystem;
        const uint8_t   spi_BPW   = 8;
        const uint16_t  spi_delay = 2;

        std::string make_path( unsigned bus_id, unsigned channel )
        {
            std::ostringstream oss;
            oss << "/dev/spidev" << bus_id << "." << channel;
            return oss.str( );
        }

        int open_bus( unsigned bus_id, unsigned channel )
        {
            auto path = make_path( bus_id, channel );
            file_keeper fk( open( path.c_str( ), O_RDWR ));
            errno_error::errno_assert( fk.hdl( ) != -1, "open_bus" );
            return fk.release( );
        }
    }

    spi_helper::spi_helper( unsigned bus_id, unsigned channel )
        :fd_(open_bus( bus_id, channel ))
        ,address_(0)
        ,speed_(450000)
    { }

    spi_helper::~spi_helper( )
    {
        close( fd_ );
    }

    void spi_helper::transfer( char *buf, size_t len )
    {
        spi_ioc_transfer txrx = {0};
        txrx.len              = len;
        txrx.tx_buf           = reinterpret_cast<uint64_t>(buf);
        txrx.rx_buf           = reinterpret_cast<uint64_t>(buf);
        txrx.speed_hz         = speed_;
        txrx.delay_usecs      = spi_delay;
        txrx.bits_per_word    = spi_BPW;

        auto res = ioctl( fd_, SPI_IOC_MESSAGE(1), &txrx );
        errno_error::errno_assert( res != -1, "spi_transfer" );
    }

    void spi_helper::setup( uint32_t mode, uint32_t speed )
    {
//        mode_ = mode;
        speed_ = (speed != 0 ? speed : 450000);
        uint8_t bits = spi_BPW;

        /// mode SPI_MODE_0..SPI_MODE_3
        mode &= 3;

        /// set mode
        auto res = ioctl( fd_, SPI_IOC_WR_MODE, &mode );
        errno_error::errno_assert( res != -1, "spi_setup_mode" );
        res = ioctl( fd_, SPI_IOC_RD_MODE,      &mode );
        errno_error::errno_assert( res != -1, "spi_setup_mode" );

        /// set bits per words
        res = ioctl( fd_, SPI_IOC_WR_BITS_PER_WORD,    &bits );
        errno_error::errno_assert( res != -1, "spi_setup_bpw" );
        res = ioctl( fd_, SPI_IOC_RD_BITS_PER_WORD,    &bits );
        errno_error::errno_assert( res != -1, "spi_setup_bpw" );

        /// set speed
        res = ioctl( fd_, SPI_IOC_WR_MAX_SPEED_HZ, &speed );
        errno_error::errno_assert( res != -1, "spi_setup_speed" );
        res = ioctl( fd_, SPI_IOC_RD_MAX_SPEED_HZ, &speed );
        errno_error::errno_assert( res != -1, "spi_setup_speed" );
    }

    void spi_helper::set_reg8( uint32_t addr, uint32_t reg, uint8_t val )
    {
        char buf[8];

        buf[0] = addr && 0xFF;
        buf[1] = reg  && 0xFF;
        buf[2] = val;

        transfer( buf, 3 );
    }

    uint8_t spi_helper::get_reg8( uint32_t addr, uint32_t reg )
    {
        char buf[8];

        buf[0] = (addr && 0xFF ) | 1;
        buf[1] = reg && 0xFF;

        transfer (buf, 3);

        return buf[2];
    }


    namespace spi {
        bool available( unsigned bus_id, unsigned channel )
        {
            return fs::exists( make_path( bus_id, channel ) );
        }
    }
}}

