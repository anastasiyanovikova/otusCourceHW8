#pragma once

#include <vector>
#include <iostream>
#include "boost/filesystem.hpp"
#include "boost/crc.hpp"
#include "boost/uuid/detail/md5.hpp"

enum hashAlg
{
    crc32 = 0,
    md5
};

class hashCalculator
{
public:
    hashCalculator(const boost::filesystem::path &path, hashAlg hash, int blockSize = 1);
    void getCurrentHash();
    bool getIsEqual(const hashCalculator& other) const;
    bool isFinish() const;
    virtual void calculateHash(const char* const data, std::size_t size) = 0;
    virtual bool isEqual(const hashCalculator& other) const  = 0;
protected: 
    std::ifstream m_fs;
    int m_block_size;
    const hashAlg m_hash;
    bool m_finish;
};

class crc32_hashCalculator: public hashCalculator
{
public:
    crc32_hashCalculator(const boost::filesystem::path &path, int blockSize = 1);
    void calculateHash(const char* const data, std::size_t size) override;
    bool isEqual(const hashCalculator& other) const  override;
private:
    boost::crc_32_type::value_type m_hash;
};

class md5_hashCalculator: public hashCalculator
{
public:
    md5_hashCalculator(const boost::filesystem::path &path, int blockSize = 1);
    void calculateHash(const char* const data, std::size_t size) override;
    bool isEqual(const hashCalculator& other) const  override;
private:
    boost::uuids::detail::md5::digest_type m_hash;
};

class fileStruct 
{
public:
    fileStruct(const boost::filesystem::path& path, std::uintmax_t fileSize): m_path(path),
    m_fileSize(fileSize){}
    
    boost::filesystem::path m_path;
    std::uintmax_t m_fileSize;
    std::unique_ptr<hashCalculator> hasCalc;
};


class bayan
{
public:

    using FileInfoVectorItC = std::vector<fileStruct>::iterator;
    using FileInfoGroup = std::pair<FileInfoVectorItC, FileInfoVectorItC>;

    bayan();
    bool setIncludeDirs(std::vector<boost::filesystem::path> dirs);
    bool setExcludeDirs(std::vector<boost::filesystem::path> dirs);
    bool setLevel(int lvl);
    bool setFileSize(int fsize);
    bool setBlockSize(int bsize);
    bool setMask(std::string mask);
    bool setHash(std::string hash);

    void run();

    std::unique_ptr<hashCalculator> makeHashCalculator(const boost::filesystem::path& path, hashAlg hash, std::size_t blockSize);

private:
    std::vector<boost::filesystem::path> m_include_dirs;
    std::vector<boost::filesystem::path> m_exclude_dirs;
    int m_scan_level;
    int m_minimum_size;
    std::string m_mask;
    int m_size_block;
    hashAlg m_hash;

    void printDublicates(std::vector<FileInfoGroup> dublicates) const;

    std::vector<FileInfoGroup> scan();

    void checkSize(const boost::filesystem::path &file, std::vector<fileStruct>& checkFiles);
    std::vector<FileInfoGroup> makeHashGroups(FileInfoVectorItC a, FileInfoVectorItC b);
};
