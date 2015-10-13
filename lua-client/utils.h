#ifndef FR_LUA_UTILS_H
#define FR_LUA_UTILS_H

#include <stdlib.h>

#ifdef _WIN32
#include "boost/asio/windows/stream_handle.hpp"
#include <tchar.h>
#else
#include "boost/asio/posix/stream_descriptor.hpp"
#endif

#include "fr-lua.h"

namespace boost { namespace asio {
    class io_service;
}}

namespace fr { namespace lua {
    class state;
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

    inline bool is_number( const objects::base *o )
    {
        return o->type_id( ) == objects::base::TYPE_NUMBER;
    }

    inline bool is_string( const objects::base *o )
    {
        return o->type_id( ) == objects::base::TYPE_STRING;
    }

    inline bool is_table( const objects::base *o )
    {
        return o->type_id( ) == objects::base::TYPE_TABLE
            ;
    }

    template <typename T>
    objects::base_sptr vector2table( const T &data )
    {
        auto t(std::make_shared<objects::table>( ));

        for( auto &d: data ) {
            t->add( objects::new_object( d ) );
        }

        return t;
    }

    template <typename T>
    T table2pair_vector( const objects::base *tobj )
    {
        T res;
        using value_type = typename T::value_type;

        for( size_t i=0; i<tobj->count( ); i++ ) {
            auto p(tobj->at(i));
            auto reg(p->at(0));
            auto val(p->at(1));
            if( is_number( reg ) ) {
                if( is_number( val ) ) {
                    res.push_back( value_type( reg->inum( ), val->inum( ) ) );
                } else if( is_table( val ) ) {
                    auto t = table2pair_vector<T>( val );
                    res.insert( res.end( ), t.begin( ), t.end( ) );
                }
            }
        }
        return std::move(res);
    }

    template <typename T>
    T table2pair_vector( lua::state &ls, int id )
    {
        auto t = ls.get_type( id );
        if( t == objects::base::TYPE_TABLE ) {
            auto tobj = ls.get_object( id );
            return table2pair_vector<T>( tobj.get( ) );
        }
        return T( );
    }

    template <typename T>
    T table2vector( const objects::base *tobj )
    {
        T res;
        using value_type = typename T::value_type;

        for( size_t i=0; i<tobj->count( ); i++ ) {
            auto p(tobj->at(i));
            auto reg(p->at(0));
            auto val(p->at(1));
            if( is_number( reg ) ) {
                if( is_number( val ) ) {
                    res.push_back( static_cast<value_type>(val->inum( )) );
                } else if( is_table( val ) ) {
                    auto t = table2vector<T>( val );
                    res.insert( res.end( ), t.begin( ), t.end( ) );
                }
            }
        }
        return std::move(res);
    }

    template <typename T>
    T table2vector( lua::state &ls, int id )
    {
        auto t = ls.get_type( id );
        if( t == objects::base::TYPE_TABLE ) {
            auto tobj = ls.get_object( id );
            return table2vector<T>( tobj.get( ) );
        }
        return T( );
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
            hdl_.assign( STDOUT_FILENO );
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
