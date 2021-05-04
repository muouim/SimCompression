#ifndef METADEDUP_DATASET_H_
#define METADEDUP_DATASET_H_

/* define metadata variables */
/* file recipe entry size: chunk hash (20 bytes), chunk size (8 bytes), key (32 bytes )*/
#define FR_ENTRY_FP 20 
#define FR_ENTRY_SIZE 8 
#define FR_ENTRY_OTHER 2 

#define FR_ENTRY_LENGTH (FR_ENTRY_FP + FR_ENTRY_SIZE + FR_ENTRY_OTHER) 

#define KR_ENTRY_LENGTH 32 

/* fingerprint index entry size: chunk hash (20 bytes), address (8 bytes), other (2 bytes)  */
#define IDX_ENTRY_FP FR_ENTRY_FP
#define IDX_ENTRY_OTHER FR_ENTRY_OTHER
#define IDX_ENTRY_ADDR 8
#define IDX_ENTRY_LENGTH (IDX_ENTRY_FP + IDX_ENTRY_ADDR + IDX_ENTRY_OTHER)

/* define dataset-specific variables */
/* we transform the format of all other datasets to that of the FSL dataset, thus the fingerprint size is fixed of 6 bytes */
#define FP_SIZE 18
#endif


