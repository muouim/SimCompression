#ifndef SIMCOMP_NTRANSFORM_HPP
#define SIMCOMP_NTRANSFORM_HPP

#include "base.hpp"

class NTransformFunctions  : public SimCompFeatureExtract{
public:
    NTransformFunctions();
    ~NTransformFunctions();
    bool extractFeatures(u_char* srcContent, uint32_t srcLength, uint32_t targetFeatureNumber, vector<Feature>& featureList);
    bool extractSuperFeaturesViaFeatures(vector<Feature> featureList, vector<SuperFeature>& superFeatureList);
    bool extractSuperFeatures(u_char* srcContent, uint32_t srcLength, uint32_t targetSuperFeatureNumber, vector<SuperFeature>& superFeatureList);
};

#endif