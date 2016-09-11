#include <sstream>
#include <stdexcept>
#include <stdlib.h>
#include <limits.h>

#include "utils.h"

#ifdef _WIN32
#include <windows.h>
#endif

namespace {

    enum color_value {
         COLOR_NONE     = 0
        ,COLOR_RED      = 1
        ,COLOR_GREEN    = 2
        ,COLOR_BLUE     = 3
        ,COLOR_YELLOW   = 4
        ,COLOR_WHITE    = 5
        ,COLOR_CYAN     = 6
    };

#ifdef _WIN32

    enum colors_enum
    {
        Black       = 0,
        Grey        = FOREGROUND_INTENSITY,
        LightGrey   = FOREGROUND_RED   | FOREGROUND_GREEN
                                       | FOREGROUND_BLUE,
        White       = FOREGROUND_RED   | FOREGROUND_GREEN
                                       | FOREGROUND_BLUE
                                       | FOREGROUND_INTENSITY,
        Blue        = FOREGROUND_BLUE,
        Green       = FOREGROUND_GREEN,
        Cyan        = FOREGROUND_GREEN | FOREGROUND_BLUE,
        Red         = FOREGROUND_RED,
        Purple      = FOREGROUND_RED   | FOREGROUND_BLUE,
        LightBlue   = FOREGROUND_BLUE  | FOREGROUND_INTENSITY,
        LightGreen  = FOREGROUND_GREEN | FOREGROUND_INTENSITY,
        LightCyan   = FOREGROUND_GREEN | FOREGROUND_BLUE
                                       | FOREGROUND_INTENSITY,
        LightRed    = FOREGROUND_RED   | FOREGROUND_INTENSITY,
        LightPurple = FOREGROUND_RED   | FOREGROUND_BLUE
                                       | FOREGROUND_INTENSITY,
        Orange      = FOREGROUND_RED   | FOREGROUND_GREEN,
        Yellow      = FOREGROUND_RED   | FOREGROUND_GREEN
                                       | FOREGROUND_INTENSITY,
    };

    const unsigned color_map[ ] = {
        LightGrey,
        LightRed, LightGreen, LightBlue,
        Yellow, White, Cyan
    };
#else

    static const char * cp_none      = "\033[0m";
    //static const char * cp_none      = "\x1b[0m";
    //static const char * cp_black     = "\x1b[30;1m";
    static const char * cp_red       = "\x1b[31;1m";
    static const char * cp_green     = "\x1b[32;1m";
    static const char * cp_yellow    = "\x1b[33;1m";
    //static const char * cp_orange    = "\x1b[33;1m";
    static const char * cp_blue      = "\x1b[34;1m";
    //static const char * cp_purple    = "\x1b[35;1m";
    static const char * cp_cyan      = "\x1b[36;1m";
    static const char * cp_white     = "\x1b[37;1m";

    const char *color_map[ ] = {
        cp_none,
        cp_red, cp_green, cp_blue,
        cp_yellow, cp_white, cp_cyan
    };
#endif
    const size_t color_count = sizeof(color_map)/sizeof(color_map[0]);

    std::ostream &set_stream_color( std::ostream &s, unsigned color )
    {
        if( color >= color_count ) {
            color = COLOR_NONE;
        }
#ifdef _WIN32
        SetConsoleTextAttribute( GetStdHandle( STD_OUTPUT_HANDLE ),
                                 color_map[color] );
#else
        s << color_map[color];
#endif
        return s;
    }
}

namespace fr { namespace utilities {

    namespace console {
        std::ostream &light ( std::ostream &s )
        {
            return set_stream_color(s, COLOR_WHITE);
        }

        std::ostream &red   ( std::ostream &s )
        {
            return set_stream_color(s, COLOR_RED);
        }

        std::ostream &green ( std::ostream &s )
        {
            return set_stream_color(s, COLOR_GREEN);
        }

        std::ostream &blue  ( std::ostream &s )
        {
            return set_stream_color(s, COLOR_BLUE);
        }

        std::ostream &yellow( std::ostream &s )
        {
            return set_stream_color(s, COLOR_YELLOW);
        }

        std::ostream &none  ( std::ostream &s )
        {
            return set_stream_color(s, COLOR_NONE);
        }

        std::ostream &cyan  ( std::ostream &s )
        {
            return set_stream_color(s, COLOR_CYAN);
        }
    }

    h2b_result bin2hex( void const *bytes, size_t length )
    {

        using result_type = h2b_result;

        if( (NULL == bytes) || (0 == length) ) {
            return h2b_result::fail("nullptr");
        }

        static
            unsigned char hexes_[ ] = {
                '0', '1', '2', '3', '4',
                '5', '6', '7', '8', '9',
                'A', 'B', 'C', 'D', 'E', 'F' };

        struct hi_lo_struct {
            char line[3];
            hi_lo_struct( ) { line[2] = 0; }
            const char * operator( )(const char in_) {
                line[0] = hexes_[(in_ >> 4) & 0x0F];
                line[1] = hexes_[(in_ & 0xF)];
                return line;
            }
        } int_to_hex;

        result_type tmp("asda");

        tmp->reserve( length * 2 + 1);
        char const * c = static_cast<char const *>(bytes);
        for(size_t b(0), e(length); b!=e; ++b) {
            tmp->append(int_to_hex(c[b]));
        }
        return std::move(tmp);
    }

    h2b_result bin2hex( std::string const &input )
    {
        return bin2hex( input.c_str( ), input.size( ) );
    }

    h2b_result hex2bin( std::string const &input )
    {
        using result_type = h2b_result;

        struct {
            unsigned char operator ( ) ( const char in_ ) {
                switch(in_) {
                case '0':   case '1':
                case '2':   case '3':
                case '4':   case '5':
                case '6':   case '7':
                case '8':   case '9':
                    return in_ - '0';
                case 'a':   case 'b':
                case 'c':   case 'd':
                case 'e':   case 'f':
                    return in_ - 'a' + 10;
                case 'A':   case 'B':
                case 'C':   case 'D':
                case 'E':   case 'F':
                    return in_ - 'A' + 10;
                default:
                    return 0xFF;
                }
            }
        } static hex_to_int;

        result_type res;
        std::string tmp;
        tmp.reserve( input.size( ) / 2 + 1 );

        for( auto b = input.begin( ), e = input.end( ); b!=e; ++b ) {

            unsigned char next = 0;
            unsigned char h = hex_to_int( *b );

            if( h == 0xFF ) {
                return h2b_result::fail( "Bad serialized string" );
            }

            next = (h << 4);
            if( ++b == e ) {
                tmp.push_back(next);
                break;
            }

            unsigned char l = hex_to_int(*b);
            next |= (l & 0xF);
            tmp.push_back(next);
        }
        res->swap(tmp);
        return std::move(res);
    }

    endpoint_info get_endpoint_info( const std::string &ep )
    {
        endpoint_info res;
        std::string   tmp_addr;

        size_t delim_pos = ep.find_last_of( ':' );

        if( delim_pos == std::string::npos ) {

            /// local: <localname>
            tmp_addr = ep;
            res.type = endpoint_info::ENDPOINT_LOCAL;

        } else {

            /// tcp: <addr>:port
            res.type = endpoint_info::ENDPOINT_TCP;
            tmp_addr.assign( ep.begin( ), ep.begin( ) + delim_pos );

            std::string svc_tmp( ep.begin( ) + delim_pos + 1, ep.end( ) );
            //int port = atoi( svc_tmp.c_str( ) );
            char *end = nullptr;
            auto str_res = strtoul( svc_tmp.c_str( ), &end, 10 );
            if( str_res == ULONG_MAX && errno == EINVAL ) {
                res.type = endpoint_info::ENDPOINT_NONE;
            } else {
                res.addpess = tmp_addr;
                res.service = static_cast<uint16_t>(str_res); // can be 0 =)
                return res;
            }

        }

        if( tmp_addr.size( ) > 0 && tmp_addr[0] == '@' ) {
            /// ssl !<ep>
            res.addpess.assign( tmp_addr.begin( ) + 1, tmp_addr.end( ) );
            res.flags = endpoint_info::FLAG_SSL;
        } else {
            res.addpess.assign( tmp_addr.begin( ), tmp_addr.end( ) );
        }

        return res;
    }

    std::ostream & operator << ( std::ostream &os, const endpoint_info &ei )
    {
        static const char *ssl_flag[2] = { "", "@" };

        if( !ei ) {
            os << "";
        } else if( ei.is_local( ) ) {
            os << ei.addpess;
        } else {
            os << ssl_flag[ei.is_ssl( ) ? 1 : 0]
               << ei.addpess << ":" << ei.service;
        }
        return os;
    }


}}

