#ifndef FR_ERRNOCHECK_H
#define FR_ERRNOCHECK_H

#include <errno.h>
#include <string.h>
#include <sstream>

#include "vtrc/common/exception.h"

namespace fr { namespace agent {

    struct errno_error {

        static void throw_error( const char *func )
        {
            std::stringstream oss;
            if( func ) {
                oss << "'" << func << "' failed. ";
            }
            oss << "errno = " << errno
                << " (" << strerror( errno ) << ")";
            vtrc::common::throw_system_error( errno, oss.str( ) );
        }

        static void errno_assert( bool cond, const char *func = NULL )
        {
            if( !cond ) throw_error( func );
        }


    };

}}

#endif // ERRNOCHECK_H
