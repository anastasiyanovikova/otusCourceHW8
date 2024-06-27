#pragma once

#include "boost/program_options.hpp"
#include "boost/filesystem.hpp"
#include "boost/filesystem/path.hpp"
#include "boost/filesystem/fstream.hpp"
#include "boost/program_options/variables_map.hpp"
#include <vector>
#include <iostream>

class options
{
public:
  explicit options(int argc, const char** argv);
  int exec();

private:

  boost::program_options::options_description m_desc{"base options"};
  boost::program_options::variables_map m_vm;

  std::vector<boost::filesystem::path> m_include_dirs;
  std::vector<boost::filesystem::path> m_exclude_dirs;
  int m_scan_level;
  int m_minimum_size;
  std::string m_mask;
  int m_size_block;
  std::string m_hash;
};

