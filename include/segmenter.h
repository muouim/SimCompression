#ifndef METADEDUP_SEGMENTER_H_
#define METADEDUP_SEGMENTER_H_

#include <bits/stdc++.h>
#include <openssl/bn.h>
#include <openssl/md5.h>
#include <queue>

typedef struct {
    unsigned char fp[6];
    uint64_t size;
} ChunkInfo;

class Segmenter {
public:
    /* constructor */
    Segmenter();

    /* deconstructor */
    ~Segmenter();

    /* add a chunk into current segment */
    size_t AddChunk(ChunkInfo item);
    bool PopChunk(ChunkInfo& item);
    size_t GetNumOfChunks() { return chunkSegment.size(); }

private:
    /* segment */
    std::queue<ChunkInfo> chunkSegment;

    /* end of segment */
    bool EndOfSegment(const char* hash);

    /* segment size */
    size_t segmentSize = 0;

    /* parameters for segmentation */
    BIGNUM* bnDivisor = NULL;
    BIGNUM* bnPattern = NULL;
};

#endif
