
#include "subsys-fs.h"
#include "protocol/fs.pb.h"
#include "protocol/ferro.pb.h"

#include "application.h"

#include "vtrc-stdint.h"
#include "files.h"

#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <utime.h>

#include "vtrc/common/closure-holder.h"
#include "vtrc/server/channels.h"
#include "vtrc/common/exception.h"

#include "vtrc-mutex.h"
#include "vtrc-atomic.h"

#include "boost/filesystem.hpp"

#include "subsys-reactor.h"
#include "subsys-logging.h"

#include "vtrc-bind.h"

#define LOG(lev) log_(lev) << "[fs] "
#define LOGINF   LOG(logger::level::info)
#define LOGDBG   LOG(logger::level::debug)
#define LOGERR   LOG(logger::level::error)
#define LOGWRN   LOG(logger::level::warning)

namespace fr { namespace agent { namespace subsys {

    namespace {

        const std::string subsys_name( "fs" );

        namespace vcomm = vtrc::common;
        namespace proto = fr::proto;

        int flags_to_native( vtrc::uint32_t value )
        {
            static const struct {
                vtrc::uint32_t p;
                mode_t         n;
            } values_map[ ] = {
                { proto::fs::open_file_values::RDONLY,   O_RDONLY   },
                { proto::fs::open_file_values::WRONLY,   O_WRONLY   },
                { proto::fs::open_file_values::RDWR,     O_RDWR     },
                { proto::fs::open_file_values::CREAT,    O_CREAT    },
                { proto::fs::open_file_values::EXCL,     O_EXCL     },
                { proto::fs::open_file_values::APPEND,   O_APPEND   },
                { proto::fs::open_file_values::NONBLOCK, O_NONBLOCK },
                { proto::fs::open_file_values::ASYNC,    O_ASYNC    },
                { proto::fs::open_file_values::SYNC,     O_SYNC     },
                { proto::fs::open_file_values::TRUNC,    O_TRUNC    }
            };

            static
            const size_t value_size = sizeof(values_map)/sizeof(values_map[0]);

            int flags = 0;

            for( size_t i=0; i<value_size; ++i ) {
                if( value & values_map[i].p ) {
                    flags |= values_map[i].n;
                }
            }

            return flags;
        }

        mode_t mode_to_native( vtrc::uint32_t value )
        {
            mode_t res = 0;

            switch( value ) {
                case 0:
                    res = 0; break;
                case proto::fs::open_file_values::IRWXU:
                    res = S_IRWXU; break;
                case proto::fs::open_file_values::IRUSR:
                    res = S_IRUSR; break;
                case proto::fs::open_file_values::IWUSR:
                    res = S_IWUSR; break;
                case proto::fs::open_file_values::IXUSR:
                    res = S_IXUSR; break;
                case proto::fs::open_file_values::IRWXG:
                    res = S_IRWXG; break;
                case proto::fs::open_file_values::IRGRP:
                    res = S_IRGRP; break;
                case proto::fs::open_file_values::IWGRP:
                    res = S_IWGRP; break;
                case proto::fs::open_file_values::IXGRP:
                    res = S_IXGRP; break;
                case proto::fs::open_file_values::IRWXO:
                    res = S_IRWXO; break;
                case proto::fs::open_file_values::IROTH:
                    res = S_IROTH; break;
                case proto::fs::open_file_values::IWOTH:
                    res = S_IWOTH; break;
                case proto::fs::open_file_values::IXOTH:
                    res = S_IXOTH; break;
                default:
                    vcomm::throw_system_error( EINVAL, "Bad mode value" );
            }
            return res;
        }

        namespace bfs = boost::filesystem;

        typedef std::map<vtrc::uint32_t, bfs::path>               path_map;
        typedef std::map<vtrc::uint32_t, bfs::directory_iterator> iterator_map;

        typedef proto::events::Stub stub_type;

        using vtrc::server::channels::unicast::create_event_channel;

        typedef vtrc::shared_ptr<vcomm::rpc_channel> rpc_channel_sptr;

        typedef vtrc::shared_ptr<agent::file_iface> file_sptr;
        typedef vtrc::weak_ptr<agent::file_iface>   file_wptr;
        typedef std::map<vtrc::uint32_t, file_sptr> file_map;

        agent::file_iface::seek_whence value_to_enum( vtrc::uint32_t v )
        {
            switch ( v ) {
            case proto::fs::POS_SEEK_CUR:
            case proto::fs::POS_SEEK_SET:
            case proto::fs::POS_SEEK_END:
                return static_cast<agent::file_iface::seek_whence>(v);
            }
            return agent::file_iface::F_SEEK_SET;
        }

        class proto_fs_impl: public fr::proto::fs::instance {

            path_map            path_;
            vtrc::mutex  path_lock_;

            iterator_map        iters_;
            vtrc::mutex         iters_lock_;

            subsys::reactor    &reactor_;

            typedef vtrc::lock_guard<vtrc::mutex> locker_type;

            inline vtrc::uint32_t next_index( )
            {
                return reactor_.next_op_id( );
            }

            bfs::path path_from_request( const proto::fs::handle_path* request,
                                         vtrc::uint32_t &hdl )
            {
                bfs::path p(request->path( ));

                if( !request->has_hdl( ) || p.is_absolute( ) ) {
                    /// ok. new instance requested
                    p.normalize( );
                    hdl = next_index( );

                } else {

                    /// old path must be used
                    hdl = request->hdl( ).value( );

                    locker_type l( path_lock_ );
                    path_map::const_iterator f( path_.find( hdl ) );

                    if( f == path_.end( ) ) {
                        vcomm::throw_system_error( EINVAL, "Bad fs handle" );
                    }
                    p = f->second;
                    p /= request->path( );
                    p.normalize( );
                }
                return p;
            }

            void open( ::google::protobuf::RpcController* /*controller*/,
                       const ::fr::proto::fs::handle_path* request,
                       ::fr::proto::fs::handle_path* response,
                       ::google::protobuf::Closure* done) override
            {
                vcomm::closure_holder holder(done);

                vtrc::uint32_t hdl;
                bfs::path p(path_from_request( request, hdl ));
                {
                    locker_type l( path_lock_ );
                    path_.insert( std::make_pair( hdl, p ) );
                }
                response->mutable_hdl( )->set_value( hdl );
                response->set_path( p.string( ) );
            }

            void cd(::google::protobuf::RpcController* /*controller*/,
                         const ::fr::proto::fs::handle_path* request,
                         ::fr::proto::fs::handle_path* response,
                         ::google::protobuf::Closure* done) override
            {
                vcomm::closure_holder holder(done);

                locker_type ul( path_lock_ );

                vtrc::uint32_t hdl( request->hdl( ).value( ) );

                path_map::iterator f( path_.find( hdl ) );

                if( f == path_.end( ) ) {
                    vcomm::throw_system_error( EINVAL, "Bad fs handle" );
                }

                bfs::path req( request->path( ) );
                bfs::path p;
                if( req.is_absolute( ) ) {
                    p = req;
                } else {
                    p = f->second / request->path( );
                }

                p.normalize( );
                response->set_path( p.string( ) );
                /// set new path
                f->second = p;
            }

            void pwd(::google::protobuf::RpcController* /*controller*/,
                         const ::fr::proto::fs::handle_path* request,
                         ::fr::proto::fs::handle_path* response,
                         ::google::protobuf::Closure* done) override
            {
                vcomm::closure_holder holder(done);
                vtrc::uint32_t hdl;
                bfs::path p(path_from_request( request, hdl ));
                response->set_path( p.string( ) );
            }

            void exists(::google::protobuf::RpcController* /*controller*/,
                         const ::fr::proto::fs::handle_path* request,
                         ::fr::proto::fs::element_info* response,
                         ::google::protobuf::Closure* done) override
            {
                vcomm::closure_holder holder(done);
                vtrc::uint32_t hdl;

                bfs::path p( path_from_request( request, hdl ) );
                response->set_is_exist( bfs::exists( p ) );
            }

            void file_size(::google::protobuf::RpcController* /*controller*/,
                         const ::fr::proto::fs::handle_path* request,
                         ::fr::proto::fs::file_position* response,
                         ::google::protobuf::Closure* done) override
            {
                vcomm::closure_holder holder(done);

                vtrc::uint32_t hdl;
                bfs::path p( path_from_request( request, hdl ) );

                response->set_position( bfs::file_size( p ) );
            }

            void fill_info( bfs::path const &p,
                            proto::fs::element_info* response )
            {
                bool is_exists = bfs::exists( p );
                response->set_is_exist( is_exists );
                if( is_exists ) {
                    bool is_dir = bfs::is_directory( p );
                    response->set_is_symlink( bfs::is_symlink( p ) );
                    response->set_is_directory( is_dir );
                    response->set_is_empty( true );

                    if( is_dir ) try {
                        response->set_is_empty( bfs::is_empty( p ) );
                    } catch( ... ) { ;;; }

                    response->set_is_regular( bfs::is_regular_file( p ) );
                }
            }

            void info( ::google::protobuf::RpcController* /*controller*/,
                       const ::fr::proto::fs::handle_path* request,
                       ::fr::proto::fs::element_info* response,
                       ::google::protobuf::Closure* done) override
            {
                vcomm::closure_holder holder(done);
                vtrc::uint32_t hdl;
                bfs::path p( path_from_request( request, hdl ) );
                fill_info( p, response );
            }

            void get_stat( ::google::protobuf::RpcController* /*controller*/,
                           const ::fr::proto::fs::handle_path* request,
                           ::fr::proto::fs::element_stat*      response,
                           ::google::protobuf::Closure* done) override
            {
                vcomm::closure_holder holder(done);
                vtrc::uint32_t hdl;
                bfs::path p( path_from_request( request, hdl ) );

                struct stat ss = { 0 };
                int res = ::stat( p.string( ).c_str( ), &ss );

                if( -1 == res ) {
                    vcomm::throw_system_error( errno, "::stat" );
                }

                response->set_atime(    ss.st_atime     );
                response->set_mtime(    ss.st_mtime     );
                response->set_ctime(    ss.st_ctime     );
                response->set_blocks(   ss.st_blocks    );
                response->set_blksize(  ss.st_blksize   );
                response->set_size(     ss.st_size      );
                response->set_rdev(     ss.st_rdev      );
                response->set_gid(      ss.st_gid       );
                response->set_uid(      ss.st_uid       );
                response->set_nlink(    ss.st_nlink     );
                response->set_mode(     ss.st_mode      );
                response->set_ino(      ss.st_ino       );
                response->set_dev(      ss.st_dev       );

            }

            void mkdir(::google::protobuf::RpcController* /*controller*/,
                         const ::fr::proto::fs::handle_path* request,
                         ::fr::proto::fs::handle_path* response,
                         ::google::protobuf::Closure* done) override
            {
                vcomm::closure_holder holder(done);
                vtrc::uint32_t hdl;
                bfs::path p( path_from_request( request, hdl ) );
                bfs::create_directories( p );
                response->mutable_hdl( )->set_value( hdl );
            }

            void rename(::google::protobuf::RpcController* /*controller*/,
                         const ::fr::proto::fs::rename_req* request,
                         ::fr::proto::fs::handle_path* response,
                         ::google::protobuf::Closure* done) override
            {
                vcomm::closure_holder holder(done);
                vtrc::uint32_t shdl;
                vtrc::uint32_t dhdl;

                bfs::path s( path_from_request( &request->src( ), shdl ) );
                bfs::path d( path_from_request( &request->dst( ), dhdl ) );

                bfs::rename( s, d );
                response->mutable_hdl( )->set_value( dhdl );

            }

            void del(::google::protobuf::RpcController* /*controller*/,
                         const ::fr::proto::fs::handle_path* request,
                         ::fr::proto::fs::handle_path* response,
                         ::google::protobuf::Closure* done) override
            {
                vcomm::closure_holder holder(done);
                vtrc::uint32_t hdl = 0;
                bfs::path p(path_from_request( request, hdl ));
                bfs::remove( p );
                response->mutable_hdl( )->set_value( hdl );
            }

            void remove_all(::google::protobuf::RpcController* /*controller*/,
                         const ::fr::proto::fs::handle_path* request,
                         ::fr::proto::fs::handle_path* response,
                         ::google::protobuf::Closure* done) override
            {
                vcomm::closure_holder holder(done);
                vtrc::uint32_t hdl = 0;
                bfs::path p(path_from_request( request, hdl ));
                bfs::remove_all( p );
                response->mutable_hdl( )->set_value( hdl );
            }

            void fill_iter_info( const bfs::directory_iterator &iter,
                                 vtrc::uint32_t hdl,
                                 proto::fs::iterator_info* response)
            {
                response->mutable_hdl( )->set_value( hdl );
                response->set_end( iter == bfs::directory_iterator( ));
                if( !response->end( ) ) {
                    std::string path( iter->path( ).string( ) );
                    response->set_path( path );
                    fill_info( iter->path( ), response->mutable_info( ) );
                }
            }

            bfs::directory_iterator &get_iter_unsafe( vtrc::uint32_t hdl )
            {
                iterator_map::iterator f( iters_.find( hdl ) );
                if( f == iters_.end( ) ) {
                    vcomm::throw_system_error( EINVAL, "Bad iterator handle" );
                }
                return f->second;
            }

            bfs::directory_iterator &get_iter( vtrc::uint32_t hdl )
            {
                locker_type l( iters_lock_ );
                return get_iter_unsafe( hdl );
            }

            void iter_begin(::google::protobuf::RpcController* /*controller*/,
                         const ::fr::proto::fs::handle_path* request,
                         ::fr::proto::fs::iterator_info* response,
                         ::google::protobuf::Closure* done) override
            {
                vcomm::closure_holder holder(done);

                vtrc::uint32_t hdl;
                bfs::path p(path_from_request( request, hdl ));

                bfs::directory_iterator new_iterator(p);
                vtrc::uint32_t iter_hdl = next_index( );

                locker_type usl( iters_lock_);
                iters_.insert( std::make_pair( iter_hdl, new_iterator ) );
                fill_iter_info( new_iterator, iter_hdl, response );
            }

            void iter_next(::google::protobuf::RpcController* /*controller*/,
                         const ::fr::proto::fs::iterator_info* request,
                         ::fr::proto::fs::iterator_info* response,
                         ::google::protobuf::Closure* done) override
            {
                vcomm::closure_holder holder(done);

                vtrc::uint32_t hdl( request->hdl( ).value( ) );
                locker_type usl( iters_lock_ );

                bfs::directory_iterator &iter( get_iter_unsafe( hdl ) );
                if( iter != bfs::directory_iterator( ) ) {
                    ++iter;
                    fill_iter_info( iter, hdl, response );
                } else {
                    vcomm::throw_system_error( ENODATA, "End iterator." );
                }
            }

            void iter_info(::google::protobuf::RpcController* /*controller*/,
                         const ::fr::proto::fs::iterator_info* request,
                         ::fr::proto::fs::element_info* response,
                         ::google::protobuf::Closure* done) override
            {
                vcomm::closure_holder holder(done);
                vtrc::uint32_t hdl( request->hdl( ).value( ) );
                bfs::directory_iterator iter( get_iter( hdl ) );
                fill_info( iter->path( ), response );
            }

            void iter_clone(::google::protobuf::RpcController* /*controller*/,
                         const ::fr::proto::fs::iterator_info* request,
                         ::fr::proto::fs::iterator_info* response,
                         ::google::protobuf::Closure* done) override
            {
                vcomm::closure_holder holder(done);

                vtrc::uint32_t hdl( request->hdl( ).value( ) );
                bfs::directory_iterator iter(get_iter( hdl ));
                vtrc::uint32_t new_hdl = next_index( );
                fill_iter_info( iter, hdl, response );

                locker_type usl( iters_lock_ );
                iters_.insert( std::make_pair( new_hdl, iter ) );
            }

            void close(::google::protobuf::RpcController*   /*controller*/,
                         const ::fr::proto::handle*         request,
                         ::fr::proto::empty*                /*response*/,
                         ::google::protobuf::Closure* done) override
            {
                vcomm::closure_holder holder(done);

                {
                    locker_type ul( path_lock_ );
                    path_map::iterator f( path_.find( request->value( ) ) );
                    if( f != path_.end( ) ) {
                        path_.erase( f );
                        return;
                    }
                }

                {
                    locker_type ul( iters_lock_ );
                    iterator_map::iterator f( iters_.find( request->value( )));
                    if( f != iters_.end( ) ) {
                        iters_.erase( f );
                    }
                }
            }

            void truncate( ::google::protobuf::RpcController* /*controller*/,
                           const ::fr::proto::fs::truncate_req* request,
                           ::fr::proto::empty* /*response*/,
                           ::google::protobuf::Closure* done) override
            {
                vcomm::closure_holder holder(done);
                vtrc::uint32_t dhdl;
                bfs::path p( path_from_request( &request->src( ), dhdl ) );
                bfs::resize_file( p, request->off( ) );
            }

            void update_time( ::google::protobuf::RpcController* /*controller*/,
                              const ::fr::proto::fs::update_time_req* request,
                              ::fr::proto::empty* /*response*/,
                              ::google::protobuf::Closure* done) override
            {
                vcomm::closure_holder holder(done);
                vtrc::uint32_t dhdl;
                bfs::path p( path_from_request( &request->src( ), dhdl ) );

                utimbuf times = { (__time_t)request->actime( ),
                                  (__time_t)request->modtime( ) };

                int res = ::utime( p.string( ).c_str( ), &times );
                if( -1 == res ) {
                    vcomm::throw_system_error( errno, "utime failed" );
                }
                //bfs::last_write_time(  )
            }

            void chmod( ::google::protobuf::RpcController* /*controller*/,
                        const ::fr::proto::fs::chmod_req* request,
                        ::fr::proto::empty* /*response*/,
                        ::google::protobuf::Closure* done) override
            {
                vcomm::closure_holder holder(done);
                vtrc::uint32_t dhdl;
                bfs::path p( path_from_request( &request->src( ), dhdl ) );
                int res = ::chmod( p.string( ).c_str( ), request->mode( ) );
                if( -1 == res ) {
                    vcomm::throw_system_error( errno, "utime failed" );
                }
            }


            void read_file(::google::protobuf::RpcController* /*controller*/,
                         const ::fr::proto::fs::read_file_req* request,
                         ::fr::proto::fs::read_file_res* response,
                         ::google::protobuf::Closure* done) override
            {
                vcomm::closure_holder holder(done);
                size_t len = request->len( ) > 44000 ? 44000 : request->len( );
                if( 0 == len ) {
                    return;
                }

                vtrc::uint32_t dhdl;
                bfs::path p( path_from_request( &request->dst( ), dhdl ) );

                std::vector<char> data(len);

                std::unique_ptr<file_iface> f( file::create(
                                                   p.string( ), O_RDONLY ) );
                if( request->has_from( ) ) {
                    f->seek( request->from( ).position( ),
                             value_to_enum(request->from( ).whence( )) );
                }
                size_t r = f->read( &data[0], len );
                response->set_data( &data[0], r );
            }

            void write_file(::google::protobuf::RpcController*  /*controller*/,
                         const ::fr::proto::fs::write_file_req* request,
                         ::fr::proto::fs::write_file_res* response,
                         ::google::protobuf::Closure* done) override
            {
                vcomm::closure_holder holder(done);
                size_t len = request->data( ).size( ) > 44000
                           ? 44000
                           : request->data( ).size( );

                vtrc::uint32_t dhdl;
                bfs::path p( path_from_request( &request->dst( ), dhdl ) );


                int flags = O_WRONLY | O_CREAT;
                mode_t mode = S_IRUSR | S_IWUSR;

                std::unique_ptr<file_iface> f ( file::create( p.string( ),
                                                flags, mode ) );

                if( request->has_from( ) ) {
                    f->seek( request->from( ).position( ),
                             value_to_enum(request->from( ).whence( )) );
                }

                response->set_len( f->write( request->data( ).c_str( ), len ) );
            }

        public:
            proto_fs_impl( fr::agent::application * app,
                           vcomm::connection_iface_wptr /*cl*/ )
                :reactor_(app->subsystem<subsys::reactor>( ))
            { }

            static const std::string &name( )
            {
                return fr::proto::fs::instance::descriptor( )->full_name( );
            }
        };

        class proto_file_impl: public fr::proto::fs::file {

            vcomm::connection_iface_wptr  client_;
            file_map                      files_;
            vtrc::mutex                   files_lock_;

            subsys::reactor              &reactor_;

            rpc_channel_sptr             event_channel_;
            stub_type                    events_;

        public:

            typedef vtrc::lock_guard<vtrc::mutex> locker_type;

            proto_file_impl( fr::agent::application *app,
                             vcomm::connection_iface_wptr &cli)
                :client_(cli)
                ,reactor_(app->subsystem<subsys::reactor>( ))
                ,event_channel_(create_event_channel(client_.lock( )))
                ,events_(event_channel_.get( ))
            { }

            ~proto_file_impl( )
            {
                try {
                    destroy_all( );
                } catch( ... ) { }
            }

            void destroy_all( )
            {
                locker_type lck( files_lock_ );
                for( file_map::iterator b(files_.begin( )), e(files_.end( ));
                     b!=e; ++b)
                {
                    reactor_.del_fd( b->second->handle( ) );
                }
            }

            static const std::string &name( )
            {
                return fr::proto::fs::file::descriptor( )->full_name( );
            }

            inline vtrc::uint32_t next_id( )
            {
                return reactor_.next_op_id( );
            }

        private:

            void del_file( vtrc::uint32_t id )
            {
                locker_type lck( files_lock_ );
                files_.erase( id );
            }

            vtrc::uint32_t add_file( file_sptr &f )
            {
                vtrc::uint32_t id = next_id( );
                locker_type lck( files_lock_ );
                files_[id] = f;
                return id;
            }

            file_sptr get_file( vtrc::uint32_t id )
            {
                locker_type lck( files_lock_ );
                file_map::iterator f( files_.find(id) );
                if( f == files_.end( ) ) {
                    vcomm::throw_system_error( EBADF, "Bad file number." );
                }
                return f->second;
            }

            void open( ::google::protobuf::RpcController* /*controller*/,
                       const ::fr::proto::fs::file_open_req* request,
                       ::fr::proto::handle*                  response,
                       ::google::protobuf::Closure* done ) override
            {
                vcomm::closure_holder holder(done);

                mode_t mode =  mode_to_native(request->mode( ));
                int   flags = flags_to_native(request->flags( ));

                std::string fmode( request->strmode( ) );

                if( (flags & O_CREAT) && (0 == mode) ) {
                    mode = S_IRUSR | S_IWUSR;
                }

                file_sptr new_file;

                namespace afile     = fr::agent::file;
                namespace adevice   = fr::agent::device;

                if( request->has_strmode( ) ) { // fopen

                    new_file.reset( request->as_device( )
                        ? adevice::create( request->path( ), fmode )
                        : afile::create( request->path( ), fmode ) );

                } else if( request->as_device( ) ) { // open

                    new_file.reset( (mode == 0)
                        ? adevice::create( request->path( ), flags )
                        : adevice::create( request->path( ), flags, mode )
                    );

                } else { // open

                    new_file.reset( (mode == 0)
                        ? afile::create( request->path( ), flags )
                        : afile::create( request->path( ), flags, mode )
                    );

                }

                response->set_value( add_file( new_file ) );
            }

            void tell(::google::protobuf::RpcController* /*controller*/,
                         const ::fr::proto::handle*      request,
                         ::fr::proto::fs::file_position* response,
                         ::google::protobuf::Closure* done) override
            {
                vcomm::closure_holder holder(done);
                file_sptr f(get_file( request->value( ) ));
                response->set_position( f->tell( ) );
            }

            void seek(::google::protobuf::RpcController* /*controller*/,
                         const ::fr::proto::fs::file_set_position* request,
                         ::fr::proto::fs::file_position* response,
                         ::google::protobuf::Closure* done) override
            {
                vcomm::closure_holder holder(done);
                file_sptr f(get_file( request->hdl( ).value( ) ));

                response->set_position( f->seek( request->position( ),
                                          value_to_enum(request->whence( )) ) );
            }

            void ioctl(::google::protobuf::RpcController* controller,
                         const ::fr::proto::fs::ioctl_req* request,
                         ::fr::proto::fs::ioctl_res* response,
                         ::google::protobuf::Closure* done) override
            {
                vcomm::closure_holder holder(done);
                file_sptr f(get_file( request->hdl( ).value( ) ));
                f->ioctl( request->code( ),
                          static_cast<unsigned long>( request->parameter( ) ) );

            }

            void ioctl_ptr(::google::protobuf::RpcController* controller,
                         const ::fr::proto::fs::ioctl_req* request,
                         ::fr::proto::fs::ioctl_res* response,
                         ::google::protobuf::Closure* done) override
            {
                vcomm::closure_holder holder(done);
                file_sptr f(get_file( request->hdl( ).value( ) ));

                response->mutable_ptr_param( )->assign( request->ptr_param( ) );
                auto data = request->ptr_param( ).empty( )
                          ? nullptr
                          : &(*response->mutable_ptr_param( ))[0];
                f->ioctl( request->code( ), data );
            }

            void read(::google::protobuf::RpcController* /*controller*/,
                         const ::fr::proto::fs::file_data_block* request,
                         ::fr::proto::fs::file_data_block* response,
                         ::google::protobuf::Closure* done) override
            {
                vcomm::closure_holder holder(done);

                file_sptr f(get_file( request->hdl( ).value( ) ));
                size_t max_block = request->length( );

                vtrc::uint64_t pos = 0;

                if( request->has_cust_pos( ) ) {
                    pos = f->tell( );
                    f->seek( request->cust_pos( ).position( ),
                             value_to_enum(request->cust_pos( ).whence( )) );
                }

                if( max_block > 44000 ) { /// protocol violation
                    max_block = 44000;
                }

                if( max_block == 0 ) { /// nothing to do here
                    return;
                }

                std::vector<char> data(max_block);
                size_t result = f->read( &data[0], max_block );

                if( request->cust_pos( ).set_back( ) ) {
                    f->seek( pos, agent::file_iface::F_SEEK_SET );
                }

                response->set_data( &data[0], result );
            }

            void write(::google::protobuf::RpcController* /*controller*/,
                         const ::fr::proto::fs::file_data_block* request,
                         ::fr::proto::fs::file_data_block* response,
                         ::google::protobuf::Closure* done) override
            {
                vcomm::closure_holder holder(done);
                file_sptr f(get_file( request->hdl( ).value( ) ));

                if( request->data( ).size( ) == 0 ) {
                    response->set_length( 0 );
                } else {

                    vtrc::uint64_t position = 0;

                    if( request->has_cust_pos( ) ) {
                        position = f->tell( );
                        f->seek( request->cust_pos( ).position( ),
                                 value_to_enum(request->cust_pos( ).whence( )));
                    }

                    size_t total = request->data( ).size( );
                    size_t pos = 0;

                    while ( pos != total ) {
                        pos += f->write( &request->data( )[pos], total - pos);
                    }

                    if( request->cust_pos( ).set_back( ) ) {
                        f->seek( position, agent::file_iface::F_SEEK_SET );
                    }

                    response->set_length( total );
                }
            }

            void flush(::google::protobuf::RpcController* /*controller*/,
                         const ::fr::proto::handle*       request,
                         ::fr::proto::empty*              /*response*/,
                         ::google::protobuf::Closure* done) override
            {
                vcomm::closure_holder holder(done);
                file_sptr f(get_file( request->value( ) ));
                f->flush( );
            }

            struct value_data {
                file_wptr        inst;
                int              fd;
                subsys::reactor *reactor;
                size_t           op;
                value_data( file_sptr &f, subsys::reactor *r, size_t o )
                    :inst(f)
                    ,fd(f->handle( ))
                    ,reactor(r)
                    ,op(o)
                { }
            };

            bool event_handler( unsigned /*events*/,
                                std::uint64_t tick_count,
                                value_data &data,
                                vcomm::connection_iface_wptr cli )
            { try {

                vcomm::connection_iface_sptr lock(cli.lock( ));

                if( !lock ) {
                    data.reactor->del_fd( data.fd );
                    return false;
                }

                proto::async_op_data req;
                req.set_id( data.op );

                std::vector<char> buf(1024);
                int res = ::read( data.fd, &buf[0], buf.size( ) );
                req.set_tick_count( tick_count );

                if( -1 == res ) {
                    req.mutable_error( )->set_code( errno );
                    req.mutable_error( )->set_text( strerror(errno) );
                } else {
                    req.set_data( &buf[0], res );
                }

                events_.async_op( NULL, &req, NULL, NULL );
                return true;

            } catch( ... ) {
                return false;
            } }

            void register_for_events(::google::protobuf::RpcController* ,
                         const ::fr::proto::fs::register_req* request,
                         ::fr::proto::fs::register_res* response,
                         ::google::protobuf::Closure* done) override
            {
                namespace ph = vtrc::placeholders;

                vcomm::closure_holder holder(done);
                file_sptr f(get_file( request->hdl( ).value( ) ));

                size_t op_id(reactor_.next_op_id( ));

                agent::reaction_callback
                        cb( vtrc::bind( &proto_file_impl::event_handler, this,
                                         ph::_1, ph::_2,
                                         value_data( f, &reactor_, op_id),
                                         client_) );
                reactor_.add_fd( f->handle( ),
                                 EPOLLIN | EPOLLET | EPOLLPRI, cb );

                response->set_async_op_id( op_id );
            }

            void unregister(::google::protobuf::RpcController* /*controller*/,
                         const ::fr::proto::fs::register_req* request,
                         ::fr::proto::empty*                   /*response*/,
                         ::google::protobuf::Closure* done) override
            {
                vcomm::closure_holder holder(done);
                file_sptr f(get_file( request->hdl( ).value( ) ));
                reactor_.del_fd( f->handle( ) );
            }

            void close(::google::protobuf::RpcController* /*controller*/,
                         const ::fr::proto::handle*       request,
                         ::fr::proto::empty*              /*response*/,
                         ::google::protobuf::Closure* done) override
            {
                vcomm::closure_holder holder(done);
                locker_type lck( files_lock_ );
                file_map::iterator f(files_.find( request->value( ) ));
                if( f != files_.end( ) ) {
                    reactor_.del_fd( f->second->handle( ) );
                    files_.erase( f );
                }
            }

        };

        application::service_wrapper_sptr create_fs_inst(
                                      fr::agent::application *app,
                                      vtrc::common::connection_iface_wptr cl)
        {
            auto inst(vtrc::make_shared<proto_fs_impl>( app, cl ));
            return app->wrap_service( cl, inst );
        }

        application::service_wrapper_sptr create_file_inst(
                                      fr::agent::application *app,
                                      vtrc::common::connection_iface_wptr cl)
        {
            auto inst(vtrc::make_shared<proto_file_impl>( app, cl ) );
            return app->wrap_service( cl, inst );
        }

    }

    struct fs::impl {

        application     *app_;
        logger          &log_;
        impl( application *app )
            :app_(app)
            ,log_(app_->get_logger( ))
        {

        }

        void reg_creator( const std::string &name,
                          application::service_getter_type func )
        {
            app_->register_service_factory( name, func );
        }

        void unreg_creator( const std::string &name )
        {
            app_->unregister_service_factory( name );
        }
    };


    fs::fs( application *app )
        :impl_(new impl(app))
    { }

    fs::~fs( )
    {
        delete impl_;
    }

    /// static
    vtrc::shared_ptr<fs> fs::create( application *app )
    {
        vtrc::shared_ptr<fs> new_inst(new fs(app));
        return new_inst;
    }

    const std::string &fs::name( )  const
    {
        return subsys_name;
    }

    void fs::init( )
    {

    }

    void fs::start( )
    {
        impl_->reg_creator( proto_fs_impl::name( ),   create_fs_inst );
        impl_->reg_creator( proto_file_impl::name( ), create_file_inst );
        impl_->LOGINF << "Started.";

    }

    void fs::stop( )
    {
        impl_->unreg_creator( proto_fs_impl::name( ) );
        impl_->unreg_creator( proto_file_impl::name( ) );
        impl_->LOGINF << "Stopped.";
    }

}}}


