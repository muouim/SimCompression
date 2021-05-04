#include "dector.hpp"

extern Configure config;
extern Database fp2ChunkDB;
extern Database fileName2metaDB;

using bsoncxx::builder::basic::kvp;
using bsoncxx::builder::basic::make_array;
using bsoncxx::builder::basic::make_document;

using bsoncxx::builder::stream::close_array;
using bsoncxx::builder::stream::close_document;
using bsoncxx::builder::stream::document;
using bsoncxx::builder::stream::finalize;
using bsoncxx::builder::stream::open_array;
using bsoncxx::builder::stream::open_document;

mongocxx::instance inst{};

Dector::Dector(StorageCore *storageObj, Index fileindex_1, Index fileindex_2, int indexflag) {
    storageObj_ = storageObj;
    fineseObj = new FineseFunctions;

    string logfile("similar"); //(char *)fileindex_1->fileNameHash);
    logfile += ".log";
    log.open(logfile, ios::out);

    containerpairnumber = 0;
    segmentpairnumber = 0;
    transferpairnumber = 0;

    if (indexflag == 1) {
        ContainerDector(&fileindex_1, &fileindex_2);
    } else
        truetransferDector(&fileindex_1, &fileindex_2);
}

Dector::~Dector() {
}

//container level索引
vector<Pair> Dector::ContainerDector(Index *fileindex_1, Index *fileindex_2) {

    return containerPair;
}

//segment level 索引
vector<Pair> Dector::SegmentDector(Index *fileindex_1, Index *fileindex_2) {

    return segmentPair;
}

//chunk level 索引
vector<Pair> Dector::transferDector(Index *fileindex_1, Index *fileindex_2) {

    return transferPair;
}

// sliding window
vector<Pair> Dector::truetransferDector(Index *fileindex_1, Index *fileindex_2) {
    size_t uniqueSizes = 0;
    size_t equalSizes = 0;
    size_t newuniqueSizes = 0;
    size_t olduniqueSizes = 0;
    SimCompSimilarityCheck check;

    Pair temp_pair;
    int flag = 0;
    int before = 0;
    int after = 0; //标识
    //flag =1代表当前window内就找到了下一个相等的位置，=2当前没找到，向后接着找
    //after=2,代表需要找到下一个相等的位置，flag=2，代表找下一个相等的位置
    ChunkList_t newrestoredChunkList;
    ChunkList_t oldrestoredChunkList;
    int size = config.getMinSlidingWinSize();
    mongocxx::client conn{mongocxx::uri{}};
    for (int i_1 = 0, i_2 = 0; i_1 < fileindex_1->chunknumber && i_2 <= fileindex_2->chunknumber;) {
        int count = 0;
        int stand1 = 0, stand2 = 0;
        string hash1((char *)fileindex_1->chunk[i_1].fingerprint, CHUNK_HASH_SIZE),
            hash2((char *)fileindex_2->chunk[i_2].fingerprint, CHUNK_HASH_SIZE);
        if (hash1 == hash2) {
            if (flag == 2) {
                after = 0;
                flag = 0;
                temp_pair.newID = (i_1 - 1);
                temp_pair.oldID = (i_2 - 1);
                transferPair.push_back(temp_pair);
                transferpairnumber++;
                stand1 = transferPair[transferPair.size() - 2].newID;
                stand2 = transferPair[transferPair.size() - 2].oldID;

                log.write((const char *)fileindex_1->chunk[i_1 - 1].fingerprint, CHUNK_HASH_SIZE);
                log.write((const char *)fileindex_2->chunk[i_2 - 1].fingerprint, CHUNK_HASH_SIZE);
                //if(i_1-stand1>size||i_2-stand2>size)continue;//不能让group内chunk过多
                ChunkList_t newrestoredChunkList;
                ChunkList_t oldrestoredChunkList;

                std::cout << "old " << stand1 << " " << i_1 - 1 << std::endl;

                if (stand1 < i_1 - 2)
                    if (storageObj_->restoreRecipeAndChunk((char *)fileindex_1->fileNameHash, stand1 + 1, i_1 - 1,
                                                           oldrestoredChunkList)) {}

                //取出region内chunk old
                std::cout << "new " << stand2 << " " << i_2 - 1 << std::endl;
                if (stand2 < i_2 - 2)
                    if (storageObj_->restoreRecipeAndChunk((char *)fileindex_2->fileNameHash, stand2 + 1, i_2 - 1,
                                                           newrestoredChunkList)) {}
            }
            equalSizes += fileindex_2->chunk[i_2].size;
            size = config.getMinSlidingWinSize();
            i_1++;
            i_2++;
            continue;
        } else {
            stand1 = i_1;
            stand2 = i_2;
            for (int k = 0; k < size; k++) {
                string temphash1((char *)fileindex_1->chunk[i_1 + k].fingerprint, CHUNK_HASH_SIZE),
                    temphash2((char *)fileindex_2->chunk[i_2].fingerprint, CHUNK_HASH_SIZE);

                if (temphash1 == temphash2) {
                    if (flag == 2) {
                        i_1 += k;
                        size = config.getMinSlidingWinSize();
                        after = 1;
                        break;
                    }
                    size = config.getMinSlidingWinSize();
                    flag = 1;
                    i_1 += k;
                    break;
                } else if (i_1 + k >= fileindex_1->chunknumber) {
                    i_1 = stand1;
                    i_2++;
                    k = -1;
                    count++;
                    if (count >= size) {
                        size = config.getMaxSlidingWinSize();
                        i_1 = i_2;
                        flag = 2;
                        before = 1;
                        break;
                    }
                } else if (k == size - 1) {
                    i_1 = stand1;
                    i_2++;
                    k = -1;
                    count++;
                    if (count >= size) {
                        size = config.getMaxSlidingWinSize();
                        i_1 = i_2;
                        flag = 2;
                        before = 1;
                        break;
                    }
                }
            }
            //Transfer region pair//1为old 2为new
            if (flag == 1 && i_1 != stand1 && i_2 != stand2 && (i_1 - 1 != stand1) && i_2 - 1 != stand2) {
                temp_pair.newID = stand1;
                temp_pair.oldID = stand2;
                transferPair.push_back(temp_pair);
                temp_pair.newID = (i_1 - 1);
                temp_pair.oldID = (i_2 - 1);
                transferPair.push_back(temp_pair);
                transferpairnumber += 2;

                //region写入log
                if (fileindex_2->chunk[stand2].unique != 0 && fileindex_2->chunk[i_2 - 1].unique != 0) {
                    log.write((const char *)fileindex_1->chunk[stand1].fingerprint, CHUNK_HASH_SIZE);
                    log.write((const char *)fileindex_2->chunk[stand2].fingerprint, CHUNK_HASH_SIZE);
                    log.write((const char *)fileindex_1->chunk[i_1 - 1].fingerprint, CHUNK_HASH_SIZE);
                    log.write((const char *)fileindex_2->chunk[i_2 - 1].fingerprint, CHUNK_HASH_SIZE);
                }

                //取出region内chunk new
                std::cout << "old " << stand1 << " " << i_1 - 1 << std::endl;
                if (stand1 < i_1 - 2)
                    if (storageObj_->restoreRecipeAndChunk((char *)fileindex_1->fileNameHash, stand1 + 1, i_1 - 1,
                                                           oldrestoredChunkList)) {}

                //取出region内chunk old
                std::cout << "new " << stand2 << " " << i_2 - 1 << std::endl;
                if (stand2 < i_2 - 2)
                    if (storageObj_->restoreRecipeAndChunk((char *)fileindex_2->fileNameHash, stand2 + 1, i_2 - 1,
                                                           newrestoredChunkList)) {}
                flag = 0;
            }
            if (flag == 1 && ((i_1 - 1 == stand1 && i_2 - 1 == stand2) || (i_1 == stand1 && i_2 == stand2))) {
                temp_pair.newID = stand1;
                temp_pair.oldID = stand2;
                transferPair.push_back(temp_pair);
                transferpairnumber++;
                std::cout << "old " << stand1 << " new " << stand2 << std::endl;
                if (fileindex_2->chunk[stand2].unique != 0) {
                    log.write((const char *)fileindex_1->chunk[stand1].fingerprint, CHUNK_HASH_SIZE);
                    log.write((const char *)fileindex_2->chunk[stand2].fingerprint, CHUNK_HASH_SIZE);
                }
                flag = 0;
            }
            if (flag == 2 && before == 1 && after == 0) {
                cout << "before" << endl;
                temp_pair.newID = stand1;
                temp_pair.oldID = stand2;
                transferPair.push_back(temp_pair);
                transferpairnumber++;
                if (fileindex_2->chunk[stand2].unique != 0) {
                    log.write((const char *)fileindex_1->chunk[stand1].fingerprint, CHUNK_HASH_SIZE);
                    log.write((const char *)fileindex_2->chunk[stand2].fingerprint, CHUNK_HASH_SIZE);
                }
                before = 0;
                after = 2;
            }
            if (flag == 2 && after == 1) {
                cout << "after" << endl;
                after = 0;
                flag = 0;
                temp_pair.newID = (i_1 - 1);
                temp_pair.oldID = (i_2 - 1);
                transferPair.push_back(temp_pair);
                transferpairnumber++;
                stand1 = transferPair[transferPair.size() - 2].newID;
                stand2 = transferPair[transferPair.size() - 2].oldID;
                if (fileindex_2->chunk[i_2 - 1].unique != 0) {
                    log.write((const char *)fileindex_1->chunk[i_1 - 1].fingerprint, CHUNK_HASH_SIZE);
                    log.write((const char *)fileindex_2->chunk[i_2 - 1].fingerprint, CHUNK_HASH_SIZE);
                }
                //if(i_1-stand1>size||i_2-stand2>size)continue;//不能让group内chunk过多

                std::cout << "old " << stand1 << " " << i_1 - 1 << std::endl;
                if (stand1 < i_1 - 2)
                    if (storageObj_->restoreRecipeAndChunk((char *)fileindex_1->fileNameHash, stand1 + 1, i_1 - 1,
                                                           oldrestoredChunkList)) {}

                //取出region内chunk old
                std::cout << "new " << stand2 << " " << i_2 - 1 << std::endl;
                if (stand2 < i_2 - 2)
                    if (storageObj_->restoreRecipeAndChunk((char *)fileindex_2->fileNameHash, stand2 + 1, i_2 - 1,
                                                           newrestoredChunkList)) {}
            }
        }
    }
    cout << "end" << endl;
    for (auto it = oldrestoredChunkList.begin(); it != oldrestoredChunkList.end(); it++) {
        if (fileindex_1->chunk[it->ID].unique == 0) continue;
        olduniqueSizes += it->logicDataSize;
        vector<SuperFeature> oldsuperFeatureList;
        fineseObj->extractSuperFeatures(it->logicData, 4096 / 2, config.getSFNumber(), oldsuperFeatureList);

        auto db = conn["test"];
        if (config.getSFNumber() == 4) {
            std::string temp((char *)it->chunkHash, CHUNK_HASH_SIZE);
            bsoncxx::document::value restaurant_doc = make_document(kvp("flag", "0"), kvp("hash", temp), kvp("group", "-1"),                                                                //std::to_string((int)it->ID),temp),
                                                                    kvp("SF", make_array(oldsuperFeatureList[0], oldsuperFeatureList[1], oldsuperFeatureList[2], oldsuperFeatureList[3]))); //std::to_string((int)it->ID)
            auto res = db["restaurants"].insert_one(std::move(restaurant_doc));
        }

        if (config.getSFNumber() == 2) {
            std::string temp((char *)it->chunkHash, CHUNK_HASH_SIZE);
            bsoncxx::document::value restaurant_doc = make_document(kvp("SF", make_document(kvp("F", oldsuperFeatureList[0]), kvp("S", oldsuperFeatureList[1]))),
                                                                    kvp("flag", "0"), kvp("hash", temp), kvp("group", "-1")); //std::to_string((int)it->ID)
            auto res = db["restaurants"].insert_one(std::move(restaurant_doc));
        }
    }

    //此处需要判断，region内部的chunk也有可能是duplicate的
    for (auto it = newrestoredChunkList.begin(); it != newrestoredChunkList.end(); it++) {
        if (fileindex_2->chunk[it->ID].unique == 0) continue;
        newuniqueSizes += it->logicDataSize;

        vector<SuperFeature> newsuperFeatureList;
        fineseObj->extractSuperFeatures(it->logicData, 4096 / 2, config.getSFNumber(), newsuperFeatureList);

        auto db = conn["test"];

        if (config.getSFNumber() == 4) {
            std::string temp((char *)it->chunkHash, CHUNK_HASH_SIZE);
            bsoncxx::document::value restaurant_doc = make_document(kvp("flag", "0"), kvp("hash", temp), kvp("group", "-1"),                                                                //std::to_string((int)it->ID),temp),
                                                                    kvp("SF", make_array(newsuperFeatureList[0], newsuperFeatureList[1], newsuperFeatureList[2], newsuperFeatureList[3]))); //std::to_string((int)it->ID)
            auto res = db["restaurants"].insert_one(std::move(restaurant_doc));
        }

        if (config.getSFNumber() == 2) {
            std::string temp((char *)it->chunkHash, CHUNK_HASH_SIZE);
            bsoncxx::document::value restaurant_doc = make_document(kvp("SF", make_document(kvp("F", newsuperFeatureList[0]), kvp("S", newsuperFeatureList[1]))),
                                                                    kvp("flag", "0"), kvp("hash", temp), kvp("group", "-1")); //std::to_string((int)it->ID)
            auto res = db["restaurants"].insert_one(std::move(restaurant_doc));
        }
    }
    std::cout << std::endl;

    auto db = conn["test"];
    string groupid;
    auto group_number = db["restaurants"].find(make_document(kvp("group_number", "0")));
    for (auto &&doc : group_number) groupid = doc["group_number"].get_utf8().value.to_string();
    if (atoi(groupid.c_str()) <= 0) {
        bsoncxx::document::value restaurant_doc = make_document(kvp("group_number", "0"), kvp("number", "0"));
        auto db = conn["test"];
        auto res = db["restaurants"].insert_one(std::move(restaurant_doc));
    }
    uniqueSizes = newuniqueSizes + olduniqueSizes;
    cout << transferpairnumber << endl;
    for (int i = 0; i < transferpairnumber; i++)
        cout << "Tnew transfer: " << transferPair[i].newID << " Told transfer: " << transferPair[i].oldID << endl;

    cout << "TotalUniqueSize: " << uniqueSizes << endl;
    cout << "oldUniqueSize: " << olduniqueSizes << endl;
    cout << "newUniqueSize: " << newuniqueSizes << endl;

    log.close();
    return transferPair;
}