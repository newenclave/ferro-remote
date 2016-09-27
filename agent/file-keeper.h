#ifndef FR_FILEKEEPER_H
#define FR_FILEKEEPER_H

#include <unistd.h>
#include "vtrc-memory.h"

namespace fr { namespace agent {

    struct file_keeper {

        int id_;

        explicit file_keeper( int fd )
            :id_(fd)
        { }

        ~file_keeper( )
        {
            if( -1 != id_ ) {
                close( id_ );
            }
        }

        int release( )
        {
            int tmp = id_;
            id_ = -1;
            return tmp;
        }

        int hdl( )
        {
            return id_;
        }
    };

    typedef vtrc::shared_ptr<file_keeper> file_keeper_sptr;

}}

#endif // FILEKEEPER_H
