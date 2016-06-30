#include "application.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

#include "vtrc-memory.h"
#include "fr-client.h"
#include "vtrc-common/vtrc-pool-pair.h"

#include "boost/program_options.hpp"
#include "boost/system/error_code.hpp"

#include "interfaces/IFilesystem.h"
#include "interfaces/IFile.h"

#include "vtrc-common/vtrc-rpc-channel.h"
#include "vtrc-common/vtrc-delayed-call.h"


namespace fr { namespace fuse {

    application *g_app = nullptr;
    boost::program_options::variables_map g_opts;

    namespace interfaces  = fr::client::interfaces;
    namespace file_iface  = interfaces::file;
    namespace fs_iface    = interfaces::filesystem;

//    namespace vclient = vtrc::client;
    namespace vcomm   = vtrc::common;

    THREAD_LOCAL int local_result = 0;

    struct application::impl {

        vcomm::pool_pair                    pp_;
        client::core::client_core           client_;
        std::unique_ptr<fs_iface::iface>    fs_;
        vcomm::delayed_call                 retry_timer_;

        std::string mount_point_;
        std::string server_;

        impl( )
            :pp_(1, 1)
            ,client_(pp_)
            ,retry_timer_(pp_.get_io_service( ))
        {
            mount_point_ = g_opts["point"].as<std::string>( );
            server_      = g_opts["server"].as<std::string>( );

            client_.on_connect_connect( [this]( ) {
                on_connect( );
            } );
            try_connect( );
        }

        void retry_handle( const VTRC_SYSTEM::error_code &err )
        {
            if( !err ) {
                try_connect( );
            }
        }

        void try_connect( )
        {
            client_.async_connect( server_,
            [this]( const VTRC_SYSTEM::error_code &err ) {
                if( err ) {
                    return;
                }
            } );
        }

        void on_connect(  )
        {
            fs_.reset( fs_iface::create( client_, "/" ) );

            /// avaoid exceptions
            fs_->channel( )->set_proto_error_callback(   &impl::proto_error   );
            fs_->channel( )->set_channel_error_callback( &impl::channel_error );
        }

        static impl *imp( )
        {
            return g_app->impl_;
        }

        static fs_iface::iface *fs( )
        {
            return g_app->impl_->fs_.get( );
        }

        static void proto_error( unsigned code, unsigned, const char * )
        {
            errno = code;
            local_result = 1;
        }

        static void channel_error( const char * )
        {
            errno = EIO;
            local_result = 1;
        }


        static int getattr( const char *path, struct stat *st )
        {
            local_result = 0;
            fs_iface::stat_data sd;
            fs( )->stat( path, sd );

            st->st_dev     = sd.dev;
            st->st_ino     = sd.ino;
            st->st_mode    = sd.mode;
            st->st_nlink   = sd.nlink;
            st->st_uid     = sd.uid;
            st->st_gid     = sd.gid;
            st->st_rdev    = sd.rdev;
            st->st_size    = sd.size;
            st->st_blksize = sd.blksize;
            st->st_blocks  = sd.blocks;
            st->st_atime   = sd.atime;
            st->st_mtime   = sd.mtime;
            st->st_ctime   = sd.ctime;

            return local_result;
        }

        static void *init_app( )
        {
            g_app = g_app ? g_app : new application( );
            return static_cast<void *>(g_app);
        }

        static void destroy_app( void *app )
        {
            try {
                auto a = static_cast<application *>(app);
                a->stopall( );
            } catch( const std::exception &ex ) {
                ;;;;
            }
            delete static_cast<application *>(app);
        }

    };

    application::application( )
        :impl_(new impl)
    { }

    application::~application( )
    {
        delete impl_;
    }

    void application::stopall( )
    {
        impl_->pp_.stop_all( );
        impl_->pp_.join_all( );
    }

    fuse_operations application::set_operations( )
    {
        fuse_operations res = {0};
        res.init    = &impl::init_app;
        res.destroy = &impl::destroy_app;
        return res;
    }

} }
