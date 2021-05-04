#ifndef GENERALDEDUPSYSTEM__CHUNKER_HPP
#define GENERALDEDUPSYSTEM__CHUNKER_HPP

#include <fstream>
#include <sys/time.h>

#include "dataset.h"
#include "database.hpp"
#include "configure.hpp"
#include "dataStructure.hpp"
#include "cryptoPrimitive.hpp"
#include "storageCore.hpp"

#include "simcomp/finese.hpp"
#include "simcomp/ntransform.hpp"
#include "simcomp/check.hpp"

#include <openssl/bn.h>
#include <openssl/md5.h>
#include "leveldb/db.h"
#include "leveldb/write_batch.h"


//#include "exchange.hh"
//extern Configure config;//global 引用其他文件中的变量

#define AVG_SEGMENT_SIZE ((2 << 20)) //1MB defaults
#define MIN_SEGMENT_SIZE ((2 << 19)) //512KB
#define MAX_SEGMENT_SIZE ((2 << 21)) //2MB

#define AVG_CONTAINER_SIZE ((2 << 29)) //4MB defaults
#define MIN_CONTAINER_SIZE ((2 << 28)) //2MB
#define MAX_CONTAINER_SIZE ((2 << 30)) //8MB

#define SEDIVISOR ((AVG_SEGMENT_SIZE - MIN_SEGMENT_SIZE) / (8 * (2 << 10)))
#define SEPATTERN ((AVG_SEGMENT_SIZE - MIN_SEGMENT_SIZE) / (8 * (2 << 10))) - 1

#define CODIVISOR ((AVG_CONTAINER_SIZE - MIN_CONTAINER_SIZE) / AVG_SEGMENT_SIZE )
#define COPATTERN ((AVG_CONTAINER_SIZE - MIN_CONTAINER_SIZE) / AVG_SEGMENT_SIZE ) - 1

class Chunker {
  private:
    CryptoPrimitive *cryptoObj;
    StorageCore *storageObj_;
    RecipeList_t recipeList;

    // Chunker type setting (FIX_SIZE_TYPE or VAR_SIZE_TYPE)
    int ChunkerType;
    // chunk size setting
    int avgChunkSize;
    int minChunkSize;
    int maxChunkSize;
    // sliding window size
    int slidingWinSize;

    u_char *waitingForChunkingBuffer, *chunkBuffer;
    uint64_t ReadSize;

    uint32_t polyBase;
    /*the modulus for limiting the value of the polynomial in rolling hash*/
    uint32_t polyMOD;
    /*note: to avoid overflow, _polyMOD*255 should be in the range of "uint32_t"*/
    /*      here, 255 is the max value of "unsigned char"                       */
    /*the lookup table for accelerating the power calculation in rolling hash*/
    uint32_t *powerLUT;
    /*the lookup table for accelerating the byte remove in rolling hash*/
    uint32_t *removeLUT;
    /*the mask for determining an anchor*/
    uint32_t anchorMask;
    /*the value for determining an anchor*/
    uint32_t anchorValue;

    uint64_t totalSize;

    Data_t recipe;

    Index fileindex;
    std::ifstream chunkingFile;
    BIGNUM* bnDivisor = NULL;
    BIGNUM* bnPattern = NULL;
    int fileindex_flag;
    void ChunkerInit(string path,int index_flag);
    void loadChunkFile(string path);
    void fixSizeChunking();
    void varSizeChunking();
    bool EndOfSegment(const u_char* hash,int segmentSize,int flag);
    std::ifstream &getChunkingFile();
  
  public:

    Chunker(StorageCore *storageObj ,std::string path,int fileindex);
    ~Chunker();
    bool chunking();
    bool saveToFile(string fileName);
    Index loadIndex(string fileName);

    Index returnindex();
};

#endif // GENERALDEDUPSYSTEM_CHUNKER_HPP
