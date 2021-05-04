#ifndef GENERALDEDUPSYSTEM_CHUNK_HPP
#define GENERALDEDUPSYSTEM_CHUNK_HPP

#include "configure.hpp"
#include <bits/stdc++.h>
#include <vector>


using namespace std;

typedef struct  {
    int containerID;
    int startSegmentID, endSegmentID;
    u_char hash[CHUNK_HASH_SIZE];

}L6;

typedef struct  {
    int segmentID;
    int startChunkID, endChunkID;
    u_char hash[CHUNK_HASH_SIZE];

}LP;

typedef struct  {
    int chunkID;
    int size;
    int unique;
    u_char fingerprint[CHUNK_HASH_SIZE];

}L0;
    
typedef struct {
    u_char fileNameHash[FILE_NAME_HASH_SIZE];
    int segmentnumber;
    int containernumber;
    int chunknumber;
    vector<L0>chunk;
    vector<LP>segment;
    vector<L6>container;

}Index;

typedef struct {
    int newID;
    int oldID;

}Pair;

typedef struct {
    u_char hash[CHUNK_HASH_SIZE];
} Hash_t;
// system basic data structures
typedef struct {
    uint32_t ID;
    int type;
    int logicDataSize;
    u_char logicData[MAX_CHUNK_SIZE];
    u_char chunkHash[CHUNK_HASH_SIZE];
    u_char encryptKey[CHUNK_ENCRYPT_KEY_SIZE];
} Chunk_t;

typedef struct {
    int logicDataSize;
    char logicData[MAX_CHUNK_SIZE];
    char chunkHash[CHUNK_HASH_SIZE];
} StorageCoreData_t;

typedef struct {
    uint32_t ID;
    int logicDataSize;
    char logicData[MAX_CHUNK_SIZE];
} RetrieverData_t;

typedef struct {
    uint32_t chunkID;
    int chunkSize;
    u_char chunkHash[CHUNK_HASH_SIZE];

} RecipeEntry_t;

typedef vector<Chunk_t> ChunkList_t;
typedef vector<RecipeEntry_t> RecipeList_t;

typedef struct {
    uint64_t fileSize;
    u_char fileNameHash[FILE_NAME_HASH_SIZE];
    uint64_t totalChunkNumber;
} FileRecipeHead_t;

typedef struct {
    uint64_t fileSize;
    u_char fileNameHash[FILE_NAME_HASH_SIZE];
    uint64_t totalChunkKeyNumber;
} KeyRecipeHead_t;

typedef struct {
    FileRecipeHead_t fileRecipeHead;
    KeyRecipeHead_t keyRecipeHead;
} Recipe_t;

typedef struct {
    union {
        Chunk_t chunk;
        Recipe_t recipe;
    };
    int dataType;
} Data_t;

typedef struct {
    u_char originHash[CHUNK_HASH_SIZE];

} KeyGenEntry_t;

typedef struct {
    int fd;
    int epfd;
    u_char hash[CHUNK_HASH_SIZE];

} Message_t;

typedef struct {
    int messageType;
    int clientID;
    int dataSize;
} NetworkHeadStruct_t;

// database data structures

typedef struct {
    u_char containerName[16];
    uint32_t offset;
    uint32_t length;
    uint32_t count;
    uint32_t deltaCount;

} keyForChunkHashDB_t;

typedef struct {
    char RecipeFileName[FILE_NAME_HASH_SIZE];
    uint32_t version;
} keyForFilenameDB_t;

typedef vector<uint32_t> RequiredChunk_t;

#endif // GENERALDEDUPSYSTEM_CHUNK_HPP
