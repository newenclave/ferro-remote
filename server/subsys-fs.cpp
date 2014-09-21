
#include "subsys-fs.h"
#include "protocol/fs.pb.h"

#include "application.h"

#include "vtrc-stdint.h"
#include "files.h"

#include <fcntl.h>

namespace fr { namespace server { namespace subsys {

    namespace {

        namespace vcomm = vtrc::common;
        namespace proto = fr::protocol;

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

        const std::string subsys_name( "fs" );

        class proto_fs_impl: public fr::protocol::fs::instance {

        public:
            proto_fs_impl( fr::server::application *app,
                           vcomm::connection_iface_wptr )
            { }

            static const std::string &name( )
            {
                return fr::protocol::fs::instance::descriptor( )->full_name( );
            }
        };

        class proto_file_impl: public fr::protocol::fs::file {

        public:
            proto_file_impl( fr::server::application *app,
                           vcomm::connection_iface_wptr )
            { }

            static const std::string &name( )
            {
                return fr::protocol::fs::file::descriptor( )->full_name( );
            }
        };

        application::service_wrapper_sptr create_fs_inst(
                                          fr::server::application *,
                                          vtrc::common::connection_iface_wptr)
        {
            return application::service_wrapper_sptr( );
        }

        application::service_wrapper_sptr create_file_inst(
                                          fr::server::application *,
                                          vtrc::common::connection_iface_wptr)
        {
            return application::service_wrapper_sptr( );
        }

    }

    struct fs::impl {
        application *app_;
        impl( application *app )
            :app_(app)
        { }
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
        impl_->app_->register_service_creator(
                    proto_fs_impl::name( ), create_fs_inst );
        impl_->app_->register_service_creator(
                    proto_file_impl::name( ), create_file_inst );
    }

    void fs::stop( )
    {
        impl_->app_->unregister_service_creator( proto_fs_impl::name( ) );
        impl_->app_->unregister_service_creator( proto_file_impl::name( ) );
    }

}}}

    
