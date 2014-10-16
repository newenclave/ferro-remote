#ifndef FR_CLIENT_H
#define FR_CLIENT_H

#include <string>

#include "vtrc-common/vtrc-pool-pair.h"
#include "vtrc-function.h"
#include "vtrc-common/vtrc-closure.h"
#include "vtrc-common/vtrc-signal-declaration.h"

#include "vtrc-stdint.h"
#include "interfaces/IAsyncOperation.h"

namespace boost { namespace system {
    class error_code;
}}

namespace vtrc { namespace common {
    class rpc_channel;
}}

namespace fr {  namespace client { namespace core {

    class client_core {

        struct         impl;
        friend struct  impl;
        impl          *impl_;

        VTRC_DECLARE_SIGNAL( on_init_error, void( const char *message ) );
        VTRC_DECLARE_SIGNAL( on_connect,    void( ) );
        VTRC_DECLARE_SIGNAL( on_disconnect, void( ) );
        VTRC_DECLARE_SIGNAL( on_ready,      void( ) );

    public:

        client_core( vtrc::common::pool_pair &pp );
        ~client_core(  );

    public:

        /// format is:
        ///  /path/with/file_name for UNIX socket
        ///  server:port          for tcp
        void connect( const std::string &server );
        void connect( const std::string &server, bool tcp_nowait );

        ///
        /// here is:
        /// vtrc::common::system_closure_type ==
        ///         void func( const boost::system::error_code &err )
        ///
        typedef vtrc::common::system_closure_type async_closure_func;

        void async_connect( const std::string &server, async_closure_func f );
        void async_connect( const std::string &server,
                            bool tcp_nowait, async_closure_func f );

        void reconnect( );
        void async_reconnect( async_closure_func f );
        void disconnect( );

        vtrc::common::rpc_channel *create_channel( );
        vtrc::common::rpc_channel *create_channel( unsigned flags );

        void register_async_op( size_t id, async_op_callback_type cb );
        void unregister_async_op( size_t id );

        void set_id ( const std::string &id );
        void set_key( const std::string &key );
        void set_id_key( const std::string &id, const std::string &key );

        std::string get_id( ) const;
        bool has_key( ) const;

    };

}}}

#endif // FRCLIENT_H
