
#include "client-core/interfaces/IFilesystem.h"

#include "protocol/fs.pb.h"

#include "client-core/fr-client.h"

#include "vtrc-common/vtrc-stub-wrapper.h"
#include "vtrc-common/vtrc-rpc-channel.h"
#include "vtrc-stdint.h"

namespace fr { namespace client { namespace interfaces {

    namespace {
        namespace vcomm = vtrc::common;
        namespace fproto = protocol::fs;

        typedef fproto::instance::Stub          stub_type;
        typedef vcomm::stub_wrapper<stub_type>  client_type;
    }

namespace filesystem {

    struct directory_iterator_impl {


        vtrc::shared_ptr<vcomm::rpc_channel> channel_;

        mutable client_type   client_;
        vtrc::uint32_t        hdl_;
        iterator_value        val_;
        bool                  end_;
        mutable info_data     info_;
        mutable bool          info_clean_;

        directory_iterator_impl( const vtrc::shared_ptr<vcomm::rpc_channel> &c,
              const fproto::iterator_info &info )
            :channel_(c)
            ,client_(channel_)
            ,hdl_(info.hdl( ).value( ))
            ,info_clean_(false)
        {
            copy_data( info );
        }

        void copy_data( const fproto::iterator_info &info )
        {
            val_.path   = info.path( );
            end_        = info.end( );
            info_clean_ = false;
        }

        directory_iterator_impl *clone( ) const
        {
            fproto::iterator_info req_res;
            client_.call( &stub_type::iter_clone, &req_res, &req_res );
            return new directory_iterator_impl( channel_, req_res );
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

        iterator_value &get( )
        {
            return val_;
        }

        const iterator_value &get( ) const
        {
            return val_;
        }

        const info_data &info( ) const
        {
            if( !info_clean_ ) {
                fproto::iterator_info req;
                fproto::element_info  res;
                client_.call( &stub_type::iter_info, &req, &res );
                info_.is_directory = res.is_directory( );
                info_.is_exist     = res.is_exist( );
                info_.is_empty     = res.is_empty( );
                info_.is_regular   = res.is_regular( );
                info_.is_symlink   = res.is_symlink( );
                info_clean_        = true;
            }
            return info_;
        }
    };

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
        return impl_->val_;
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

            vtrc::shared_ptr<vcomm::rpc_channel> channel_;
            mutable client_type client_;
            std::string         path_;
            vtrc::uint32_t      hdl_;

            fs_impl( core::client_core &cl, const std::string &path )
                :channel_(cl.create_channel( ))
                ,client_(channel_)
                ,path_(path)
                ,hdl_(open_fs_inst(client_, path_))
            { }

            ~fs_impl( ) try {

                fproto::handle req;
                req.set_value( hdl_ );
                client_.call_request( &stub_type::close, &req );

            } catch( ... ) { }

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

            filesystem::directory_iterator begin( ) const override
            {
                fproto::handle_path     req;
                fproto::iterator_info   res;
                req.mutable_hdl( )->set_value( hdl_ );

                client_.call( &stub_type::iter_begin, &req, &res );

                return filesystem::directory_iterator(
                   new filesystem::directory_iterator_impl( channel_, res ));
            }

            filesystem::directory_iterator end( ) const override
            {
                return filesystem::directory_iterator( );
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
