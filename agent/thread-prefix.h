#ifndef FR_THREADPREFIX_H
#define FR_THREADPREFIX_H

#include <string>

namespace fr { namespace agent {
    const std::string &get_thread_prefix( );
    void set_thread_prefix( const std::string &val );

    struct thread_prefix_keeper {
        std::string old_;
        thread_prefix_keeper( const std::string &new_val )
            :old_(get_thread_prefix( ))
        {
            set_thread_prefix( new_val );
        }
        ~thread_prefix_keeper( )
        {
            set_thread_prefix( old_ );
        }
    };
}}

#endif // FR_THREADPREFIX_H
