

#include "client-core/interfaces/ILogger.h"
#include "protocol/logger.pb.h"

#include "client-core/fr-client.h"

#include "vtrc-common/vtrc-stub-wrapper.h"
#include "vtrc-common/vtrc-rpc-channel.h"

namespace fr { namespace client { namespace interfaces {

    namespace {

        namespace log_proto = fr::proto::logger;
        namespace vcomm = vtrc::common;
        typedef vcomm::rpc_channel                           channel_type;
        typedef log_proto::instance::Stub                    stub_type;
        typedef vcomm::stub_wrapper<stub_type, channel_type> client_type;

        typedef std::shared_ptr<channel_type> channel_sptr;

        logger::log_level proto2level( unsigned lvl )
        {
            switch(lvl) {
            case logger::zero    :
            case logger::error   :
            case logger::warning :
            case logger::info    :
            case logger::debug   :
                return static_cast<logger::log_level>(lvl);
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

        void channel_error( const char * /*mess*/ )
        {
            //std::cerr << "logger channel error: " << mess << "\n";
        }

        class logger_impl: public logger::iface {

            core::client_core  &cl_;
            mutable client_type client_;
            channel_sptr        default_channel_;

        public:

            logger_impl( core::client_core &cl )
                :cl_(cl)
                ,client_(cl_.create_channel( ), true)
                ,default_channel_(cl_.create_channel( ))
            {
                const unsigned def_flags = vcomm::rpc_channel::DISABLE_WAIT;

                // disable wait for the logger
                client_.channel( )->set_flags( def_flags );

                // disable exceptions for the logger
                client_.channel( )->set_channel_error_callback( channel_error );
            }

            void set_level( logger::log_level lvl ) const override
            {
                log_proto::set_level_req req;
                req.set_level( level2proto( lvl ) );
                client_.call_request( &stub_type::set_level, &req );
            }

            logger::log_level get_level( ) const override
            {
                client_type tmp_client( default_channel_ );
                //client_.channel( )
                //       ->set_channel_error_callback( channel_error );

                log_proto::get_level_res res;
                res.set_level( level2proto( logger::info ) );
                tmp_client.call_response( &stub_type::get_level, &res );
                return proto2level( res.level( ) );
            }

            void write( logger::log_level lvl,
                        const std::string &text ) const override
            {
                log_proto::log_req req;
                req.set_level( level2proto( lvl ) );
                req.set_text( text );
                client_.call_request( &stub_type::send_log, &req );
            }

            static
            void async_cb( unsigned /*err*/, const std::string &data,
                           logger::on_write_callback cb )
            {
                log_proto::write_data req;
                req.ParseFromString( data );
                cb( proto2level( req.level( ) ), req.microsec( ), req.text( ) );
            }

            void subscribe( logger::on_write_callback cb ) const override
            {
                namespace ph = std::placeholders;
                client_type tmp(default_channel_);
                log_proto::subscribe_res res;
                tmp.call_response( &stub_type::subscribe, &res );
                cl_.register_async_op( res.async_op_id( ),
                                       std::bind( &logger_impl::async_cb,
                                                   ph::_1, ph::_2, cb ) );
            }

            void unsubscribe( ) const override
            {
                client_.call( &stub_type::unsubscribe );
            }

        };
    }

    namespace logger {
        iface_ptr create( core::client_core &cl )
        {
            return new logger_impl( cl );
        }
    }

}}}
