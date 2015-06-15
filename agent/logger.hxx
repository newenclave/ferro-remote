#ifndef FR_LOGGER_CXX
#define FR_LOGGER_CXX

#include "vtrc-common/vtrc-signal-declaration.h"
#include <sstream>
#include <stdint.h>

namespace fr { namespace agent {

    class logger {

    public:

        enum level {
             zero    = 0
            ,error   = 1
            ,warning = 2
            ,info    = 3
            ,debug   = 4
        };

        VTRC_DECLARE_SIGNAL( on_write, void( level, uint64_t microsec,
                                             std::string const &text,
                                             std::string const &ready ) );

    private:

        level level_;

        struct string_accumulator {

            logger                 &parent_;
            level                   level_;
            std::ostringstream      oss_;
            bool                    act_;

            string_accumulator( logger &parent, level lev )
                :parent_(parent)
                ,level_(lev)
                ,act_(true)
            { }

            string_accumulator( string_accumulator &other )
                :parent_(other.parent_)
                ,level_(other.level_)
                ,act_(true)
            {
                other.act_ = false;
            }

            string_accumulator& operator = ( string_accumulator &other )
            {
                level_     = other.level_;
                other.act_ = false;
                return *this;
            }

            ~string_accumulator( )
            {
                if( act_ && ( level_ <= parent_.level_ ) ) {
                    parent_.send_data( level_, oss_.str( ) );
                }
            }

            template<typename T>
            string_accumulator &operator << ( const T &data )
            {
                if( level_ <= parent_.level_ ) {
                    oss_ << data;
                }
                return *this;
            }
        };

    public:

        virtual void send_data( level lev, const std::string &data )
        {
            on_write_( lev, 0, data, data );
        }

        string_accumulator operator ( )( level lev )
        {
            string_accumulator res( *this, lev );
            return res;
        }

        string_accumulator operator ( )( )
        {
            string_accumulator res( *this, level_ );
            return res;
        }

        logger( level lev )
            :level_(lev)
        { }

        virtual ~logger( )
        { }

        level get_level( ) const
        {
            return level_;
        }

        void set_level( level lev )
        {
            level_ = lev;
        }

    };

}}

#endif // FR_LOGGER_CXX
