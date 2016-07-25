#ifndef TA_AGENT_LOGGER_H
#define TA_AGENT_LOGGER_H

#include <cstdint>
#include <functional>
#include <thread>

#include "common/logger.hpp"
#include "common/noexcept.hpp"
#include "vtrc-common/vtrc-signal-declaration.h"
#include "boost/date_time/posix_time/posix_time.hpp"

namespace boost { namespace asio {
        class io_service;
}}

namespace fr { namespace agent {

    struct log_record_info {
        int                      level;
        boost::posix_time::ptime when;
        std::string              name;
        std::string              tprefix;
        std::thread::id          tid;
    };

    using logger_data_type   = std::vector<std::string>;
    using logger_signal_type = void ( const log_record_info,
                                      logger_data_type const & );

    class logger: public common::logger {

        struct  impl;
        impl   *impl_;
        VTRC_DECLARE_SIGNAL( on_write, logger_signal_type );

    public:

        using level = common::logger::level;
        static const int id = 0;

        logger( boost::asio::io_service &ios, level lvl,
                const char *split_string = "\n" );
        ~logger( );

        void dispatch( std::function<void ( )> call );

        static const char *level2str( level lvl, const char *def = "unk" )
        {
            switch( lvl ) {
            case level::zero:
                return "zer";
            case level::error:
                return "err";
            case level::warning:
                return "wrn";
            case level::info:
                return "inf";
            case level::debug:
                return "dbg";
            default:
                return def;
            }
        }

        static level str2level( const char *str, level def = level::info )
        {
            struct lvl2str {
                const char *str_;
                level       lvl_;
            };

            static lvl2str levels[ ] =
            {
                { "zer",      level::zero    },
                { "err",      level::error   },
                { "wrn",      level::warning },
                { "inf",      level::info    },
                { "dbg",      level::debug   },

                { "zero",     level::zero    },
                { "error",    level::error   },
                { "warning",  level::warning },
                { "info",     level::info    },
                { "debug",    level::debug   },

                { "z",        level::zero    },
                { "e",        level::error   },
                { "w",        level::warning },
                { "i",        level::info    },
                { "d",        level::debug   },

                { "ZER",      level::zero    },
                { "ERR",      level::error   },
                { "WRN",      level::warning },
                { "INF",      level::info    },
                { "DBG",      level::debug   },

                { "0",        level::zero    },
                { "1",        level::error   },
                { "2",        level::warning },
                { "3",        level::info    },
                { "4",        level::debug   },
            };

            for( auto &lvl: levels ) {
                if( 0 == strcmp( lvl.str_, str ) ) {
                    return lvl.lvl_;
                }
            }
            return def;
        }

    private:

        void send_data( level lev, const std::string &name,
                                   const std::string &data ) override;
        void send_data_nosplit( level lev, const std::string &name,
                                const std::string &data ) override;

        void do_write( const log_record_info &info,
                       std::string const &data, bool split ) NOEXCEPT;

    };
}}

#endif // LOGGER_H

