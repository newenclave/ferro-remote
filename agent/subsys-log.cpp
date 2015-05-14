
#include "application.h"
#include "subsys-log.h"
#include "subsys-config.h"

#include "boost/program_options/variables_map.hpp"
#include "boost/date_time/posix_time/posix_time.hpp"
#include "boost/asio/io_service.hpp"
#include "boost/asio/strand.hpp"

#include "vtrc-common/config/vtrc-chrono.h"

#include <string>
#include <list>
#include <fstream>
#include <functional>

namespace fr { namespace agent { namespace subsys {

    namespace {

        const std::string subsys_name( "log" );

        namespace po = boost::program_options;
        namespace bs = boost::signals2;
        namespace ba = boost::asio;
        namespace ph = std::placeholders;

        typedef std::shared_ptr<std::ostream> ostream_sptr;

        typedef std::pair<ostream_sptr, bs::connection> connection_pair;

        ostream_sptr open_file( const std::string &path )
        {
            return ostream_sptr( new std::ofstream( path, std::ios::app ) );
        }

        std::vector<std::string> log_files( const po::variables_map &vm )
        {
            if( vm.count( "log" ) ) {
                return vm["log"].as< std::vector<std::string> >( );
            }
            return std::vector<std::string>( );
        }

        logger::level log_level( const po::variables_map &vm )
        {
            const struct loglevels {
                const char      *name_;
                logger::level    lev_;
            } static names[ ] = {
                 { "zero",    logger::zero    }
                ,{ "error",   logger::error   }
                ,{ "warning", logger::warning }
                ,{ "info",    logger::info    }
                ,{ "debug",   logger::debug   }
                ,{ nullptr,   logger::zero    }
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

        void log_slot( std::ostream &o, logger::level lev,
                       const std::string &data )
        {
            const char *names[ ] = {
                "ZRO", "ERR",  "WRN", "INF", "DBG"
            };
            o << boost::posix_time::microsec_clock::local_time( )
              << " [" << names[lev] << "] " << data << "\n";
            //o.flush( );
        }
    }

    struct log::impl {

        class my_logger: public logger {

            impl *parent_;

        public:

            my_logger( impl *parent )
                :logger(logger::info)
                ,parent_(parent)
            { }

            void write_signal( level lev, const std::string &data )
            {
                this->get_on_write( )( lev, data );
            }

            void send_data( level lev, const std::string &data )
            {
                parent_->dispatcher_.post( std::bind(
                        &my_logger::write_signal, this, lev, data ) );
            }
        };

        application                 *app_;
        ba::io_service::strand       dispatcher_;
        my_logger                    logger_;
        std::vector<connection_pair> connections_;
        bs::connection               stdout_connection_;
        bs::connection               stderr_connection_;

        void set_connections( const std::vector<std::string> &files )
        {
            std::vector<connection_pair> tmp;

            try {
                bool sout = false;
                bool serr = false;
                for( auto &f: files ) {
                    if( f == "-" ) {
                        sout = true;
                        stdout_connection_ = logger_.on_write_connect(
                                    std::bind( log_slot, std::ref( std::cout ),
                                               ph::_1, ph::_2 ) );
                    } if( f == "-!" ) {
                        serr = true;
                        stderr_connection_ = logger_.on_write_connect(
                                    std::bind( log_slot, std::ref( std::cerr ),
                                               ph::_1, ph::_2 ) );
                    } else {
                        ostream_sptr out = open_file( f );
                        bs::connection sc( logger_.on_write_connect(
                                std::bind( log_slot, std::ref( *out ),
                                           ph::_1, ph::_2 )
                            ) );
                        tmp.push_back( std::make_pair( out, sc ) );
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
            ,logger_(this)
        {
            config &c(app_->subsystem<config>( ));
            logger_.set_level( log_level( c.variables( ) ) );

            std::vector<std::string> files( log_files( c.variables( ) ) );

            set_connections( files );
        }

        void flush_all( )
        {
            for( auto &f: connections_ ) {
                f.first->flush( );
            }
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

    }

    void log::stop( )
    {

    }


}}}

    
