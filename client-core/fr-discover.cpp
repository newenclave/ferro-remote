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
                              const udp_responce_info * )
        {
            if( !ec ) {
                return false;
            } else {
                return true;
            }
        }

        struct sender: public udp_pinger {

            typedef udp_pinger_sptr shared_type;
            typedef vtrc::weak_ptr<udp_pinger> weak_type;

            ba::io_service            &ios_;
            ba::ip::udp::socket       socket_;
            delayed_call              timeout_;
            int                       to_;
            ba::ip::udp::endpoint     from_;
            std::vector<char>         data_;
            std::string               mess_;
            udp_pinger::handler_type  handler_;
            std::string               local_bind_;

            sender( ba::io_service &ios )
                :ios_(ios)
                ,socket_(ios_)
                ,timeout_(ios_)
                ,to_(0)
                ,data_(4069)
                ,handler_(handler_default)
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
                                 ph::error,
                                 weak_type( shared_from_this( ) ) ) );
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
                        handler_( ecode, NULL );
                        return;
                    }
                }

                std::cout << socket_.local_endpoint( ).address( ).to_string( )
                          << ":" << socket_.local_endpoint( ).port( )
                          << std::endl;

                socket_.async_send_to( ba::buffer(mess_), ep,
                                       vtrc::bind( &sender::handle_send, this,
                                           ph::error,
                                           weak_type(shared_from_this( ) ) ) );
            }

            void handle_timeout( bs::error_code const &err, weak_type sdr )
            {
                if( !err ) {
                    shared_type l = sdr.lock( );
                    if( l ) {
                        socket_.close( );
                    }
                }
                //std::cerr << "Timeout!!! " << err.message( ) << "\n";
            }

            void handle_recv( bs::error_code const &err, size_t length,
                              weak_type sdr )
            {
                shared_type l = sdr.lock( );
                if( !l ) {
                    return;
                }

                if( err ) {
                    handler_( err, NULL );
                } else {
                    udp_responce_info inf;
                    inf.data    = &data_[0];
                    inf.length  = length;
                    inf.from    = &from_;

                    if( handler_( err, &inf ) ) {
                        handle_send( err, sdr );
                    }
                }
            }

            void handle_send( bs::error_code const &err, weak_type sdr )
            {
                shared_type l = sdr.lock( );
                if( !l ) {
                    return;
                }

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
                    handler_( err, NULL );
                }

            }

            void stop( ) override
            {
                socket_.close( );
            }

            void async_ping( const std::string &addr, int to, handler_type h )
            {
                if( socket_.is_open( ) ) {
                    socket_.close( );
                }

                utilities::endpoint_info inf =
                        utilities::get_endpoint_info( addr );

                if( !inf.is_ip( ) ) {
                    throw std::runtime_error( std::string("Invalid address ") +
                                              addr );
                }

                handler_ = h ? h : &handler_default;
                to_ = to;
                ping( inf.addpess, inf.service );

            }

            void async_ping( const std::string &addr, int to, handler_type h,
                             const std::string &local_bind )
            {
                if( socket_.is_open( ) ) {
                    socket_.close( );
                }

                utilities::endpoint_info inf =
                        utilities::get_endpoint_info( addr );

                if( !inf.is_ip( ) ) {
                    throw std::runtime_error( std::string("Invalid address ") +
                                              addr );
                }

                handler_ = h ? h : &handler_default;
                to_ = to;
                mcast( inf.addpess, inf.service, local_bind );
            }

        };

        typedef vtrc::shared_ptr<sender> sender_sptr;
        typedef std::map<size_t, sender_sptr> sender_map;
    }

    udp_pinger_sptr create_udp_pinger( boost::asio::io_service &ios )
    {
        //enum_local_addreses( );
        return vtrc::make_shared<sender>( vtrc::ref(ios) );
    }

}}}

