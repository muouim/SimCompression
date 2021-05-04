#include "manager.hpp"
using namespace std;
extern Configure config;

extern Database fp2ChunkDB;
extern Database fileName2metaDB;
extern Database deltaChunkDB;
extern Database containerDB;

Manager::Manager(StorageCore *storageObj) {
    storageObj_ = storageObj;
    DeltacontainerNamePrefix_ = config.getDeltaContainerRootPath();
    containerNamePrefix_ = config.getContainerRootPath();
    maxContainerSize_ = config.getMaxContainerSize();

    containerNameTail_ = ".container";
    ifstream fin;
    fin.open(".StorageConfig", ifstream::in);
    if (fin.is_open()) {
        fin >> lastContainerFileName_;
        fin.close();

    } else {
        lastContainerFileName_ = "abcdefghijklmno";
    }
}

Manager::~Manager() {
}

bool Manager::updateContainer() {
    leveldb::Iterator *iterator = deltaChunkDB.iterator();
    if (!iterator) {
        std::cerr << "can not new iterator" << std::endl;
    }

    //遍历Delta之后的Container，进行重写并改变db
    iterator->SeekToFirst();
    keyForChunkHashDB_t key;
    int count = 0;
    //改变db
    while (iterator->Valid()) {
        count++;
        leveldb::Slice sKey = iterator->key();
        string ans;

        bool status = fp2ChunkDB.query(sKey.ToString(), ans);
        if (status) {
            memcpy(&key, &ans[0], sizeof(keyForChunkHashDB_t));
            //最后一个container先不更新
            string name((char *)key.containerName);
            if (name == lastContainerFileName_) {
                iterator->Next();
                continue;
            }
            string deltaans;
            status = deltaChunkDB.query(sKey.ToString(), deltaans);
            keyForChunkHashDB_t update_key;
            memcpy(&update_key, &deltaans[0], sizeof(keyForChunkHashDB_t));

            string containerSize;
            string containerKey(containerNamePrefix_ + (char *)key.containerName + containerNameTail_);
            containerDB.query(containerKey, containerSize);

            int updateContainersize = stoi(containerSize) - (key.length - update_key.length);
            key.deltaCount = update_key.deltaCount;
            key.length = update_key.length;
            containerDB.insert(containerKey, to_string(updateContainersize));

            string dbValue;
            dbValue.resize(sizeof(keyForChunkHashDB_t));
            memcpy(&dbValue[0], &key, sizeof(keyForChunkHashDB_t));

            fp2ChunkDB.insert(sKey.ToString(), dbValue);
            char delta_data[MAX_CHUNK_SIZE * 2] = {0};

            readDeltaContainer(update_key, delta_data);
            updateContainerFile(key, update_key, delta_data);

            deltaChunkDB.delete_(sKey.ToString());
            count--;
        } else {
            cerr << "StorageCore : chunk not in database" << endl;
        }
        leveldb::Slice sVal = iterator->value();
        iterator->Next();
    }

    if (count == 0) {
        cout << "Delete DeltaContainer" << std::endl;
        system("rm ./Delta/*");
    }
    delete (iterator);
    return true;
}

bool Manager::readDeltaContainer(keyForChunkHashDB_t key, char *data) {
    
    ifstream containerIn;
    string containerNameStr((char *)key.containerName, lastContainerFileName_.length());
    string readName = DeltacontainerNamePrefix_ + containerNameStr + containerNameTail_;

    containerIn.open(readName, std::ifstream::in | std::ifstream::binary);
    if (!containerIn.is_open()) {
        std::cerr << "StorageCore : Can not open Container: " << readName << endl;
        return false;
    }
    containerIn.seekg(0, ios_base::end);
    int containerSize = containerIn.tellg();
    containerIn.seekg(0, ios_base::beg);
    containerIn.read(currentReadContainer_.body_, containerSize);
    if (containerIn.gcount() != containerSize) {
        cerr << "StorageCore : read container error" << endl;
        return false;
    }
    containerIn.close();
    currentReadContainer_.used_ = containerSize;
    memcpy(data, currentReadContainer_.body_ + key.offset, key.length);
    currentReadContainerFileName_ = containerNameStr;
    return true;
}
bool Manager::updateContainerFile(keyForChunkHashDB_t key, keyForChunkHashDB_t update_key, char *data) {
    
    ifstream containerIn;
    fstream containerUpdate;
    string containerNameStr((char *)key.containerName, lastContainerFileName_.length());
    string readName = containerNamePrefix_ + containerNameStr + containerNameTail_;
    //重新写入
    containerUpdate.open(readName, fstream::binary | fstream::out | fstream::in);
    if (!containerUpdate.is_open()) {
        std::cerr << "StorageCore : Can not update Container: " << readName << endl;
        return false;
    }
    containerUpdate.seekp(key.offset);
    containerUpdate.write(data, update_key.length);
    containerUpdate.close();
    return true;
}

bool Manager::rewriteContainer() {
    //1.此处进行container当前大小的判断，合并进行了delta的container来进行空间节省
    //加了一个container db，contianer name->size
    vector<string> containerList;
    leveldb::Iterator *citerator = containerDB.iterator();
    if (!citerator)
        std::cerr << "can not new iterator" << std::endl;

    citerator->SeekToFirst();
    keyForChunkHashDB_t ckey;
    while (citerator->Valid()) {
        string containerSize;
        leveldb::Slice sKey = citerator->key();
        bool status = containerDB.query(sKey.ToString(), containerSize);
        if (status) {
            int size = stoi(containerSize);
            if (size <= 8300000 && sKey.ToString() != containerNamePrefix_ + lastContainerFileName_ + containerNameTail_) {
                cout << "Rewrite Container: " << sKey.ToString() << " Size: " << size << endl;
                containerList.push_back(sKey.ToString());
                containerDB.delete_(sKey.ToString());
            }
        } else {
            std::cerr << "Container not exist" << std::endl;
        }
        leveldb::Slice sVal = citerator->value();
        citerator->Next();
    }
    delete (citerator);
    if (containerList.size() == 0) return true;

    //2.重写对应container里面的chunk
    //将需要重写的container内的chunk取出来重新存，然后最后删掉需要重写的container
    leveldb::Iterator *iterator = fp2ChunkDB.iterator();
    if (!iterator)
        std::cerr << "can not new iterator" << std::endl;

    iterator->SeekToFirst();
    keyForChunkHashDB_t key;
    while (iterator->Valid()) {
        string ans;
        leveldb::Slice sKey = iterator->key();
        bool status = fp2ChunkDB.query(sKey.ToString(), ans);

        if (status) {
            int re_flag = 0;
            memcpy(&key, &ans[0], sizeof(keyForChunkHashDB_t));
            for (int i = 0; i < containerList.size(); i++) {
                string temp(containerNamePrefix_ + (char *)key.containerName + containerNameTail_);
                if (temp == containerList[i]) {
                    re_flag = 1;
                    break;
                }
            }
            if (re_flag == 0) {
                iterator->Next();
                continue;
            } else {
                string chunkData;
                string chunkHash(sKey.ToString());
                storageObj_->retriveChunk((char *)chunkHash.c_str(), chunkData);
                storageObj_->saveChunk((char *)chunkHash.c_str(), (char *)chunkData.c_str(), chunkData.size());
                //不用管db，save的时候自己会更新
            }
        } else {
            cerr << "sssStorageCore : chunk not in database" << endl;
        }
        leveldb::Slice sVal = iterator->value();
        iterator->Next();
    }

    //3.删除原来container
    for (int i = 0; i < containerList.size(); i++) {
        string containerKey(containerList[i]);
        cout << "Delete: " << containerKey << endl;
        remove((char *)containerKey.c_str());
    }

    delete (iterator);
    return true;
}
bool Manager::GC() {
    //判断cout是否为0，为0就删除对应chunk的db，删掉之后rewrite就不会有对应的chunk了
    leveldb::Iterator *iterator = fp2ChunkDB.iterator();
    if (!iterator) {
        std::cerr << "can not new iterator" << std::endl;
    }

    iterator->SeekToFirst();
    std::cout << "=========== iterator begin ===========" << std::endl;
    keyForChunkHashDB_t key;

    while (iterator->Valid()) {
        string ans;
        leveldb::Slice sKey = iterator->key();
        bool status = fp2ChunkDB.query(sKey.ToString(), ans);

        if (status) {
            memcpy(&key, &ans[0], sizeof(keyForChunkHashDB_t));
            if (key.count == 0) fp2ChunkDB.delete_(sKey.ToString());
        } else {
            cerr << "sssStorageCore : chunk not in database" << endl;
        }
        leveldb::Slice sVal = iterator->value();
        iterator->Next();
    }
    std::cout << "=========== iterator end ===========" << std::endl;
    delete (iterator);
    return true;
}