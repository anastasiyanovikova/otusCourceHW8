#include "options.h"
#include "bayan.h"

options::options(int argc, const char** argv)
{
  m_desc.add_options()
  ("help", "help message")
  ("include, i", boost::program_options::value<std::vector<boost::filesystem::path>>(&m_include_dirs)->composing(), "set include dirs")
  ("exclude, e", boost::program_options::value<std::vector<boost::filesystem::path>>(&m_exclude_dirs)->composing(), "set exclude dirs")
  ("level, l", boost::program_options::value<int>(&m_scan_level)->default_value(0), "set scan level")
  ("size, s", boost::program_options::value<int>(&m_minimum_size)->default_value(8), "minimum file size")
  ("mask, m", boost::program_options::value<std::string>(&m_mask)->default_value(""), "mask")
  ("block, b", boost::program_options::value<int>(&m_size_block)->default_value(1), "block size")
  ("alg_hash, a", boost::program_options::value<std::string>(&m_hash)->default_value(""), "hash");

  boost::program_options::positional_options_description pos;
  pos.add("include", -1);
  const auto parse = boost::program_options::command_line_parser(argc, argv).options(m_desc).positional(pos).run();

  boost::program_options::store(parse, m_vm);
  boost::program_options::notify(m_vm);
}

int options::exec()
{
  if(m_vm.count("help"))
  {
    std::cout << m_desc << std::endl;
    return 1;
  }

  bayan bayanWorker;

  if(m_vm.count("include"))
  {
    std::cout << "include " << m_include_dirs.size() << std::endl;
    bayanWorker.setIncludeDirs(m_include_dirs);
  }
  if(m_vm.count("exclude"))
  {
    std::cout << "exclude " << m_exclude_dirs.size() << std::endl;
    bayanWorker.setExcludeDirs(m_exclude_dirs);
  }

  if(m_vm.count("level"))
  {
    std::cout << "level " << m_scan_level << std::endl;
    bayanWorker.setLevel(m_scan_level);
  }

  if(m_vm.count("size"))
  {
    std::cout << "size " << m_minimum_size << std::endl;
    bayanWorker.setFileSize(m_minimum_size);
  }
  if(m_vm.count("mask"))
  {
    std::cout << "mask "  << m_mask << std::endl;
    bayanWorker.setMask(m_mask);
  }
  if(m_vm.count("block"))
  {
    std::cout << "block " << m_size_block << std::endl;
    bayanWorker.setBlockSize(m_size_block);
  }
  if(m_vm.count("alg_hash"))
  {
    std::cout << "hash " << m_hash << std::endl;
    bayanWorker.setHash(m_hash);
  }
  bayanWorker.run();
  return 1;
}