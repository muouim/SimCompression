#include <iostream>
#include "chunker.hpp"
#include "dector.hpp"
#include "compressor.hpp"
#include "manager.hpp"

Configure config("../config.json");

Database fp2ChunkDB;
Database fileName2metaDB;
Database deltaChunkDB;
Database containerDB;

StorageCore *storageObj;
StorageCore *sstorageObj;

void CTRLC(int s) {
    cerr << "server close" << endl;

    if (storageObj != nullptr) delete storageObj;
    if (sstorageObj != nullptr) delete sstorageObj;

    exit(0);
}
int main(int argc, char *argv[]) {
    struct sigaction sa;
    sa.sa_handler = SIG_IGN;
    sigaction(SIGPIPE, &sa, 0);

    sa.sa_handler = CTRLC;
    sigaction(SIGKILL, &sa, 0);
    sigaction(SIGINT, &sa, 0);

    storageObj = new StorageCore();
    fp2ChunkDB.openDB(config.getFp2ChunkDBName());
    fileName2metaDB.openDB(config.getFp2MetaDBame());
    deltaChunkDB.openDB(config.getdeltaChunkDBame());
    containerDB.openDB(config.getcontainerDBame());

    if (strcmp("-c", argv[1]) == 0) {
        string fileName(argv[2]);
        struct timeval start, end;
        int indexflag = config.getIndexflag();

        Chunker *chunkerfile = new Chunker(storageObj, fileName, !indexflag);
        chunkerfile->chunking();

        if (storageObj != nullptr) delete storageObj;

        int version = 0;
        fstream indexfile;
        fileName = fileName.substr(0, fileName.size() - 2);
        indexfile.open("./Index/" + fileName + "-" + to_string(version));

        while (indexfile.is_open()) {
            indexfile.close();
            version++;
            indexfile.open("./Index/" + fileName + "-" + to_string(version));
        }
        chunkerfile->saveToFile(fileName + "-" + to_string(version));
        indexfile.close();

        if (version < 1) {
            delete chunkerfile;
            cout << "New File, No old version index is found" << endl;
        } else {
            if (sstorageObj != nullptr) delete sstorageObj;
            sstorageObj = new StorageCore();
            gettimeofday(&start, NULL);
            Index c = chunkerfile->loadIndex(fileName + "-" + to_string(version - 1));
            Index d = chunkerfile->returnindex();

            delete chunkerfile;

            Dector dector(sstorageObj, c, d, !indexflag);

            int num = dector.gettransfercount();

            gettimeofday(&end, NULL);
            double time_taken = (end.tv_sec - start.tv_sec) * 1e6;
            time_taken = (time_taken + (end.tv_usec - start.tv_usec)) * 1e-6;
            printf("No index: Transfer region number: %d, Total time is %lf s\n", num, time_taken);
            indexfile.close();
            if (sstorageObj != nullptr) delete sstorageObj;
        }
    } else if (strcmp("-rw", argv[1]) == 0) {
        struct timeval start, end;
        gettimeofday(&start, NULL);

        if (sstorageObj != nullptr) delete sstorageObj;
        sstorageObj = new StorageCore();

        //DeltaCompressor
        DeltaCompressor *compressor = new DeltaCompressor(sstorageObj);
        compressor->compress();
        delete compressor;

        if (sstorageObj != nullptr) delete sstorageObj;
        sstorageObj = new StorageCore();

        Manager *manager = new Manager(sstorageObj);
        //manager->updateContainer();
        delete manager;

        sstorageObj = new StorageCore();

        manager = new Manager(sstorageObj);
        //manager->rewriteContainer();
        delete manager;

        gettimeofday(&end, NULL);
        double time_taken = (end.tv_sec - start.tv_sec) * 1e6;
        time_taken = (time_taken + (end.tv_usec - start.tv_usec)) * 1e-6;
        printf("Compression time is %lf s\n", time_taken);

        if (sstorageObj != nullptr) delete sstorageObj;
    } else if (strcmp("-rs", argv[1]) == 0) {
        if (sstorageObj != nullptr) delete sstorageObj;
        sstorageObj = new StorageCore();

        string fileName(argv[2]);
        ChunkList_t newrestoredChunkList;
        Chunker *chunkerfile = new Chunker(sstorageObj, fileName, 1);

        Index c = chunkerfile->loadIndex(fileName);
        cout << "____" << c.chunknumber << "___" << endl;
        ofstream fout;
        fout.open(fileName.substr(0, fileName.size() - 2));

        if (sstorageObj->restoreRecipeAndChunk((char *)c.fileNameHash, 0,
                                               c.chunknumber, newrestoredChunkList))
            for (auto it = newrestoredChunkList.begin(); it != newrestoredChunkList.end(); it++) {
                cout << "____" << it->ID << "___" << endl;
                cout << "________________" << endl;
                fout.write((char *)it->logicData, it->logicDataSize);
            }
        fout.close();
    } else {
        struct timeval start, end;
        //两种方式chunking的同时生成index
        int indexflag = config.getIndexflag();
        /*Chunker *test1=new Chunker(storageObj,argv[1],indexflag);
        test1->chunking();

        Chunker *test2=new Chunker(storageObj,argv[2],indexflag);
        test2->chunking();*/

        Chunker *test3 = new Chunker(storageObj, argv[1], !indexflag);
        test3->chunking();
        Chunker *test4 = new Chunker(storageObj, argv[2], !indexflag);
        test4->chunking();
        if (storageObj != nullptr) delete storageObj;

        gettimeofday(&start, NULL);
        sstorageObj = new StorageCore();

        gettimeofday(&start, NULL);
        Index c = test3->returnindex(), d = test4->returnindex();

        delete test3;
        delete test4;

        Dector dector2(sstorageObj, c, d, !indexflag);

        int num2 = dector2.gettransfercount();

        gettimeofday(&end, NULL);
        double time_taken = (end.tv_sec - start.tv_sec) * 1e6;
        time_taken = (time_taken + (end.tv_usec - start.tv_usec)) * 1e-6;
        printf("No index: Transfer region number: %d, Total time is %lf s\n", num2, time_taken);
        ChunkList_t newrestoredChunkList;

        if (sstorageObj != nullptr) delete sstorageObj;

        sstorageObj = new StorageCore();

        //DeltaCompressor
        DeltaCompressor *compressor = new DeltaCompressor(sstorageObj);
        compressor->compress();
        delete compressor;

        if (sstorageObj != nullptr) delete sstorageObj;
        sstorageObj = new StorageCore();

        Manager *manager = new Manager(sstorageObj);
        manager->updateContainer();
        delete manager;

        sstorageObj = new StorageCore();

        manager = new Manager(sstorageObj);
        manager->rewriteContainer();
        delete manager;

        if (sstorageObj != nullptr) delete sstorageObj;
        sstorageObj = new StorageCore();

        if (sstorageObj->restoreRecipeAndChunk((char *)d.fileNameHash, 0, 1253,
                                               newrestoredChunkList))
            for (auto it = newrestoredChunkList.begin(); it != newrestoredChunkList.end(); it++) {
                /*cout<<"____"<<it->ID<<"___"<<endl;
                                cout<<it->logicData<<endl;
                                cout<<"________________"<< endl;*/
            }
    }
    return 0;
}
