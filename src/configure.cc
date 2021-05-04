//
// Created by a on 11/17/18.
//

#include "configure.hpp"

Configure::~Configure() {
}
Configure::Configure() {
}
Configure::Configure(std::string path) {
    this->readConf(path);
}

void Configure::readConf(std::string path) {
    //boost read_json
    using namespace boost;
    using namespace boost::property_tree;
    ptree root;
    read_json<ptree>(path, root);

    //Chunker Configure
    _runningType = root.get<uint64_t>("ChunkerConfig._runningType");
    _chunkingType = root.get<uint64_t>("ChunkerConfig._chunkingType");
    _maxChunkSize = root.get<uint64_t>("ChunkerConfig._maxChunkSize");
    _minChunkSize = root.get<uint64_t>("ChunkerConfig._minChunkSize");
    _slidingWinSize = root.get<uint64_t>("ChunkerConfig._slidingWinSize");

    _maxslidingWinSize = root.get<uint64_t>("IndexConfig._maxslidingWinSize");
    _minslidingWinSize = root.get<uint64_t>("IndexConfig._minslidingWinSize");
    _segmentSize = root.get<uint64_t>("ChunkerConfig._segmentSize");
    _averageChunkSize = root.get<uint64_t>("ChunkerConfig._avgChunkSize");
    _ReadSize = root.get<uint64_t>("ChunkerConfig._ReadSize");

    _Fileindex = root.get<uint64_t>("ChunkerConfig._Fileindex");
    _SFNumber = root.get<uint64_t>("ChunkerConfig._SFNumber");

    _maxContainerSize = root.get<uint64_t>("SPConfig._maxContainerSize");

    _RecipeRootPath = root.get<std::string>("server._RecipeRootPath");
    _containerRootPath = root.get<std::string>("server._containerRootPath");
    _fp2ChunkDBName = root.get<std::string>("server._fp2ChunkDBName");
    _fp2MetaDBame = root.get<std::string>("server._fp2MetaDBame");
    _deltaChunkDBame = root.get<std::string>("server._deltaChunkDBame");
    _containerDBame = root.get<std::string>("server._containerDBame");

    _maxDeltaContainerSize = root.get<uint64_t>("SPConfig._maxDeltaContainerSize");

    _deltacontainerRootPath = root.get<std::string>("server._deltacontainerRootPath");
}

uint64_t Configure::getRunningType() {
    return _runningType;
}

// chunking settings
uint64_t Configure::getChunkingType() {
    return _chunkingType;
}

uint64_t Configure::getMaxChunkSize() {
    return _maxChunkSize;
}

uint64_t Configure::getMinChunkSize() {
    return _minChunkSize;
}

uint64_t Configure::getAverageChunkSize() {
    return _averageChunkSize;
}

uint64_t Configure::getSlidingWinSize() {
    return _slidingWinSize;
}
uint64_t Configure::getMaxSlidingWinSize() {
    return _maxslidingWinSize;
}
uint64_t Configure::getMinSlidingWinSize() {
    return _minslidingWinSize;
}
uint64_t Configure::getSegmentSize() {
    return _segmentSize;
}

uint64_t Configure::getReadSize() {
    return _ReadSize;
}
uint64_t Configure::getIndexflag() {
    return _Fileindex;
}
uint64_t Configure::getMaxContainerSize() {
    return _maxContainerSize;
}

uint64_t Configure::getMaxDeltaContainerSize() {
    return _maxDeltaContainerSize;
}
uint64_t Configure::getSFNumber() {
    return _SFNumber;
}

std::string Configure::getRecipeRootPath() {
    return _RecipeRootPath;
}

std::string Configure::getContainerRootPath() {
    return _containerRootPath;
}

std::string Configure::getFp2ChunkDBName() {
    return _fp2ChunkDBName;
}

std::string Configure::getFp2MetaDBame() {
    return _fp2MetaDBame;
}

std::string Configure::getdeltaChunkDBame() {
    return _deltaChunkDBame;
}

std::string Configure::getcontainerDBame() {
    return _containerDBame;
}
std::string Configure::getDeltaContainerRootPath() {
    return _deltacontainerRootPath;
}