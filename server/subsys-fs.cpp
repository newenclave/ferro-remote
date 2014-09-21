
#include "subsys-fs.h"
#include "protocol/fs.pb.h"

#include "application.h"

#include "vtrc-stdint.h"
#include "files.h"

#include <fcntl.h>

#include "vtrc-common/vtrc-closure-holder.h"
#include "vtrc-common/vtrc-mutex-typedefs.h"

#include "vtrc-atomic.h"
#include "vtrc-common/vtrc-exception.h"

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

            static const struct {
                vtrc::uint32_t p;
                mode_t         n;
            } values_map[ ] = {
                { proto::fs::open_file_values::IRWXU, S_IRWXU },
                { proto::fs::open_file_values::IRUSR, S_IRUSR },
                { proto::fs::open_file_values::IWUSR, S_IWUSR },
                { proto::fs::open_file_values::IXUSR, S_IXUSR },
                { proto::fs::open_file_values::IRWXG, S_IRWXG },
                { proto::fs::open_file_values::IRGRP, S_IRGRP },
                { proto::fs::open_file_values::IWGRP, S_IWGRP },
                { proto::fs::open_file_values::IXGRP, S_IXGRP },
                { proto::fs::open_file_values::IRWXO, S_IRWXO },
                { proto::fs::open_file_values::IROTH, S_IROTH },
                { proto::fs::open_file_values::IWOTH, S_IWOTH },
                { proto::fs::open_file_values::IXOTH, S_IXOTH },
            };

            if( value & values_map[ 0].p ) { res |= values_map[ 0].n; }
            if( value & values_map[ 1].p ) { res |= values_map[ 1].n; }
            if( value & values_map[ 2].p ) { res |= values_map[ 2].n; }
            if( value & values_map[ 3].p ) { res |= values_map[ 3].n; }
            if( value & values_map[ 4].p ) { res |= values_map[ 4].n; }
            if( value & values_map[ 5].p ) { res |= values_map[ 5].n; }
            if( value & values_map[ 6].p ) { res |= values_map[ 6].n; }
            if( value & values_map[ 7].p ) { res |= values_map[ 7].n; }
            if( value & values_map[ 8].p ) { res |= values_map[ 8].n; }
            if( value & values_map[ 9].p ) { res |= values_map[ 9].n; }
            if( value & values_map[10].p ) { res |= values_map[10].n; }
            if( value & values_map[11].p ) { res |= values_map[11].n; }

            return res;
        }

        class proto_fs_impl: public fr::protocol::fs::instance {

        public:
            proto_fs_impl( fr::server::application * /*app*/,
                           vcomm::connection_iface_wptr /*cl*/ )
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

    
