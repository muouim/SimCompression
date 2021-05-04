#ifndef GENERALDEDUPSYSTEM_MANAGER_HPP
#define GENERALDEDUPSYSTEM_MANAGER_HPP

#include "compressor.hpp"

class Manager {
    private:
        Container currentContainer_;
        Container currentReadContainer_;

        std::string lastContainerFileName_;
        std::string currentReadContainerFileName_;
        std::string containerNamePrefix_;
        std::string DeltacontainerNamePrefix_;

        std::string containerNameTail_;

        StorageCore *storageObj_;
        uint64_t maxContainerSize_;

    public:

        Manager(StorageCore *storageObj);
        ~Manager();

        bool updateContainer();
        bool rewriteContainer();
        bool readDeltaContainer(keyForChunkHashDB_t key, char *data);
        bool updateContainerFile(keyForChunkHashDB_t key,keyForChunkHashDB_t update_key, char *data);
        bool GC();
};
#endif