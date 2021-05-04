#ifndef SIMCOMP_CHECK_HPP
#define SIMCOMP_CHECK_HPP


#include "base.hpp"

class SimCompSimilarityCheck {
private:
    bool euclideanDistanceCompute(u_char* firstSrcContent, uint32_t firstSrcLength, u_char* secondSrcContent, uint32_t secondSrcLength, double& finalDistanceResult);
    bool hammingDistanceCompute(u_char* firstSrcContent, uint32_t firstSrcLength, u_char* secondSrcContent, uint32_t secondSrcLength, double& finalDistanceResult);
    bool editDistanceCompute(u_char* firstSrcContent, uint32_t firstSrcLength, u_char* secondSrcContent, uint32_t secondSrcLength, double& finalDistanceResult);
    // any other distance compute algorithms
public:
    SimCompSimilarityCheck();
    ~SimCompSimilarityCheck();
    bool similarityCheckViaDistanceCompute(uint32_t computeMethodType, u_char* firstSrcContent, uint32_t firstSrcLength, u_char* secondSrcContent, uint32_t secondSrcLength, double& finalDistanceResult);
    bool similarityCheckViaSuperFeatures(vector<SuperFeature>& firstSuperFeatureList, vector<SuperFeature>& secondSuperFeatureList, double& finalDistanceResult);
};

#endif