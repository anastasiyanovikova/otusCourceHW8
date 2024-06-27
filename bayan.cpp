#include "bayan.h"
#include <regex>

hashCalculator::hashCalculator(const boost::filesystem::path &path, hashAlg hash, int blockSize)
: m_fs {path.native(), std::ios::in | std::ios::binary},
 m_block_size(blockSize),m_hash(hash), m_finish(false)
{
    m_fs.seekg(0);
}

void hashCalculator::getCurrentHash()
{
    std::string buf(m_block_size, '0');
    if(!m_fs.read(&buf[0], buf.size()))
    {
        m_finish = true;
        m_fs.close();
    }

    calculateHash(buf.c_str(), buf.size());
}
bool hashCalculator::getIsEqual(const hashCalculator& other) const
{
    if(m_hash == other.m_hash && m_block_size == other.m_block_size && m_finish == other.m_finish)
        return isEqual(other);
    return false;
}
bool hashCalculator::isFinish() const
{
    return m_finish;
}

crc32_hashCalculator::crc32_hashCalculator(const boost::filesystem::path &path, int blockSize)
: hashCalculator(path, hashAlg::crc32, blockSize), m_hash(0)
{
}
void crc32_hashCalculator::calculateHash(const char* const data, std::size_t size)
{
    boost::crc_32_type seed;
    seed.process_bytes(data, size);
    m_hash = seed.checksum();
}
bool crc32_hashCalculator::isEqual(const hashCalculator& other) const
{
    try
    {
        const auto& otherCalc = static_cast<const crc32_hashCalculator&>(other);
        return m_hash == otherCalc.m_hash;
    }
    catch(const std::bad_cast& e)
    {
        std::cout << e.what()<<std::endl;
        return false;
    }
    return false;
}

md5_hashCalculator::md5_hashCalculator(const boost::filesystem::path &path, int blockSize)
: hashCalculator(path, hashAlg::md5, blockSize)
{
}
void md5_hashCalculator::calculateHash(const char* const data, std::size_t size)
{
    boost::uuids::detail::md5 seed;
    seed.process_bytes(data, size);
    seed.get_digest(m_hash);
}
bool md5_hashCalculator::isEqual(const hashCalculator& other) const
{
    try
    {
        const auto& otherCalc = static_cast<const md5_hashCalculator&>(other);
        const auto char_dig = reinterpret_cast<const char*>(&m_hash);
        const auto char_dig2 = reinterpret_cast<const char*>(&otherCalc.m_hash);
        return char_dig == char_dig2;
        //return m_hash  == otherCalc.m_hash;
    }
    catch(const std::bad_cast& e)
    {
        std::cout << e.what()<<std::endl;
        return false;
    }
    return false;
}

bayan::bayan()
{

}
bool bayan::setIncludeDirs(std::vector<boost::filesystem::path> dirs)
{
    m_include_dirs = dirs;
    return true;
}
bool bayan::setExcludeDirs(std::vector<boost::filesystem::path> dirs)
{
    m_exclude_dirs = dirs;
    return true;
}
bool bayan::setLevel(int lvl)
{
    if(lvl >= 0)
        m_scan_level = lvl;
    else
        m_scan_level = 0;
    return true;
}
bool bayan::setFileSize(int fsize)
{
    if(fsize > 0)
        m_minimum_size = fsize;
    else
        m_minimum_size = 1;
    return true;
}
bool bayan::setBlockSize(int bsize)
{
    if(bsize > 0)
        m_size_block = bsize;
    else
        m_size_block = 1;
    return true;
}
bool bayan::setMask(std::string mask)
{
    m_mask = mask;
    return true;
}
bool bayan::setHash(std::string hash)
{
    if(hash == "crc32")
        m_hash = crc32;
    else if(hash == "md5")
        m_hash = md5;
    else
        m_hash = crc32;
    return true;
}

void bayan::run()
{
    std::cout << " run bayan "<<std::endl;
    scan();
    std::cout << "end run bayan "<<std::endl;
}

std::unique_ptr<hashCalculator> bayan::makeHashCalculator(const boost::filesystem::path& path, hashAlg hash, std::size_t blockSize)
{
    switch (hash)
    {
    case hashAlg::crc32:
        return std::make_unique<crc32_hashCalculator>(path, blockSize);
    case hashAlg::md5:
        return std::make_unique<md5_hashCalculator>(path, blockSize);
    default:
        return {};
    }
}

void bayan::printDublicates(std::vector<FileInfoGroup> dublicates) const
{
    for(auto &&dubl: dublicates)
    {
        std::transform(dubl.first, dubl.second, std::ostream_iterator<std::string>(std::cout, ""),[](auto &&item)
        {
            return item.m_path.native() + " " + std::to_string(item.m_fileSize) + '\n';
        });
        std::cout << std::endl;
    }   
}

std::vector<std::pair<std::vector<fileStruct>::iterator, std::vector<fileStruct>::iterator>> bayan::scan()
{
    std::vector<fileStruct> filesInfo;

    for(auto &&path: m_include_dirs)
    {
        boost::filesystem::recursive_directory_iterator it(path, boost::filesystem::directory_options::skip_permission_denied);
        boost::filesystem::recursive_directory_iterator itEnd;
        for(; it != itEnd; ++it)
        {
            if(boost::filesystem::is_regular_file(it->status()) && !boost::filesystem::is_symlink(it->symlink_status()))
            {
                if(m_mask == "")
                    checkSize(it->path(), filesInfo);
                else
                {
                    std::regex regExpr(m_mask, std::regex_constants::icase);
                    if(std::regex_match(it->path().filename().native(), regExpr))
                        checkSize(it->path(), filesInfo);
                }
            }
            else if(boost::filesystem::is_directory(it.status()))
            {
                if(it.depth() >= m_scan_level)
                    it.disable_recursion_pending();
                else
                {
                    const auto absPath = boost::filesystem::canonical(*it);
                    for(auto && excludeDir: m_exclude_dirs)
                    {
                        if(absPath == boost::filesystem::canonical(excludeDir))
                            it.disable_recursion_pending();
                    }
                }
            }
        }
    }

    if(filesInfo.empty())
        return {};

    std::vector<FileInfoGroup> fileGroups;
    auto it = filesInfo.begin();
    auto end = filesInfo.end();
    for(; it != end; )
    {
        const auto prevIt = it;
        it = std::partition(prevIt, end, [size = prevIt->m_fileSize](auto &&fileStruct)
        {
            return size == fileStruct.m_fileSize;
        });

        const auto d = std::distance(prevIt, it);
        if(d == 1)
            continue;

        auto &&sizeGroups = makeHashGroups(prevIt, it);
        printDublicates(sizeGroups);
        std::move(sizeGroups.begin(), sizeGroups.end(), std::back_inserter(fileGroups));
        
    }
    return fileGroups;
}

void bayan::checkSize(const boost::filesystem::path &file, std::vector<fileStruct>& checkFiles)
{
    const auto fSize = boost::filesystem::file_size(file);
    if(m_minimum_size > 0)
    {
        if(fSize > (uintmax_t) m_minimum_size)
            checkFiles.emplace_back(fileStruct(file, fSize));
    }
    else
        checkFiles.emplace_back(fileStruct(file, fSize));
}
std::vector<bayan::FileInfoGroup> bayan::makeHashGroups(FileInfoVectorItC a, FileInfoVectorItC b)
{
    if(a == b)
        return {};

    std::vector<FileInfoGroup> res;

    std::for_each(a, b, [this](auto && fi)
    {
        fi.hasCalc = makeHashCalculator(fi.m_path, m_hash, m_size_block);
        fi.hasCalc->getCurrentHash();
    });

    auto l = a;
    std::stack<FileInfoVectorItC> rstack;
    rstack.push(b);
    while(l != b)
    {
        auto it = std::partition(l, rstack.top(), [l](const auto &fi)
        {
            return l->hasCalc->isEqual(*fi.hasCalc);
        });

        const auto d = std::distance(l, it);
        if(d == 1)
        {
            std::advance(l, 1);
            continue;
        }
        else if(d == 0)
        {
            l = rstack.top();
            rstack.pop();
            continue;
        }

        if(l->hasCalc->isFinish())
        {
            res.emplace_back(FileInfoGroup(l, it));
            l = it;
            if(rstack.top() != b)
                rstack.pop();
            continue;
        }

        if(rstack.top() != it)
            rstack.push(it);

        std::for_each(l, rstack.top(), [](auto &&fi)
        {
            fi.hasCalc->getCurrentHash();
        });
    }
    return res;
}