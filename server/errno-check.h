#ifndef FR_ERRNOCHECK_H
#define FR_ERRNOCHECK_H

#include <errno.h>
#include <string.h>

namespace fr { namespace server {

    struct errno_error {

        static void throw_error( const char *func )
        {
            std::stringstream oss;
            if( func ) {
                oss << "'" << func << "' failed. ";
            }
            oss << "errno = " << errno
                << " (" << strerror( errno ) << ")";
            throw std::runtime_error( oss.str( ) );
        }

        static void errno_assert( bool cond, const char *func = NULL )
        {
            if( !cond ) throw_error( func );
        }


    };

}}

#endif // ERRNOCHECK_H
