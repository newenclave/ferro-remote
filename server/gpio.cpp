
#include <sstream>
#include <stdlib.h>
#include <fcntl.h>

#include "gpio.h"

#include "boost/filesystem.hpp"

#include "errno-check.h"

namespace fr { namespace server {

    namespace {

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
            int res = write( res, data, length );

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
            int res = read( res, buf, 256 );
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
                       impl_->id_lit_.c_str( ), impl_->id_lit_.size( ) );
    }

    void gpio_inst::unexp( ) const
    {
        write_to_file( gpio_unexport_path,
                       impl_->id_lit_.c_str( ), impl_->id_lit_.size( ) );

    }

    gpio::direction_type gpio_inst::direction( ) const
    {

    }

    void  gpio_inst::set_direction( gpio::direction_type val ) const
    {

    }

    gpio::edge_type gpio_inst::edge( ) const
    {

    }

    void  gpio_inst::set_edge( gpio::edge_type val ) const
    {

    }

    unsigned gpio_inst::value( ) const
    {

    }

    void gpio_inst::set_value( unsigned val ) const
    {

    }

}}

