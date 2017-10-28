#include "libtorrent/session.hpp"
#include "libtorrent/add_torrent_params.hpp"
#include "libtorrent/torrent_handle.hpp"

namespace lt = libtorrent;
int main(int argc, char const* argv[])
{
        if (argc != 2) {
                fprintf(stderr, "usage: <magnet-url>\n");
                return 1;
        }
        lt::session ses;

        lt::add_torrent_params atp;
        atp.url = argv[1];
        atp.save_path = "."; // save in current dir
        lt::torrent_handle h = ses.add_torrent(atp);

        // ...
}