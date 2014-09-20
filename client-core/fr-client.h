#ifndef FR_CLIENT_H
#define FR_CLIENT_H

namespace fr {  namespace client { namespace core {

    class client {

        struct impl;
        impl   impl_;

    public:

        client( );
        ~client(  );

    public:

        /// format is:
        ///  /path/with/file_name for UNIX socket
        ///  server:port          for tcp
        void connect( const std::string &server );
        void connect( const std::string &server, bool tcp_nowait );

        void reconnect( );
        void disconnect( );
    };

}}}

#endif // FRCLIENT_H
