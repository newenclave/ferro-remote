#include <memory>
#include <chrono>
#include <fstream>
#include <map>
#include <syslog.h>

#include "subsys-logging.h"
#include "subsys-reactor.h"
#include "application.h"

#include "logger.h"
#include "utils.h"

#include "vtrc-common/vtrc-delayed-call.h"

#include "boost/date_time/posix_time/posix_time.hpp"
#include "boost/algorithm/string.hpp"

#include "protocol/logger.pb.h"
#include "protocol/ferro.pb.h"

#include "vtrc-common/vtrc-stub-wrapper.h"
#include "vtrc-common/vtrc-closure-holder.h"
#include "vtrc-server/vtrc-channels.h"

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
        const std::string syslog_name  = "syslog";

        using level      = agent::logger::level;
        using stringlist = std::vector<std::string>;

        const std::string subsys_name( "logging" );
        namespace bsig = boost::signals2;
        namespace bpt  = boost::posix_time;
        namespace signal = boost::signals2;
        namespace vcomm  = vtrc::common;
        namespace vserv  = vtrc::server;

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

        namespace sproto = fr::proto;
        typedef   sproto::events::Stub events_stub_type;
        typedef   vcomm::stub_wrapper<
            events_stub_type, vcomm::rpc_channel
        > event_client_type;

        using vserv::channels::unicast::create_event_channel;

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

//        std::string thread_id( std::thread::id id )
//        {
//            std::ostringstream oss;
//#ifdef _WIN32
//            oss << id;
//#else
//            oss << std::hex << id;
//#endif
//            return oss.str( );
//        }

        void chan_err( const char * /*mess*/ )
        {
            //std::cerr << "logger channel error: " << mess << "\n";
        }

        logger::level proto2lvl( unsigned proto_level )
        {
            switch(proto_level) {
            case static_cast<unsigned>(logger::level::zero    ):
            case static_cast<unsigned>(logger::level::error   ):
            case static_cast<unsigned>(logger::level::warning ):
            case static_cast<unsigned>(logger::level::info    ):
            case static_cast<unsigned>(logger::level::debug   ):
                return static_cast<logger::level>(proto_level);
            }
            return logger::level::info;
        }

        fr::proto::logger::log_level lvl2proto( unsigned lvl )
        {
            switch( lvl ) {
            case fr::proto::logger::zero    :
            case fr::proto::logger::error   :
            case fr::proto::logger::warning :
            case fr::proto::logger::info    :
            case fr::proto::logger::debug   :
                return static_cast<fr::proto::logger::log_level>(lvl);
            }
            return fr::proto::logger::info;
        }

        int level2syslog( int /*logger::level*/ lvl )
        {
            switch( lvl ) {
            case static_cast<int>( logger::level::zero    ): return LOG_EMERG;
            case static_cast<int>( logger::level::error   ): return LOG_ERR;
            case static_cast<int>( logger::level::warning ): return LOG_WARNING;
            case static_cast<int>( logger::level::info    ): return LOG_INFO;
            case static_cast<int>( logger::level::debug   ): return LOG_DEBUG;
            }
            return LOG_INFO;
        }

        class proto_looger_impl: public fr::proto::logger::instance {

            typedef proto_looger_impl this_type;

            fr::agent::logger     &lgr_;
            signal::connection     connect_;
            subsys::reactor       &reactor_;
            event_client_type      eventor_;

        public:

            size_t next_op_id( )
            {
                return reactor_.next_op_id( );
            }

            static const std::string &name( )
            {
                return fr::proto::logger::instance::descriptor( )->full_name( );
            }

            ~proto_looger_impl( )
            {
                connect_.disconnect( );
            }

            proto_looger_impl( fr::agent::application *app,
                               vcomm::connection_iface_wptr cli)
                :lgr_(app->get_logger( ))
                ,reactor_(app->subsystem<subsys::reactor>( ))
                ,eventor_(create_event_channel(cli.lock( ), true), true)
            {
                eventor_.channel( )->set_channel_error_callback( chan_err );
            }

            void send_log( ::google::protobuf::RpcController*  /*controller*/,
                           const ::fr::proto::logger::log_req* request,
                           ::fr::proto::empty*                 /*response*/,
                           ::google::protobuf::Closure* done ) override
            {
                vcomm::closure_holder holder( done );
                logger::level lvl = request->has_level( )
                                  ? proto2lvl( request->level( ) )
                                  : logger::level::info;
                lgr_(lvl) << request->text( );
            }

            void set_level(::google::protobuf::RpcController*   /*controller*/,
                         const ::fr::proto::logger::set_level_req* request,
                         ::fr::proto::empty*                    /*response*/,
                         ::google::protobuf::Closure* done) override
            {
                vcomm::closure_holder holder( done );
                logger::level lvl = request->has_level( )
                                  ? proto2lvl( request->level( ) )
                                  : logger::level::info;
                lgr_.set_level( lvl );
            }

            void get_level(::google::protobuf::RpcController* /*controller*/,
                         const ::fr::proto::empty*            /*request*/,
                         ::fr::proto::logger::get_level_res*  response,
                         ::google::protobuf::Closure* done) override
            {
                vcomm::closure_holder holder( done );
                response->set_level( lvl2proto(
                         static_cast<unsigned>(lgr_.get_level( )) ) );
            }

            static vtrc::uint64_t time2ticks( const boost::posix_time::ptime &t)
            {
                using namespace boost::posix_time;
                static const ptime epoch( ptime::date_type( 1970, 1, 1 ) );
                time_duration from_epoch = t - epoch;
                return from_epoch.ticks( );
            }

            void on_write2( const log_record_info info,
                           logger_data_type const &data,
                           size_t opid )
            {
                fr::proto::logger::write_data req;
                req.set_level( lvl2proto( static_cast<unsigned>(info.level) ) );
                std::ostringstream oss;
                for( auto &d: data ) {
                    oss << d << "\n";
                }
                req.set_text( oss.str( ) );
                req.set_microsec( time2ticks( info.when ) );

                fr::proto::async_op_data areq;
                areq.set_id( opid );
                areq.set_data( req.SerializeAsString( ) );
                areq.set_tick_count( application::tick_count( ) );

                eventor_.call_request( &events_stub_type::async_op, &areq );

            }

#if 0
            void on_write( logger::level lvl, uint64_t microsec,
                           const std::string &data,
                           const std::string &/*format*/,
                           size_t opid )
            {
                fr::proto::logger::write_data req;
                req.set_level( level2proto( static_cast<unsigned>(lvl) ) );
                req.set_text( data );
                req.set_microsec( microsec );

                fr::proto::async_op_data areq;
                areq.set_id( opid );
                areq.set_data( req.SerializeAsString( ) );
                eventor_.call_request( &events_stub_type::async_op, &areq );

            }
#endif

            void subscribe(::google::protobuf::RpcController* /*controller*/,
                         const ::fr::proto::empty*            /*request*/,
                         ::fr::proto::logger::subscribe_res* response,
                         ::google::protobuf::Closure* done) override
            {
                vcomm::closure_holder holder( done );
                size_t op_id = next_op_id( );

                namespace ph = std::placeholders;

                connect_ = lgr_.on_write_connect(
                                std::bind( &this_type::on_write2, this,
                                           ph::_1, ph::_2, op_id ) );

                response->set_async_op_id( op_id );
            }

            void unsubscribe(::google::protobuf::RpcController* /*controller*/,
                         const ::fr::proto::empty*      /*request*/,
                         ::fr::proto::empty*            /*response*/,
                         ::google::protobuf::Closure* done) override
            {
                vcomm::closure_holder holder( done );
                connect_.disconnect( );
            }

        };

        application::service_wrapper_sptr create_service( application *app,
                                              vcomm::connection_iface_wptr cl )
        {
            auto inst(vtrc::make_shared<proto_looger_impl>( app, cl ));
            return app->wrap_service( cl, inst );
        }

        std::ostream &output( std::ostream &o,
                              const log_record_info &loginf,
                              const std::string &s )
        {
            level lvl = static_cast<level>(loginf.level);
            o << loginf.when
              << " " << loginf.tprefix
              << " [" << agent::logger::level2str(lvl) << "] "
              << s
                   ;
            return o;
        }

        std::ostream &output2( std::ostream &o,
                               const log_record_info &loginf,
                               const std::string &s )
        {
            level lvl = static_cast<level>(loginf.level);
            o << loginf.when.time_of_day( )
              << " " << loginf.tprefix
              << " <" << agent::logger::level2str(lvl) << "> "
              << s
                   ;
            return o;
        }

        struct log_output {
            int min_;
            int max_;
            log_output( int min, int max )
                :min_(min)
                ,max_(max)
            { }

            int get_min( ) const
            {
                return min_;
            }

            int get_max( ) const
            {
                return max_;
            }

            void set_min( int val )
            {
                min_ = val;
            }

            void set_max( int val )
            {
                max_ = val;
            }

            virtual ~log_output( ) { }
            virtual void write( const log_record_info &loginf,
                                stringlist const &data ) = 0;
            virtual size_t length( ) const = 0;
        };

        struct console_output: public log_output {
            bsig::scoped_connection conn_;
            std::ostream &stream_;
            console_output( int min, int max, std::ostream &stream )
                :log_output(min, max)
                ,stream_(stream)
            { }

            void write( const log_record_info &loginf, stringlist const &data )
            {
                int lvl = loginf.level;
                level_color _( stream_, static_cast<logger::level>(lvl) );
                if( (lvl >= get_min( )) && (lvl <= get_max( )) ) {
                    for( auto &s: data ) {
                        output( stream_, loginf, s ) << std::endl;
                    }
                }
            }

            size_t length( ) const
            {
                return 0;
            }
        };

        struct cerr_output: public console_output {
            cerr_output( int min, int max )
                :console_output(min, max, std::cerr)
            { }
        };

        struct cout_output: public console_output {
            cout_output( int min, int max )
                :console_output(min, max, std::cout)
            { }
        };

        struct syslog_output: public log_output {

            syslog_output(int min, int max)
                :log_output(min, max)
            { }

            void write( const log_record_info &loginf, stringlist const &data )
            {
                int lvl = loginf.level;
                if( (lvl >= get_min( )) && (lvl <= get_max( ) ) ) {
                    for( auto &s: data ) {
                        std::ostringstream oss;
                        output2( oss, loginf, s );
                        syslog( level2syslog( loginf.level ),
                                "%s", oss.str( ).c_str( ) );
                    }
                }
            }

            size_t length( ) const
            {
                return 0;
            }
        };

        struct file_output: public log_output {
            std::atomic<size_t> length_;
            ostream_uptr        stream_;
            file_output( int min, int max, const std::string &path )
                :log_output(min, max)
                ,length_(0)
            {
                size_t len = 0;
                stream_ = open_file( path, &len );
                length_ = len;
            }

            void write( const log_record_info &loginf, stringlist const &data )
            {
                int lvl = loginf.level;
                std::ostringstream oss;
                if( (lvl >= get_min( )) && (lvl <= get_max( )) ) {
                    for( auto &s: data ) {
                        output( oss, loginf, s ) << "\n";
                    }
                }
                length_    += oss.tellp( );
                (*stream_) << oss.str( );
            }

            size_t length( ) const
            {
                return length_;
            }
        };

        struct output_connection {
            bsig::scoped_connection connection_;
            std::unique_ptr<log_output> output_;
        };

        using connection_map = std::map<std::string, output_connection>;

        std::unique_ptr<log_output> create_by_name( const std::string &path,
                                                    int minlvl, int maxlvl,
                                                    agent::logger   &log_ )
        {
            std::unique_ptr<log_output> res;

            if( path == stdout_name || path == stdout_name2 ) { /// cout
                res.reset(new cout_output( minlvl, maxlvl ) );
            } else if( path == stderr_name ) {                  /// cerr
                res.reset(new cerr_output( minlvl, maxlvl ) );
            } else if( path == syslog_name ) {                  /// syslog
                res.reset(new syslog_output( minlvl, maxlvl ) );
            } else {
                try {
                    res.reset(new file_output( minlvl, maxlvl, path ));
                } catch( std::exception &ex ) {
                    //std::cerr
                    LOGERR
                        << "failed to add log file " << path << "; "
                        << ex.what( );
                    std::cerr
                        << "failed to add log file " << path << "; "
                        << ex.what( ) << std::endl;
                }
            }
            return std::move(res);
        }

        bool is_syslog( const std::string &path )
        {
            return path == syslog_name;
        }

        bool is_stdout( const std::string &path )
        {
            return path == stdout_name2;
        }
    }

    struct logging::impl {

        application     *app_;
        agent::logger   &log_;

        connection_info  stdout_connection_;
        connection_info  stderr_connection_;
        connection_info  syslog_connection_;

        stream_list      streams_;
        bool             syslog_;

        connection_map           connections_;
        //bsig ::scoped_connection slot_;

        impl( application *app )
            :app_(app)
            ,log_(app_->get_logger( ))
            ,syslog_(false)
        { }

        void reg_creator( const std::string &name,
                          application::service_getter_type func )
        {
            app_->register_service_factory( name, func );
        }

        void unreg_creator( const std::string &name )
        {
            app_->unregister_service_factory( name );
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

        void log_output_slot( log_output *out, const log_record_info &loginf,
                              stringlist const &data )
        {
            out->write( loginf, data );
        }

        /// dispatcher!
        void add_logger( const std::string &path, level minl, level maxl )
        {

            namespace ph = std::placeholders;

            bool slog = is_syslog( path );
            if( slog && syslog_ ) {
                LOGDBG << "Syslog is already here!";
                return;
            }

            output_connection conn;
            conn.output_ = create_by_name( path,
                                           static_cast<int>(minl),
                                           static_cast<int>(maxl), log_ );
            if( slog ) {
                openlog( "ferro_remote", 0, LOG_USER );
                syslog_ = true;
            }

            conn.connection_ = log_.on_write_connect(
                        std::bind( &impl::log_output_slot, this,
                                   conn.output_.get( ),
                                   ph::_1, ph::_2 ) );

            if( is_stdout(path) ) {
                connections_[stdout_name] = std::move(conn);
            } else {
                connections_[path] = std::move(conn);
            }

            return;
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
            bool slog = is_syslog( name );

            connections_.erase( name );

            LOGINF << "Erased: " << name;

            if( slog && syslog_ ) {
                syslog_ = false;
                closelog( );
            }
            return;
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
        impl_->reg_creator( proto_looger_impl::name( ),  create_service );
        impl_->LOGINF << "Started.";
    }

    void logging::stop( )
    {
        impl_->unreg_creator( proto_looger_impl::name( ) );
        impl_->LOGINF << "Stopped.";
    }

}}}

    
