#ifndef SIMCOMP_FINESE_HPP
#define SIMCOMP_FINESE_HPP

#include "base.hpp"

class FineseFunctions : public SimCompFeatureExtract{
private:
    //vector<Feature>List;
public:
    FineseFunctions();
    ~FineseFunctions();
    bool extractFeatures(u_char* srcContent, uint32_t srcLength, uint32_t targetFeatureNumber, vector<Feature>& featureList);
    bool extractSuperFeaturesViaFeatures(vector<Feature> featureList, vector<SuperFeature>& superFeatureList);
    bool extractSuperFeatures(u_char* srcContent, uint32_t srcLength, uint32_t targetSuperFeatureNumber, vector<SuperFeature>& superFeatureList);
};

#endif