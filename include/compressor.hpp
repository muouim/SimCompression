#ifndef GENERALDEDUPSYSTEM_DELTACOMPRESSOR_HPP
#define GENERALDEDUPSYSTEM_DELTACOMPRESSOR_HPP


#include "dector.hpp"
#include <bits/stdc++.h>

class DeltaContainer {
  public:
    uint32_t used_ = 0;
    char body_[2 << 23]; // 1 M container size
    DeltaContainer() {}
    ~DeltaContainer() {}
    bool saveTOFile(string fileName);
};
class DeltaCompressor {

    private:
        StorageCore *storageObj_;
        std::string lastContainerFileName_;
        std::string currentReadContainerFileName_;
        std::string containerNamePrefix_;
        std::string containerNameTail_;
        uint64_t maxContainerSize_;
        CryptoPrimitive *cryptoObj_;

        DeltaContainer currentContainer_;
        DeltaContainer currentReadContainer_;

        vector<vector<string>>groupList;
        bool writeDeltaContainer(keyForChunkHashDB_t &key, char *data);
        bool readDeltaContainer(keyForChunkHashDB_t key, char *data);
    public:
        DeltaCompressor(StorageCore *storageObj);

        ~DeltaCompressor();

        void CompressorInit(string path);

        bool compress();
        bool saveDeltaChunk(char *chunkHash, std::string baseHash,char *chunkData, int chunkSize);
        bool restoreDeltaChunk(char *chunkHash, std::string &chunkDataStr);

};
#endif