
#include <sstream>
#include <iostream>

#include <stdlib.h>
#include <fcntl.h>

#include "gpio-helper.h"

#include "boost/filesystem.hpp"
#include "boost/system/error_code.hpp"

#include "errno-check.h"

namespace fr { namespace agent {

    namespace {

        namespace vcomm = vtrc::common;
        namespace bsys  = boost::system;

        const std::string direct_index[ ] = {
            std::string( "in\n" ), std::string( "out\n" )
        };

        const std::string edge_index[ ] = {
            std::string( "none\n" ),    std::string( "rising\n" ),
            std::string( "falling\n" ), std::string( "both\n" )
        };

        namespace bfs = boost::filesystem;

        const std::string gpio_sysfs_path   ( "/sys/class/gpio"          );
        const std::string gpio_export_path  ( "/sys/class/gpio/export"   );
        const std::string gpio_unexport_path( "/sys/class/gpio/unexport" );

        const std::string value_name(      "value" );
        const std::string direction_name(  "direction" );
        const std::string edge_name(       "edge" );
        const std::string active_low_name( "active_low" );

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

        std::string value_path_string( unsigned id )
        {
            std::ostringstream oss;
            oss << make_gpio_path( id ) << "/" << value_name;
            return oss.str( );
        }

        void write_to_file( const std::string &path,
                            const char *data, size_t length )
        {
            file_keeper fd( open( path.c_str( ), O_WRONLY | O_SYNC ) );
            errno_error::errno_assert( fd.hdl( ) != -1, "open" );

            int res = write( fd.hdl( ), data, length );
            errno_error::errno_assert( res != -1, "write" );
        }

        bool try_open_file( const std::string &path, int flags = O_RDONLY )
        {
            file_keeper fd( open( path.c_str( ), flags ) );
            return fd.hdl( ) != -1;
        }

        std::string read_from_file( int fd )
        {
            static const size_t buffer_length = 32;

            std::vector<char> buf( buffer_length );

            int res = read( fd, &buf[0], buffer_length );
            lseek( fd, 0, SEEK_SET );

            errno_error::errno_assert( res != -1, "read" );

            return std::string( &buf[0] );
        }

        std::string read_from_file( const std::string &path )
        {
            file_keeper fd( open( path.c_str( ), O_RDONLY ) );

            errno_error::errno_assert( fd.hdl( ) != -1, "open" );

            return read_from_file( fd.hdl( ) );
        }

        bool check_name( const std::string &test, const std::string &templ )
        {
            size_t max = test.size( ) > templ.size( )
                       ? templ.size( ) : test.size( );
            return (memcmp( test.c_str( ), templ.c_str( ), max ) == 0);
        }

    }

    namespace gpio {
        bool available( )
        {
            bsys::error_code ec;
            return bfs::exists( gpio_sysfs_path, ec );
        }
    }

    struct gpio_helper::impl {

        unsigned     id_;
        std::string  path_;
        std::string  value_path_;
        mutable bool own_export_;
        int          value_fd_;

        impl( unsigned id )
            :id_(id)
            ,path_(make_gpio_path(id_))
            ,value_path_(value_path_string(id_))
            ,own_export_(false)
            ,value_fd_(-1)
        { }

        void exp( ) const
        {
            std::string id_lit(id_to_string(id_));

            write_to_file( gpio_export_path,
                           id_lit.c_str( ), id_lit.size( ) + 1 );
            own_export_ = true;
        }

        void unexp( ) const
        {
            std::string id_lit(id_to_string(id_));
            write_to_file( gpio_unexport_path,
                           id_lit.c_str( ), id_lit.size( ) + 1 );
            own_export_ = false;
        }

        ~impl( ) try {
            if( -1 != value_fd_ ) {
                close( value_fd_ );
            }
        } catch ( ... ) {
            ;;;
        }

    };

    gpio_helper::gpio_helper( unsigned id )
        :impl_(new impl(id))
    { }

    gpio_helper::~gpio_helper( )
    {
        delete impl_;
    }

    bool gpio_helper::exists( ) const
    {
        return bfs::exists( impl_->path_ );
    }

    unsigned gpio_helper::id( ) const
    {
        return impl_->id_;
    }

    void gpio_helper::exp( ) const
    {
        impl_->exp( );
    }

    void gpio_helper::unexp( ) const
    {
        impl_->unexp( );
    }

    gpio::direction_type gpio_helper::direction( ) const
    {
        std::ostringstream oss;

        oss << impl_->path_ << "/" << direction_name;

        std::string pos = read_from_file( oss.str( ) );

        if( check_name(pos, direct_index[gpio::DIRECT_IN]) ) {
            return gpio::DIRECT_IN;
        } else if( check_name(pos, direct_index[gpio::DIRECT_OUT]) ) {
            return gpio::DIRECT_OUT;
        } else {
            vcomm::throw_system_error( EINVAL, pos.c_str( ) );
        }
        return gpio::DIRECT_OUT;
    }

    void  gpio_helper::set_direction( gpio::direction_type val ) const
    {
        std::ostringstream oss;
        oss << impl_->path_ << "/" << direction_name;

        write_to_file( oss.str( ),
                       direct_index[val].c_str( ),
                       direct_index[val].size( ) + 1 );
    }

    bool gpio_helper::edge_supported( ) const
    {
        std::ostringstream oss;
        oss << impl_->path_ << "/" << edge_name;
        return try_open_file( oss.str( ) );
    }

    gpio::edge_type gpio_helper::edge( ) const
    {
        std::ostringstream oss;
        oss << impl_->path_ << "/" << edge_name;

        std::string pos;
        try {
            pos.assign( read_from_file( oss.str( ) ) );
        } catch( ... ) {
            return gpio::EDGE_NONE;
        }

        if( check_name(pos, edge_index[gpio::EDGE_NONE]) ) {
            return gpio::EDGE_NONE;
        } else if( check_name(pos, edge_index[gpio::EDGE_RISING]) ) {
            return gpio::EDGE_RISING;
        } else if( check_name(pos, edge_index[gpio::EDGE_FALLING]) ) {
            return gpio::EDGE_FALLING;
        } else if( check_name(pos, edge_index[gpio::EDGE_BOTH]) ) {
            return gpio::EDGE_BOTH;
        }
        return gpio::EDGE_NONE;
    }

    void  gpio_helper::set_edge( gpio::edge_type val ) const
    {
        std::ostringstream oss;
        oss << impl_->path_ << "/" << edge_name;
        write_to_file( oss.str( ),
                       edge_index[val].c_str( ),
                       edge_index[val].size( ) + 1 );
    }

    unsigned gpio_helper::value( ) const
    {
        std::string pos = read_from_file( impl_->value_path_ );
        assert( pos[0] == '0' || pos[0] == '1' );
        return pos[0] - '0';
    }

    unsigned gpio_helper::active_low( ) const
    {
        std::ostringstream oss;
        oss << impl_->path_ << "/" << active_low_name;

        std::string pos = read_from_file( oss.str( ) );
        assert( pos[0] == '0' || pos[0] == '1' );
        return pos[0] - '0';
    }

    unsigned gpio_helper::value_by_fd( int fd )
    {
        std::string pos = read_from_file( fd );
        assert( pos[0] == '0' || pos[0] == '1' );
        return pos[0] - '0';
    }

    void gpio_helper::set_value( unsigned val ) const
    {
        char data[2] = {0};
        data[0] = char(val + '0');
        write_to_file( impl_->value_path_, data, 2 );
    }

    void gpio_helper::set_active_low( unsigned val ) const
    {
        char data[2] = {0};
        data[0] = char(val + '0');
        std::ostringstream oss;
        oss << impl_->path_ << "/" << active_low_name;

        write_to_file( oss.str( ), data, 2 );
    }

    int gpio_helper::value_fd( ) const
    {
        if( impl_->value_fd_ == -1  ) {
            int res = open( impl_->value_path_.c_str( ),
                                                      O_RDONLY | O_NONBLOCK );

            if( -1 == res ) {
                vcomm::throw_system_error( errno, "open" );
            }
            impl_->value_fd_ = res;
        }
        return impl_->value_fd_;
    }

    bool gpio_helper::value_opened( ) const
    {
        return impl_->value_fd_ != -1;
    }

}}

