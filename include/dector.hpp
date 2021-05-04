#ifndef GENERALDEDUPSYSTEM_DECTOR_HPP
#define GENERALDEDUPSYSTEM_DECTOR_HPP
#include "chunker.hpp"
#include <chrono>
 
#include <bsoncxx/builder/stream/document.hpp>
#include <bsoncxx/builder/basic/array.hpp>
#include <bsoncxx/builder/basic/document.hpp>
#include <bsoncxx/builder/basic/kvp.hpp>
#include <bsoncxx/types.hpp>


#include <mongocxx/stdx.hpp>
#include <mongocxx/client.hpp>
#include <mongocxx/instance.hpp>
#include <mongocxx/options/find.hpp>
#include <mongocxx/uri.hpp>
#include <bsoncxx/json.hpp>
#include <iostream>


class Dector {

private:

    StorageCore *storageObj_;
    ofstream log;
    
    FineseFunctions *fineseObj;
    NTransformFunctions *NtransObj;

    vector<Pair> containerPair;
    vector<Pair> segmentPair;
    vector<Pair>transferPair;
    int containerpairnumber;
    int segmentpairnumber;
    int transferpairnumber;

public:
    Dector(StorageCore *storageObj,Index fileindex_1,Index fileindex_2,int indexflag);
    ~Dector();

    vector<Pair> truetransferDector(Index *fileindex_1,Index *fileindex_2);
    
    vector<Pair> ContainerDector(Index *fileindex_1,Index *fileindex_2);
    vector<Pair> SegmentDector(Index *fileindex_1,Index *fileindex_2);
    vector<Pair> transferDector(Index *fileindex_1,Index *fileindex_2);
    
    vector<Pair> gettransfers() {
        return transferPair;
    }
    int gettransfercount() {

        return transferpairnumber;
    }
};
#endif