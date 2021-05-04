#include "compressor.hpp"
#include <bits/stdc++.h>

using namespace std;
extern Configure config;

extern Database fp2ChunkDB;
extern Database fileName2metaDB;
extern Database deltaChunkDB;
using bsoncxx::builder::basic::kvp;
using bsoncxx::builder::basic::make_array;
using bsoncxx::builder::basic::make_document;

DeltaCompressor::DeltaCompressor(StorageCore *storageObj) {
    storageObj_ = storageObj;

    containerNamePrefix_ = config.getDeltaContainerRootPath();
    maxContainerSize_ = config.getMaxDeltaContainerSize();
    containerNameTail_ = ".container";
    ifstream fin;
    fin.open(".DeltaConfig", ifstream::in);
    if (fin.is_open()) {
        fin >> lastContainerFileName_;
        fin >> currentContainer_.used_;
        fin.close();

        // read last container
        fin.open(containerNamePrefix_ + lastContainerFileName_ + containerNameTail_,
                 ifstream::in | ifstream::binary);
        fin.read(currentContainer_.body_, currentContainer_.used_);
        fin.close();
    } else {
        lastContainerFileName_ = "abcdefghijklmno";
        currentContainer_.used_ = 0;
    }
    cryptoObj_ = new CryptoPrimitive();
}

DeltaCompressor::~DeltaCompressor() {
    ofstream fout;
    fout.open(".DeltaConfig", ofstream::out);
    fout << lastContainerFileName_ << endl;
    fout << currentContainer_.used_ << endl;
    fout.close();

    string writeContainerName = containerNamePrefix_ + lastContainerFileName_ + containerNameTail_;
    currentContainer_.saveTOFile(writeContainerName);

    delete cryptoObj_;
}
void DeltaCompressor::CompressorInit(string path) {
}

bool DeltaCompressor::compress() {
    mongocxx::client conn{mongocxx::uri{}};
    int totalSize = 0;
    int deltaSize = 0;
    SimCompSimilarityCheck check;
    double result = 0.0;
    fstream fin;
    fin.open("similar.log");
    //Transfer-region 的delta compression
    while (!fin.eof() && fin.is_open()) {
        //根据hash取出basechunk和newhash
        char basehash[CHUNK_HASH_SIZE] = {0};
        fin.read(basehash, CHUNK_HASH_SIZE);
        string basechunk;
        if (storageObj_->restoreChunk((char *)basehash, basechunk)) {
        }
        if (fin.eof()) break;

        char newhash[CHUNK_HASH_SIZE] = {0};
        fin.read(newhash, CHUNK_HASH_SIZE);

        string newchunk;
        if (storageObj_->restoreChunk((char *)newhash, newchunk)) {
        }

        string hash1(basehash, CHUNK_HASH_SIZE);
        string hash2(newhash, CHUNK_HASH_SIZE);

        if (hash1 == hash2) {
            cout << "hash error" << endl;
            continue;
        }
        if (basechunk.substr(0, basechunk.size() - 2) == newchunk.substr(0, newchunk.size() - 2)) {
            cout << "chunk error" << endl;
            continue;
        }

        //计算delta

        char delta[MAX_CHUNK_SIZE * 2] = {0};
        char nodelta[MAX_CHUNK_SIZE * 2] = {0};

        delta_create(basechunk.c_str(), basechunk.size(), newchunk.c_str(), newchunk.size(), delta);
        delta_apply(basechunk.c_str(), basechunk.size(), (char *)delta, strlen(delta), nodelta);

        string data(basehash, CHUNK_HASH_SIZE);
        data += delta;

        if (newchunk.size() <= data.size() || data.size() >= MAX_CHUNK_SIZE) {
            cout << "similar error" << endl;
            continue;
        }

        deltaSize += data.size();
        totalSize += newchunk.size();
    }
    fin.close();
    remove("similar.log");

    int totalSize_ = totalSize;
    int deltaSize_ = deltaSize;

    //region内部chunk的delta compression
    //flag: 0->未delta、1->已delta、2->base;
    //group: -1->未分组、0-n->在哪个分组
    //group 划分
    auto db = conn["test"];
    //auto cursor = db["restaurants"].find({make_document(kvp("group","-1"))});
    mongocxx::pipeline stages;
    mongocxx::options::aggregate options;
    options.allow_disk_use(true);
    options.batch_size(30);

    stages.match(make_document(kvp("group", "-1")))
        .group(make_document(kvp("_id", "$SF.F"),
                             kvp("count", make_document(kvp("$sum", 1)))));

    auto cursor = db["restaurants"].aggregate(stages, options);
    int a = 0;

    for (auto &&doc : cursor) {
        int count = doc["count"].get_int32().value;

        if (count > 1) {
            mongocxx::options::find options2;
            options2.no_cursor_timeout(true);

            auto sfgroup = db["restaurants"].find(make_document(kvp("SF.F", doc["_id"].get_utf8().value.to_string())), options2);

            vector<string> group;
            for (auto &&doc2 : sfgroup) {
                std::string temphash = doc2["hash"].get_utf8().value.to_string();
                group.push_back(temphash);
                std::cout << a << std::endl;
            }
            groupList.push_back(group);
            db["restaurants"].update_many(
                make_document(kvp("SF.F", doc["_id"].get_utf8().value.to_string())),
                make_document(kvp("$set", make_document(kvp("group", "1"))))); //有问题,没更新,好像是结束之后才会更新*/
            std::cout << bsoncxx::to_json(doc) << " " << a << " " << doc["_id"].get_utf8().value.to_string() << std::endl;
        }
        a++;
    }
    std::cout << "_______________________" << std::endl;
    mongocxx::pipeline stages2;

    stages2.match(make_document(kvp("group", "-1")))
        .group(make_document(kvp("_id", "$SF.S"),
                             kvp("count", make_document(kvp("$sum", 1)))));

    auto cursor2 = db["restaurants"].aggregate(stages2, options);

    for (auto &&doc : cursor2) {
        int count = doc["count"].get_int32().value;

        if (count > 1) {
            mongocxx::options::find options2;
            options2.no_cursor_timeout(true);
            auto sfgroup = db["restaurants"].find(make_document(kvp("SF.S", doc["_id"].get_utf8().value.to_string())), options2);

            vector<string> group;
            for (auto &doc2 : sfgroup) {
                std::string temphash = doc2["hash"].get_utf8().value.to_string();
                group.push_back(temphash);
                std::cout << a << std::endl;
            }
            groupList.push_back(group);
            db["restaurants"].update_many(
                make_document(kvp("SF.S", doc["_id"].get_utf8().value.to_string())),
                make_document(kvp("$set", make_document(kvp("group", "1"))))); //有问题,没更新,好像是结束之后才会更新*/
            std::cout << bsoncxx::to_json(doc) << " " << a << std::endl;
        }
        a++;
    }
    int number = groupList.size();
    cout << "______________" << groupList.size() << " " << a << endl;

    //根据划分的group进行delta compression，并存储delta chunk
    for (int i = 0; i < groupList.size(); i++) {
        string basechunk;
        if (storageObj_->restoreChunk((char *)groupList[i][0].c_str(), basechunk)) {
            deltaSize += basechunk.size();
            totalSize += basechunk.size();
        } else {
            cout << "restore error" << endl;
            continue;
        }

        string newchunk;
        cout << groupList[i].size() << endl;

        for (int j = 1; j < groupList[i].size(); j++) {
            if (groupList[i][j] == groupList[i][0]) {
                cout << "same error" << endl;
                continue;
            }
            if (storageObj_->restoreChunk((char *)groupList[i][j].c_str(), newchunk)) {
                totalSize += newchunk.size();

                char delta[MAX_CHUNK_SIZE * 2] = {0};
                delta_create(basechunk.c_str(), basechunk.size(), newchunk.c_str(), newchunk.size(), delta);
                char nodelta[MAX_CHUNK_SIZE * 2] = {0};

                delta_apply(basechunk.c_str(), basechunk.size(), (char *)delta, strlen(delta), nodelta);

                string data((char *)groupList[i][0].c_str(), CHUNK_HASH_SIZE);
                data += delta;

                string temp(nodelta, strlen(nodelta));

                cout << "_____________________" << endl;

                if (newchunk.size() <= data.size() || data.size() >= MAX_CHUNK_SIZE) {
                    cout << "group error" << endl;
                    deltaSize += newchunk.size();
                    continue;
                }
                deltaSize += data.size();
                totalSize_ += newchunk.size();
                deltaSize_ += data.size();
            } else {
                cerr << "DeltaCore : can not restore deltachunk" << endl;
            }
        }
        cout << "______________" << endl;
    }
    cout << "______________" << number << endl;

    std::cout << "Unique chunks Total size:" << totalSize << " bytes" << std::endl;
    std::cout << "Delta chunks Total size:" << deltaSize << " bytes" << std::endl;

    std::cout << "(Delta)Unique chunks Total size:" << totalSize_ << " bytes" << std::endl;
    std::cout << "(Delta)Delta chunks Total size:" << deltaSize_ << " bytes" << std::endl;
    return true;
}
bool DeltaCompressor::saveDeltaChunk(char *chunkhash, std::string baseHash, char *chunkData, int chunkSize) {
    //TODO::用一个db存hash->keyForChunkHashDB_t 记录delta之后chunk的信息，将chunk写入Message
    string chunkHash(chunkhash, CHUNK_HASH_SIZE);

    keyForChunkHashDB_t key;
    key.length = chunkSize;
    key.deltaCount = 0;

    //取chunkdb，delta+1
    string ans;
    bool status = fp2ChunkDB.query(chunkHash, ans);
    if (status) {
        memcpy(&key, &ans[0], sizeof(keyForChunkHashDB_t));
        key.deltaCount++;
    } else {
        cerr << "Delta StorageCore : chunk not in database" << endl;
        return false;
    }
    key.length = chunkSize;
    //记录要更新的db
    status = writeDeltaContainer(key, chunkData);
    if (!status) {
        std::cerr << "Error write Delta container" << endl;
        return status;
    }

    string dbValue;
    dbValue.resize(sizeof(keyForChunkHashDB_t));
    memcpy(&dbValue[0], &key, sizeof(keyForChunkHashDB_t));
    status = deltaChunkDB.insert(chunkHash, dbValue);
    if (!status) {
        std::cerr << "Can't insert Delta chunk to database" << endl;
        return false;
    } else {
        currentContainer_.used_ += key.length;
        return true;
    }
}

bool DeltaCompressor::restoreDeltaChunk(char *chunkhash, std::string &chunkDataStr) {
    string chunkHash(chunkhash, CHUNK_HASH_SIZE);
    keyForChunkHashDB_t key;
    string ans;
    bool status = deltaChunkDB.query(chunkHash, ans);
    if (status) {
        memcpy(&key, &ans[0], sizeof(keyForChunkHashDB_t));
        char chunkData[key.length];
        if (readDeltaContainer(key, chunkData)) {
            chunkDataStr.resize(key.length);
            memcpy(&chunkDataStr[0], chunkData, key.length);
            return true;
        } else {
            cerr << "Delta StorageCore : can not read container for chunk" << endl;
            return false;
        }
    } else {
        cerr << "Delta StorageCore : chunk not in database" << endl;
        return false;
    }
}
bool DeltaCompressor::writeDeltaContainer(keyForChunkHashDB_t &key, char *data) {
    if (key.length + currentContainer_.used_ < maxContainerSize_) {
        memcpy(&currentContainer_.body_[currentContainer_.used_], data, key.length);
        memcpy(key.containerName, &lastContainerFileName_[0], lastContainerFileName_.length());
    } else {
        string writeContainerName = containerNamePrefix_ + lastContainerFileName_ + containerNameTail_;
        currentContainer_.saveTOFile(writeContainerName);
        next_permutation(lastContainerFileName_.begin(), lastContainerFileName_.end());
        currentContainer_.used_ = 0;
        memcpy(&currentContainer_.body_[currentContainer_.used_], data, key.length);
        memcpy(key.containerName, &lastContainerFileName_[0], lastContainerFileName_.length());
    }
    key.offset = currentContainer_.used_;
    return true;
}

bool DeltaCompressor::readDeltaContainer(keyForChunkHashDB_t key, char *data) {
    ifstream containerIn;
    string containerNameStr((char *)key.containerName, lastContainerFileName_.length());
    string readName = containerNamePrefix_ + containerNameStr + containerNameTail_;
    if (containerNameStr.compare(currentReadContainerFileName_) == 0) {
        memcpy(data, currentReadContainer_.body_ + key.offset, key.length);
        return true;
    } else if (containerNameStr.compare(lastContainerFileName_) == 0) {
        memcpy(data, currentContainer_.body_ + key.offset, key.length);
        return true;
    } else {
        containerIn.open(readName, std::ifstream::in | std::ifstream::binary);
        if (!containerIn.is_open()) {
            std::cerr << "Delta StorageCore : Can not open Container: " << readName << endl;
            return false;
        }
        containerIn.seekg(0, ios_base::end);
        int containerSize = containerIn.tellg();
        containerIn.seekg(0, ios_base::beg);
        containerIn.read(currentReadContainer_.body_, containerSize);
        if (containerIn.gcount() != containerSize) {
            cerr << "Delta StorageCore : read container error" << endl;
            return false;
        }
        containerIn.close();
        currentReadContainer_.used_ = containerSize;
        memcpy(data, currentReadContainer_.body_ + key.offset, key.length);
        currentReadContainerFileName_ = containerNameStr;
        return true;
    }
}

bool DeltaContainer::saveTOFile(string fileName) {
    ofstream containerOut;
    containerOut.open(fileName, std::ofstream::out | std::ofstream::binary);
    if (!containerOut.is_open()) {
        cerr << "Can not open Delta Container file : " << fileName << endl;
        return false;
    }
    containerOut.write(this->body_, this->used_);
    cerr << "Delta Container : save " << setbase(10) << this->used_ << " bytes to file system" << endl;
    containerOut.close();
    return true;
}