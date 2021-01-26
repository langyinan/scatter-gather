////////////////////////////////////////////////////////////////////////////////
//
//  File           : sg_driver.c
//  Description    : This file contains the driver code to be developed by
//                   the students of the 311 class.  See assignment details
//                   for additional information.
//
//   Author        : Yinan Lang
//   Last Modified : 12/8/2020
// 

// Include Files
#include <stdlib.h>
#include <cmpsc311_log.h>

// Project Includes
#include <sg_cache.h>

// Defines

// Datacache Structure
struct datacache{

    char *buf;                // Data
    SG_Block_ID blockID;      // Block ID
    SG_Node_ID nodeID;        // Node ID
    int timer;                // A timer

};

// Global Variables
struct datacache cache[128];
int hit = 0;
int miss = 0;

// Functional Prototypes

//
// Functions

////////////////////////////////////////////////////////////////////////////////
//
// Function     : initSGCache
// Description  : Initialize the cache of block elements
//
// Inputs       : maxElements - maximum number of elements allowed
// Outputs      : 0 if successful, -1 if failure

int initSGCache( uint16_t maxElements ) {

    for (int x = 0; x < 128; x++){

        cache[x].blockID = 0;
        cache[x].nodeID = 0;
        cache[x].buf = 0;
        cache[x].timer = 0;

    }

    hit = 0;
    miss = 0;

    return( 0 );

}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : closeSGCache
// Description  : Close the cache of block elements, clean up remaining data
//
// Inputs       : none
// Outputs      : 0 if successful, -1 if failure

int closeSGCache( void ) {

    double num;
    num = ((double)hit / ( (double)hit + (double)miss));

    printf("Cache hit: %d times, Cache miss %d times, Total Access: %d times, Hit Rate is: %.2lf.\n", hit, miss, (hit + miss), num);

    return( 0 );

}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : getSGDataBlock
// Description  : Get the data block from the block cache
//
// Inputs       : nde - node ID to find
//                blk - block ID to find
// Outputs      : pointer to block or NULL if not found

char *getSGDataBlock( SG_Node_ID nde, SG_Block_ID blk ) {

    miss++;
    hit++;

    for (int x = 0; x < 128; x++){

        cache[x].timer++;

        if (cache[x].blockID == blk && cache[x].nodeID == nde){

            cache[x].timer = 0;
            hit++;
            return cache[x].buf;

        }

    }

    return NULL;

}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : putSGDataBlock
// Description  : Get the data block from the block cache
//
// Inputs       : nde - node ID to find
//                blk - block ID to find
//                block - block to insert into cache
// Outputs      : 0 if successful, -1 if failure

int putSGDataBlock( SG_Node_ID nde, SG_Block_ID blk, char *block ) {

    int count = cache[0].timer;
    int number = 0;
    int z = 0;
    miss++;

    if (cache[127].nodeID != 0){

        for (int y = 0; y < (SG_MAX_CACHE_ELEMENTS - 1); y++){

            if (count < cache[y+1].timer){

                count = cache[y+1].timer;
                number = y + 1;
                cache[y].timer++;

            }

        }

        cache[number].nodeID = nde;
        cache[number].blockID = blk;
        cache[number].buf = block;
        cache[number].timer = 0;

    }
    else{
        
        while(cache[z].nodeID != 0){

            z++;

        }

        cache[z].nodeID = nde;
        cache[z].blockID = blk;
        cache[z].buf = block;
        cache[z].timer = 0;

    }

    return( 0 );
}
