# SimCompression
a library for similarity-based compression

## Build Guide 
* This library is build based on CMake tool-chain with c++ compilers which has support for at least c++14 standers.
* Additional library is listing here:
    * Openssl version 1.1.1c
    * [RocksDB](https://github.com/facebook/rocksdb) provided by facebook
    
## Library APIs

This library mainly provides data similarity discrimination and accuracy check function based on feature extraction.

### Library Contents: 

* Finese Feature & Super Feature Extract Functions
* N-Transform Feature & Super Feature Extract Functions
* Euclidean Distance & Hamming Distance Compute Functions
* Delta Compression Functions

### Library APIs

#### Basic Classes:
##### Feature Extracting 
```c++
class SimCompFeatureExtract {
Public:
    SimCompFeatureExtract();
    ~SimCompFeatureExtract();
    virtual bool extractFeatures(u_char* srcContent, uint32_t srcLength, uint32_t targetFeatureNumber, vector<Feature>& featureList) = 0;
    virtual bool extractSuperFeaturesViaFeatures(vector<Feature> featureList, vector<SuperFeature>& superFeatureList) = 0;
    virtual bool extractSuperFeatures(u_char* srcContent, uint32_t srcLength, uint32_t targetSuperFeatureNumber, vector<SuperFeature>& superFeatureList) = 0;
}
```

##### Similarity Check
```c++
class SimCompSimilarityCheck {
Private:
    bool euclideanDistanceCompute(u_char* firstSrcContent, uint32_t firstSrcLength, u_char* secondSrcContent, uint32_t secondSrcLength, double& finalDistanceResult);
    bool hammingDistanceCompute(u_char* firstSrcContent, uint32_t firstSrcLength, u_char* secondSrcContent, uint32_t secondSrcLength, double& finalDistanceResult);
    // any other distance compute algorithms
Public:
    SimCompSimilarityCheck();
    ~SimCompSimilarityCheck();
    bool similarityCheckViaDistanceCompute(uint32_t computeMethodType, u_char* firstSrcContent, uint32_t firstSrcLength, u_char* secondSrcContent, uint32_t secondSrcLength, double& finalDistanceResult);
    bool similarityCheckViaSuperFeatures(vector<SuperFeature>& firstSuperFeatureList, vector<SuperFeature>& secondSuperFeatureList, double& finalDistanceResult);
}
```

##### Compression

```c++
class SimCompCompression {
Private:
     bool deltaCompression(u_char* baseSrcContent, uint32_t baseSrcLength, u_char* compressionSrcContent, uint32_t compressionSrcLength, u_char* compressionResultContent, uint32_t& compressionResultLength, DCStructure& additionalInformationForDeltaCompression);
    
Public:
    SimCompCompression();
    ~SimCompCompression();
    bool deltaCompressions(u_char* baseSrcContent, uint32_t baseSrcLength, u_char* compressionSrcContentList, vector<uint32_t> compressionSrcLengthList, u_char* compressionResultContent, vector<uint32_t>& compressionResultLengthList, vector<DCStructure>& additionalInformationForDeltaCompressionList);
}
```


#### Derived Classes

##### Feature Extracting
```c++
class FineseFunctions : public SimCompFeatureExtract{
Public:
    FineseFunctions();
    ~FineseFunctions();
    bool extractFeatures(u_char* srcContent, uint32_t srcLength, uint32_t targetFeatureNumber, vector<Feature>& featureList);
    bool extractSuperFeaturesViaFeatures(vector<Feature> featureList, vector<SuperFeature>& superFeatureList);
    bool extractSuperFeatures(u_char* srcContent, uint32_t srcLength, uint32_t targetSuperFeatureNumber, vector<SuperFeature>& superFeatureList);
}
```

```c++
class NTransformFunctions  : public SimCompFeatureExtract{
Public:
    NTransformFunctions();
    ~NTransformFunctions();
    bool extractFeatures(u_char* srcContent, uint32_t srcLength, uint32_t targetFeatureNumber, vector<Feature>& featureList);
    bool extractSuperFeaturesViaFeatures(vector<Feature> featureList, vector<SuperFeature>& superFeatureList);
    bool extractSuperFeatures(u_char* srcContent, uint32_t srcLength, uint32_t targetSuperFeatureNumber, vector<SuperFeature>& superFeatureList);
}
```

## Sliding window method

### Two paramaters

* Sliding window size (size)
* Number of swipes when the window is completely different (n)

### Decision method

1. Init sliding window in both old and new stream with same size;
2. Compare two sliding window:
    1. The chunks in the window are exactly the same. Slide the start position of the new window to the end position of the current window. 
    2. The chunks at the beginning of the window are the same, and thereafter are completely different,two window slide down to the last same chunk: 
        * Old stream's window slide down n times to compare with new stream's current window: if all different, it's not delete, or it's delete
        * New stream's window slide down n times to compare with old stream's current window: if all different, it's not insert, or it's insert
        * Both 1 & 2 are different, consider it's modify
    3. Both beginning and ending have same chunks, but middle is different:
        * New stream same chunk ending before old stream is delete
        * New stream same chunk ending after old stream is insert
        * Two stream end with same chunk in current window is modify

## System Command

Implement SimCompression into a deduplication system

```shell
./bin/simcomp -c filename-n # -c Store the backup file, n represent the backup version number

-rw # -rw Perform Similarity Detection and Delta Compression

-rs -filename-n # -rw Restore the backup file, n represent the backup version number
```

