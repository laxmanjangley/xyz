#include <iostream>
#include <thread>
#include <chrono>
#include <fstream>

#include <boost/make_shared.hpp>

#include <libtorrent/session.hpp>
#include <libtorrent/add_torrent_params.hpp>
#include <libtorrent/torrent_handle.hpp>
#include <libtorrent/alert_types.hpp>
#include <libtorrent/bencode.hpp>
#include "libtorrent/peer_info.hpp"
#include <libtorrent/torrent_status.hpp>
#include "libtorrent/torrent_info.hpp"
#include <libtorrent/magnet_uri.hpp>


namespace lt = libtorrent;
using clk = std::chrono::steady_clock;
using lt::torrent_handle;





// return the name of a torrent status enum
char const* state(lt::torrent_status::state_t s)
{
  switch(s) {
    case lt::torrent_status::checking_files: return "checking";
    case lt::torrent_status::downloading_metadata: return "dl metadata";
    case lt::torrent_status::downloading: return "downloading";
    case lt::torrent_status::finished: return "finished";
    case lt::torrent_status::seeding: return "seeding";
    case lt::torrent_status::allocating: return "allocating";
    case lt::torrent_status::checking_resume_data: return "checking resume";
    default: return "<>";
  }
}


std::string print_endpoint(lt::tcp::endpoint const& ep)
{
	using namespace lt;
	lt::error_code ec;
	char buf[200];
	address const& addr = ep.address();
	std::snprintf(buf, sizeof(buf), "%s:%d", addr.to_string(ec).c_str(), ep.port());
	return buf;
}


int main(int argc, char const* argv[])
{
  // if (argc != 2) {
  //   std::cerr << "usage: " << argv[0] << " <magnet-url>" << std::endl;
  //   return 1;
  // }
	std::cout<<argv[1]<<std::endl;
  lt::settings_pack pack;
  pack.set_int(lt::settings_pack::alert_mask
    , lt::alert::error_notification
    | lt::alert::storage_notification
    | lt::alert::status_notification);

  lt::error_code ec;
  lt::session ses(pack);
  lt::add_torrent_params atp;
   // lt::parse_magnet_uri(argv[1], atp, ec);
  clk::time_point last_save_resume = clk::now();
  // load resume data from disk and pass it in as we add the magnet link
  std::ifstream ifs(".r_file", std::ios_base::binary);
  ifs.unsetf(std::ios_base::skipws);
  // atp.resume_data.assign(std::istream_iterator<char>(ifs)
  //   , std::istream_iterator<char>());
  // atp.url = argv[1];
  atp.ti = boost::make_shared<lt::torrent_info>(std::string(argv[1]), boost::ref(ec), 0);
  atp.save_path = argv[2]; // save in current dir
  atp.flags |= lt::add_torrent_params::flag_seed_mode;
  ses.async_add_torrent(atp);
  lt::torrent_handle h;
  // this is the handle we'll set once we get the notification of it being
  // added

  std::vector<lt::peer_info> peers;

  
  for (;;) {
    std::vector<lt::alert*> alerts;
    ses.pop_alerts(&alerts);

    for (lt::alert const* a : alerts) {
    	//
    	std::cout<<a->message()<<std::endl; 
      if (auto at = lt::alert_cast<lt::add_torrent_alert>(a)) {
        h = at->handle;
        h.get_peer_info(peers);
      }
      // if we receive the finished alert or an error, we're done
      if (lt::alert_cast<lt::torrent_finished_alert>(a)) {
        h.save_resume_data();
        h.get_peer_info(peers);
        // goto done;
      }
      if (lt::alert_cast<lt::torrent_error_alert>(a)) {
        std::cout << a->message() << std::endl;
        goto done;
      }
      if(auto at = lt::alert_cast<lt::piece_finished_alert>(a)) {
      	std::cout<<"Piece finished : "<<at->piece_index<<std::endl;
      }
      // when resume data is ready, save it
      if (auto rd = lt::alert_cast<lt::save_resume_data_alert>(a)) {
        std::ofstream of(".r_file", std::ios_base::binary);
        of.unsetf(std::ios_base::skipws);
        lt::bencode(std::ostream_iterator<char>(of)
          , *rd->resume_data);
      }

      if (auto st = lt::alert_cast<lt::state_update_alert>(a)) {
      	// std::cout<<a->message()<<std::endl;
        if (st->status.empty()) continue;
        std::cout.flush();
        // we only have a single torrent, so we know which one
        // the status is for
        lt::torrent_status const& s = st->status[0];
        // s.state_t = static_cast<lt::torrent_status::state_t>(5);
        // std::type(s.state)
        // // s.state="downloading";
        std::cout << "\r" << state(s.state) << " "
          << (s.download_payload_rate / 1000) << " kB/s "
          << (s.total_done / 1000) << " kB ("
          << (s.progress_ppm / 10000) << "%) downloaded\x1b[K\n";
        std::cout.flush();
      }
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    if(!peers.empty()){
    	std::string out;
    	std::cout<<"peer info :";
    	char str[500];
    	for(auto peer : peers){
    		if (peer.flags & (lt::peer_info::handshake | lt::peer_info::connecting))
				continue;
			std::snprintf(str, sizeof(str), "%-30s ", (::print_endpoint(peer.ip) +
							(peer.flags & lt::peer_info::utp_socket ? " [uTP]" : "") +
							(peer.flags & lt::peer_info::i2p_socket ? " [i2p]" : "")
							).c_str());
						out += str;
			std::snprintf(str, sizeof(str), "Total download from peer : %lld", peer.total_download);
			out += str;
			std::cout<<out<<std::endl;
    	}
    }


    // save resume data once every 30 seconds
    if (clk::now() - last_save_resume > std::chrono::seconds(30)) {
      h.save_resume_data();
      last_save_resume = clk::now();
    }
  }

  // TODO: ideally we should save resume data here

done:
  std::cout << "\ndone, shutting down" << std::endl;
}