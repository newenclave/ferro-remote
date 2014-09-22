
#include <sstream>
#include <stdlib.h>
#include <fcntl.h>

#include "gpio.h"

#include "boost/filesystem.hpp"

#include "errno-check.h"

namespace fr { namespace server {

    namespace {

        namespace bfs = boost::filesystem;

        const std::string gpio_sysfs_path(      "/sys/class/gpio" );
        const std::string gpio_export_path(     "/sys/class/gpio/export" );
        const std::string gpio_unexport_path(   "/sys/class/gpio/unexport" );

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

        void write_to_file( const std::string &path, const std::string &data )
        {
            int res = open( path.c_str( ), O_WRONLY );
            errno_error::errno_assert( res != -1, "open" );
            res = write( res, data.size( ) ? &data[0] : "", data.size( ) );
            if( -1 == res ) {
                close( res );
                errno_error::throw_error( "write" );
            }
            close( res );
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
        write_to_file( gpio_export_path, impl_->id_lit_ );
    }

    void gpio_inst::unexp( ) const
    {
        write_to_file( gpio_unexport_path, impl_->id_lit_ );
    }

}}

