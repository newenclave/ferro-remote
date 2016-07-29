#include <iostream>
#include <set>

#include "vtrc-common/vtrc-delayed-call.h"
#include "client-core/fr-discover.h"

#include "boost/asio.hpp"

#include "vtrc-bind.h"
#include "vtrc-memory.h"
#include "vtrc-mutex.h"
#include "vtrc-atomic.h"
#include "vtrc-ref.h"

#include "utils.h"

namespace fr {  namespace client { namespace core {

    namespace {

        namespace ba = boost::asio;
        namespace bs = boost::system;
        namespace ph = vtrc::placeholders;

        typedef vtrc::common::delayed_call delayed_call;
        typedef delayed_call::milliseconds milliseconds;

        bool handler_default( const bs::error_code &ec,
                              const char *, size_t )
        {
            if( !ec ) {
                return false;
            } else {
                return true;
            }
        }

        struct sender: public discover::pinger {

            typedef discover::pinger_sptr shared_type;

            ba::io_service          &ios_;
            ba::ip::udp::socket     socket_;
            delayed_call            timeout_;
            int                     to_;
            ba::ip::udp::endpoint   from_;
            std::vector<char>       data_;
            std::string             mess_;
            discover::handler_type  handler_;

            sender( ba::io_service &ios, int to, discover::handler_type  h )
                :ios_(ios)
                ,socket_(ios_)
                ,timeout_(ios_)
                ,to_(to)
                ,data_(4069)
                ,handler_(h ? h : &handler_default)
            { }

            void ping( const std::string &addr, unsigned short port )
            {
                mess_ = "?";
                using ba::ip::udp;
                using ba::ip::address;

                udp::endpoint ep( address::from_string( addr ), port );

                socket_.open( ep.address( ).is_v6( )
                              ? udp::v6( )
                              : udp::v4( ));

                socket_.async_send_to(
                    ba::buffer(mess_), ep,
                    vtrc::bind( &sender::handle_send, this,
                                 ph::error, shared_from_this( )));
            }

            void mcast( const std::string &addr, unsigned short port,
                        const std::string &local )
            {
                mess_ = "?";
                using ba::ip::udp;
                using ba::ip::address;

                udp::endpoint ep( address::from_string( addr ), port );

                bool is_v6 = ep.address( ).is_v6( );
                socket_.open( is_v6 ? udp::v6( ) : udp::v4( ));

                if( !local.empty( ) ) {
                    udp::endpoint bep( address::from_string( local ), 0 );
                    bs::error_code ecode;
                    socket_.bind( bep, ecode );
                    if( ecode ) {
                        handler_( ecode, &data_[0], 0 );
                        return;
                    }
                }

                std::cout << socket_.local_endpoint( ).address( ).to_string( )
                          << ":" << socket_.local_endpoint( ).port( )
                          << std::endl;

                socket_.async_send_to( ba::buffer(mess_), ep,
                                       vtrc::bind( &sender::handle_send, this,
                                                    ph::error,
                                                    shared_from_this( ) ) );
            }

            void handle_timeout( bs::error_code const &err, shared_type sdr )
            {
                socket_.close( );
            }

            void handle_recv( bs::error_code const &err, size_t length,
                              shared_type sdr )
            {
                if( err ) {
                    handler_( err, &data_[0], 0 );
                } else {
                    if( handler_( err, &data_[0], length ) ) {
                        handle_send( err, sdr );
                    }
                }
            }

            void handle_send( bs::error_code const &err, shared_type sdr )
            {
                if( !err ) {
                    from_ = ba::ip::udp::endpoint( );

                    socket_.async_receive_from(
                            ba::buffer(&data_[0], data_.size( )), from_,
                            vtrc::bind( &sender::handle_recv, this,
                                         ph::error, ph::bytes_transferred,
                                         sdr ) );

                    timeout_.call_from_now(
                         vtrc::bind( &sender::handle_timeout, this,
                                     ph::error, sdr ),
                         milliseconds(to_) );
                } else {
                    handler_( err, &data_[0], 0 );
                }

            }

            void stop( ) override
            {
                socket_.close( );
            }
        };

        typedef vtrc::shared_ptr<sender> sender_sptr;
        typedef std::map<size_t, sender_sptr> sender_map;
#if 0
        void enum_local_addreses( std::vector<std::string> &out )
        {
            try {
                using boost::asio::ip::udp;
                boost::asio::io_service io_service;
                udp::resolver resolver(io_service);

//                boost::asio::ip::address addr;
//                udp::resolver::query query(addr.to_string(), "");
                udp::resolver::query query( boost::asio::ip::host_name( ), "" );

                for( udp::resolver::iterator b(resolver.resolve(query)), e;
                     b!=e; ++b )
                {
                    udp::endpoint ep = *b;
                    out.push_back( ep.address().to_string() );
                }
            } catch ( const std::exception &ex ) {
                std::cerr << "Error getting local addresses: "
                    << ex.what();
            }
        }
#endif

    }

    struct discover::impl {
        ba::io_service &ios_;
        impl(boost::asio::io_service &ios)
            :ios_(ios)
        { }

        ~impl(  )
        { }
    };

    discover::discover( boost::asio::io_service &ios )
        :impl_(new impl(ios))
    { }

    discover::~discover(  )
    {
        delete impl_;
    }

    discover::pinger_sptr discover::add_ping( const std::string &addr,
                                        int timeout,
                                        handler_type hdlr )
    {
        utilities::endpoint_info inf = utilities::get_endpoint_info( addr );

        if( !inf.is_ip( ) ) {
            throw std::runtime_error( std::string("Invalid address ") +
                                      addr );
        }

        sender_sptr s = vtrc::make_shared<sender>( vtrc::ref(impl_->ios_),
                                                   timeout, hdlr );
        s->ping( inf.addpess, inf.service  );

        return s;
    }

    discover::pinger_sptr discover::add_mcast( const std::string &addr,
                                               int timeout,
                                               handler_type hdlr )
    {
        return add_mcast( addr, timeout, hdlr, "" );
    }

    discover::pinger_sptr discover::add_mcast( const std::string &addr,
                                               int timeout,
                                               handler_type hdlr,
                                               const std::string &local_bind )
    {
        utilities::endpoint_info inf = utilities::get_endpoint_info( addr );

        if( !inf.is_ip( ) ) {
            throw std::runtime_error( std::string("Invalid address ") +
                                      addr );
        }

        sender_sptr s = vtrc::make_shared<sender>( vtrc::ref(impl_->ios_),
                                                   timeout, hdlr );

        s->mcast( inf.addpess, inf.service, local_bind );

        return s;
    }

}}}

