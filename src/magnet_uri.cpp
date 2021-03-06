#include "libtorrent/entry.hpp"
#include "libtorrent/bencode.hpp"
#include "libtorrent/torrent_info.hpp"
#include "libtorrent/file.hpp"
#include "libtorrent/storage.hpp"
#include "libtorrent/hasher.hpp"
#include "libtorrent/create_torrent.hpp"
#include "libtorrent/file.hpp"
#include "libtorrent/file_pool.hpp"
#include "libtorrent/magnet_uri.hpp"
#include "libtorrent/hex.hpp" // for from_hex

#include <boost/bind.hpp>
#include <fstream>

#ifdef TORRENT_WINDOWS
#include <direct.h> // for _getcwd
#endif

using namespace libtorrent;

std::vector<char> load_file(std::string const& filename)
{
  std::vector<char> ret;
  std::fstream in;
  in.exceptions(std::ifstream::failbit);
  in.open(filename.c_str(), std::ios_base::in | std::ios_base::binary);
  in.seekg(0, std::ios_base::end);
  size_t const size = in.tellg();
  in.seekg(0, std::ios_base::beg);
  ret.resize(size);
  in.read(ret.data(), ret.size());
  return ret;
}

std::string branch_path(std::string const& f)
{
  if (f.empty()) return f;

#ifdef TORRENT_WINDOWS
  if (f == "\\\\") return "";
#endif
  if (f == "/") return "";

  int len = f.size();
  // if the last character is / or \ ignore it
  if (f[len-1] == '/' || f[len-1] == '\\') --len;
  while (len > 0)
  {
    --len;
    if (f[len] == '/' || f[len] == '\\')
      break;
  }

  if (f[len] == '/' || f[len] == '\\') ++len;
  return std::string(f.c_str(), len);
}

// do not include files and folders whose
// name starts with a .
bool file_filter(std::string const& f)
{
  if (f.empty()) return false;

  char const* first = f.c_str();
  char const* sep = strrchr(first, '/');
#if defined(TORRENT_WINDOWS) || defined(TORRENT_OS2)
  char const* altsep = strrchr(first, '\\');
  if (sep == NULL || altsep > sep) sep = altsep;
#endif
  // if there is no parent path, just set 'sep'
  // to point to the filename.
  // if there is a parent path, skip the '/' character
  if (sep == NULL) sep = first;
  else ++sep;

  // return false if the first character of the filename is a .
  if (sep[0] == '.') return false;

  fprintf(stderr, "%s\n", f.c_str());
  return true;
}

void print_progress(int i, int num)
{
  fprintf(stderr, "\r%d/%d", i+1, num);
}

void print_usage()
{
  return;
}

int main(int argc, char* argv[])
{
  using namespace libtorrent;

  std::string creator_str = "libtorrent";
  std::string comment_str;

  if (argc < 2)
  {
    print_usage();
    return 1;
  }

#ifndef BOOST_NO_EXCEPTIONS
  try
  {
#endif
    std::vector<std::string> web_seeds;
    std::vector<std::string> trackers;
    std::vector<std::string> collections;
    std::vector<sha1_hash> similar;
    int pad_file_limit = -1;
    int piece_size = 0;
    int flags = 0;
    std::string root_cert;

    std::string outfile;
    std::string merklefile;
#ifdef TORRENT_WINDOWS
    // don't ever write binary data to the console on windows
    // it will just be interpreted as text and corrupted
    outfile = "a.torrent";
#endif

    for (int i = 2; i < argc; ++i)
    {
      if (argv[i][0] != '-')
      {
        print_usage();
        return 1;
      }

      switch (argv[i][1])
      {
        case 'w':
          ++i;
          web_seeds.push_back(argv[i]);
          break;
        case 't':
          ++i;
          trackers.push_back(argv[i]);
          break;
        case 'M':
          flags |= create_torrent::mutable_torrent_support;
          pad_file_limit = 0x4000;
          break;
        case 'p':
          ++i;
          pad_file_limit = atoi(argv[i]);
          flags |= create_torrent::optimize_alignment;
          break;
        case 's':
          ++i;
          piece_size = atoi(argv[i]);
          break;
        case 'm':
          ++i;
          merklefile = argv[i];
          flags |= create_torrent::merkle;
          break;
        case 'o':
          ++i;
          outfile = argv[i];
          break;
        case 'l':
          flags |= create_torrent::symlinks;
          break;
        case 'C':
          ++i;
          creator_str = argv[i];
          break;
        case 'c':
          ++i;
          comment_str = argv[i];
          break;
        case 'r':
          ++i;
          root_cert = argv[i];
          break;
        case 'S':
          {
          ++i;
          if (strlen(argv[i]) != 40)
          {
            fprintf(stderr, "invalid info-hash for -S. "
              "Expected 40 hex characters\n");
            print_usage();
            return 1;
          }
          sha1_hash ih;
          if (!from_hex(argv[i], 40, (char*)&ih[0]))
          {
            fprintf(stderr, "invalid info-hash for -S\n");
            print_usage();
            return 1;
          }
          similar.push_back(ih);
          }
          break;
        case 'L':
          ++i;
          collections.push_back(argv[i]);
          break;
        default:
          print_usage();
          return 1;
      }
    }

    file_storage fs;
    std::string full_path = argv[1];
#ifdef TORRENT_WINDOWS
    if (full_path[1] != ':')
#else
    if (full_path[0] != '/')
#endif
    {
      char cwd[TORRENT_MAX_PATH];
#ifdef TORRENT_WINDOWS
      _getcwd(cwd, sizeof(cwd));
      full_path = cwd + ("\\" + full_path);
#else
      getcwd(cwd, sizeof(cwd));
      full_path = cwd + ("/" + full_path);
#endif
    }

    add_files(fs, full_path, file_filter, flags);
    if (fs.num_files() == 0)
    {
      fputs("no files specified.\n", stderr);
      return 1;
    }

    create_torrent t(fs, piece_size, pad_file_limit, flags);
    int tier = 0;
    // trackers.push_back("http://tracker.opentrackr.org:1337/announce");
    for (std::vector<std::string>::iterator i = trackers.begin()
      , end(trackers.end()); i != end; ++i, ++tier)
      t.add_tracker(*i, tier);
    // web_seeds.push_back("localhost:1234");
    for (std::vector<std::string>::iterator i = web_seeds.begin()
      , end(web_seeds.end()); i != end; ++i)
      t.add_url_seed(*i);

    for (std::vector<std::string>::iterator i = collections.begin()
      , end(collections.end()); i != end; ++i)
      t.add_collection(*i);

    for (std::vector<sha1_hash>::iterator i = similar.begin()
      , end(similar.end()); i != end; ++i)
      t.add_similar_torrent(*i);

    error_code ec;
    set_piece_hashes(t, branch_path(full_path)
      , boost::bind(&print_progress, _1, t.num_pieces()), ec);
    if (ec)
    {
      fprintf(stderr, "%s\n", ec.message().c_str());
      return 1;
    }

    fprintf(stderr, "\n");
    t.set_creator(creator_str.c_str());
    if (!comment_str.empty())
      t.set_comment(comment_str.c_str());

    if (!root_cert.empty())
    {
      std::vector<char> pem = load_file(root_cert);
      t.set_root_cert(std::string(&pem[0], pem.size()));
    }

    // create the torrent and print it to stdout
    std::vector<char> torrent;
    bencode(back_inserter(torrent), t.generate());

    if (!outfile.empty())
    {
      std::fstream out;
      out.exceptions(std::ifstream::failbit);
      out.open(outfile.c_str(), std::ios_base::out | std::ios_base::binary);
      out.write(&torrent[0], torrent.size());
    }
    else
    {
      fwrite(&torrent[0], 1, torrent.size(), stdout);
    }

    bdecode_node e;
    int item_limit = 1000000;
    int depth_limit = 1000;

    int pos = -1;
    int ret = bdecode(&torrent[0], &torrent[0] + torrent.size(), e, ec, &pos
      , depth_limit, item_limit);

    if (ret != 0)
    {
      fprintf(stderr, "failed to decode: '%s' at character: %d\n", ec.message().c_str(), pos);
      return 1;
    }

    torrent_info tt(e, ec);

    // entry gen = t.generate();
    fprintf(stdout, "%s\n", make_magnet_uri(tt).c_str());

    if (!merklefile.empty())
    {
      std::fstream merkle;
      merkle.exceptions(std::ifstream::failbit);
      merkle.open(merklefile.c_str(), std::ios_base::out | std::ios_base::binary);
      merkle.write(reinterpret_cast<char const*>(&t.merkle_tree()[0]), t.merkle_tree().size() * 20);
    }

#ifndef BOOST_NO_EXCEPTIONS
  }
  catch (std::exception& e)
  {
    fprintf(stderr, "%s\n", e.what());
  }
#endif

  return 0;
}