
#include "subsys-fs.h"
#include "protocol/fs.pb.h"

#include "application.h"

namespace fr { namespace server { namespace subsys {

    namespace {

        namespace vcomm = vtrc::common;

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

    
