#include "chunker.hpp"
#include <string>
extern Configure config;

extern Database fp2ChunkDB;
extern Database fileName2metaDB;

struct timeval timestartChunker;
struct timeval timeendChunker;

Chunker::Chunker(StorageCore *storageObj,std::string path,int index_flag) {
    
    storageObj_=storageObj;

    loadChunkFile(path);
    ChunkerInit(path,index_flag);
    char buffer[32];
    memset(buffer, 0, 32);
    bnDivisor = BN_new();
    bnPattern = BN_new();
    sprintf(buffer, "%d", SEDIVISOR);
    BN_dec2bn(&bnDivisor, buffer);
    sprintf(buffer, "%d", SEPATTERN);
    BN_dec2bn(&bnPattern, buffer);

}

Chunker::~Chunker() {
}

bool Chunker::saveToFile(string fileName) {
    
    fileName="./Index/"+fileName;
    ofstream indexOut;
    indexOut.open(fileName, std::ofstream::out );
    if (!indexOut.is_open()) {
        //cerr << "Can not open Container file : " << fileName << endl;
        return false;
    }
    indexOut.close();
    
    indexOut.open(fileName, ios::app);
    indexOut.write((char *)&fileindex.containernumber,sizeof(int));
    indexOut.write((char *)&fileindex.segmentnumber,sizeof(int));
    indexOut.write((char *)&fileindex.chunknumber,sizeof(int));
    indexOut.write((char *)&fileindex.fileNameHash,sizeof(fileindex.fileNameHash));

    for(int i = 0; i<fileindex.containernumber;i++) {

        indexOut.write((char *)&fileindex.container[i],sizeof(L6));
    }
    for(int i = 0; i<fileindex.segmentnumber;i++) {

        indexOut.write((char *)&fileindex.segment[i],sizeof(LP));
    }
    for(int i = 0; i<fileindex.chunknumber;i++) {

        indexOut.write((char *)&fileindex.chunk[i],sizeof(L0));
    }
    indexOut.close();

    return true;
}
Index Chunker::loadIndex(string fileName) {
    
    Index fileindex;        
    fstream indexfile;

    fileName="./Index/"+fileName;

    indexfile.open(fileName);
    indexfile.read((char *)&fileindex.containernumber,sizeof(int));
    indexfile.read((char *)&fileindex.segmentnumber,sizeof(int));
    indexfile.read((char *)&fileindex.chunknumber,sizeof(int));
    indexfile.read((char *)&fileindex.fileNameHash,sizeof(fileindex.fileNameHash));

    for(int i = 0; i<fileindex.containernumber;i++) {
        L6 temp;
        indexfile.read((char *)&temp,sizeof(L6));
        fileindex.container.push_back(temp);
    }
    for(int i = 0; i<fileindex.segmentnumber;i++) {
        
        LP temp;
        indexfile.read((char *)&temp,sizeof(LP));        
        fileindex.segment.push_back(temp);

    }
    for(int i = 0; i<fileindex.chunknumber;i++) {
        
        L0 temp;
        indexfile.read((char *)&temp,sizeof(L0));
        fileindex.chunk.push_back(temp);

    }
    indexfile.close();
    return fileindex;
}
std::ifstream &Chunker::getChunkingFile() {
    if (!chunkingFile.is_open()) {
        cerr << "Chunker : chunking file open failed" << endl;
        exit(1);
    }
    return chunkingFile;
}
//chunking的同时生成recipe和index，然后放到detector 
void Chunker::ChunkerInit(string path,int index_flag) {
    fileindex_flag=index_flag;
    u_char filePathHash[FILE_NAME_HASH_SIZE]={0};
    cryptoObj->generateHash((u_char *)&path[0], path.length(), filePathHash);
    memcpy(recipe.recipe.fileRecipeHead.fileNameHash, filePathHash, FILE_NAME_HASH_SIZE);
    memcpy(recipe.recipe.keyRecipeHead.fileNameHash, filePathHash, FILE_NAME_HASH_SIZE);
    
    memcpy(fileindex.fileNameHash, filePathHash, FILE_NAME_HASH_SIZE);

    ChunkerType = (int)config.getChunkingType();

    if (ChunkerType == CHUNKER_VAR_SIZE_TYPE) {
        int numOfMaskBits;
        avgChunkSize = (int)config.getAverageChunkSize();
        minChunkSize = (int)config.getMinChunkSize();
        maxChunkSize = (int)config.getMaxChunkSize();
        slidingWinSize = (int)config.getSlidingWinSize();
        ReadSize = config.getReadSize();
        ReadSize = ReadSize * 1024 * 1024;
        waitingForChunkingBuffer = new u_char[ReadSize]();
        chunkBuffer = new u_char[maxChunkSize]();

        if (waitingForChunkingBuffer == NULL || chunkBuffer == NULL) {
            cerr << "Chunker : Memory malloc error" << endl;
            exit(1);
        }
        if (minChunkSize >= avgChunkSize) {
            cerr << "Chunker : minChunkSize should be smaller than avgChunkSize!" << endl;
            exit(1);
        }
        if (maxChunkSize <= avgChunkSize) {
            cerr << "Chunker : maxChunkSize should be larger than avgChunkSize!" << endl;
            exit(1);
        }

        /*initialize the base and modulus for calculating the fingerprint of a
         * window*/
        /*these two values were employed in open-vcdiff:
         * "http://code.google.com/p/open-vcdiff/"*/
        polyBase = 257; /*a prime larger than 255, the max value of "unsigned char"*/
        polyMOD = (1 << 23) - 1; /*polyMOD - 1 = 0x7fffff: use the last 23 bits of a
                                    polynomial as its hash*/
        /*initialize the lookup table for accelerating the power calculation in
         * rolling hash*/
        powerLUT = (uint32_t *)malloc(sizeof(uint32_t) * slidingWinSize);
        /*powerLUT[i] = power(polyBase, i) mod polyMOD*/
        powerLUT[0] = 1;
        for (int i = 1; i < slidingWinSize; i++) {
            /*powerLUT[i] = (powerLUT[i-1] * polyBase) mod polyMOD*/
            powerLUT[i] = (powerLUT[i - 1] * polyBase) & polyMOD;
        }
        /*initialize the lookup table for accelerating the byte remove in rolling
         * hash*/
        removeLUT = (uint32_t *)malloc(sizeof(uint32_t) * 256); /*256 for unsigned char*/
        for (int i = 0; i < 256; i++) {
            /*removeLUT[i] = (- i * powerLUT[_slidingWinSize-1]) mod polyMOD*/
            removeLUT[i] = (i * powerLUT[slidingWinSize - 1]) & polyMOD;
            if (removeLUT[i] != 0) {

                removeLUT[i] = (polyMOD - removeLUT[i] + 1) & polyMOD;
            }
            /*note: % is a remainder (rather than modulus) operator*/
            /*      if a < 0,  -polyMOD < a % polyMOD <= 0       */
        }

        /*initialize the anchorMask for depolytermining an anchor*/
        /*note: power(2, numOfanchorMaskBits) = avgChunkSize*/
        numOfMaskBits = 1;
        while ((avgChunkSize >> numOfMaskBits) != 1) {

            numOfMaskBits++;
        }
        anchorMask = (1 << numOfMaskBits) - 1;
        /*initialize the value for depolytermining an anchor*/
        anchorValue = 0;
    } else if (ChunkerType == CHUNKER_FIX_SIZE_TYPE) {

        avgChunkSize = (int)config.getAverageChunkSize();
        minChunkSize = (int)config.getMinChunkSize();
        maxChunkSize = (int)config.getMaxChunkSize();
        ReadSize = config.getReadSize();
        ReadSize = ReadSize * 1024 * 1024;
        waitingForChunkingBuffer = new u_char[ReadSize]();
        chunkBuffer = new u_char[avgChunkSize]();

        if (waitingForChunkingBuffer == NULL || chunkBuffer == NULL) {
            cerr << "Chunker : Memory Error" << endl;
            exit(1);
        }
        if (minChunkSize != avgChunkSize || maxChunkSize != avgChunkSize) {
            cerr << "Chunker : Error: minChunkSize and maxChunkSize should be same "
                    "in fixed size mode!"
                 << endl;
            exit(1);
        }
        if (ReadSize % avgChunkSize != 0) {
            cerr << "Chunker : Setting fixed size chunking error : ReadSize not "
                    "compat with average chunk size"
                 << endl;
        }
    }
}

bool Chunker::chunking() {
    /*fixed-size Chunker*/
    if (ChunkerType == CHUNKER_FIX_SIZE_TYPE) {
        fixSizeChunking();
    }
    /*variable-size Chunker*/
    if (ChunkerType == CHUNKER_VAR_SIZE_TYPE) {
        varSizeChunking();
    }
    return true;
}
//在chunking时生成fileindex，然后调用detector，找到transfer region，delta，group
void Chunker::fixSizeChunking() {
    gettimeofday(&timestartChunker, NULL);
    std::ifstream &fin = getChunkingFile();
    uint64_t chunkIDCounter = 0;
    memset(chunkBuffer, 0, sizeof(char) * avgChunkSize);
    uint64_t fileSize = 0;
    u_char hash[CHUNK_HASH_SIZE]={0};
    /*start chunking*/
    while (true) {
        memset((char *)waitingForChunkingBuffer, 0, sizeof(unsigned char) * ReadSize);
        fin.read((char *)waitingForChunkingBuffer, sizeof(char) * ReadSize);
        uint64_t totalReadSize = fin.gcount();
        fileSize += totalReadSize;
        uint64_t chunkedSize = 0;
        if (totalReadSize == ReadSize) {
            while (chunkedSize < totalReadSize) {
                memset(chunkBuffer, 0, sizeof(char) * avgChunkSize);
                memcpy(chunkBuffer, waitingForChunkingBuffer + chunkedSize, avgChunkSize);

                if (!cryptoObj->generateHash(chunkBuffer, avgChunkSize, hash)) {
                    cerr << "Chunker : fixed size chunking: compute hash error" << endl;
                }
                Data_t tempChunk;
                tempChunk.chunk.ID = chunkIDCounter;
                tempChunk.chunk.logicDataSize = avgChunkSize;
                memcpy(tempChunk.chunk.logicData, chunkBuffer, avgChunkSize);
                memcpy(tempChunk.chunk.chunkHash, hash, CHUNK_HASH_SIZE);
                tempChunk.dataType = DATA_TYPE_CHUNK;
                //insertMQToKeyClient(tempChunk);
                chunkIDCounter++;
                chunkedSize += avgChunkSize;
            }
        } else {
            uint64_t retSize = 0;
            while (chunkedSize < totalReadSize) {
                memset(chunkBuffer, 0, sizeof(char) * avgChunkSize);
                Data_t tempChunk;
                if (retSize > avgChunkSize) {
                    memcpy(chunkBuffer, waitingForChunkingBuffer + chunkedSize, avgChunkSize);
                    if (!cryptoObj->generateHash(chunkBuffer, avgChunkSize, hash)) {
                        cerr << "Chunker : fixed size chunking: compute hash error" << endl;
                    }
                    tempChunk.chunk.ID = chunkIDCounter;
                    tempChunk.chunk.logicDataSize = avgChunkSize;
                    memcpy(tempChunk.chunk.logicData, chunkBuffer, avgChunkSize);
                    memcpy(tempChunk.chunk.chunkHash, hash, CHUNK_HASH_SIZE);
                } else {
                    memcpy(chunkBuffer, waitingForChunkingBuffer + chunkedSize, retSize);
                    if (!cryptoObj->generateHash(chunkBuffer, retSize, hash)) {
                        cerr << "Chunker : fixed size chunking: compute hash error" << endl;
                    }
                    tempChunk.chunk.ID = chunkIDCounter;
                    tempChunk.chunk.logicDataSize = retSize;
                    memcpy(tempChunk.chunk.logicData, chunkBuffer, retSize);
                    memcpy(tempChunk.chunk.chunkHash, hash, CHUNK_HASH_SIZE);
                }
                retSize = totalReadSize - chunkedSize;
                tempChunk.dataType = DATA_TYPE_CHUNK;
                //insertMQToKeyClient(tempChunk);
                chunkIDCounter++;
                chunkedSize += avgChunkSize;
            }
        }
        if (fin.eof()) {
            break;
        }
    }
    recipe.recipe.fileRecipeHead.totalChunkNumber = chunkIDCounter;
    recipe.recipe.keyRecipeHead.totalChunkKeyNumber = chunkIDCounter;
    recipe.recipe.fileRecipeHead.fileSize = fileSize;
    recipe.recipe.keyRecipeHead.fileSize = recipe.recipe.fileRecipeHead.fileSize;
    recipe.dataType = DATA_TYPE_RECIPE;
    //insertMQToKeyClient(recipe);
    /*if (setJobDoneFlag() == false) {
        cerr << "Chunker : set chunking done flag error" << endl;
    }*/
    cout << "Chunker : Fixed chunking over:\nTotal file size = " << recipe.recipe.fileRecipeHead.fileSize
         << "; Total chunk number = " << recipe.recipe.fileRecipeHead.totalChunkNumber << endl;
    gettimeofday(&timeendChunker, NULL);
    long diff =
        1000000 * (timeendChunker.tv_sec - timestartChunker.tv_sec) + timeendChunker.tv_usec - timestartChunker.tv_usec;
    double second = diff / 1000000.0;
    cout << "Chunker : total work time is" << diff << " us = " << second << " s" << endl;
}

void Chunker::varSizeChunking() {

    uint64_t totalSize = 0;
    gettimeofday(&timestartChunker, NULL);
    string segmenthash,containerhash;
    int segmentsize=0,containersize=0;
    fileindex.segmentnumber=0;fileindex.containernumber=0;fileindex.chunknumber=0;
    L0 chunkIndex;LP segmentIndex; L6 containerIndex;
    segmentIndex.startChunkID=fileindex.chunknumber;
    containerIndex.startSegmentID=fileindex.containernumber;

    uint16_t winFp;
    uint64_t chunkBufferCnt = 0, chunkIDCnt = 0;
    ifstream &fin = getChunkingFile();
    uint64_t fileSize = 0;
    u_char hash[CHUNK_HASH_SIZE]={0};
    /*start chunking*/
    while (true) {
        memset((char *)waitingForChunkingBuffer, 0, sizeof(unsigned char) * ReadSize);
        fin.read((char *)waitingForChunkingBuffer, sizeof(unsigned char) * ReadSize);
        int len = fin.gcount();
        fileSize += len;
        for (int i = 0; i < len; i++) {

            chunkBuffer[chunkBufferCnt] = waitingForChunkingBuffer[i];

            /*full fill sliding window*/
            if (chunkBufferCnt < slidingWinSize) {
                winFp = winFp + (chunkBuffer[chunkBufferCnt] * powerLUT[slidingWinSize - chunkBufferCnt - 1]) &
                        polyMOD; // Refer to doc/Chunking.md hash function:RabinChunker
                chunkBufferCnt++;
                continue;
            }
            winFp &= (polyMOD);

            /*slide window*/
            unsigned short int v = chunkBuffer[chunkBufferCnt - slidingWinSize]; // queue
            winFp = ((winFp + removeLUT[v]) * polyBase + chunkBuffer[chunkBufferCnt]) &
                    polyMOD; // remove queue front and add queue tail
            chunkBufferCnt++;

            /*chunk's size less than minChunkSize*/
            if (chunkBufferCnt < minChunkSize) continue;

            /*find chunk pattern*/
            if ((winFp & anchorMask) == anchorValue) {

                if (!cryptoObj->generateHash(chunkBuffer, chunkBufferCnt, hash)) {
                    cerr << "Chunker : average size chunking compute hash error" << endl;
                    return;
                }
                
                Data_t tempChunk;
                tempChunk.chunk.ID = chunkIDCnt;
                tempChunk.chunk.logicDataSize = chunkBufferCnt;
                memset(tempChunk.chunk.logicData,0,MAX_CHUNK_SIZE);
                memcpy(tempChunk.chunk.logicData, chunkBuffer, chunkBufferCnt);

                memcpy(tempChunk.chunk.chunkHash, hash, CHUNK_HASH_SIZE);
                tempChunk.dataType = DATA_TYPE_CHUNK;
                
                RecipeEntry_t newRecipeEntry;
                newRecipeEntry.chunkID = tempChunk.chunk.ID;
                newRecipeEntry.chunkSize = tempChunk.chunk.logicDataSize;
                memcpy(newRecipeEntry.chunkHash, hash, CHUNK_HASH_SIZE);
                recipeList.push_back(newRecipeEntry);

                string ans;
                if (!fp2ChunkDB.query((char *)hash, ans)){
                    //std::cout<<"unique"<<std::endl;
                    chunkIndex.unique=1;
                    totalSize+=tempChunk.chunk.logicDataSize;
                    if (!storageObj_->saveChunk((char *)hash, (char *)tempChunk.chunk.logicData, tempChunk.chunk.logicDataSize)) {
                        cerr << "DedupCore : dedup stage 2 report error" << endl;
                        return;
                    }
                }
                else {
                    
                    keyForChunkHashDB_t key;
                    memcpy(&key, &ans[0], sizeof(keyForChunkHashDB_t));
                    key.count++;

                    string dbValue;
                    dbValue.resize(sizeof(keyForChunkHashDB_t));
                    memcpy(&dbValue[0], &key, sizeof(keyForChunkHashDB_t));
                    fp2ChunkDB.insert((char *)hash, dbValue);
                    chunkIndex.unique=0;
                }

                if(fileindex_flag==0){
                    chunkIndex.chunkID=fileindex.chunknumber;
                    chunkIndex.size=CHUNK_HASH_SIZE;
                    memcpy(chunkIndex.fingerprint, hash, CHUNK_HASH_SIZE);
                    fileindex.chunk.push_back(chunkIndex);
                    ++fileindex.chunknumber;
                }
                else if(fileindex_flag==1){//3-level-index
                    chunkIndex.chunkID=fileindex.chunknumber;
                    chunkIndex.size=CHUNK_HASH_SIZE;
                    memcpy(chunkIndex.fingerprint, hash, CHUNK_HASH_SIZE);
                    fileindex.chunk.push_back(chunkIndex);
                    if(EndOfSegment(hash,segmentsize,0)){

                        segmenthash+=(char *)hash;
                        unsigned char tempsegmenthash[CHUNK_HASH_SIZE];
                        segmentIndex.segmentID=fileindex.segmentnumber;
                        segmentIndex.endChunkID=fileindex.chunknumber;
                        if (!cryptoObj->generateHash((unsigned char*)segmenthash.c_str(), segmenthash.size(), tempsegmenthash)) {
                            cerr << "Chunker : average size chunking compute hash error" << endl;
                            return;
                        }
                        //MD5((unsigned char*)segmenthash.c_str(), segmenthash.size(), tempsegmenthash); 
                        memcpy(segmentIndex.hash, tempsegmenthash, CHUNK_HASH_SIZE);
                        fileindex.segment.push_back(segmentIndex);
                        segmenthash.clear();
                        if(EndOfSegment(fileindex.segment[fileindex.segmentnumber].hash,containersize,1)) {
                            
                            //std::cout<<fileindex.containernumber<<std::endl;
                            containerhash+=(char*)tempsegmenthash;
                            unsigned char tempcontainerhash[CHUNK_HASH_SIZE];
                            segmentsize=0;
                            containersize=0;
                            containerIndex.containerID=fileindex.containernumber;
                            containerIndex.endSegmentID=fileindex.segmentnumber;
                            if (!cryptoObj->generateHash((unsigned char*)containerhash.c_str(), containerhash.size(), tempcontainerhash)){
                                cerr << "Chunker : average size chunking compute hash error" << endl;
                                return;
                            }                            
                            //MD5((unsigned char*)containerhash.c_str(), containerhash.size(), tempcontainerhash); 
                            memcpy(containerIndex.hash, tempcontainerhash, CHUNK_HASH_SIZE);   
                            containerhash.clear();
                            fileindex.container.push_back(containerIndex);
                            fileindex.containernumber++;
                            containerIndex.startSegmentID=++fileindex.segmentnumber;
                            segmentIndex.startChunkID=++fileindex.chunknumber;     
                        }
                        else  {
                            fileindex.segmentnumber++;
                            segmentIndex.startChunkID=++fileindex.chunknumber;            
                            containerhash+=(char*)tempsegmenthash;
                            containersize+=segmentsize;
                            segmentsize=0;
                        }    
                    }
                    else {

                        segmenthash+=(char *)hash;
                        fileindex.chunknumber++;
                        segmentsize+=chunkBufferCnt;
                    }
                }

                /*if (!insertMQToKeyClient(tempChunk)) {
                    cerr << "Chunker : error insert chunk to keyClient MQ for chunk ID = " << tempChunk.chunk.ID
                         << endl;
                    return;
                }*/
                chunkIDCnt++;
                chunkBufferCnt = winFp = 0;
            }

            /*chunk's size exceed maxChunkSize*/
            if (chunkBufferCnt >= maxChunkSize) {
                if (!cryptoObj->generateHash(chunkBuffer, chunkBufferCnt, hash)) {
                    cerr << "Chunker : average size chunking compute hash error" << endl;
                    return;
                }
                Data_t tempChunk;
                tempChunk.chunk.ID = chunkIDCnt;
                tempChunk.chunk.logicDataSize = chunkBufferCnt;
                memset(tempChunk.chunk.logicData,0,MAX_CHUNK_SIZE);

                memcpy(tempChunk.chunk.logicData, chunkBuffer, chunkBufferCnt);
                memcpy(tempChunk.chunk.chunkHash, hash, CHUNK_HASH_SIZE);
                tempChunk.dataType = DATA_TYPE_CHUNK;
                
                RecipeEntry_t newRecipeEntry;
                newRecipeEntry.chunkID = tempChunk.chunk.ID;
                newRecipeEntry.chunkSize = tempChunk.chunk.logicDataSize;
                memcpy(newRecipeEntry.chunkHash, hash, CHUNK_HASH_SIZE);
                recipeList.push_back(newRecipeEntry);

                string ans;
                if (!fp2ChunkDB.query((char *)hash, ans)){
                    chunkIndex.unique=1;
                    totalSize+=tempChunk.chunk.logicDataSize;
                    if (!storageObj_->saveChunk((char *)hash, (char *)tempChunk.chunk.logicData, tempChunk.chunk.logicDataSize)) {
                        cerr << "DedupCore : dedup stage 2 report error" << endl;
                        return;
                    }
                }
                else {
                    
                    keyForChunkHashDB_t key;
                    memcpy(&key, &ans[0], sizeof(keyForChunkHashDB_t));
                    key.count++;

                    string dbValue;
                    dbValue.resize(sizeof(keyForChunkHashDB_t));
                    memcpy(&dbValue[0], &key, sizeof(keyForChunkHashDB_t));
                    fp2ChunkDB.insert((char *)hash, dbValue);
                    chunkIndex.unique=0;
                }

                if(fileindex_flag==0){
                    chunkIndex.chunkID=fileindex.chunknumber;
                    chunkIndex.size=CHUNK_HASH_SIZE;
                    memcpy(chunkIndex.fingerprint,hash, CHUNK_HASH_SIZE);
                    fileindex.chunk.push_back(chunkIndex);
                    ++fileindex.chunknumber;
                }
                else if(fileindex_flag==1){//3-level-index
                    chunkIndex.chunkID=fileindex.chunknumber;
                    chunkIndex.size=CHUNK_HASH_SIZE;
                    memcpy(chunkIndex.fingerprint, hash, CHUNK_HASH_SIZE);
                    fileindex.chunk.push_back(chunkIndex);
                    if(EndOfSegment(hash,segmentsize,0)){

                        segmenthash+=(char *)hash;
                        unsigned char tempsegmenthash[CHUNK_HASH_SIZE];
                        segmentIndex.segmentID=fileindex.segmentnumber;
                        segmentIndex.endChunkID=fileindex.chunknumber;
                         if (!cryptoObj->generateHash((unsigned char*)segmenthash.c_str(), segmenthash.size(), tempsegmenthash)) {
                            cerr << "Chunker : average size chunking compute hash error" << endl;
                            return;
                        }
                        //MD5((unsigned char*)segmenthash.c_str(), segmenthash.size(), tempsegmenthash); 
                        memcpy(segmentIndex.hash, tempsegmenthash, CHUNK_HASH_SIZE);
                        fileindex.segment.push_back(segmentIndex);
                        segmenthash.clear();
                        if(EndOfSegment(fileindex.segment[fileindex.segmentnumber].hash,containersize,1)) {
                            //std::cout<<fileindex.containernumber<<std::endl;

                            containerhash+=(char*)tempsegmenthash;
                            unsigned char tempcontainerhash[CHUNK_HASH_SIZE];
                            segmentsize=0;
                            containersize=0;
                            containerIndex.containerID=fileindex.containernumber;
                            containerIndex.endSegmentID=fileindex.segmentnumber;
                            if (!cryptoObj->generateHash((unsigned char*)containerhash.c_str(), containerhash.size(), tempcontainerhash)) {
                                cerr << "Chunker : average size chunking compute hash error" << endl;
                                return;
                            }                            
                            //MD5((unsigned char*)containerhash.c_str(), containerhash.size(), tempcontainerhash); 
                            memcpy(containerIndex.hash, tempcontainerhash, CHUNK_HASH_SIZE);   
                            containerhash.clear();
                            fileindex.container.push_back(containerIndex);
                            fileindex.containernumber++;
                            containerIndex.startSegmentID=++fileindex.segmentnumber;
                            segmentIndex.startChunkID=++fileindex.chunknumber;     
                        }
                        else  {
                            fileindex.segmentnumber++;
                            segmentIndex.startChunkID=++fileindex.chunknumber;            
                            containerhash+=(char*)tempsegmenthash;
                            containersize+=segmentsize;
                            segmentsize=0;
                        }    
                    }
                    else {
                        segmenthash+=(char *)hash;
                        fileindex.chunknumber++;
                        segmentsize+=chunkBufferCnt;
                    }
                }

                 /* if (!insertMQToKeyClient(tempChunk)) {
                    cerr << "Chunker : error insert chunk to keyClient MQ for chunk ID = " << tempChunk.chunk.ID
                         << endl;
                    return;
                }*/
                chunkIDCnt++;
                chunkBufferCnt = winFp = 0;
            }
        }
        if (fin.eof()) {
            break;
        }
    }

    /*add final chunk*/
    if (chunkBufferCnt != 0) {

        if (!cryptoObj->generateHash(chunkBuffer, chunkBufferCnt, hash)) {
            cerr << "Chunker : average size chunking compute hash error" << endl;
            return;
        }
        Data_t tempChunk;
        tempChunk.chunk.ID = chunkIDCnt;
        tempChunk.chunk.logicDataSize = chunkBufferCnt;
        memset(tempChunk.chunk.logicData,0,MAX_CHUNK_SIZE);

        memcpy(tempChunk.chunk.logicData, chunkBuffer, chunkBufferCnt);
        memcpy(tempChunk.chunk.chunkHash, hash, CHUNK_HASH_SIZE);
        tempChunk.dataType = DATA_TYPE_CHUNK;
        
        RecipeEntry_t newRecipeEntry;
        newRecipeEntry.chunkID = tempChunk.chunk.ID;
        newRecipeEntry.chunkSize = tempChunk.chunk.logicDataSize;
        memcpy(newRecipeEntry.chunkHash, hash, CHUNK_HASH_SIZE);
        recipeList.push_back(newRecipeEntry);
        
        string ans;
        if (!fp2ChunkDB.query((char *)hash, ans)){
            //cout<<"unique "<< endl;
            chunkIndex.unique=1;
            totalSize+=tempChunk.chunk.logicDataSize;
            if (!storageObj_->saveChunk((char *)hash, (char *)tempChunk.chunk.logicData, tempChunk.chunk.logicDataSize)) {
                cerr << "DedupCore : dedup stage 2 report error" << endl;
                return;
            }
        }  
        else {
                    
            keyForChunkHashDB_t key;
            memcpy(&key, &ans[0], sizeof(keyForChunkHashDB_t));
            key.count++;

            string dbValue;
            dbValue.resize(sizeof(keyForChunkHashDB_t));
            memcpy(&dbValue[0], &key, sizeof(keyForChunkHashDB_t));
            fp2ChunkDB.insert((char *)hash, dbValue);
            chunkIndex.unique=0;
        }

        if(fileindex_flag==0){
            chunkIndex.chunkID=fileindex.chunknumber;
            chunkIndex.size=CHUNK_HASH_SIZE;
            memcpy(chunkIndex.fingerprint, hash, CHUNK_HASH_SIZE);
            fileindex.chunk.push_back(chunkIndex);
            ++fileindex.chunknumber;
        }
        else if(fileindex_flag==1){//3-level-index
            chunkIndex.chunkID=fileindex.chunknumber;
            chunkIndex.size=CHUNK_HASH_SIZE;
            memcpy(chunkIndex.fingerprint, hash, CHUNK_HASH_SIZE);
            fileindex.chunk.push_back(chunkIndex);
            ++fileindex.chunknumber;
            segmenthash+=(char *)hash;
            segmentIndex.segmentID=fileindex.segmentnumber;segmentIndex.endChunkID=fileindex.chunknumber;//final segment
            unsigned char tempsegmenthash[CHUNK_HASH_SIZE];
            MD5((unsigned char*)segmenthash.c_str(), segmenthash.size(), tempsegmenthash); 
            memcpy(segmentIndex.hash, tempsegmenthash, CHUNK_HASH_SIZE);
            fileindex.segment.push_back(segmentIndex);
            fileindex.segmentnumber++;
            segmenthash.clear();
            containerIndex.containerID=fileindex.containernumber;containerIndex.endSegmentID=fileindex.segmentnumber-1;//final container
            unsigned char tempcontainerhash[CHUNK_HASH_SIZE];
            containerhash+=(char*)tempsegmenthash;
            MD5((unsigned char*)containerhash.c_str(), containerhash.size(), tempcontainerhash);         
            memcpy(containerIndex.hash, tempcontainerhash, CHUNK_HASH_SIZE);   
            fileindex.container.push_back(containerIndex);
            fileindex.containernumber++;
            containerhash.clear();
        }
        /*if (!insertMQToKeyClient(tempChunk)) {
            cerr << "Chunker : error insert chunk to keyClient MQ for chunk ID = " << tempChunk.chunk.ID << endl;
            return;
        }*/

        
        chunkIDCnt++;
        chunkBufferCnt = winFp = 0;
    }
    recipe.recipe.fileRecipeHead.totalChunkNumber = chunkIDCnt;
    recipe.recipe.fileRecipeHead.fileSize = fileSize;
    recipe.recipe.keyRecipeHead.fileSize = recipe.recipe.fileRecipeHead.fileSize;
    recipe.dataType = DATA_TYPE_RECIPE;
    /*if (!insertMQToKeyClient(recipe)) {
        cerr << "Chunker : error insert recipe head to keyClient MQ" << endl;
        return;
    }
    if (setJobDoneFlag() == false) {
        cerr << "Chunker: set chunking done flag error" << endl;
        return;
    }*/
    std::cout<<recipeList.size()<<endl;
    if (!storageObj_->checkRecipeStatus(recipe.recipe, recipeList)) {
        cerr << "StorageCore : verify Recipe fail, send resend flag" << endl;
 
    } else {
        
    }
    cout << "Chunker : variable size chunking over:\nTotal file size = " << recipe.recipe.fileRecipeHead.fileSize
         << "; Total chunk number = " << recipe.recipe.fileRecipeHead.totalChunkNumber << endl;
    gettimeofday(&timeendChunker, NULL);
    long diff =
        1000000 * (timeendChunker.tv_sec - timestartChunker.tv_sec) + timeendChunker.tv_usec - timestartChunker.tv_usec;
    double second = diff / 1000000.0;
    cout << "Chunker : total work time is " << diff << " us = " << second << " s" << endl;
    cout << "Chunker : Save " <<totalSize<< " bytes" << endl;

    return;
}


bool Chunker::EndOfSegment(const u_char* hash,int size,int flag)
{

    if(flag==0) {
        if (size < MIN_SEGMENT_SIZE) {
            return false;
        }
        if (size > MAX_SEGMENT_SIZE) {
            return true;
        }
        unsigned char mask[CHUNK_HASH_SIZE];
        memset(mask, '0', CHUNK_HASH_SIZE);
        int metaFPInt = *(int *) hash;
        int remainder = metaFPInt & (SEDIVISOR - 1);
        int ret_flag = 0;
        if(remainder == SEPATTERN) {
            ret_flag = 1;
        }
  
        int unknownCodeRet = memcmp(hash + (CHUNK_HASH_SIZE - 9), mask, 9);

        if((ret_flag == 1 && size >= MIN_SEGMENT_SIZE)  ||unknownCodeRet == 0 ) {
            //std::cout<<"finds"<<std::endl;
            return true;
        }
        return false;

    }
    else if(flag==1) {
        if (size < MIN_CONTAINER_SIZE) {
            return false;
        }
        if (size > MAX_CONTAINER_SIZE) {
            return true;
        }
        unsigned char mask[CHUNK_HASH_SIZE];
        memset(mask, '0', CHUNK_HASH_SIZE);
        int metaFPInt = *(int *) hash;
        int remainder = metaFPInt & (CODIVISOR - 1);
        int ret_flag = 0;
        if(remainder == COPATTERN) {
            ret_flag = 1;
        }
  
        int unknownCodeRet = memcmp(hash + (CHUNK_HASH_SIZE - 9), mask, 9);

        if((ret_flag == 1 && size >= MIN_CONTAINER_SIZE) ||unknownCodeRet == 0 ) {
            //std::cout<<"findc"<<std::endl;
            return true;
        }
        return false;

    }
  

}

Index Chunker::returnindex() {
    return fileindex;
}

void Chunker::loadChunkFile(std::string path) {
    
    if (chunkingFile.is_open()) 
        chunkingFile.close();
        
    chunkingFile.open(path, std::ios::binary);
    if (!chunkingFile.is_open()) {
        cerr << "Chunker : open file: " << path << "error" << endl;
        exit(1);
    }
}

     /*string segmenthash,containerhash,temp;
    int segmentsize=0,containersize=0;
    fileindex.segmentnumber=0;fileindex.containernumber=0;fileindex.chunknumber=0;
    L0 tempchunk;LP tempsegment; L6 tempcontainer;
    tempsegment.startChunkID=fileindex.chunknumber;
    tempcontainer.startSegmentID=fileindex.containernumber;
    loadChunkFile(path);
    std::ifstream &fin = getChunkingFile();
    if(fileindex_flag==0){
        while (!fin.eof()) {
            getline(fin, temp);
            temp=temp.substr(0,18);
            tempchunk.chunkID=fileindex.chunknumber;
            tempchunk.size=temp.size();
            memcpy(tempchunk.fingerprint, temp.c_str(), temp.size());
            fileindex.chunk.push_back(tempchunk);
            ++fileindex.chunknumber;
        }
    }
    else if(fileindex_flag==1){
        while (!fin.eof()) {//3-level-index
            getline(fin, temp);
            temp=temp.substr(0,18);
            tempchunk.chunkID=fileindex.chunknumber;
            tempchunk.size=temp.size();
            memcpy(tempchunk.fingerprint, temp.c_str(), temp.size());
            fileindex.chunk.push_back(tempchunk);
            if(EndOfSegment(temp.c_str(),segmentsize,0)){

                segmenthash+=temp;
                unsigned char tempsegmenthash[CHUNK_HASH_SIZE];
                tempsegment.segmentID=fileindex.segmentnumber;
                tempsegment.endChunkID=fileindex.chunknumber;
                MD5((unsigned char*)segmenthash.c_str(), segmenthash.size(), tempsegmenthash); 
                memcpy(tempsegment.hash, tempsegmenthash, CHUNK_HASH_SIZE);
                fileindex.segment.push_back(tempsegment);
                segmenthash.clear();
                if(EndOfSegment(fileindex.segment[fileindex.segmentnumber].hash,containersize,1)) {
                    
                    containerhash+=(char*)tempsegmenthash;
                    unsigned char tempcontainerhash[CHUNK_HASH_SIZE];
                    segmentsize=0;
                    containersize=0;
                    tempcontainer.containerID=fileindex.containernumber;
                    tempcontainer.endSegmentID=fileindex.segmentnumber;
                    MD5((unsigned char*)containerhash.c_str(), containerhash.size(), tempcontainerhash); 
                    memcpy(tempcontainer.hash, tempcontainerhash, CHUNK_HASH_SIZE);   
                    containerhash.clear();
                    fileindex.container.push_back(tempcontainer);
                    fileindex.containernumber++;
                    tempcontainer.startSegmentID=++fileindex.segmentnumber;
                    tempsegment.startChunkID=++fileindex.chunknumber;     
                }
                else  {

                    fileindex.segmentnumber++;
                    tempsegment.startChunkID=++fileindex.chunknumber;            
                    containerhash+=(char*)tempsegmenthash;
                    containersize+=segmentsize;
                    segmentsize=0;
                }    
            }
            else {

                segmenthash+=temp;
                fileindex.chunknumber++;
                segmentsize+=8192;
            }
        }


        tempsegment.segmentID=fileindex.segmentnumber;tempsegment.endChunkID=fileindex.chunknumber;//final segment
        unsigned char tempsegmenthash[CHUNK_HASH_SIZE];
        MD5((unsigned char*)segmenthash.c_str(), segmenthash.size(), tempsegmenthash); 
        memcpy(tempsegment.hash, tempsegmenthash, CHUNK_HASH_SIZE);
        fileindex.segment.push_back(tempsegment);
        fileindex.segmentnumber++;
        segmenthash.clear();
        tempcontainer.containerID=fileindex.containernumber;tempcontainer.endSegmentID=fileindex.segmentnumber-1;//final container
        unsigned char tempcontainerhash[CHUNK_HASH_SIZE];
        containerhash+=(char*)tempsegmenthash;
        MD5((unsigned char*)containerhash.c_str(), containerhash.size(), tempcontainerhash);         
        memcpy(tempcontainer.hash, tempcontainerhash, CHUNK_HASH_SIZE);   
        fileindex.container.push_back(tempcontainer);
        fileindex.containernumber++;
        containerhash.clear();
    }*/