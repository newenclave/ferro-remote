
#include "client-core/interfaces/IFs.h"

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

        };
    }

    namespace filesystem {
        iface_ptr create( core::client_core &cl, const std::string &path )
        {
            return new fs_impl( cl, path );
        }
    }

}}}
