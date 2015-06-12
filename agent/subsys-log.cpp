
#include "application.h"
#include "subsys-log.h"
#include "subsys-config.h"
#include "subsys-reactor.h"

#include "boost/program_options/variables_map.hpp"
#include "boost/date_time/posix_time/posix_time.hpp"
#include "boost/asio/io_service.hpp"
#include "boost/asio/strand.hpp"

#include "vtrc-chrono.h"
#include "vtrc-atomic.h"
#include "vtrc-common/vtrc-closure-holder.h"

#include <string>
#include <list>
#include <fstream>
#include <functional>

#include "protocol/logger.pb.h"
#include "protocol/ferro.pb.h"

#define LOG(lev) logger_(lev) << "[log] "
#define LOGINF   LOG(logger::info)
#define LOGDBG   LOG(logger::debug)
#define LOGERR   LOG(logger::error)
#define LOGWRN   LOG(logger::warning)

namespace fr { namespace agent { namespace subsys {

    namespace {

        const std::string subsys_name( "log" );

        namespace po = boost::program_options;
        namespace bs = boost::signals2;
        namespace ba = boost::asio;
        namespace ph = std::placeholders;

        namespace vcomm  = vtrc::common;
        namespace vserv  = vtrc::server;
        namespace gpb    = google::protobuf;

        typedef std::shared_ptr<std::ostream>  ostream_sptr;

        namespace sproto = fr::proto;
        typedef   sproto::events::Stub events_stub_type;

        ostream_sptr open_file( const std::string &path, size_t *size )
        {
            ostream_sptr res( new std::ofstream( path, std::ios::app ) );
            if( size )  {
                *size = res->tellp( );
            }
            return res;
        }

        struct ostream_info {

            std::string          path_;
            ostream_sptr         stream_;
            vtrc::atomic<size_t> size_;
            bs::connection       connect_;

            ostream_info( std::string const &path )
                :path_(path)
            {
                size_t s = 0;
                stream_ = open_file( path_, &s );
                size_   = s;
            }
        };

        typedef std::shared_ptr<ostream_info> ostream_info_sptr;
        typedef std::vector<std::string>      string_vector;

        string_vector log_files( const po::variables_map &vm )
        {
            if( vm.count( "log" ) ) {
                return vm["log"].as<string_vector>( );
            }
            return std::vector<std::string>( );
        }

        logger::level log_level( const po::variables_map &vm )
        {
            static const struct loglevels {
                const char      *name_;
                logger::level    lev_;
            } names[ ] = {
                 { "zero",    logger::zero    }
                ,{ "ZER",     logger::zero    }
                ,{ "error",   logger::error   }
                ,{ "ERR",     logger::error   }
                ,{ "warning", logger::warning }
                ,{ "WRN",     logger::warning }
                ,{ "info",    logger::info    }
                ,{ "INF",     logger::info    }
                ,{ "debug",   logger::debug   }
                ,{ "DBG",     logger::debug   }
                ,{  nullptr,  logger::zero    }
            };

            if( vm.count( "log-level" ) ) {

                std::string lev = vm["log-level"].as<std::string>( );
                const loglevels *p = names;

                while( p->name_ ) {
                    if( 0 == lev.compare( p->name_ ) ) {
                        return p->lev_;
                    }
                    p++;
                }
            }
            return logger::info;
        }

        void ostream_log_slot( std::ostream &o, logger::level /*lev*/,
                               const std::string &data )
        {
            o << data;
            //o.flush( );
        }

        void log_slot( ostream_info_sptr o, logger::level lev,
                       const std::string &data )
        {
            ostream_log_slot( *o->stream_, lev, data );
            o->size_ += data.size( );
        }

        logger::level proto2level( unsigned proto_level )
        {
            switch(proto_level) {
            case logger::zero    :
            case logger::error   :
            case logger::warning :
            case logger::info    :
            case logger::debug   :
                return static_cast<logger::level>(proto_level);
            }
            return logger::info;
        }

        fr::proto::logger::log_level level2proto( unsigned lvl )
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

        class my_logger: public logger {

            ba::io_service::strand  &dispatcher_;

        public:

            my_logger( ba::io_service::strand &dispatcher )
                :logger(logger::info)
                ,dispatcher_(dispatcher)
            { }

            void do_write( level lev, const std::string &data )
            {
                this->get_on_write( )( lev, data );
            }

            void send_data( level lev, const std::string &data )
            {
                static const char *names[ ] = {
                    "ZER", "ERR",  "WRN", "INF", "DBG"
                };

                std::ostringstream oss;
                oss << boost::posix_time::microsec_clock::local_time( )
                    << " [" << names[lev] << "] " << data << "\n";

                dispatcher_.post( std::bind( &my_logger::do_write, this,
                                              lev, oss.str( ) ) );

            }
        };

        class proto_looger_impl: public fr::proto::logger::instance {

            typedef proto_looger_impl this_type;

            fr::agent::logger &lgr_;
            bs::connection     connect_;
            subsys::reactor   &reactor_;

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
                               vcomm::connection_iface_wptr /*cli*/)
                :lgr_(app->subsystem<subsys::log>( ).get_logger( ))
                ,reactor_(app->subsystem<subsys::reactor>( ))
            { }

            void send_log( ::google::protobuf::RpcController*  /*controller*/,
                           const ::fr::proto::logger::log_req* request,
                           ::fr::proto::logger::empty*         /*response*/,
                           ::google::protobuf::Closure* done ) override
            {
                vcomm::closure_holder holder( done );
                logger::level lvl = request->has_level( )
                                  ? proto2level( request->level( ) )
                                  : logger::info;
                lgr_(lvl) << request->text( );
            }

            void set_level(::google::protobuf::RpcController*   /*controller*/,
                         const ::fr::proto::logger::set_level_req* request,
                         ::fr::proto::logger::empty*             /*response*/,
                         ::google::protobuf::Closure* done) override
            {
                vcomm::closure_holder holder( done );
                logger::level lvl = request->has_level( )
                                  ? proto2level( request->level( ) )
                                  : logger::info;
                lgr_.set_level( lvl );
            }

            void get_level(::google::protobuf::RpcController* /*controller*/,
                         const ::fr::proto::logger::empty*    /*request*/,
                         ::fr::proto::logger::get_level_res*  response,
                         ::google::protobuf::Closure* done) override
            {
                vcomm::closure_holder holder( done );
                response->set_level( level2proto( lgr_.get_level( ) ) );
            }

            void on_write( logger::level lvl,
                           const std::string &data, size_t opid )
            {
//                std::cout << "EVENT!!!!: " << lvl << " " << data
//                          << " " << opid << "\n";
            }

            void subscribe(::google::protobuf::RpcController* /*controller*/,
                         const ::fr::proto::logger::empty*    /*request*/,
                         ::fr::proto::logger::subscribe_res* response,
                         ::google::protobuf::Closure* done) override
            {
                vcomm::closure_holder holder( done );
                size_t op_id = next_op_id( );

                namespace ph = std::placeholders;

                connect_ = lgr_.on_write_connect(
                                std::bind( &this_type::on_write, this,
                                           ph::_1, ph::_2, op_id ) );

                response->set_async_op_id( op_id );
            }

            void unsubscribe(::google::protobuf::RpcController* /*controller*/,
                         const ::fr::proto::logger::empty*      /*request*/,
                         ::fr::proto::logger::empty*            /*response*/,
                         ::google::protobuf::Closure* done) override
            {
                vcomm::closure_holder holder( done );
                connect_.disconnect( );
            }

        };

        application::service_wrapper_sptr create_service(
                                      fr::agent::application *app,
                                      vtrc::common::connection_iface_wptr cl )
        {
            vtrc::shared_ptr<proto_looger_impl>
                    inst(vtrc::make_shared<proto_looger_impl>( app, cl ));
            return app->wrap_service( cl, inst );
        }
    }

    struct log::impl {

        application                    *app_;
        ba::io_service::strand          dispatcher_;
        my_logger                       logger_;
        std::vector<ostream_info_sptr>  connections_;
        bs::connection                  stdout_connection_;
        bs::connection                  stderr_connection_;

        void set_connections( const std::vector<std::string> &files )
        {
            std::vector<ostream_info_sptr> tmp;

            try {

                bool sout = false;
                bool serr = false;

                for( auto &f: files ) {

                    if( f == "-" ) {

                        sout = true;
                        stdout_connection_ = logger_.on_write_connect(
                                std::bind( ostream_log_slot,
                                           std::ref( std::cout ),
                                           ph::_1, ph::_2 ) );

                    } else if( f == "-!" ) {

                        serr = true;
                        stderr_connection_ = logger_.on_write_connect(
                                std::bind( ostream_log_slot,
                                           std::ref( std::cerr ),
                                           ph::_1, ph::_2 ) );

                    } else {
                        ostream_info_sptr next (
                                    std::make_shared<ostream_info>( f ) );
                        next->connect_ = logger_.on_write_connect(
                                std::bind( log_slot, next,
                                           ph::_1, ph::_2 )
                            );
                        tmp.push_back( next );
                    }
                }
                if( !sout ) {
                    stdout_connection_.disconnect( );
                }
                if( !serr ) {
                    stderr_connection_.disconnect( );
                }
                connections_.swap( tmp );
            } catch( const std::exception &ex ) {
                std::cerr << "Set slots failed: " << ex.what( ) << "\n";
            }
        }

        impl( application *app )
            :app_(app)
            ,dispatcher_(app_->get_io_service( ))
            ,logger_(dispatcher_)
        {
            config &c(app_->subsystem<config>( ));
            logger_.set_level( log_level( c.variables( ) ) );

            std::vector<std::string> files( log_files( c.variables( ) ) );

            set_connections( files );
        }

        void flush_all( )
        {
            for( auto &f: connections_ ) {
                f->stream_->flush( );
            }
        }

        void reg_creator( const std::string &name,
                          application::service_getter_type func )
        {
            app_->register_service_creator( name, func );
        }

        void unreg_creator( const std::string &name )
        {
            app_->unregister_service_creator( name );
        }

        ~impl( )
        {
            flush_all( );
        }
    };

    log::log( application *app )
        :impl_(new impl(app))
    { }

    log::~log( )
    {
        delete impl_;
    }

    /// static
    vtrc::shared_ptr<log> log::create( application *app )
    {
        vtrc::shared_ptr<log> new_inst(new log(app));
        return new_inst;
    }

    const std::string &log::name( )  const
    {
        return subsys_name;
    }

    logger &log::get_logger( )
    {
        return impl_->logger_;
    }

    void log::init( )
    {

    }

    void log::start( )
    {
        impl_->reg_creator( proto_looger_impl::name( ), create_service );
        impl_->LOGINF << "Started.";
    }

    void log::stop( )
    {
        impl_->unreg_creator( proto_looger_impl::name( ) );
        impl_->LOGINF << "Stopped.";
    }

}}}

    
