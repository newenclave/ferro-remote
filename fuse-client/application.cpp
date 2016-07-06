#include <iostream>
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
#include "vtrc-common/vtrc-exception.h"
#include "vtrc-common/vtrc-hash-iface.h"

#include "vtrc-common/protocol/vtrc-errors.pb.h"

const std::string log_path = "/home/data/fuselog/";

static void log( const std::string &line )
{
    return;
    std::cerr << line << "\n";
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

    std::string hash_key( const std::string &id, const std::string &key )
    {
        std::string key_info( id + key );
        vcomm::hash_iface_uptr s(vcomm::hash::sha2::create256( ));
        std::string hs(s->get_data_hash( &key_info[0], key_info.size( ) ));
        return hs;
    }

    struct file_wrapper {
        file_iface::iface_ptr ptr_;
        off_t                 offset_;

        file_wrapper( )
            :offset_(0)
        { }

        ~file_wrapper( )
        {
            if( ptr_ ) delete ptr_;
        }
    };

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
            ,retry_timer_(pp_.get_rpc_service( ))
        {
            std::string id;
            std::string key;

            if( g_opts.count( "id" ) ) {
                id = g_opts["id"].as<std::string>( );
            }

            if( g_opts.count( "key" ) ) {
                key = g_opts["key"].as<std::string>( );
            }

            bool empty = key.empty( );
            key = hash_key( id, key );

            if( !id.empty( ) ) {
                client_.set_id( id );
            }

            if( !empty ) {
                client_.set_key( key );
            }
        }

        void start( )
        {
            mount_point_ = g_opts["point"].as<std::string>( );
            server_      = g_opts["server"].as<std::string>( );

            client_.on_disconnect_connect( [this]( ) {
                start_retry( );
            } );

//            client_.on_ready_connect( [this]( ) {
//                on_connect( );
//            } );

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

        void config_channel( vcomm::rpc_channel *channel )
        {
            /// avoiding exceptions
            channel->set_proto_error_callback(   &impl::proto_error   );
            channel->set_channel_error_callback( &impl::channel_error );
        }

        void on_connect(  )
        {
            log( std::string(__func__) + (local_result ? "1" : "0") );
            fs_.reset( fs_iface::create( client_, "" ) );

            log( std::string(__func__) + (local_result ? "-1" : "-0") );
            config_channel( fs_->channel( ) );
        }

        static void proto_error( unsigned code, unsigned cat, const char *mess )
        {
            std::ostringstream oss;
            oss << __func__  << " " << mess << " " << code << " " << cat;
            log( oss.str( ) );
            errno = code;
            local_result = -code;
        }

        static void channel_error( const char * )
        {
            log( std::string(__func__) );
            errno = EIO;
            local_result = -EIO;
        }

        static impl *imp( )
        {
            return g_app->impl_;
        }

        static fs_iface::iface *fs( )
        {
            return g_app->impl_->fs_.get( );
        }

        static bool cancel_error( unsigned code )
        {
            return  ( code == vtrc::rpc::errors::ERR_CANCELED )
                 || ( code == vtrc::rpc::errors::ERR_TIMEOUT )
                  ;
        }

        static int mknod(const char *path, mode_t m, dev_t d)
        {
            local_result = 0;
            if( !S_ISREG(m) ) {
                return -EOPNOTSUPP;
            }

            log( std::string( "mknode " ) + path );
            try {
                std::unique_ptr<file_iface::iface> f(
                    file_iface::create( imp( )->client_, path, O_CREAT, 0 ));
            } catch( const vtrc::common::exception &ex) {
                return - ex.code( );
            } catch( const std::exception & ) {
                return -1;
            }
            return local_result;
        }

        static int unlink(const char *path)
        {
            local_result = 0;
            imp( )->fs_->del( path );
            return local_result;
        }

        static int rmdir(const char *path)
        {
            local_result = 0;
            imp( )->fs_->remove_all( path );
            return local_result;
        }

        static int rename(const char *path, const char *newname )
        {
            local_result = 0;
            if( !fs( ) ) {
                return -EIO;
            }
            fs( )->rename( path, newname );
            return local_result;
        }

        static int mkdir(const char *path, mode_t /*mode*/)
        {
            local_result = 0;
            imp( )->fs_->mkdir( path );
            return local_result;
        }

        static int open(const char *path, struct fuse_file_info *inf)
        {
            local_result = 0;
            file_wrapper *fw = nullptr;
            try {
                file_iface::iface *ptr =
                      file_iface::create(imp( )->client_, path, inf->flags, 0);
                imp( )->config_channel( ptr->channel( ) );
                fw = new file_wrapper;
                fw->ptr_ = ptr;
            } catch( const vtrc::common::exception &ex) {
                ;;;
                return ex.code( );
            } catch( const std::exception & ) {
                ;;;
                return -1;
            }

            std::ostringstream oss;
            oss << "open file " << path << " " << inf->flags;
            log(oss.str( ));

            inf->fh = reinterpret_cast<decltype(inf->fh)>(fw);
            log( std::string( "open file " ) + path );
            return local_result;
        }

        static int release(const char *, struct fuse_file_info *fi)
        {
            local_result = 0;
            auto ptr = reinterpret_cast<file_wrapper *>(fi->fh);
            if( ptr ) {
                delete ptr;
            }
            return local_result;
        }

        static int read( const char */*path*/, char *buf, size_t len,
                         off_t off,
                         struct fuse_file_info *inf )
        {
            local_result = 0;
            size_t cur = 0;
            auto ptr = reinterpret_cast<file_wrapper *>(inf->fh);
            if( ptr ) {
                if( ptr->offset_ != off ) {
                    ptr->ptr_->seek( off, file_iface::POS_SEEK_SET );
                    if( local_result ) {
                        return local_result;
                    }
                    ptr->offset_ = off;
                }
                while( cur < len ) {
                    local_result = 0;
                    auto next = ptr->ptr_->read( buf + cur, len - cur );

                    if( 0 != local_result ) {
                        return local_result;
                    }

                    if( 0 == next ) {
                        return cur;
                    }
                    cur += next;
                    ptr->offset_ += next;
                }
            } else {
                return -1;
            }
            return (int)(cur);
        }

        static int flush( const char */*path*/, struct fuse_file_info *inf )
        {
            local_result = 0;
            auto ptr = reinterpret_cast<file_wrapper *>(inf->fh);
            if( ptr ) {
                ptr->ptr_->flush( );
            }
            return local_result;
        }

        static int write( const char */*path*/, const char *buf, size_t len,
                          off_t off,
                          struct fuse_file_info *inf )
        {
            local_result = 0;
            size_t cur = 0;
            auto ptr = reinterpret_cast<file_wrapper *>(inf->fh);
            if( ptr ) {
                if( ptr->offset_ != off ) {
                    ptr->ptr_->seek( off, file_iface::POS_SEEK_SET );
                    if( local_result ) {
                        return local_result;
                    }
                    ptr->offset_ = off;
                }
                while( cur < len ) {
                    local_result = 0;
                    auto next = ptr->ptr_->write( buf + cur, len - cur );
                    if( 0 != local_result ) {
                        return local_result;
                    }
                    if( 0 == next ) {
                        return cur;
                    }
                    cur += next;
                    ptr->offset_ += next;
                }
            } else {
                return -1;
            }
            return (int)(cur);
        }

        static int getattr( const char *path, struct stat *st )
        {
            local_result = 0;
            errno = 0;
            fs_iface::stat_data sd = {0};

            if( !fs( ) ) {
                return -EIO;
            }
//            st->st_dev     = 0;
//            st->st_ino     = 0;
//            st->st_mode    = 0;
//            st->st_nlink   = 0;
//            st->st_uid     = 0;
//            st->st_gid     = 0;
//            st->st_rdev    = 0;
//            st->st_size    = 0;
//            st->st_blksize = 0;
//            st->st_blocks  = 0;
//            st->st_atime   = 0;
//            st->st_mtime   = 0;
//            st->st_ctime   = 0;

            fs( )->stat( path, sd );

            if( !local_result ) {
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
            }

            log( std::string(__func__) + (local_result ? "1" : "0") );

            return local_result;
        }

        static int opendir(const char *path, struct fuse_file_info *info)
        {
            local_result = 0;
            if( !fs( ) ) {
                return -EIO;
            }
            dir_iterator *ptr = fs( )->begin_iterate( path );
            if( ptr ) {
                imp( )->config_channel( ptr->channel( ) );
            }
            info->fh = reinterpret_cast<decltype(info->fh)>(ptr);
            log( std::string(__func__) + (local_result ? "1" : "0") + path );
            return local_result;
        }

        static int releasedir( const char */*path*/,
                               fuse_file_info *info)
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

        static int readdir( const char */*path*/, void *buf,
                            fuse_fill_dir_t filer, off_t,
                            fuse_file_info *info)
        {
            local_result = 0;

            auto ptr = reinterpret_cast<dir_iterator *>(info->fh);

            while( !ptr->end( ) ) {
                local_result = 0;
                auto path = leaf(ptr->get( ).path);
                log( std::string(__func__) + " next " + path );
                if( filer( buf, path.c_str( ), NULL, 0 ) != 0 ) {
                    return -ENOMEM;
                }
                if( cancel_error( -local_result ) ) {
                    return local_result;
                }
                ptr->next( );
            }
            return local_result;
        }

        static int chmod( const char *, mode_t )
        {
            return 0;
        }

        static int chown( const char *, uid_t, gid_t )
        {
            return 0;
        }

        static int utime( const char *, struct utimbuf * )
        {
            return 0;
        }

        static int truncate(const char *, off_t)
        {
            return -ENOTSUP;
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
        impl_->client_.disconnect( );
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

        //res.setxattr
        res.rename          = &impl::rename;
        res.mkdir           = &impl::mkdir;
        res.rmdir           = &impl::rmdir;
        res.unlink          = &impl::unlink;
        res.mknod           = &impl::mknod;
        res.open            = &impl::open;
        res.release         = &impl::release;
        res.read            = &impl::read;
        res.write           = &impl::write;
        res.flush           = &impl::flush;

        res.truncate        = &impl::truncate;
        res.chown           = &impl::chown; /// fake
        res.chmod           = &impl::chmod; /// fake
        res.utime           = &impl::utime; /// fake

        return res;
    }

} }
