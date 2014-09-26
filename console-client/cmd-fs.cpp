#include <iostream>
#include <fstream>

#include "interfaces/IFile.h"
#include "interfaces/IFilesystem.h"

#include "command-iface.h"

#include "boost/program_options.hpp"

#include "interfaces/IOS.h"
#include "vtrc-common/vtrc-exception.h"
#include "vtrc-bind.h"

#ifdef _MSC_VER
#include <windows.h>
#define sleep_ Sleep /// milliseconds
#define MILLISECONDS( x ) x
#else
#include <unistd.h>
#define sleep_ usleep /// microseconds
#define MILLISECONDS( x ) ((x) * 1000)
#endif

namespace fr { namespace cc { namespace cmd {

    namespace {

        namespace po    = boost::program_options;
        namespace core  = client::core;
        namespace fs    = client::interfaces::filesystem;
        namespace fsf   = client::interfaces::file;

        const char *cmd_name = "fs";

        struct impl: public command_iface {

            const char *name( ) const
            {
                return cmd_name;
            }

            void event_cb( unsigned err, const std::string &data )
            {
                if( !err ) {
                    std::cout << "Got " << data.size( ) << " bytes of data\n";
                } else {
                    std::cout << "Got " << err << " as error\n";
                }
            }

            void download_file( const std::string &remote_path,
                                const std::string &local_path,
                                core::client_core &cl )
            {
                vtrc::unique_ptr<fsf::iface> impl(
                          fsf::create( cl, remote_path, fsf::flags::RDONLY ) );

                std::ofstream out(local_path, std::ofstream::out);
                std::vector<char> buf(4096);
                size_t total = 0;
                while( size_t r = impl->read( &buf[0], buf.size( ) ) ) {
                    out.write( &buf[0], r );
                    total += r;
                }
                std::cout << total << " bytes downloaded\n";
            }

            void upload_file( const std::string &local_path,
                              const std::string &remote_path,
                              core::client_core &cl )
            {
                vtrc::unique_ptr<fsf::iface> impl(
                          fsf::create( cl, remote_path,
                                    fsf::flags::WRONLY | fsf::flags::CREAT ) );

                std::ifstream inp( local_path );
                std::vector<char> buf(4096);
                size_t total = 0;
                while( size_t w = inp.readsome( &buf[0], buf.size( ) ) ) {
                    size_t pos = 0;
                    while( pos != w ) {
                        pos += impl->write( &buf[pos], w - pos );
                    }
                    total += w;
                }
                std::cout << total << " bytes uploaded\n";
            }

            void exec( po::variables_map &vm, core::client_core &cl )
            {
                if( vm.count( "list" ) ) {
                    std::string p(vm["list"].as<std::string>( ));
                    vtrc::unique_ptr<fs::iface> i( fs::create( cl, p ) );
                    for( auto &d: *i ) {
                        std::cout << d.path << "\n";
                    }
                    std::cout << "\n";
                }

                if( vm.count( "pull" ) ) {
                    std::string remote(vm["pull"].as<std::string>( ));
                    std::string o;
                    if( vm.count( "output" ) ) {
                        o = vm["output"].as<std::string>( );
                    }
                    download_file( remote, o, cl );

                } else if( vm.count( "push" ) ) {

                    std::string local(vm["push"].as<std::string>( ));
                    std::string o;
                    if( vm.count( "output" ) ) {
                        o = vm["output"].as<std::string>( );
                    }
                    upload_file( local, o, cl );
                }

                if( vm.count( "wait" ) ) {
                    std::string p(vm["wait"].as<std::string>( ));

                    unsigned to = vm.count( "timeout" )
                                ? vm["timeout"].as<unsigned>( )
                                : 0;

                    vtrc::unique_ptr<fsf::iface> i(
                                fsf::create( cl, p, fsf::flags::RDONLY ) );

                    fsf::file_event_callback cb(vtrc::bind(
                                                    &impl::event_cb, this,
                                                    vtrc::placeholders::_1,
                                                    vtrc::placeholders::_2 ));
                    i->register_for_events( cb );

                    sleep_( MILLISECONDS(to) * 1000 );
                }
            }

            void add_options( po::options_description &desc )
            {
                /// reserver as common
                /// "help,h"
                /// "command,c"
                /// "server,s"
                /// "io-pool-size,i"
                /// "rpc-pool-size,r"
                /// "only-pool,o"
                desc.add_options( )
                    ("list,l", po::value<std::string>( ), "show directory list")

                    ("pull,d", po::value<std::string>( ), "download file")
                    ("push,u", po::value<std::string>( ), "upload file")
                    ("output,O", po::value<std::string>( ), "output path/name")

                    ("wait,w", po::value<std::string>( ), "wait file event")
                    ("timeout,t", po::value<unsigned>( ), "timeout for 'wait'"
                                                          "; seconds")
                    ;
            }

            std::string help( ) const
            {
                return std::string( );
            }

            std::string desc( ) const
            {
                return std::string( "fs command" );
            }

        };
    }

    namespace fs {
        command_sptr create( )
        {
            return command_sptr( new impl );
        }
    }

}}}
