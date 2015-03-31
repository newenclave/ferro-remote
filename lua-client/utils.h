#ifndef FR_LUA_UTILS_H
#define FR_LUA_UTILS_H

#include <stdlib.h>

#ifdef _WIN32
#include "boost/asio/windows/stream_handle.hpp"
#include <tchar.h>
#else
#include "boost/asio/posix/stream_descriptor.hpp"
#endif

namespace boost { namespace asio {
    class io_service;
}}

namespace fr { namespace lua { namespace utils {

    typedef void * handle;

    template <typename T>
    handle to_handle( T value  )
    {
        return reinterpret_cast<handle>(value);
    }

    template <typename T>
    T from_handle( const handle value  )
    {
        return reinterpret_cast<T>(value);
    }

    class console_handle {
#ifdef _WIN32
        boost::asio::windows::stream_handle hdl_;
#else
        boost::asio::posix::stream_descriptor hdl_;
#endif

        void assign_handle( )
        {
#ifdef _WIN32
            HANDLE hdl = CreateFile( _T("CONIN$"),
                            GENERIC_READ | GENERIC_WRITE,
                            FILE_SHARE_READ | FILE_SHARE_WRITE,
                            NULL,
                            OPEN_EXISTING,
                            FILE_FLAG_OVERLAPPED,
                            NULL );
            hdl_.assign( hdl );
#else
            hdl_.assign( STDIN_FILENO );
#endif
        }

    public:

        console_handle( boost::asio::io_service &ios )
            :hdl_(ios)
        {
            assign_handle( );
        }

#ifdef _WIN32
        const boost::asio::windows::stream_handle &hdl( ) const
        {
            return hdl_;
        }

        boost::asio::windows::stream_handle &hdl( )
        {
            return hdl_;
        }
#else
        const boost::asio::posix::stream_descriptor &hdl( ) const
        {
            return hdl_;
        }

        boost::asio::posix::stream_descriptor &hdl( )
        {
            return hdl_;
        }
#endif

    };

    namespace ccout {
        std::ostream &light ( std::ostream &s );
        std::ostream &red   ( std::ostream &s );
        std::ostream &green ( std::ostream &s );
        std::ostream &blue  ( std::ostream &s );
        std::ostream &cyan  ( std::ostream &s );
        std::ostream &yellow( std::ostream &s );
        std::ostream &none  ( std::ostream &s );
    }

}}}

#endif // FR_LUA_UTILS_H
