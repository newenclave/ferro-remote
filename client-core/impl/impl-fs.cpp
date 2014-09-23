
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

        typedef fproto::instance::Stub    stub_type;
        typedef vcomm::stub_wrapper<stub_type>  client_type;

        vtrc::uint32_t open_fs_inst( client_type &cl, const std::string &path )
        {
            fproto::handle_path req_res;
            req_res.set_path( path );

            cl.call( &stub_type::open, &req_res, &req_res );
            return req_res.hdl( ).value( );
        }

        struct fs_impl: public filesystem::iface {

            mutable client_type client_;
            std::string         path_;
            vtrc::uint32_t      hdl_;

            fs_impl( core::client_core &cl, const std::string &path )
                :client_(cl.create_channel( ), true)
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

        };
    }

    namespace filesystem {
        iface_ptr create( core::client_core &cl, const std::string &path )
        {
            return new fs_impl( cl, path );
        }

        directory_iterator_ptr directory_begin( core::client_core &cl,
                                                const std::string &path )
        {

        }

    }

}}}
