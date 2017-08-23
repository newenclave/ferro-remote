#include <iostream>
#include "application.h"

#include <mutex>
#include <memory>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <dirent.h>

#include "vtrc-memory.h"
#include "fr-client.h"
#include "vtrc/common/pool-pair.h"

#include "boost/system/error_code.hpp"

#include "interfaces/IFilesystem.h"
#include "interfaces/IFile.h"
#include "interfaces/ILogger.h"

#include "vtrc/common/rpc-channel.h"
#include "vtrc/common/delayed-call.h"
#include "vtrc/common/exception.h"
#include "vtrc/common/hash/iface.h"

#include "vtrc-common/protocol/vtrc-errors.pb.h"

#define FUSE_CALL_INIT \
    local_result = 0

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
    THREAD_LOCAL const char *local_message = "";

    bool g_log = false;

    const char *leaf( const std::string &path )
    {
        auto pos = path.find_last_of( '/' );
        if( pos != std::string::npos ) {
            return path.c_str( ) + pos + 1;
        }
        return path.c_str( );
    }

    std::string hash_key( const std::string &id, const std::string &key )
    {
        std::string key_info( id + key );
        vcomm::hash::iface::uptr s(vcomm::hash::sha2::create256( ));
        std::string hs(s->get_data_hash( &key_info[0], key_info.size( ) ));
        return hs;
    }

    struct file_wrapper {
        std::unique_ptr<file_iface::iface> ptr_;
        off_t                              offset_;

        file_wrapper( file_iface::iface *ptr )
            :ptr_(ptr)
            ,offset_(0)
        { }
    };

    struct directory_wrapper {
        std::unique_ptr<dir_iterator> ptr_;
        directory_wrapper( dir_iterator *ptr )
            :ptr_(ptr)
        { }
    };

    template <typename T>
    class locked_map {
        std::map<std::uint64_t, std::shared_ptr<T> > store_;
        mutable std::mutex                           store_lock_;

        using locker_type = std::lock_guard<std::mutex>;

    public:
        typedef std::shared_ptr<T> T_sptr;
        typedef std::uint64_t index_type;
        T_sptr get( index_type id )
        {
            locker_type lg(store_lock_);
            auto f = store_.find( id );
            if( f != store_.end( )) {
                return f->second;
            }
            return T_sptr( );
        }

        void store( index_type id, T_sptr val )
        {
            locker_type lg(store_lock_);
            store_[id] = val;
        }

        void erase( index_type id )
        {
            locker_type lg(store_lock_);
            store_.erase( id );
        }

        size_t size( ) const
        {
            locker_type lg(store_lock_);
            return store_.size( );
        }
    };

    using directory_map = locked_map<directory_wrapper>;
    using file_map      = locked_map<file_wrapper>;

    struct application::impl {

        vcomm::pool_pair                   pp_;
        client::core::client_core          client_;
        std::unique_ptr<fs_iface::iface>   fs_;
        vcomm::delayed_call                retry_timer_;

        std::string mount_point_;
        std::string server_;

        std::atomic<std::uint64_t>  index_;

        directory_map   dirs_;
        file_map        files_;

        impl( )
            :pp_(1, 1)
            ,client_(pp_)
            ,retry_timer_(pp_.get_rpc_service( ))
            ,index_(100)
        {
            std::string id;
            std::string key;

            if( g_opts.count( "id" ) ) {
                id = g_opts["id"].as<std::string>( );
            }

            if( g_opts.count( "key" ) ) {
                key = g_opts["key"].as<std::string>( );
            }

            g_log = g_opts.count( "debug" ) > 0;

            if( !id.empty( ) ) {
                client_.set_id( id );
            }

            if( !key.empty( ) ) {
                client_.set_key( hash_key( id, key ) );
            }
        }

        std::uint64_t next_id( )
        {
            return ++index_;
        }

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
                if( g_log ) {
                    std::cout << " >>>> try_connect exception: "
                              << ex.what( ) << std::endl;
                }
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
            fs_.reset( fs_iface::create( client_, "" ) );
            config_channel( fs_->channel( ) );
        }

        std::uint64_t create_file_impl( const char *path, int flags )
        {
            auto nid = next_id( );
            auto res = std::make_shared<file_wrapper>(
                        file_iface::create( client_, path, flags, 0 ) );

            config_channel( res->ptr_->channel( ) );
            files_.store( nid, res );
            return nid;
        }

        std::uint64_t create_dir_impl( const char *path )
        {
            FUSE_CALL_INIT;
            auto nid = next_id( );
            auto res = std::make_shared<directory_wrapper>(
                                                fs_->begin_iterate( path ) );
            if( 0 == local_result ) {
                config_channel( res->ptr_->channel( ) );
                dirs_.store( nid, res );
                return nid;
            }
            return 0;
        }

        static void proto_error( unsigned code, unsigned cat, const char *mess )
        {
            errno = code;
            local_result  = -code;
            local_message =  mess;
        }

        static void channel_error( const char *mess )
        {
            errno = EIO;
            local_result  = -EIO;
            local_message =  mess;
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
            FUSE_CALL_INIT;
            if( !S_ISREG(m) ) {
                return -EOPNOTSUPP;
            }

            try {
                std::unique_ptr<file_iface::iface> f(
                    file_iface::create( imp( )->client_, path, O_CREAT, 0 ));
            } catch( const vtrc::common::exception &ex) {
                if( g_log ) {
                    std::cout << " >>>> mknod vtrc exception: "
                              << ex.what( ) << std::endl;
                }
                return - ex.code( );
            } catch( const std::exception &ex ) {
                if( g_log ) {
                    std::cout << " >>>> mknod exception: "
                              << ex.what( ) << std::endl;
                }
                return -1;
            }
            return local_result;
        }

        static int unlink(const char *path)
        {
            FUSE_CALL_INIT;
            imp( )->fs_->del( path );
            return local_result;
        }

        static int rmdir(const char *path)
        {
            FUSE_CALL_INIT;
            imp( )->fs_->remove_all( path );
            return local_result;
        }

        static int rename( const char *path, const char *newname )
        {
            FUSE_CALL_INIT;
            if( !fs( ) ) {
                return -EIO;
            }
            fs( )->rename( path, newname );
            return local_result;
        }

        static int mkdir(const char *path, mode_t /*mode*/)
        {
            FUSE_CALL_INIT;
            imp( )->fs_->mkdir( path );
            return local_result;
        }

        static int open( const char *path, struct fuse_file_info *inf )
        {
            FUSE_CALL_INIT;
            std::uint64_t nid = 0;
            try {
                nid = imp( )->create_file_impl( path, inf->flags );
                if( g_log ) {
                    std::cout << " >>>> open file: " << path << std::endl;
                    std::cout << "\ttotal files: "
                              << imp( )->files_.size( ) << std::endl;
                }
            } catch( const vtrc::common::exception &ex) {
                if( g_log ) {
                    std::cout << " >>>> open file: " << path << std::endl;
                    std::cout << "\tFAILED: " << ex.what( )  << std::endl;
                }
                return - ex.code( );
            } catch( const std::exception &ex ) {
                if( g_log ) {
                    std::cout << " >>>> open file exception: "
                              << ex.what( ) << std::endl;
                }
                return -1;
            }

            inf->fh = nid;
            return local_result;
        }

        static int release(const char *path, struct fuse_file_info *fi)
        {
            FUSE_CALL_INIT;
            if( fi->fh ) {
                imp( )->files_.erase( fi->fh );
                if( g_log ) {
                    std::cout << " >>>> release file: " << path << std::endl;
                    std::cout << "\ttotal files: "
                              << imp( )->files_.size( ) << std::endl;
                }
            }
            return local_result;
        }

        template <typename Func, typename BufType>
        static int read_write_impl( Func call,
                                    const char *path, const char *func_name,
                                    BufType buf,
                                    size_t len, off_t off,
                                    struct fuse_file_info *inf )
        {
            FUSE_CALL_INIT;
            size_t cur = 0;
            auto ptr = imp( )->files_.get( inf->fh );
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
                    auto next = (*(ptr->ptr_).*call)( buf + cur, len - cur );

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
                if( g_log ) {
                    std::cout << " >>>> bad handle for file ("
                              << func_name << "): " << path << std::endl;
                }
                return -1;
            }
            return (int)(cur);
        }

        static int read( const char *path, char *buf, size_t len,
                         off_t off, struct fuse_file_info *inf )
        {

            return read_write_impl( &file_iface::iface::read, path, "read",
                                    buf, len, off, inf);
        }

        static int write( const char *path, const char *buf, size_t len,
                          off_t off, struct fuse_file_info *inf )
        {
            return read_write_impl( &file_iface::iface::write, path, "write",
                                    buf, len, off, inf);
        }

        static int flush( const char */*path*/, struct fuse_file_info *inf )
        {
            FUSE_CALL_INIT;
            auto ptr = imp( )->files_.get( inf->fh );
            if( ptr ) {
                ptr->ptr_->flush( );
            }
            return local_result;
        }


        static int getattr( const char *path, struct stat *st )
        {
            FUSE_CALL_INIT;
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

            return local_result;
        }

        static int opendir(const char *path, struct fuse_file_info *info)
        {
            FUSE_CALL_INIT;
            if( !fs( ) ) {
                return -EIO;
            }
            info->fh = imp( )->create_dir_impl( path );
            if( g_log ) {
                std::cout << " >>>> open dir: " << path << std::endl;
                std::cout << "\ttotal dirs: "
                          << imp( )->dirs_.size( ) << std::endl;
            }
            return local_result;
        }

        static int releasedir( const char *path,
                               fuse_file_info *info)
        {
            FUSE_CALL_INIT;
            if( info->fh ) {
                imp( )->dirs_.erase( info->fh );
                if( g_log ) {
                    std::cout << " >>>> release dir: " << path << std::endl;
                    std::cout << "\ttotal dirs: "
                              << imp( )->dirs_.size( ) << std::endl;
                }
            }
            return local_result;
        }

        static int readdir( const char */*path*/, void *buf,
                            fuse_fill_dir_t filer, off_t,
                            fuse_file_info *info)
        {
            FUSE_CALL_INIT;
            auto ptr_d = imp( )->dirs_.get( info->fh );
            if( !ptr_d ) {
                return -EBADF;
            }

            auto ptr = ptr_d->ptr_.get( );

            while( !ptr->end( ) ) {
                local_result = 0;
                auto path = leaf(ptr->get( ).path);
                if( filer( buf, path, NULL, 0 ) != 0 ) {
                    return -ENOMEM;
                }
                if( cancel_error( -local_result ) ) {
                    return local_result;
                }
                ptr->next( );
            }
            return local_result;
        }

        static int chmod( const char *path, mode_t mode )
        {
            FUSE_CALL_INIT;
            if( !fs( ) ) {
                return -EIO;
            }
            fs( )->chmod( path, mode );
            return local_result;
        }

        static int chown( const char *, uid_t, gid_t )
        {
            return 0;
        }

        static int utime( const char *path, struct utimbuf *buf )
        {
            FUSE_CALL_INIT;
            if( !fs( ) ) {
                return -EIO;
            }
            fs( )->update_time( path, buf->actime, buf->modtime );
            return local_result;
        }

        static int truncate( const char *path, off_t offset )
        {
            FUSE_CALL_INIT;
            if( !fs( ) ) {
                return -EIO;
            }
            fs( )->truncate( path, offset );
            return local_result;
        }

        static void *init_app( )
        {
            g_app = new application( );
            g_app->start( );
            return static_cast<void *>(g_app);
        }

        static void destroy_app( void *app )
        {
            try {
                auto a = static_cast<application *>(app);
                a->stopall( );
            } catch( const std::exception &ex ) {
                if( g_log ) {
                    std::cout << " >>>> destroy_app exception: "
                              << ex.what( ) << std::endl;
                }
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
        res.chmod           = &impl::chmod;
        res.utime           = &impl::utime;

        return res;
    }

} }
