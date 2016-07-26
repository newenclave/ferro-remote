#include <memory>
#include <chrono>
#include <fstream>
#include <map>

#include "subsys-logging.h"
#include "application.h"

#include "logger.h"
#include "utils.h"

#include "vtrc-common/vtrc-delayed-call.h"

#include "boost/date_time/posix_time/posix_time.hpp"
#include "boost/algorithm/string.hpp"

#define LOG(lev) log_(lev, "logging") << "[log] "
#define LOGINF   LOG(logger::level::info)
#define LOGDBG   LOG(logger::level::debug)
#define LOGERR   LOG(logger::level::error)
#define LOGWRN   LOG(logger::level::warning)

namespace fr { namespace agent { namespace subsys {

    namespace {

        const std::string stdout_name  = "stdout";
        const std::string stdout_name2 = "-";
        const std::string stderr_name  = "stderr";

        using level      = agent::logger::level;
        using stringlist = std::vector<std::string>;

        const std::string subsys_name( "logging" );
        namespace bsig = boost::signals2;
        namespace bpt  = boost::posix_time;

        inline logger::level str2lvl( const char *str )
        {
            return logger::str2level( str );
        }

        struct connection_info {
            bsig::scoped_connection conn_;
        };

        struct level_color {
            std::ostream &o_;
            level_color( std::ostream &o, level lvl )
                :o_(o)
            {
                switch( lvl ) {
                case level::zero:
                    o << utilities::console::cyan;
                    break;
                case level::error:
                    o << utilities::console::red;
                    break;
                case level::warning:
                    o << utilities::console::yellow;
                    break;
                case level::info:
                    o << utilities::console::green;
                    break;
                case level::debug:
                    o << utilities::console::light;
                    break;
                default:
                    o << utilities::console::none;
                }
            }

            ~level_color( )
            {
                o_ << utilities::console::none;
                o_.flush( );
            }
        };

        using ostream_uptr = std::unique_ptr<std::ostream>;

        ostream_uptr open_file( const std::string &path, size_t *size )
        {
            std::unique_ptr<std::ofstream>
                   res( new std::ofstream( path.c_str( ), std::ios::app ) );

            if( !res->is_open( ) ) {
                return ostream_uptr( );
            }

            if( size )  {
                *size = res->tellp( );
            }
            return std::move(res);
        }

        struct ostream_inf {
            ostream_uptr            stream_;
            size_t                  length_;
            level                   min_;
            level                   max_;
            std::string             path_;
            bsig::scoped_connection conn_;
            ostream_inf(const std::string &path)
                :length_(0)
                ,min_(level::zero)
                ,max_(level::debug)
                ,path_(path)
            { }
        };

        using ostream_sptr = std::shared_ptr<ostream_inf>;
        using ostream_wptr = std::weak_ptr<ostream_inf>;
        using stream_list  = std::list<ostream_inf>;

        std::string str2logger( const std::string &str,
                                std::string &fromlvl, std::string &tolvl )
        {
            size_t delim_pos = str.find_last_of( '[' );
            std::string path;

            if( delim_pos == std::string::npos ) {
                path = str;
            } else {

                path = std::string( str.begin( ), str.begin( ) + delim_pos );
                auto *to = &fromlvl;
                bool found_int = true;

                for( auto d = ++delim_pos; d < str.size( ); ++d ) {
                    switch( str[d] ) {
                    case ']':
                        found_int = false;
                    case '-':
                        to->assign( str.begin( ) + delim_pos,
                                    str.begin( ) + d );
                        to = &tolvl;
                        delim_pos = d + 1;
                        break;
                    }
                }

                if( found_int ) {
                    to->assign( str.begin( ) + delim_pos, str.end( ) );
                }
            }
            return path;
        }

        std::string thread_id( std::thread::id id )
        {
            std::ostringstream oss;
#ifdef _WIN32
            oss << id;
#else
            oss << std::hex << id;
#endif
            return oss.str( );
        }

    }

    struct logging::impl {

        application     *app_;
        agent::logger   &log_;

        connection_info  stdout_connection_;
        connection_info  stderr_connection_;

        stream_list      streams_;

        impl( application *app )
            :app_(app)
            ,log_(app_->get_logger( ))
        { }

        void reg_creator( const std::string &name,
                          application::service_getter_type func )
        {
            app_->register_service_creator( name, func );
        }

        void unreg_creator( const std::string &name )
        {
            app_->unregister_service_creator( name );
        }

        struct console_info {
            std::ostream *o_;
            level minl_;
            level maxl_;
            console_info( std::ostream *o, level minl, level maxl )
                :o_(o)
                ,minl_(minl)
                ,maxl_(maxl)
            { }
        };

        static std::ostream &output( std::ostream &o,
                                     const log_record_info &loginf,
                                     const std::string &s )
        {
            level lvl = static_cast<level>(loginf.level);
            o << loginf.when
              << " " << loginf.tprefix
              << " [" << agent::logger::level2str(lvl) << "]"
              << " " << s
                   ;
            return o;
        }

        /// works on dispatcher
        void console_log( console_info &inf, const log_record_info &loginf,
                          stringlist const &data )
        {
            level lvl = static_cast<level>(loginf.level);
            level_color _( *inf.o_, lvl );
            if( (lvl >= inf.minl_) && (lvl <= inf.maxl_) ) {
                for( auto &s: data ) {
                    output( *inf.o_, loginf, s ) << std::endl;
                }
            }
        }

        /// works on dispatcher
        void file_out_log( ostream_inf &inf, const log_record_info &loginf,
                           stringlist const &data )
        {
            //console_log( *inf.stream_, inf.min_, inf.max_, lvl, tim, data );
            level lvl = static_cast<level>(loginf.level);
            std::ostringstream oss;
            if( (lvl >= inf.min_) && (lvl <= inf.max_) ) {
                for( auto &s: data ) {
                    output( oss, loginf, s ) << "\n";
                }
            }
            inf.length_    += oss.tellp( );
            (*inf.stream_) << oss.str( );
            //inf.stream_->flush( );
        }

        /// dispatcher!
        void add_logger( const std::string &path, level minl, level maxl )
        {
            namespace ph = std::placeholders;

            if( path == stdout_name || path == stdout_name2 ) { /// cout

                //stdout_connection_.conn_.disconnect( );
                stdout_connection_.conn_ = log_.on_write_connect(
                            std::bind( &impl::console_log, this,
                                       console_info(&std::cout, minl, maxl),
                                       ph::_1, ph::_2 ) );

            } else if( path == stderr_name ) {  /// cerr

                //stderr_connection_.conn_.disconnect( );
                stderr_connection_.conn_ = log_.on_write_connect(
                            std::bind( &impl::console_log, this,
                                       console_info(&std::cerr, minl, maxl),
                                       ph::_1, ph::_2 ) );

            } else {

                try {

                    size_t len = 0;
                    auto stream_impl = open_file( path, &len );

                    if( stream_impl ) {
                        streams_.emplace_back( path );
                        auto l     = streams_.rbegin( );
                        l->length_ = len;
                        l->min_    = minl;
                        l->max_    = maxl;
                        l->conn_   = log_.on_write_connect(
                                        std::bind( &impl::file_out_log, this,
                                            std::ref(*l),
                                            ph::_1, ph::_2 ) );
                        l->stream_.swap( stream_impl );
                    } else {
                        //std::cerr
                        LOGERR
                            << "failed to add log file " << path << ";"
                            ;
                        std::cerr
                            << "failed to add log file " << path << ";"
                            << std::endl;
                    }
                } catch( const std::exception &ex ) {
                    //std::cerr
                    LOGERR
                        << "failed to add log file " << path << "; "
                        << ex.what( );
                    std::cerr
                        << "failed to add log file " << path << "; "
                        << ex.what( ) << std::endl;
                }

            }
        }

        /// dispatcher!
        void add_logger( const std::string &path,
                         const std::string &from, const std::string &to )
        {
            level minl = level::zero;
            level maxl = log_.get_level( );

            if( !to.empty( ) ) {
                maxl = logger::str2level( to.c_str( ) );
                minl = logger::str2level( from.c_str( ), level::zero );
            } else if( !from.empty( ) ) {
                maxl = logger::str2level( from.c_str( ) );
            }
            add_logger( path, minl, maxl );
        }

        /// dispatcher!
        void add_logger_output( const std::string &params )
        {
            std::string from;
            std::string to;
            auto path = str2logger( params, from, to );
            add_logger( path, from, to );
        }

        /// dispatcher!
        void del_logger_output( const std::string &name )
        {
            if( name == stdout_name ) {         /// cout
                stdout_connection_.conn_.disconnect( );
            } else if( name == stderr_name ) {  /// cerr
                stderr_connection_.conn_.disconnect( );
            } else {
                for( auto b=streams_.begin( ), e=streams_.end( ); b!=e; ++b ) {
                    if( b->path_ == name ) {
                        streams_.erase( b );
                        break;
                    }
                }
            }
        }
    };

    logging::logging( application *app )
        :impl_(new impl(app))
    { }

    logging::~logging( )
    {
        delete impl_;
    }

    /// static
    logging::shared_type logging::create( application *app,
                                          const std::vector<std::string> &def )
    {
        shared_type new_inst(new logging(app));

        for( auto &d: def ) {
            new_inst->impl_->add_logger_output( d );
        }

        return new_inst;
    }

    void logging::add_logger_output( const std::string &params )
    {
        impl_->log_.dispatch( [this, params]( ) {
            impl_->add_logger_output( params );
        } );
    }

    void logging::del_logger_output( const std::string &name )
    {
        impl_->log_.dispatch( [this, name]( ) {
            impl_->del_logger_output( name );
        } );
    }

    const std::string &logging::name( )  const
    {
        return subsys_name;
    }

    void logging::init( )
    {

    }

    void logging::start( )
    {
        impl_->LOGINF << "Started.";
    }

    void logging::stop( )
    {
        impl_->LOGINF << "Stopped.";
    }

}}}

    
