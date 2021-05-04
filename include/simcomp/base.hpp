#ifndef SIMCOMP_BASE_HPP
#define SIMCOMP_BASE_HPP

#include <vector>
#include <iostream>
#include <openssl/md5.h>
#include <cstring>
#include <iomanip>
#include <math.h>
#include <cassert>
#include "rocksdb/db.h"
#include <stddef.h>

#include <stdio.h>
#include <stdlib.h>


#define  Feature  uint16_t
#define  SuperFeature  string
using namespace std; 

//typedef char u_char;



class SimCompFeatureExtract {
public:
    SimCompFeatureExtract();
    ~SimCompFeatureExtract();
    virtual bool extractFeatures(u_char* srcContent, uint32_t srcLength, uint32_t targetFeatureNumber, vector<Feature>& featureList) = 0;
    virtual bool extractSuperFeaturesViaFeatures(vector<Feature> featureList, vector<SuperFeature>& superFeatureList) = 0;
    virtual bool extractSuperFeatures(u_char* srcContent, uint32_t srcLength, uint32_t targetSuperFeatureNumber, vector<SuperFeature>& superFeatureList) = 0;
};

#endif