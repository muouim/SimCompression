#ifndef GENERALDEDUPSYSTEM_CONFIGURE_HPP
#define GENERALDEDUPSYSTEM_CONFIGURE_HPP

#include <bits/stdc++.h>
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>

using namespace std;

#define CHUNKER_FIX_SIZE_TYPE 0 // macro for the type of fixed-size chunker
#define CHUNKER_VAR_SIZE_TYPE 1 // macro for the type of variable-size chunker
#define MIN_CHUNK_SIZE 4096 // macro for the min size of variable-size chunker
#define AVG_CHUNK_SIZE 8192 // macro for the average size of variable-size chunker
#define MAX_CHUNK_SIZE 16384 // macro for the max size of variable-size chunker

#define CHUNK_FINGER_PRINT_SIZE 32
#define CHUNK_HASH_SIZE 32
#define CHUNK_ENCRYPT_KEY_SIZE 32
#define FILE_NAME_HASH_SIZE 32

#define SlidingWinSize_MIN 32
#define SlidingWinSize_MAX 64

#define DATA_TYPE_RECIPE 1
#define DATA_TYPE_CHUNK 2


#define NETWORK_MESSAGE_DATA_SIZE 12 * 1000 * 1000
#define SGX_MESSAGE_MAX_SIZE 1024 * 1024
#define NETWORK_RESPOND_BUFFER_MAX_SIZE 12 * 1000 * 1000
#define CRYPTO_BLOCK_SZIE 16



class Configure {
  private:
    // following settings configure by macro set
    uint64_t _runningType; // localDedup \ serverDedup

    // chunking settings
    uint64_t _chunkingType; // varSize \ fixedSize \ simple
    uint64_t _maxChunkSize;
    uint64_t _minChunkSize;
    uint64_t _averageChunkSize;
    uint64_t _slidingWinSize;

    uint64_t _maxslidingWinSize;
    uint64_t _minslidingWinSize;
    uint64_t _segmentSize; // if exist segment function
    uint64_t _ReadSize; // 128M per time
    uint64_t _Fileindex;
    uint64_t _maxContainerSize;
    uint64_t _SFNumber;
    uint64_t _maxDeltaContainerSize;

    // storage setting
    std::string _RecipeRootPath;
    std::string _containerRootPath;
    std::string _fp2ChunkDBName;
    std::string _fp2MetaDBame;
    std::string _deltaChunkDBame;
    std::string _containerDBame;

    std::string _deltacontainerRootPath;
  public:
    //  Configure(std::ifstream& confFile); // according to setting json to init
    //  configure class
    Configure(std::string path);

    Configure();

    ~Configure();

    void readConf(std::string path);

    uint64_t getRunningType();

    // chunking settings
    uint64_t getChunkingType();
    uint64_t getMaxChunkSize();
    uint64_t getMinChunkSize();
    uint64_t getAverageChunkSize();
    uint64_t getSlidingWinSize();
    uint64_t getMaxSlidingWinSize();
    uint64_t getMinSlidingWinSize();
    uint64_t getSegmentSize();
    uint64_t getReadSize();
    uint64_t getIndexflag();
    
    uint64_t getSFNumber();
    uint64_t getMaxContainerSize();
    uint64_t getMaxDeltaContainerSize();

    // server settings
    std::string getRecipeRootPath();
    std::string getContainerRootPath();
    std::string getFp2ChunkDBName();
    std::string getFp2MetaDBame();
    std::string getdeltaChunkDBame();
    std::string getcontainerDBame();

    std::string getDeltaContainerRootPath();

};

#endif // GENERALDEDUPSYSTEM_CONFIGURE_HPP
