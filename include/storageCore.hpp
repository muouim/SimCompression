#ifndef GENERALDEDUPSYSTEM_STORAGECORE_HPP
#define GENERALDEDUPSYSTEM_STORAGECORE_HPP

#include "configure.hpp"
#include "cryptoPrimitive.hpp"
#include "dataStructure.hpp"
#include "database.hpp"

#include "simcomp/delta.hpp"
#include "simcomp/check.hpp"

#include <bits/stdc++.h>

using namespace std;

class Container {
  public:
    uint32_t used_ = 0;
    char body_[2 << 23]; // 8 M container size
    Container() {}
    ~Container() {}
    bool saveTOFile(string fileName);
};

class StorageCore {
  private:
    std::string lastContainerFileName_;
    std::string currentReadContainerFileName_;
    std::string containerNamePrefix_;
    std::string containerNameTail_;
    std::string RecipeNamePrefix_;
    std::string RecipeNameTail_;
    CryptoPrimitive *cryptoObj_;
    Container currentContainer_;
    Container currentReadContainer_;
    uint64_t maxContainerSize_;
    bool writeContainer(keyForChunkHashDB_t &key, char *data);
    bool readContainer(keyForChunkHashDB_t key, char *data);

  public:
    StorageCore();
    ~StorageCore();

    bool saveChunks(char *data);
    bool saveRecipe(std::string recipeName, Recipe_t recipeHead, RecipeList_t recipeList, bool status);
    bool restoreRecipeAndChunk(char *fileNameHash, uint32_t startID, uint32_t endID, ChunkList_t &restoredChunkList);
    bool deleteRecipeAndChunk(char *fileNameHash, uint32_t startID, uint32_t endID);
    bool saveChunk(char * chunkHash, char *chunkData, int chunkSize);
    bool restoreChunk(char * chunkHash, std::string &chunkData);
    bool deleteChunk(char * chunkhash); 
    bool checkRecipeStatus(Recipe_t recipeHead, RecipeList_t recipeList);
    bool restoreRecipeHead(char *fileNameHash, Recipe_t &restoreRecipe);
    bool retriveChunk(char * chunkhash, std::string &chunkDataStr);

};

#endif
