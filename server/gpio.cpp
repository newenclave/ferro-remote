
#include <sstream>
#include <iostream>

#include <stdlib.h>
#include <fcntl.h>

#include "gpio.h"

#include "boost/filesystem.hpp"

#include "errno-check.h"

namespace fr { namespace server {

    namespace {

        namespace vcomm = vtrc::common;

        const std::string direct_index[ ] = {
            std::string( "in" ), std::string( "out" )
        };

        const std::string edge_index[ ] = {
            std::string( "none" ),    std::string( "rising" ),
            std::string( "falling" ), std::string( "both" )
        };

        namespace bfs = boost::filesystem;

        const std::string gpio_sysfs_path(      "/sys/class/gpio" );
        const std::string gpio_export_path(     "/sys/class/gpio/export" );
        const std::string gpio_unexport_path(   "/sys/class/gpio/unexport" );

        const std::string value_name(     "value" );
        const std::string direction_name( "direction" );
        const std::string edge_name(      "edge" );

        std::string id_to_string( unsigned id )
        {
            std::ostringstream oss;
            oss << id;
            return oss.str( );
        }

        std::string make_gpio_path( unsigned id )
        {
            std::ostringstream oss;
            oss << gpio_sysfs_path << "/" << "gpio" << id;
            return oss.str( );
        }

        void write_to_file( const std::string &path,
                            const char *data, size_t length )
        {
            int fd = open( path.c_str( ), O_WRONLY );
            errno_error::errno_assert( fd != -1, "open" );

            int res = write( fd, data, length );

            if( -1 == res ) {
                close( fd );
                errno_error::throw_error( "write" );
            }
            close( fd );
        }

        std::string read_from_file( const std::string &path )
        {
            char buf[256];
            int fd = open( path.c_str( ), O_RDONLY );
            errno_error::errno_assert( fd != -1, "open" );
            int res = read( fd, buf, 256 );
            if( -1 == res ) {
                close( fd );
                errno_error::throw_error( "read" );
            }
            close( fd );
            return std::string( buf, buf + res );
        }

    }

    struct gpio_inst::impl {

        unsigned    id_;
        std::string id_lit_;
        std::string path_;

        impl( unsigned id )
            :id_(id)
            ,id_lit_(id_to_string(id_))
            ,path_(make_gpio_path(id_))
        { }
    };

    gpio_inst::gpio_inst( unsigned id )
        :impl_(new impl(id))
    { }

    gpio_inst::~gpio_inst( )
    {
        delete impl_;
    }

    bool gpio_inst::exists( ) const
    {
        return bfs::exists( impl_->path_ );
    }

    unsigned gpio_inst::id( ) const
    {
        impl_->id_;
    }

    void gpio_inst::exp( ) const
    {
        write_to_file( gpio_export_path,
                       impl_->id_lit_.c_str( ), impl_->id_lit_.size( ) + 1 );
    }

    void gpio_inst::unexp( ) const
    {
        write_to_file( gpio_unexport_path,
                       impl_->id_lit_.c_str( ), impl_->id_lit_.size( ) + 1 );

    }

    gpio::direction_type gpio_inst::direction( ) const
    {
        std::ostringstream oss;

        oss << impl_->path_ << "/" << direction_name;

        std::string pos = read_from_file( oss.str( ) );
        if( pos == direct_index[gpio::DIRECT_IN] ) {
            return gpio::DIRECT_IN;
        } else if( pos == direct_index[gpio::DIRECT_OUT] ) {
            return gpio::DIRECT_OUT;
        } else {
            vcomm::throw_system_error( EINVAL, pos.c_str( ) );
        }
    }

    void  gpio_inst::set_direction( gpio::direction_type val ) const
    {
        std::ostringstream oss;
        oss << impl_->path_ << "/" << direction_name;

        write_to_file( oss.str( ),
                       direct_index[val].c_str( ), direct_index[val].size( ) );
    }

    gpio::edge_type gpio_inst::edge( ) const
    {
        std::ostringstream oss;
        oss << impl_->path_ << "/" << edge_name;

        std::string pos = read_from_file( oss.str( ) );
        if( pos == edge_index[gpio::EDGE_NONE] ) {
            return gpio::EDGE_NONE;
        } else if( pos == edge_index[gpio::EDGE_RISING] ) {
            return gpio::EDGE_RISING;
        } else if( pos == edge_index[gpio::EDGE_FALLING] ) {
            return gpio::EDGE_FALLING;
        } else if( pos == edge_index[gpio::EDGE_BOTH] ) {
            return gpio::EDGE_BOTH;
        } else {
            vcomm::throw_system_error( EINVAL, pos.c_str( ) );
        }
    }

    void  gpio_inst::set_edge( gpio::edge_type val ) const
    {
        std::ostringstream oss;
        oss << impl_->path_ << "/" << edge_name;
        write_to_file( oss.str( ),
                       edge_index[val].c_str( ), edge_index[val].size( ) );
    }

    unsigned gpio_inst::value( ) const
    {
        std::ostringstream oss;
        oss << impl_->path_ << "/" << value_name;

        std::string pos = read_from_file( oss.str( ) );
        assert( pos[0] == '0' || pos[0] == '1' );
        return pos[0] - '0';
    }

    void gpio_inst::set_value( unsigned val ) const
    {
        char data[2] = {0};
        data[0] = char(val + '0');
        std::ostringstream oss;
        oss << impl_->path_ << "/" << value_name;
        write_to_file( oss.str( ), data, 2 );
    }

}}

