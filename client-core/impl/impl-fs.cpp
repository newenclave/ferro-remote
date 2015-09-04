
#include <stdexcept>

#include "client-core/interfaces/IFilesystem.h"

#include "protocol/fs.pb.h"

#include "client-core/fr-client.h"

#include "vtrc-common/vtrc-stub-wrapper.h"
#include "vtrc-common/vtrc-rpc-channel.h"
#include "vtrc-stdint.h"

namespace fr { namespace client { namespace interfaces {

    namespace {

        namespace vcomm  = vtrc::common;
        namespace fproto = proto::fs;

        typedef vcomm::rpc_channel                            channel_type;
        typedef fproto::instance::Stub                        stub_type;
        typedef vcomm::stub_wrapper<stub_type, channel_type>  client_type;

        const unsigned nw_flag = vcomm::rpc_channel::DISABLE_WAIT;

        void proto2info( fproto::element_info const &data,
                         filesystem::info_data      &info )
        {
            info.is_directory = data.is_directory( );
            info.is_empty     = data.is_empty( );
            info.is_exist     = data.is_exist( );
            info.is_regular   = data.is_regular( );
            info.is_symlink   = data.is_symlink( );
        }

        struct dir_iter_impl: public filesystem::directory_iterator_impl {

            core::client_core &core_;
            vtrc::shared_ptr<vcomm::rpc_channel> channel_;

            mutable client_type         client_;
            vtrc::uint32_t              hdl_;
            filesystem::iterator_value  val_;
            bool                        end_;
            filesystem::info_data       info_;

            dir_iter_impl( core::client_core &cl,
                           const vtrc::shared_ptr<vcomm::rpc_channel> &c,
                           const fproto::iterator_info &info )
                :core_(cl)
                ,channel_(c)
                ,client_(channel_)
                ,hdl_(info.hdl( ).value( ))
            {
                copy_data( info );
            }

            ~dir_iter_impl( )
            {
                try {
                    proto::handle req;
                    req.set_value( hdl_ );
                    client_.channel( )->set_flags( nw_flag );
                    client_.call_request( &stub_type::close, &req );
                } catch( ... ) { }
            }

            vtrc::common::rpc_channel *channel( )
            {
                return client_.channel( );
            }

            const vtrc::common::rpc_channel *channel( ) const
            {
                return client_.channel( );
            }

            void copy_data( const fproto::iterator_info &info )
            {
                val_.path   = info.path( );
                end_        = info.end( );
                proto2info( info.info( ), info_ );
            }

            directory_iterator_impl *clone( ) const
            {
                return clone_impl( );
            }

            dir_iter_impl *clone_impl( ) const
            {
                fproto::iterator_info req_res;
                client_.call( &stub_type::iter_clone, &req_res, &req_res );
                return new dir_iter_impl( core_, channel_, req_res );
            }

            void next( )
            {
                fproto::iterator_info req_res;
                req_res.mutable_hdl( )->set_value( hdl_ );
                client_.call( &stub_type::iter_next, &req_res, &req_res );
                copy_data( req_res );
            }

            bool end( ) const
            {
                return end_;
            }

            filesystem::iterator_value &get( )
            {
                return val_;
            }

            const filesystem::iterator_value &get( ) const
            {
                return val_;
            }

            const filesystem::info_data &info( ) const
            {
                return info_;
            }
        };
    }

namespace filesystem {

    typedef directory_iterator::value_type iterator_value_type;

    directory_iterator::directory_iterator(directory_iterator_impl *impl )
        :impl_(impl)
    { }

    directory_iterator::directory_iterator( )
        :impl_(NULL)
    { }

    directory_iterator::~directory_iterator( )
    {
        if( impl_ ) {
            delete impl_;
        }
    }

    directory_iterator&
            directory_iterator::operator = ( directory_iterator &other )
    {
        directory_iterator_impl * new_impl =
                        other.impl_ ? other.impl_->clone( ) : NULL;
        if( impl_ ) {
            delete impl_;
        }
        impl_ = new_impl;
        return *this;
    }

    directory_iterator& directory_iterator::operator++ ( )
    {
        impl_->next( );
        return *this;
    }

    directory_iterator& directory_iterator::operator++ ( int )
    {
        impl_->next( );
        return *this;
    }

    bool directory_iterator::operator == (const directory_iterator& r) const
    {
        if( NULL == r.impl_ ) {
            return impl_->end( );
        }  else if( impl_->end( ) ) {
            return r.impl_->end( );
        } else {
            return impl_->get( ) == r.impl_->get( );
        }
    }

    bool directory_iterator::operator != (const directory_iterator& r) const
    {
        return !(operator == ( r ));
    }

    const iterator_value_type& directory_iterator::operator *( ) const
    {
        if( !impl_ ) throw std::runtime_error( "Bad iterator" );
        return impl_->get( );
    }

    const iterator_value_type* directory_iterator::operator -> ( ) const
    {
        return &(operator *( ));
    }
}

    namespace {

        vtrc::uint32_t open_fs_inst( client_type &cl, const std::string &path )
        {
            fproto::handle_path req_res;
            req_res.set_path( path );

            cl.call( &stub_type::open, &req_res, &req_res );

            return req_res.hdl( ).value( );
        }

        struct fs_impl: public filesystem::iface {

            core::client_core   &core_;
            vtrc::shared_ptr<vcomm::rpc_channel> channel_;
            mutable client_type client_;
            std::string         path_;
            vtrc::uint32_t      hdl_;

            fs_impl( core::client_core &cl, const std::string &path )
                :core_(cl)
                ,channel_(cl.create_channel( ))
                ,client_(channel_)
                ,path_(path)
                ,hdl_(open_fs_inst(client_, path_))
            { }

            fs_impl( core::client_core &cl, vtrc::common::rpc_channel *chan,
                     const std::string &path )
                :core_(cl)
                ,channel_(chan)
                ,client_(channel_)
                ,path_(path)
                ,hdl_(open_fs_inst(client_, path_))
            { }

            ~fs_impl( )
            {
                try {
                    proto::handle req;
                    req.set_value( hdl_ );
                    client_.call_request( &stub_type::close, &req );

                    //handle_close_impl( core_, hdl_ );
                } catch( ... ) {
                    ;;;
                }
            }

            vtrc::common::rpc_channel *channel( ) override
            {
                return client_.channel( );
            }

            const vtrc::common::rpc_channel *channel( ) const override
            {
                return client_.channel( );
            }

            void fill_request( fproto::handle_path &req,
                               const std::string &path ) const
            {
                req.mutable_hdl( )->set_value( hdl_ );
                req.set_path( path );
            }

            /// calls
            ///
            bool exists( const std::string &path ) const override
            {
                fproto::handle_path     req;
                fproto::element_info    res;
                fill_request( req, path );
                client_.call( &stub_type::exists, &req, &res );
                return res.is_exist( );
            }

#define SET_STAT_FIELD( data, stat, field ) data.field = stat.field( )
            void stat( const std::string &path,
                       filesystem::stat_data &data ) const override
            {
                fproto::handle_path     req;
                fill_request( req, path );
                fproto::element_stat    res;
                client_.call( &stub_type::get_stat, &req, &res );

                SET_STAT_FIELD( data, res, dev     );
                SET_STAT_FIELD( data, res, ino     );
                SET_STAT_FIELD( data, res, mode    );
                SET_STAT_FIELD( data, res, nlink   );
                SET_STAT_FIELD( data, res, uid     );
                SET_STAT_FIELD( data, res, gid     );
                SET_STAT_FIELD( data, res, rdev    );
                SET_STAT_FIELD( data, res, size    );
                SET_STAT_FIELD( data, res, blksize );
                SET_STAT_FIELD( data, res, blocks  );
                SET_STAT_FIELD( data, res, atime   );
                SET_STAT_FIELD( data, res, mtime   );
                SET_STAT_FIELD( data, res, ctime   );

            }

            void info( const std::string &path,
                       filesystem::info_data &data ) const
            {
                fproto::handle_path     req;
                fill_request( req, path );
                fproto::element_info    res;
                client_.call( &stub_type::info, &req, &res );

                SET_STAT_FIELD( data, res, is_exist );
                SET_STAT_FIELD( data, res, is_directory );
                SET_STAT_FIELD( data, res, is_empty );
                SET_STAT_FIELD( data, res, is_regular );
                SET_STAT_FIELD( data, res, is_symlink );
            }
#undef SET_STAT_FIELD

            vtrc::uint64_t file_size( const std::string &path ) const override
            {
                fproto::handle_path   req;
                fill_request( req, path );
                fproto::file_position res;
                client_.call( &stub_type::file_size, &req, &res );
                return res.position( );
            }

            void rename( const std::string &old_path,
                         const std::string &new_path ) const
            {
                fproto::rename_req req;
                fill_request( *req.mutable_src(  ), old_path );
                fill_request( *req.mutable_dst(  ), new_path );
                client_.call_request( &stub_type::rename, &req );
            }

            void cd( const std::string &path ) override
            {
                fproto::handle_path   req;
                fill_request( req, path );
                client_.call( &stub_type::cd, &req, &req );
                path_.assign( req.path( ) );
            }

            const std::string &pwd(  ) const override
            {
                return path_;
            }

            void mkdir( const std::string &path ) const override
            {
                fproto::handle_path   req;
                fill_request( req, path );
                client_.call_request( &stub_type::mkdir, &req );
            }

            void del( const std::string &path ) const override
            {
                fproto::handle_path   req;
                fill_request( req, path );
                client_.call_request( &stub_type::del, &req );
            }

            void remove_all( const std::string &path )  const
            {
                fproto::handle_path   req;
                fill_request( req, path );
                client_.call_request( &stub_type::remove_all, &req );
            }

            filesystem::directory_iterator_impl *begin_iterate(
                                        const std::string &path ) const override
            {
                fproto::handle_path     req;
                fproto::iterator_info   res;
                req.mutable_hdl( )->set_value( hdl_ );
                req.set_path( path );

                client_.call( &stub_type::iter_begin, &req, &res );

                return new dir_iter_impl( core_, channel_, res );
            }


            filesystem::directory_iterator_impl *begin_iterate( ) const override
            {
                fproto::handle_path     req;
                fproto::iterator_info   res;
                req.mutable_hdl( )->set_value( hdl_ );

                client_.call( &stub_type::iter_begin, &req, &res );

                return new dir_iter_impl( core_, channel_, res );
            }

            filesystem::directory_iterator begin( ) const override
            {
                return filesystem::directory_iterator( begin_iterate( ) );
            }

            filesystem::directory_iterator end( ) const override
            {
                return filesystem::directory_iterator( );
            }

            size_t read_file( const std::string &path,
                              void *data, size_t max ) const override
            {
                fproto::read_file_req req;
                fproto::read_file_res res;
                req.mutable_dst( )->mutable_hdl( )->set_value( hdl_ );
                req.mutable_dst( )->set_path( path );
                req.set_len( max );
                client_.call( &stub_type::read_file, &req, &res );
                size_t result = res.data( ).size( );
                memcpy( data, res.data( ).c_str( ), result );
                return result;
            }

            size_t  write_file( const std::string &path,
                                const void *data, size_t max ) const override
            {
                fproto::write_file_req req;
                fproto::write_file_res res;
                req.mutable_dst( )->mutable_hdl( )->set_value( hdl_ );
                req.mutable_dst( )->set_path( path );
                req.set_data( data, max );
                client_.call( &stub_type::write_file, &req, &res );
                return res.len( );
            }

        };
    }

    namespace filesystem {
        iface_ptr create( core::client_core &cl, const std::string &path )
        {
            return new fs_impl( cl, path );
        }
    }

}}}
