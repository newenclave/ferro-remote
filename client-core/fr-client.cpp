
#include "vtrc-client/vtrc-client.h"
#include "vtrc-mutex.h"

#include <stdlib.h>

namespace fr {  namespace client { namespace core {

    namespace vclient = vtrc::client;
    namespace vcommon = vtrc::common;

    namespace {

        enum connection_name { CONN_NONE, CONN_PIPE, CONN_TCP };

        struct connect_info {

            connection_name name_;

            union {
                struct {
                    std::string server_;
                    unsigned short port_;
                    bool tcp_nowait_;
                } tcp_;

                struct {
                    std::string file_;
                } pipe_;
            } data_;
        };

        connect_info get_connect_info( std::string const &name, bool nowait )
        {
            size_t delim_pos = name.find_last_of( ':' );

            connect_info ci;
            ci.name_ = CONN_NONE;

            if( delim_pos == std::string::npos ) {

                /// local: <localname>
                ci.name_ = CONN_PIPE;
                ci.data_.pipe_.file_ = name;

            } else {

                ci.name_ = CONN_TCP;
                ci.data_.tcp_.server_.assign( name.begin( ),
                                              name.begin( ) + delim_pos );
                ci.data_.tcp_.port_ = atoi( name.c_str( ) + delim_pos + 1 );
                ci.data_.tcp_.tcp_nowait_ = nowait;

            }

        }
    }

    struct client::impl {

        vclient::vtrc_client_sptr client_;
        vtrc::mutex               client_lock_;

    };

    client::client( )
        :impl_(new impl)
    {

    }

    client::~client(  )
    {
        delete impl_;
    }

    void client::connect( const std::string &server )
    {

    }

    void client::connect( const std::string &server, bool tcp_nowait )
    {

    }

}}}
