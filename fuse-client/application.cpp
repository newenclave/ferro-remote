#include "application.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <dirent.h>

#include "vtrc-memory.h"
#include "fr-client.h"
#include "vtrc-common/vtrc-pool-pair.h"

#include "boost/system/error_code.hpp"

#include "interfaces/IFilesystem.h"
#include "interfaces/IFile.h"
#include "interfaces/ILogger.h"

#include "vtrc-common/vtrc-rpc-channel.h"
#include "vtrc-common/vtrc-delayed-call.h"


const std::string log_path = "/home/data/fuselog/";

static void log( const std::string &line )
{
    const size_t p = getpid( );
    std::ostringstream oss;
    oss << log_path << p;
    FILE *f = fopen( oss.str( ).c_str( ), "a+" );
    if( f ) {
        fwrite( line.c_str( ), 1, line.size( ), f );
        fwrite( "\n", 1, 1, f );
        fclose( f );
    }
}

namespace fr { namespace fuse {

    application *g_app = nullptr;
    boost::program_options::variables_map g_opts;

    namespace interfaces  = fr::client::interfaces;
    namespace file_iface  = interfaces::file;
    namespace fs_iface    = interfaces::filesystem;
    namespace log_iface   = interfaces::logger;


//    namespace vclient = vtrc::client;
    namespace vcomm   = vtrc::common;

    using dir_iterator  = fs_iface::directory_iterator_impl;
    using iterator_sptr = std::shared_ptr<fs_iface::directory_iterator_impl>;
    using iterator_map  = std::map<std::uint64_t, iterator_sptr>;

    THREAD_LOCAL int local_result = 0;

    std::string leaf( const std::string &path )
    {
        auto pos = path.find_last_of( '/' );
        if( pos != std::string::npos ) {
            return std::string( path.begin( ) + pos + 1, path.end( ) );
        }
        return path;
    }

    struct application::impl {

        vcomm::pool_pair                    pp_;
        client::core::client_core           client_;
        std::unique_ptr<fs_iface::iface>    fs_;
        vcomm::delayed_call                 retry_timer_;

        std::string mount_point_;
        std::string server_;

        std::atomic<uint64_t>               index_;

        impl( )
            :pp_(1, 1)
            ,client_(pp_)
            ,retry_timer_(pp_.get_rpc_service( ))
            ,index_(100)
        { }

        void start( )
        {
            mount_point_ = g_opts["point"].as<std::string>( );
            server_      = g_opts["server"].as<std::string>( );

            client_.on_disconnect_connect( [this]( ) {
                start_retry( );
            } );
            try_connect( );
        }

        void start_retry(  )
        {
            retry_timer_.call_from_now(
            [this]( const boost::system::error_code &e ) {
                if( !e ) {
                    try_connect( );
                }
            }, vcomm::delayed_call::seconds(1) );
        }

        void try_connect( )
        {
            try {
                client_.connect( server_ );
                on_connect( );
            } catch( const std::exception &ex ) {
                log( std::string( "retry: " ) + ex.what( ) );
                start_retry( );
            }
        }

        void on_connect(  )
        {
            log( std::string(__func__) + (local_result ? "1" : "0") );
            fs_.reset( fs_iface::create( client_, "" ) );

            /// avaoid exceptions
            log( std::string(__func__) + (local_result ? "-1" : "-0") );
            fs_->channel( )->set_proto_error_callback(   &impl::proto_error   );
            fs_->channel( )->set_channel_error_callback( &impl::channel_error );
        }


        static void proto_error( unsigned code, unsigned, const char *mess )
        {
            log( std::string(__func__) + "  " + mess);
            errno = code;
            local_result = 1;
        }

        static void channel_error( const char * )
        {
            log( std::string(__func__) );
            errno = EIO;
            local_result = 1;
        }

        uint64_t netx_id( )
        {
            return index_++;
        }

        static impl *imp( )
        {
            return g_app->impl_;
        }

        static fs_iface::iface *fs( )
        {
            return g_app->impl_->fs_.get( );
        }

        static int open(const char *path, struct fuse_file_info_compat *inf)
        {
            local_result = 0;
            file_iface::iface_ptr ptr =
                    file_iface::create( imp( )->client_, path, inf->flags, 0);
            inf->fh = reinterpret_cast<decltype(inf->fh)>(ptr);
            return local_result;
        }

        static int release(const char *, struct fuse_file_info *fi)
        {
            local_result = 0;
            auto ptr = reinterpret_cast<file_iface::iface_ptr>(fi->fh);
            if( ptr ) {
                delete ptr;
            }
            return local_result;
        }

        static int read(const char *, char *, size_t, off_t,
                    struct fuse_file_info_compat *)
        {
            local_result = 0;
            return local_result;
        }

        static int write(const char *, const char *, size_t, off_t,
                  struct fuse_file_info_compat *)
        {
            local_result = 0;
            return local_result;
        }


        static int getattr( const char *path, struct stat *st )
        {
            log( std::string(__func__) + (local_result ? " 1 " : " 0 ") + path );
            if( st ) {
                log("stat ok");
            }
            local_result = 0;
            fs_iface::stat_data sd = {0};

            fs( )->stat( path, sd );

            //stat( path, st );
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

            log( std::string(__func__) + (local_result ? "1" : "0") );

            return local_result;
        }

        static int opendir(const char *path, struct fuse_file_info_compat *info)
        {
            local_result = 0;
            dir_iterator *ptr = fs( )->begin_iterate( path );
            info->fh = reinterpret_cast<decltype(info->fh)>(ptr);
            log( std::string(__func__) + (local_result ? "1" : "0") + path );
            return local_result;
        }

        static int releasedir( const char *path,
                               fuse_file_info_compat *info)
        {
            local_result = 0;

            auto ptr = reinterpret_cast<dir_iterator *>(info->fh);
            if(ptr) {
                delete ptr;
            }
            //info->fh = 0;
            log( std::string(__func__) + (local_result ? "1" : "0") );
            return local_result;
        }

        static int readdir( const char *path, void *buf,
                            fuse_fill_dir_t filer, off_t,
                            fuse_file_info_compat *info)
        {
            local_result = 0;

            auto ptr = reinterpret_cast<dir_iterator *>(info->fh);

            while( !ptr->end( ) ) {
                auto path = leaf(ptr->get( ).path);
                log( std::string(__func__) + " next " + path );
                if(filer( buf, path.c_str( ), NULL, 0 ) != 0 ) {
                    return -ENOMEM;
                }
                ptr->next( );
            }
            return local_result;
        }


        static void *init_app( )
        {
            g_app = new application( );
            log( std::string(__func__) + (local_result ? " 1" : " 0") );
            g_app->start( );
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
            log( std::string(__func__) + (local_result ? "1" : "0") );
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
        impl_->retry_timer_.cancel( );
        impl_->pp_.stop_all( );
        impl_->pp_.join_all( );
    }

    void application::start( )
    {
        impl_->start( );
    }

    fuse_operations application::set_operations( )
    {
        fuse_operations res = {0};
        res.init            = &impl::init_app;
        res.destroy         = &impl::destroy_app;
        res.getattr         = &impl::getattr;
        res.opendir         = &impl::opendir;
        res.releasedir      = &impl::releasedir;
        res.readdir         = &impl::readdir;

        res.open            = &impl::open;
        res.release         = &impl::release;
        res.read            = &impl::read;
        res.write           = &impl::write;

        return res;
    }

} }
