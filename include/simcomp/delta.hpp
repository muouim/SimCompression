#ifndef SIMCOMP_DELTA_HPP
#define SIMCOMP_DELTA_HPP

#include "base.hpp"
#include "zdelta.hpp"

class SimCompCompression {
private:
    bool deltaCompression(u_char* baseSrcContent, uint32_t baseSrcLength, u_char* compressionSrcContent, uint32_t compressionSrcLength, u_char* compressionResultContent, uint32_t& compressionResultLength);
    
public:
    SimCompCompression();
    ~SimCompCompression();
    bool deltaCompressions(u_char* baseSrcContent, uint32_t baseSrcLength, u_char* compressionSrcContentList, vector<uint32_t> compressionSrcLengthList, u_char* compressionResultContent, vector<uint32_t>& compressionResultLengthList);
};

#endif