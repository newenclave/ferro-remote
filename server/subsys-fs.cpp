
#include "subsys-fs.h"
#include "protocol/fs.pb.h"

#include "application.h"

#include "vtrc-stdint.h"
#include "files.h"

#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "vtrc-common/vtrc-closure-holder.h"
#include "vtrc-common/vtrc-mutex-typedefs.h"

#include "vtrc-atomic.h"
#include "vtrc-common/vtrc-exception.h"

#include "boost/filesystem.hpp"

namespace fr { namespace server { namespace subsys {

    namespace {

        const std::string subsys_name( "fs" );

        namespace vcomm = vtrc::common;
        namespace proto = fr::protocol;

        int flags_to_native( vtrc::uint32_t value )
        {
            int flags = 0;
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
            };

            static
            const size_t value_size = sizeof(values_map)/sizeof(values_map[0]);

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
                    res = S_IWOTH;
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


        class proto_fs_impl: public fr::protocol::fs::instance {

            path_map            path_;
            vtrc::shared_mutex  path_lock_;

            iterator_map        iters_;
            vtrc::shared_mutex  iters_lock_;

            vtrc::atomic<vtrc::uint32_t>  handle_;

            inline vtrc::uint32_t next_index( )
            {
                return ++handle_;
            }


            bfs::path path_from_request( const proto::fs::handle_path* request,
                                        vtrc::uint32_t &hdl )
            {
                bfs::path p(request->path( ));

                if( !request->has_handle( ) || p.is_absolute( ) ) {
                    /// ok. new instance requested
                    p.normalize( );
                    hdl = next_index( );

                } else {

                    /// old path must be used
                    hdl = request->handle( ).value( );

                    vtrc::shared_lock l( path_lock_ );
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

            void open(::google::protobuf::RpcController* /*controller*/,
                         const ::fr::protocol::fs::handle_path* request,
                         ::fr::protocol::fs::handle_path* response,
                         ::google::protobuf::Closure* done) override
            {
                vcomm::closure_holder holder(done);

                vtrc::uint32_t hdl;
                bfs::path p(path_from_request( request, hdl ));

                {
                    vtrc::unique_shared_lock l( path_lock_ );
                    path_.insert( std::make_pair( hdl, p ) );
                }

                response->mutable_handle( )->set_value( hdl );
                response->set_path( p.string( ) );

            }

            void cd(::google::protobuf::RpcController* /*controller*/,
                         const ::fr::protocol::fs::handle_path* request,
                         ::fr::protocol::fs::handle_path* response,
                         ::google::protobuf::Closure* done) override
            {
                vcomm::closure_holder holder(done);

                vtrc::upgradable_lock ul( path_lock_ );

                vtrc::uint32_t hdl( request->handle( ).value( ) );

                path_map::iterator f( path_.find( hdl ) );

                if( f == path_.end( ) ) {
                    vcomm::throw_system_error( EINVAL, "Bad fs handle" );
                }

                bfs::path p(f->second / request->path( ));
                p.normalize( );

                /// set new path
                vtrc::upgrade_to_unique utul( ul );
                f->second = p;
            }

            void pwd(::google::protobuf::RpcController* /*controller*/,
                         const ::fr::protocol::fs::handle_path* request,
                         ::fr::protocol::fs::handle_path* response,
                         ::google::protobuf::Closure* done) override
            {
                vcomm::closure_holder holder(done);
                vtrc::uint32_t hdl;
                bfs::path p(path_from_request( request, hdl ));
                response->set_path( p.string( ) );
            }

            void exists(::google::protobuf::RpcController* /*controller*/,
                         const ::fr::protocol::fs::handle_path* request,
                         ::fr::protocol::fs::element_info* response,
                         ::google::protobuf::Closure* done) override
            {
                vcomm::closure_holder holder(done);
                vtrc::uint32_t hdl;

                bfs::path p( path_from_request( request, hdl ) );
                response->set_is_exist( bfs::exists( p ) );
            }

            void file_size(::google::protobuf::RpcController* /*controller*/,
                         const ::fr::protocol::fs::handle_path* request,
                         ::fr::protocol::fs::file_position* response,
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
                    if( is_dir )
                        response->set_is_empty( bfs::is_empty( p ) );
                    else
                        response->set_is_empty(true);
                    response->set_is_regular( bfs::is_regular_file( p ) );
                }
            }

            void info(::google::protobuf::RpcController* /*controller*/,
                         const ::fr::protocol::fs::handle_path* request,
                         ::fr::protocol::fs::element_info* response,
                         ::google::protobuf::Closure* done) override
            {
                vcomm::closure_holder holder(done);
                vtrc::uint32_t hdl;
                bfs::path p( path_from_request( request, hdl ) );
                fill_info( p, response );
            }

            void get_stat(::google::protobuf::RpcController* /*controller*/,
                         const ::fr::protocol::fs::handle_path* request,
                         ::fr::protocol::fs::element_stat* response,
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
                         const ::fr::protocol::fs::handle_path* request,
                         ::fr::protocol::fs::handle_path* response,
                         ::google::protobuf::Closure* done) override
            {
                vcomm::closure_holder holder(done);
                vtrc::uint32_t hdl;
                bfs::path p( path_from_request( request, hdl ) );
                bfs::create_directories( p );
                response->mutable_handle( )->set_value( hdl );
            }

            void del(::google::protobuf::RpcController* /*controller*/,
                         const ::fr::protocol::fs::handle_path* request,
                         ::fr::protocol::fs::handle_path* response,
                         ::google::protobuf::Closure* done) override
            {
                vcomm::closure_holder holder(done);
                vtrc::uint32_t hdl = 0;
                bfs::path p(path_from_request( request, hdl ));
                bfs::remove( p );
                response->mutable_handle( )->set_value( hdl );
            }

            void fill_iter_info( const bfs::directory_iterator &iter,
                                 vtrc::uint32_t hdl,
                                 proto::fs::iterator_info* response)
            {
                response->mutable_handle( )->set_value( hdl );
                response->set_end( iter == bfs::directory_iterator( ));
                if( !response->end( ) ) {
                    response->set_path( iter->path( ).string( ) );
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
                vtrc::shared_lock l( iters_lock_ );
                return get_iter_unsafe( hdl );
            }

            void iter_begin(::google::protobuf::RpcController* /*controller*/,
                         const ::fr::protocol::fs::handle_path* request,
                         ::fr::protocol::fs::iterator_info* response,
                         ::google::protobuf::Closure* done) override
            {
                vcomm::closure_holder holder(done);

                vtrc::uint32_t hdl;
                bfs::path p(path_from_request( request, hdl ));

                bfs::directory_iterator new_iterator(p);
                vtrc::uint32_t iter_hdl = next_index( );

                vtrc::unique_shared_lock usl( iters_lock_);
                iters_.insert( std::make_pair( iter_hdl, new_iterator ) );
                fill_iter_info(new_iterator, iter_hdl, response);
            }

            void iter_next(::google::protobuf::RpcController* /*controller*/,
                         const ::fr::protocol::fs::iterator_info* request,
                         ::fr::protocol::fs::iterator_info* response,
                         ::google::protobuf::Closure* done) override
            {
                vcomm::closure_holder holder(done);

                vtrc::uint32_t hdl( request->handle( ).value( ) );
                vtrc::shared_lock usl( iters_lock_ );

                bfs::directory_iterator &iter( get_iter_unsafe( hdl ) );
                ++iter;
                fill_iter_info( iter, hdl, response );
            }

            void iter_info(::google::protobuf::RpcController* /*controller*/,
                         const ::fr::protocol::fs::iterator_info* request,
                         ::fr::protocol::fs::element_info* response,
                         ::google::protobuf::Closure* done) override
            {
                vcomm::closure_holder holder(done);
                vtrc::uint32_t hdl( request->handle( ).value( ) );
                bfs::directory_iterator iter( get_iter( hdl ) );
                fill_info( iter->path( ), response );
            }

            void iter_clone(::google::protobuf::RpcController* /*controller*/,
                         const ::fr::protocol::fs::iterator_info* request,
                         ::fr::protocol::fs::iterator_info* response,
                         ::google::protobuf::Closure* done) override
            {
                vcomm::closure_holder holder(done);

                vtrc::uint32_t hdl( request->handle( ).value( ) );
                bfs::directory_iterator iter(get_iter( hdl ));
                vtrc::uint32_t new_hdl = next_index( );
                fill_iter_info( iter, hdl, response );

                vtrc::unique_shared_lock usl( iters_lock_ );
                iters_.insert( std::make_pair( new_hdl, iter ) );
            }

            void close(::google::protobuf::RpcController*   /*controller*/,
                         const ::fr::protocol::fs::handle*    request,
                         ::fr::protocol::fs::empty*         /*response*/,
                         ::google::protobuf::Closure* done) override
            {
                vcomm::closure_holder holder(done);

                {
                    vtrc::upgradable_lock ul( path_lock_ );
                    path_map::iterator f( path_.find( request->value( ) ) );
                    if( f != path_.end( ) ) {
                        vtrc::upgrade_to_unique uul( ul );
                        path_.erase( f );
                        return;
                    }
                }

                {
                    vtrc::upgradable_lock ul( iters_lock_ );
                    iterator_map::iterator f( iters_.find( request->value( )));
                    if( f != iters_.end( ) ) {
                        vtrc::upgrade_to_unique uul( ul );
                        iters_.erase( f );
                    }
                }
            }

        public:
            proto_fs_impl( fr::server::application * /*app*/,
                           vcomm::connection_iface_wptr /*cl*/ )
                :handle_(100)
            { }

            static const std::string &name( )
            {
                return fr::protocol::fs::instance::descriptor( )->full_name( );
            }
        };

        class proto_file_impl: public fr::protocol::fs::file {

            typedef vtrc::shared_ptr<server::file_iface> file_sptr;
            typedef std::map<vtrc::uint32_t, file_sptr>  file_map;

            file_map            files_;
            vtrc::shared_mutex  files_lock_;

            vtrc::atomic<vtrc::uint32_t> index_;

        public:

            proto_file_impl( fr::server::application * /*app*/,
                             vcomm::connection_iface_wptr /*cli */)
                :index_(100)
            { }

            static const std::string &name( )
            {
                return fr::protocol::fs::file::descriptor( )->full_name( );
            }

            inline vtrc::uint32_t next_id( )
            {
                return ++index_;
            }

        private:

            void del_file( vtrc::uint32_t id )
            {
                vtrc::unique_shared_lock lck( files_lock_ );
                files_.erase( id );
            }

            vtrc::uint32_t add_file( file_sptr &f )
            {
                vtrc::uint32_t id = next_id( );
                vtrc::unique_shared_lock lck( files_lock_ );
                files_[id] = f;
                return id;
            }

            file_sptr get_file( vtrc::uint32_t id )
            {
                vtrc::shared_lock lck( files_lock_ );
                file_map::iterator f( files_.find(id) );
                if( f == files_.end( ) ) {
                    vcomm::throw_system_error( EBADF, "Bad file number." );
                }
                return f->second;
            }

            void open(::google::protobuf::RpcController* /*controller*/,
                         const ::fr::protocol::fs::file_open_req* request,
                         ::fr::protocol::fs::handle* response,
                         ::google::protobuf::Closure* done ) override
            {
                vcomm::closure_holder holder(done);

                mode_t mode =  mode_to_native(request->mode( ));
                int flags   = flags_to_native(request->flags( ));

                file_sptr new_file( (mode == 0)
                    ? fr::server::file::create( request->path( ), flags )
                    : fr::server::file::create( request->path( ), flags, mode )
                );
                response->set_value( add_file( new_file ) );
            }

            void tell(::google::protobuf::RpcController* /*controller*/,
                         const ::fr::protocol::fs::handle* request,
                         ::fr::protocol::fs::file_position* response,
                         ::google::protobuf::Closure* done) override
            {
                vcomm::closure_holder holder(done);
                file_sptr f(get_file( request->value( ) ));
                response->set_position( f->tell( ) );
            }

            static
            server::file_iface::seek_whence value_to_enum( vtrc::uint32_t v )
            {
                switch ( v ) {
                case proto::fs::POS_SEEK_CUR:
                    return server::file_iface::F_SEEK_CUR;
                case proto::fs::POS_SEEK_SET:
                    return server::file_iface::F_SEEK_SET;
                case proto::fs::POS_SEEK_END:
                    return server::file_iface::F_SEEK_END;
                }
                return server::file_iface::F_SEEK_SET;
            }

            void seek(::google::protobuf::RpcController* /*controller*/,
                         const ::fr::protocol::fs::file_set_position* request,
                         ::fr::protocol::fs::file_position* response,
                         ::google::protobuf::Closure* done) override
            {
                vcomm::closure_holder holder(done);
                file_sptr f(get_file( request->hdl( ).value( ) ));

                response->set_position( f->seek( request->position( ),
                                          value_to_enum(request->whence( )) ) );
            }

            void read(::google::protobuf::RpcController* /*controller*/,
                         const ::fr::protocol::fs::file_data_block* request,
                         ::fr::protocol::fs::file_data_block* response,
                         ::google::protobuf::Closure* done) override
            {
                vcomm::closure_holder holder(done);
                file_sptr f(get_file( request->hdl( ).value( ) ));
                size_t max_block = request->data( ).size( );

                if( max_block > 44000 ) { /// protocol violation
                    max_block = 44000;
                }

                if( max_block == 0 ) { /// nothing to do here
                    return;
                }

                std::vector<char> data(max_block);
                size_t result = f->read( &data[0], max_block );
                response->set_data( &data[0], result );

            }

            void write(::google::protobuf::RpcController* /*controller*/,
                         const ::fr::protocol::fs::file_data_block* request,
                         ::fr::protocol::fs::file_data_block* response,
                         ::google::protobuf::Closure* done) override
            {
                vcomm::closure_holder holder(done);
                file_sptr f(get_file( request->hdl( ).value( ) ));

                if( request->data( ).size( ) == 0 ) {
                    response->set_length( 0 );
                } else {
                    response->set_length( f->write( &request->data( )[0],
                                                 request->data( ).size( ) ) );
                }
            }

            void flush(::google::protobuf::RpcController* /*controller*/,
                         const ::fr::protocol::fs::handle* request,
                         ::fr::protocol::fs::empty* /*response*/,
                         ::google::protobuf::Closure* done) override
            {
                vcomm::closure_holder holder(done);
                file_sptr f(get_file( request->value( ) ));
                f->flush( );
            }

            void close(::google::protobuf::RpcController* controller,
                         const ::fr::protocol::fs::handle* request,
                         ::fr::protocol::fs::empty* /*response*/,
                         ::google::protobuf::Closure* done) override
            {
                vcomm::closure_holder holder(done);
                del_file( request->value( ) );
            }

        };

        application::service_wrapper_sptr create_fs_inst(
                                      fr::server::application *app,
                                      vtrc::common::connection_iface_wptr cl)
        {
            vtrc::shared_ptr<proto_fs_impl>
                    inst(vtrc::make_shared<proto_fs_impl>( app, cl ));

            return application::service_wrapper_sptr(
                        new application::service_wrapper( inst ) );
        }

        application::service_wrapper_sptr create_file_inst(
                                      fr::server::application *app,
                                      vtrc::common::connection_iface_wptr cl)
        {
            vtrc::shared_ptr<proto_file_impl>
                    inst(vtrc::make_shared<proto_file_impl>( app, cl ));
            return application::service_wrapper_sptr(
                        new application::service_wrapper( inst ) );
        }

    }

    struct fs::impl {
        application *app_;
        impl( application *app )
            :app_(app)
        { }

        void reg_creator( const std::string &name,
                          application::service_getter_type func )
        {
            app_->register_service_creator( name, func );
        }

        void unreg_creator( const std::string &name )
        {
            app_->unregister_service_creator( name );
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
        impl_->reg_creator( proto_fs_impl::name( ), create_fs_inst );
        impl_->reg_creator( proto_file_impl::name( ), create_file_inst );
    }

    void fs::stop( )
    {
        impl_->app_->unregister_service_creator( proto_fs_impl::name( ) );
        impl_->app_->unregister_service_creator( proto_file_impl::name( ) );
    }

}}}

    
