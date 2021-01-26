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

// Project Includes
#include <sg_driver.h>
#include <sg_service.h>
#include <string.h>

// Defines
//
// File system interface implementation

// Archive Structure

struct archive{

    SgFHandle fhandle;           // Filehandle
    int status;                  // Open or closed
    char *addr;                  // Where it is located
    SG_Block_ID blocks[999];     // List of blocks
    int blockcount;              // # of blocks
    SG_Node_ID nodeID[999];      // Node ID
    int size;                    // File size
    int pos;                     // Read / Write position

};

// Node Array Structure

struct nodeArray{

    SG_Node_ID nodeID;           // Node ID
    SG_SeqNum rseq;              // Remote Sequence #

};

// Global Variables

struct archive files[999];
int filecount = 0;
struct nodeArray nArray[999];
int nodecount = 0;
SG_SeqNum remote = SG_INITIAL_SEQNO;


// Driver file entry

// Global data
int sgDriverInitialized = 0;      // The flag indicating the driver initialized
SG_Block_ID sgLocalNodeId;        // The local node identifier
SG_SeqNum sgLocalSeqno;           // The local sequence number

// Driver support functions
int searchFh ( SgFHandle fh );                          // Search for the filehandle 
int sgInitEndpoint( void );                             // Initialize the endpoint
int sgCblock( SgFHandle fh, char *buf, size_t len );    // Create a new block
int sgOblock( SgFHandle fh, char *buf, size_t len );    // Obtain a block
int sgUblock( SgFHandle fh, char *buf, size_t len );    // Update a block
int updateRseq ( SG_Node_ID nid, SG_SeqNum s );         // Update Remote Sequence #
SG_SeqNum getLastRseq ( SG_Node_ID nid);                // Get the Last used Remote Sequence #

//
// Functions

////////////////////////////////////////////////////////////////////////////////
//
// Function     : sgopen
// Description  : Open the file for for reading and writing
//
// Inputs       : path - the path/filename of the file to be read
// Outputs      : file handle if successful test, -1 if failure

SgFHandle sgopen (const char *path) {

    SgFHandle refh;             // The filehandle for return

    // First check to see if we have been initialized
    if (!sgDriverInitialized) {

        // Initialize Cache
            initSGCache(32);

        // Call the endpoint initialization 
        if ( sgInitEndpoint() ) {
            logMessage( LOG_ERROR_LEVEL, "sgopen: Scatter/Gather endpoint initialization failed." );
            return( -1 );
        }

        // Set to initialized
        sgDriverInitialized = 1;

    }

    for (int x = 0; x < filecount; x++){

        if (files[filecount].addr == path){
            files[filecount].pos = 0;
            return files[filecount].fhandle;
        }

    }

    for(int x = 0; x < 999; x++){

        files[filecount].blocks[x] = 0;
        nArray[x].rseq = SG_INITIAL_SEQNO;
        nArray[x].nodeID = 0;

    }

    // Initialize the structure
    refh = filecount;
    files[filecount].fhandle = filecount;
    files[filecount].size = 0;
    files[filecount].addr = path;
    files[filecount].status = 1;
    files[filecount].pos = 0;
    files[filecount].blockcount = 0;
    filecount++;
    
    // Return the file handle 
    return( refh );
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : sgread
// Description  : Read data from the file
//
// Inputs       : fh - file handle for the file to read from
//                buf - place to put the data
//                len - the length of the read
// Outputs      : number of bytes read, -1 if failure

int sgread (SgFHandle fh, char *buf, size_t len) {

    // Check if filehandle is bad
    if (searchFh(fh) == 0){
        return -1;
    }

    // Check if the file is opened
    if (files[fh].status == 0){
        return -1;
    }

    // Check if the pointer is at the end of the file
    if (files[fh].pos == files[fh].size){
        return -1;
    }
    
    sgOblock(fh, buf, len);
    files[fh].pos = files[fh].pos + len;

    // Return the bytes processed
    return( len );
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : sgwrite
// Description  : write data to the file
//
// Inputs       : fh - file handle for the file to write to
//                buf - pointer to data to write
//                len - the length of the write
// Outputs      : number of bytes written if successful test, -1 if failure

int sgwrite (SgFHandle fh, char *buf, size_t len) {

    // Check if filehandle is bad
    if (searchFh(fh) == 0){
        return -1;
    }

    // Check if the file is opened
    if (files[fh].status == 0){
        return -1;
    }

    if (files[fh].pos == files[fh].size){

        if (files[fh].pos % 1024 == 0){

            sgCblock(fh, buf, len);
            files[fh].size = files[fh].size + len;
            files[fh].pos = files[fh].pos + len;

        }
        else{

            sgUblock(fh, buf, len);
            files[fh].size = files[fh].size + len;
            files[fh].pos = files[fh].pos + len;

        }
    }
    else{

            sgUblock(fh, buf, len);
            files[fh].size = files[fh].size + len;
            files[fh].pos = files[fh].pos + len;
  
        }

    // Log the write, return bytes written
    return( len );
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : sgseek
// Description  : Seek to a specific place in the file
//
// Inputs       : fh - the file handle of the file to seek in
//                off - offset within the file to seek to
// Outputs      : new position if successful, -1 if failure

int sgseek (SgFHandle fh, size_t off) {

    // Check if filehandle is bad
    if (searchFh(fh) == 0){
        return -1;
    }

    // Check if the file is opened
    if (files[fh].status == 0){
        return -1;
    }

    if (off <= files[fh].size){
        files[fh].pos = off;
    }

    // Return new position
    return( off );
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : sgclose
// Description  : Close the file
//
// Inputs       : fh - the file handle of the file to close
// Outputs      : 0 if successful test, -1 if failure

int sgclose (SgFHandle fh) {

    // Check if filehandle is bad
    if (searchFh(fh) == 0){
        return -1;
    }

    // Check if the file is opened
    if (files[fh].status == 0){
        return -1;
    }

    files[fh].pos = 0;
    files[fh].status = 0;

    // Return successfully
    return( 0 );
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : sgshutdown
// Description  : Shut down the filesystem
//
// Inputs       : none
// Outputs      : 0 if successful test, -1 if failure

int sgshutdown (void) {

    // Log, return successfully
    closeSGCache();
    logMessage( LOG_INFO_LEVEL, "Shut down Scatter/Gather driver." );
    return( 0 );
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : serialize_sg_packet
// Description  : Serialize a ScatterGather packet (create packet)
//
// Inputs       : loc - the local node identifier
//                rem - the remote node identifier
//                blk - the block identifier
//                op - the operation performed/to be performed on block
//                sseq - the sender sequence number
//                rseq - the receiver sequence number
//                data - the data block (of size SG_BLOCK_SIZE) or NULL
//                packet - the buffer to place the data
//                plen - the packet length (int bytes)
// Outputs      : 0 if successfully created, -1 if failure

SG_Packet_Status serialize_sg_packet(SG_Node_ID loc, SG_Node_ID rem, SG_Block_ID blk,
                                     SG_System_OP op, SG_SeqNum sseq, SG_SeqNum rseq, char *data,
                                     char *packet, size_t *plen) {

        //
        if (loc == 0){
            return 1;
        }
        if (rem == 0){
            return 2;
        }
        if (blk == 0){
            return 3;
        }
        if (op > 6){
            return 4;
        }
        if (sseq == 0){
            return 5;
        }
        if (rseq == 0){
            return 6;
        }
        // Check if these values are valid

        *plen = 0;
        int one = 1;
        int zer = 0;
        uint32_t mv = SG_MAGIC_VALUE;     

        // Define values 

        memcpy(packet, &mv, (sizeof mv));
        packet = packet + (sizeof mv);
        memcpy(packet, &loc, sizeof loc);
        packet = packet + (sizeof loc);
        memcpy(packet, &rem, (sizeof rem));
        packet = packet + (sizeof rem);
        memcpy(packet, &blk, (sizeof blk));
        packet = packet + (sizeof blk);
        memcpy(packet, &op, (sizeof op));
        packet = packet + (sizeof op);
        memcpy(packet, &sseq, (sizeof sseq));
        packet = packet + (sizeof sseq);
        memcpy(packet, &rseq, (sizeof rseq));
        packet = packet + (sizeof rseq);
        
        if(data == NULL){

            memcpy(packet, &zer, 1);
            packet++;
            *plen = 41;
            // If no data
        }
 
        else{

            memcpy(packet, &one, 1);
            packet++;

            memcpy(packet, data, SG_BLOCK_SIZE);
            packet = packet + SG_BLOCK_SIZE;
            *plen = 41 + SG_BLOCK_SIZE;
            // If there is data
        }

        memcpy(packet, &mv, (sizeof mv));
        packet = packet + (sizeof mv);

        // Copy the magic value

    return( 0 ); 
        // Return the system function return value
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : deserialize_sg_packet
// Description  : De-serialize a ScatterGather packet (unpack packet)
//
// Inputs       : loc - the local node identifier
//                rem - the remote node identifier
//                blk - the block identifier
//                op - the operation performed/to be performed on block
//                sseq - the sender sequence number
//                rseq - the receiver sequence number
//                data - the data block (of size SG_BLOCK_SIZE) or NULL
//                packet - the buffer to place the data
//                plen - the packet length (int bytes)
// Outputs      : 0 if successfully created, -1 if failure

SG_Packet_Status deserialize_sg_packet (SG_Node_ID *loc, SG_Node_ID *rem, SG_Block_ID *blk,
                                       SG_System_OP *op, SG_SeqNum *sseq, SG_SeqNum *rseq, char *data,
                                       char *packet, size_t plen) {

        packet = packet + 4;

        memcpy(loc, packet, sizeof loc);
        packet = packet + (sizeof loc);
        memcpy(rem, packet, (sizeof rem));
        packet = packet + (sizeof rem);
        memcpy(blk, packet, (sizeof blk));
        packet = packet + (sizeof blk);
        memcpy(op, packet, 4);
        packet = packet + 4;
        memcpy(sseq, packet, 2);
        packet = packet + 2;
        memcpy(rseq, packet, 2);
        packet = packet + 2;
        packet++;

        if (data == NULL){
            return 0;
        }
        else{
            memcpy(data, packet, SG_BLOCK_SIZE);
        }

        if (*loc == 0){
            return 1;
        }
        if (*rem == 0){
            return 2;
        }
        if (*blk == 0){
            return 3;
        }
        if (*op > 6){
            return 4;
        }
        if (*sseq == 0){
            return 5;
        }
        if (*rseq == 0){
            return 6;
        }

    return( 0 );
    // Return the system function return value
}

//
// Driver support functions

////////////////////////////////////////////////////////////////////////////////
//
// Function     : searchFh
// Description  : Search if a filehandle exist
//
// Inputs       : fh - filehandle to search for
// Outputs      : 1 if exist, 0 if doesn't

int searchFh ( SgFHandle fh ){

    int tmpcount = 0;

    for (tmpcount = 0; tmpcount < 1000; tmpcount++){
        if (files[tmpcount].fhandle == fh){

            return 1;       // There exist

        }
    }

    return 0;               // Doesn't exist
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : sgInitEndpoint
// Description  : Initialize the endpoint
//
// Inputs       : none
// Outputs      : 0 if successfull, -1 if failure

int sgInitEndpoint (void){

    // Local variables
    char initPacket[SG_BASE_PACKET_SIZE], recvPacket[SG_BASE_PACKET_SIZE];
    size_t pktlen, rpktlen;
    SG_Node_ID loc, rem;
    SG_Block_ID blkid;
    SG_SeqNum sloc, srem;
    SG_System_OP op;
    SG_Packet_Status ret;

    // Local and do some initial setup
    logMessage( LOG_INFO_LEVEL, "Initializing local endpoint ..." );
    sgLocalSeqno = SG_INITIAL_SEQNO;

    // Setup the packet
    pktlen = SG_BASE_PACKET_SIZE;
    if ( (ret = serialize_sg_packet( SG_NODE_UNKNOWN,  // Local ID
                                    SG_NODE_UNKNOWN,   // Remote ID
                                    SG_BLOCK_UNKNOWN,  // Block ID
                                    SG_INIT_ENDPOINT,  // Operation
                                    sgLocalSeqno++,    // Sender sequence number
                                    SG_SEQNO_UNKNOWN,  // Receiver sequence number
                                    NULL, initPacket, &pktlen)) != SG_PACKT_OK ) {
        logMessage( LOG_ERROR_LEVEL, "sgInitEndpoint: failed serialization of packet [%d].", ret );
        return( -1 );
    }

    // Send the packet
    rpktlen = SG_BASE_PACKET_SIZE;
    if ( sgServicePost(initPacket, &pktlen, recvPacket, &rpktlen) ) {
        logMessage( LOG_ERROR_LEVEL, "sgInitEndpoint: failed packet post" );
        return( -1 );
    }

    // Unpack the recieived data
    if ( (ret = deserialize_sg_packet(&loc, &rem, &blkid, &op, &sloc, 
                                    &srem, NULL, recvPacket, rpktlen)) != SG_PACKT_OK ) {
        logMessage( LOG_ERROR_LEVEL, "sgInitEndpoint: failed deserialization of packet [%d]", ret );
        return( -1 );
    }

    // Sanity check the return value
    if ( loc == SG_NODE_UNKNOWN ) {
        logMessage( LOG_ERROR_LEVEL, "sgInitEndpoint: bad local ID returned [%ul]", loc );
        return( -1 );
    }

    // Set the local node ID, log and return successfully
    sgLocalNodeId = loc;
    logMessage( LOG_INFO_LEVEL, "Completed initialization of node (local node ID %lu", sgLocalNodeId );
    return( 0 );
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : sgCblock
// Description  : Create a new block
//
// Inputs       : fh - filehandle
//                buf - character buffer
//                len - length of the operation
// Outputs      : 0 if successful, -1 if failure

int sgCblock (SgFHandle fh, char *buf, size_t len){

    // Local variables
    char initPacket[SG_DATA_PACKET_SIZE], recvPacket[SG_DATA_PACKET_SIZE];
    size_t pktlen, rpktlen;
    SG_Node_ID loc, rem;
    SG_Block_ID blkid;
    SG_SeqNum sloc, srem;
    SG_System_OP op;
    SG_Packet_Status ret;

    // Setup the packet
    pktlen = SG_DATA_PACKET_SIZE;
    if ( (ret = serialize_sg_packet( sgLocalNodeId,    // Local ID
                                    SG_NODE_UNKNOWN,   // Remote ID
                                    SG_BLOCK_UNKNOWN,  // Block ID
                                    SG_CREATE_BLOCK,   // Operation
                                    sgLocalSeqno++,    // Sender sequence number
                                    SG_SEQNO_UNKNOWN,  // Receiver sequence number
                                    buf, initPacket, &pktlen)) != SG_PACKT_OK ) {
        logMessage( LOG_ERROR_LEVEL, "sgInitEndpoint: failed serialization of packet [%d].", ret );
        return( -1 );
    }

    // Send the packet
    rpktlen = SG_DATA_PACKET_SIZE;
    if ( sgServicePost(initPacket, &pktlen, recvPacket, &rpktlen) ) {
        logMessage( LOG_ERROR_LEVEL, "sgInitEndpoint: failed packet post" );
        return( -1 );
    } 

    // Unpack the recieived data
    if ( (ret = deserialize_sg_packet(&loc, &rem, &blkid, &op, &sloc, 
                                    &srem, buf, recvPacket, rpktlen)) != SG_PACKT_OK ) {
        logMessage( LOG_ERROR_LEVEL, "sgInitEndpoint: failed deserialization of packet [%d]", ret );
        return( -1 );
    }

    // Update the sequence number srem to the node ID stored in rem
    updateRseq(rem, srem);

    // Update the local identifier 
    remote = getLastRseq(rem);

    // Sanity check the return value
    if ( loc == SG_NODE_UNKNOWN ) {
        logMessage( LOG_ERROR_LEVEL, "sgInitEndpoint: bad local ID returned [%ul]", loc );
        return( -1 );
    }

    // Set the local node ID, log and return successfully
    putSGDataBlock(rem, blkid, buf);
    sgLocalNodeId = rem;
    files[fh].nodeID[files[fh].blockcount] = rem;
    files[fh].blocks[files[fh].blockcount] = blkid;  
    files[fh].blockcount++;
    return ( 0 );
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : sgOblock
// Description  : Obtain a block
//
// Inputs       : fh - filehandle
//                buf - character buffer
//                len - length of the operation
// Outputs      : 0 if successful, -1 if failure

int sgOblock (SgFHandle fh, char *buf, size_t len){

    char *tmp;
    char a[1024];
    tmp = &a[0];

    char initPacket[SG_DATA_PACKET_SIZE], recvPacket[SG_DATA_PACKET_SIZE];
    size_t pktlen, rpktlen;
    SG_Node_ID loc, rem;
    SG_Block_ID blkid;
    SG_SeqNum sloc, srem;
    SG_System_OP op;
    SG_Packet_Status ret;
    int destblock = (files[fh].pos / 1024);

    pktlen = SG_DATA_PACKET_SIZE;
    rpktlen = SG_DATA_PACKET_SIZE;

    if (getSGDataBlock(files[fh].nodeID[destblock], files[fh].blocks[destblock] != NULL)){

        tmp = getSGDataBlock(files[fh].nodeID[destblock], files[fh].blocks[destblock]);

        if (files[fh].pos % 1024 == 0){
            memcpy(buf, tmp, 256);
        }
        if (files[fh].pos % 1024 == 256){
            memcpy(buf, tmp + 256, 256);
        }        
        if (files[fh].pos % 1024 == 512){
            memcpy(buf, tmp + 512, 256);
        }
        if (files[fh].pos % 1024 == 768){
            memcpy(buf, tmp + 768, 256);
        }

        return ( 0 );

    }
    else{

        remote = getLastRseq(files[fh].nodeID[destblock]) + 1;

        // Setup the packet
        if ( (ret = serialize_sg_packet( sgLocalNodeId,    // Local ID
                        (files[fh].nodeID[destblock]),     // Remote ID
                        (files[fh].blocks[destblock]),     // Block ID
                                        SG_OBTAIN_BLOCK,   // Operation
                                        sgLocalSeqno++,    // Sender sequence number
                                             remote,     // Receiver sequence number
                                        NULL, initPacket, &pktlen)) != SG_PACKT_OK ) {
            logMessage( LOG_ERROR_LEVEL, "sgInitEndpoint: failed serialization of packet [%d].", ret );
            return( -1 );
        }

        // Send the packet
        if ( sgServicePost(initPacket, &pktlen, recvPacket, &rpktlen) ) {
            logMessage( LOG_ERROR_LEVEL, "sgInitEndpoint: failed packet post" );
            return( -1 );
        }

        // Unpack the recieived data
        if ( (ret = deserialize_sg_packet(&loc, &rem, &blkid, &op, &sloc, 
                                        &srem, tmp, recvPacket, rpktlen)) != SG_PACKT_OK ) {
            logMessage( LOG_ERROR_LEVEL, "sgInitEndpoint: failed deserialization of packet [%d]", ret );
            return( -1 );
        }
        
        updateRseq(rem, srem);

        if (files[fh].pos % 1024 == 0){
            memcpy(buf, tmp, 256);
        }

        if (files[fh].pos % 1024 == 256){
            memcpy(buf, tmp + 256, 256);
        }

        if (files[fh].pos % 1024 == 512){    
            memcpy(buf, tmp + 512, 256);
        }

        if (files[fh].pos % 1024 == 768){
            memcpy(buf, tmp + 768, 256);
        }

        putSGDataBlock(files[fh].nodeID[destblock], files[fh].blocks[destblock], tmp);

        // Sanity check the return value
        if ( loc == SG_NODE_UNKNOWN ) {
            logMessage( LOG_ERROR_LEVEL, "sgInitEndpoint: bad local ID returned [%ul]", loc );
            return( -1 );
        }

        sgLocalNodeId = rem;
        return ( 0 );
    }
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : sgUblock
// Description  : Update a block
//
// Inputs       : fh - filehandle
//                buf - character buffer
//                len - length of the operation
// Outputs      : 0 if successful, -1 if failure

int sgUblock (SgFHandle fh, char *buf, size_t len){

    char *tmp;
    char a[1024];
    tmp = &a[0];

    char initPacket[SG_DATA_PACKET_SIZE], recvPacket[SG_DATA_PACKET_SIZE];
    size_t pktlen, rpktlen;
    SG_Node_ID loc, rem;
    SG_Block_ID blkid;
    SG_SeqNum sloc, srem;
    SG_System_OP op;
    SG_Packet_Status ret;

    pktlen = SG_DATA_PACKET_SIZE;
    rpktlen = SG_DATA_PACKET_SIZE;

    int destblock = (files[fh].pos / 1024);

    if (getSGDataBlock(files[fh].nodeID[destblock], files[fh].blocks[destblock] != NULL)){

        tmp = getSGDataBlock(files[fh].nodeID[destblock], files[fh].blocks[destblock]);

        if (files[fh].pos % 1024 == 0){
            memcpy(buf, tmp, 256);
        }
        if (files[fh].pos % 1024 == 256){
            memcpy(buf, tmp + 256, 256);
        }        
        if (files[fh].pos % 1024 == 512){
            memcpy(buf, tmp + 512, 256);
        }
        if (files[fh].pos % 1024 == 768){
            memcpy(buf, tmp + 768, 256);
        }

    }
    else{

        if (files[fh].pos % 1024 == 0){

            remote = getLastRseq(files[fh].nodeID[destblock]) + 1;

            // Setup the packet
            if ( (ret = serialize_sg_packet( sgLocalNodeId,    // Local ID
                            (files[fh].nodeID[destblock]),     // Remote ID
                            (files[fh].blocks[destblock]),     // Block ID
                                            SG_OBTAIN_BLOCK,   // Operation
                                            sgLocalSeqno++,    // Sender sequence number
                                                    remote,  // Receiver sequence number
                                            NULL, initPacket, &pktlen)) != SG_PACKT_OK ) {
                logMessage( LOG_ERROR_LEVEL, "sgInitEndpoint: failed serialization of packet [%d].", ret );
                return( -1 );
            }

            // Send the packet
            if ( sgServicePost(initPacket, &pktlen, recvPacket, &rpktlen) ) {
                logMessage( LOG_ERROR_LEVEL, "sgInitEndpoint: failed packet post" );
                return( -1 );
            } 

            // Unpack the recieived data
            if ( (ret = deserialize_sg_packet(&loc, &rem, &blkid, &op, &sloc, 
                                            &srem, tmp, recvPacket, rpktlen)) != SG_PACKT_OK ) {
                logMessage( LOG_ERROR_LEVEL, "sgInitEndpoint: failed deserialization of packet [%d]", ret );
                return( -1 );
            }

            updateRseq(rem, srem);
            memcpy(tmp, buf, 256);

            remote = getLastRseq(files[fh].nodeID[destblock]) + 1;
            // Setup the packet
            if ( (ret = serialize_sg_packet( sgLocalNodeId,    // Local ID
                            (files[fh].nodeID[destblock]),     // Remote ID
                            (files[fh].blocks[destblock]),     // Block ID
                                            SG_UPDATE_BLOCK,   // Operation
                                            sgLocalSeqno++,    // Sender sequence number
                                                    remote++,  // Receiver sequence number
                                            tmp, initPacket, &pktlen)) != SG_PACKT_OK ) {
                logMessage( LOG_ERROR_LEVEL, "sgInitEndpoint: failed serialization of packet [%d].", ret );
                return( -1 );
            }

            // Send the packet
            if ( sgServicePost(initPacket, &pktlen, recvPacket, &rpktlen) ) {
                logMessage( LOG_ERROR_LEVEL, "sgInitEndpoint: failed packet post" );
                return( -1 );
            } 

            // Unpack the recieived data
            if ( (ret = deserialize_sg_packet(&loc, &rem, &blkid, &op, &sloc, 
                                            &srem, buf, recvPacket, rpktlen)) != SG_PACKT_OK ) {
                logMessage( LOG_ERROR_LEVEL, "sgInitEndpoint: failed deserialization of packet [%d]", ret );
                return( -1 );
            }

            updateRseq(rem, srem);
 
        }

        if (files[fh].pos % 1024 == 256){

            remote = getLastRseq(files[fh].nodeID[destblock]) + 1;

            // Setup the packet
            if ( (ret = serialize_sg_packet( sgLocalNodeId,    // Local ID
                            (files[fh].nodeID[destblock]),     // Remote ID
                            (files[fh].blocks[destblock]),     // Block ID
                                            SG_OBTAIN_BLOCK,   // Operation
                                            sgLocalSeqno++,    // Sender sequence number
                                                    remote,  // Receiver sequence number
                                            NULL, initPacket, &pktlen)) != SG_PACKT_OK ) {
                logMessage( LOG_ERROR_LEVEL, "sgInitEndpoint: failed serialization of packet [%d].", ret );
                return( -1 );
            }

            // Send the packet
            if ( sgServicePost(initPacket, &pktlen, recvPacket, &rpktlen) ) {
                logMessage( LOG_ERROR_LEVEL, "sgInitEndpoint: failed packet post" );
                return( -1 );
            } 

            // Unpack the recieived data
            if ( (ret = deserialize_sg_packet(&loc, &rem, &blkid, &op, &sloc, 
                                            &srem, tmp, recvPacket, rpktlen)) != SG_PACKT_OK ) {
                logMessage( LOG_ERROR_LEVEL, "sgInitEndpoint: failed deserialization of packet [%d]", ret );
                return( -1 );
            }

            updateRseq(rem, srem);
            memcpy(tmp + 256, buf, 256);
            remote = getLastRseq(files[fh].nodeID[destblock]) + 1;

            // Setup the packet
            if ( (ret = serialize_sg_packet( sgLocalNodeId,    // Local ID
                            (files[fh].nodeID[destblock]),     // Remote ID
                            (files[fh].blocks[destblock]),     // Block ID
                                            SG_UPDATE_BLOCK,   // Operation
                                            sgLocalSeqno++,    // Sender sequence number
                                                    remote,  // Receiver sequence number
                                            tmp, initPacket, &pktlen)) != SG_PACKT_OK ) {
                logMessage( LOG_ERROR_LEVEL, "sgInitEndpoint: failed serialization of packet [%d].", ret );
                return( -1 );
            }

            // Send the packet
            if ( sgServicePost(initPacket, &pktlen, recvPacket, &rpktlen) ) {
                logMessage( LOG_ERROR_LEVEL, "sgInitEndpoint: failed packet post" );
                return( -1 );
            } 

            // Unpack the recieived data
            if ( (ret = deserialize_sg_packet(&loc, &rem, &blkid, &op, &sloc, 
                                            &srem, buf, recvPacket, rpktlen)) != SG_PACKT_OK ) {
                logMessage( LOG_ERROR_LEVEL, "sgInitEndpoint: failed deserialization of packet [%d]", ret );
                return( -1 );
            }  

            updateRseq(rem, srem);

        }

        if (files[fh].pos % 1024 == 512){
            
            remote = getLastRseq(files[fh].nodeID[destblock]) + 1;

            // Setup the packet
            if ( (ret = serialize_sg_packet( sgLocalNodeId,    // Local ID
                            (files[fh].nodeID[destblock]),     // Remote ID
                            (files[fh].blocks[destblock]),     // Block ID
                                            SG_OBTAIN_BLOCK,   // Operation
                                            sgLocalSeqno++,    // Sender sequence number
                                                    remote,  // Receiver sequence number
                                            NULL, initPacket, &pktlen)) != SG_PACKT_OK ) {
                logMessage( LOG_ERROR_LEVEL, "sgInitEndpoint: failed serialization of packet [%d].", ret );
                return( -1 );
            }
            // Send the packet
            if ( sgServicePost(initPacket, &pktlen, recvPacket, &rpktlen) ) {
                logMessage( LOG_ERROR_LEVEL, "sgInitEndpoint: failed packet post" );
                return( -1 );
            } 
            // Unpack the recieived data
            if ( (ret = deserialize_sg_packet(&loc, &rem, &blkid, &op, &sloc, 
                                            &srem, tmp, recvPacket, rpktlen)) != SG_PACKT_OK ) {
                logMessage( LOG_ERROR_LEVEL, "sgInitEndpoint: failed deserialization of packet [%d]", ret );
                return( -1 );
            } 

            updateRseq(rem, srem);
            memcpy(tmp + 512, buf, 256);
            remote = getLastRseq(files[fh].nodeID[destblock]) + 1;

            // Setup the packet
            if ( (ret = serialize_sg_packet( sgLocalNodeId,    // Local ID
                            (files[fh].nodeID[destblock]),     // Remote ID
                            (files[fh].blocks[destblock]),     // Block ID
                                            SG_UPDATE_BLOCK,   // Operation
                                            sgLocalSeqno++,    // Sender sequence number
                                                    remote,  // Receiver sequence number
                                            tmp, initPacket, &pktlen)) != SG_PACKT_OK ) {
                logMessage( LOG_ERROR_LEVEL, "sgInitEndpoint: failed serialization of packet [%d].", ret );
                return( -1 );
            }

            // Send the packet
            if ( sgServicePost(initPacket, &pktlen, recvPacket, &rpktlen) ) {
                logMessage( LOG_ERROR_LEVEL, "sgInitEndpoint: failed packet post" );
                return( -1 );
            } 

            // Unpack the recieived data
            if ( (ret = deserialize_sg_packet(&loc, &rem, &blkid, &op, &sloc, 
                                            &srem, buf, recvPacket, rpktlen)) != SG_PACKT_OK ) {
                logMessage( LOG_ERROR_LEVEL, "sgInitEndpoint: failed deserialization of packet [%d]", ret );
                return( -1 );
            }

            updateRseq(rem, srem);

        }

        if (files[fh].pos % 1024 == 768){

            remote = getLastRseq(files[fh].nodeID[destblock]) + 1;
            // Setup the packet
            if ( (ret = serialize_sg_packet( sgLocalNodeId,    // Local ID
                            (files[fh].nodeID[destblock]),     // Remote ID
                            (files[fh].blocks[destblock]),     // Block ID
                                            SG_OBTAIN_BLOCK,   // Operation
                                            sgLocalSeqno++,    // Sender sequence number
                                                    remote,  // Receiver sequence number
                                            NULL, initPacket, &pktlen)) != SG_PACKT_OK ) {
                logMessage( LOG_ERROR_LEVEL, "sgInitEndpoint: failed serialization of packet [%d].", ret );
                return( -1 );
            }

            // Send the packet
            if ( sgServicePost(initPacket, &pktlen, recvPacket, &rpktlen) ) {
                logMessage( LOG_ERROR_LEVEL, "sgInitEndpoint: failed packet post" );
                return( -1 );
            } 

            // Unpack the recieived data
            if ( (ret = deserialize_sg_packet(&loc, &rem, &blkid, &op, &sloc, 
                                            &srem, tmp, recvPacket, rpktlen)) != SG_PACKT_OK ) {
                logMessage( LOG_ERROR_LEVEL, "sgInitEndpoint: failed deserialization of packet [%d]", ret );
                return( -1 );
            }

            updateRseq(rem, srem);
            memcpy(tmp + 768, buf, 256);
            remote = getLastRseq(files[fh].nodeID[destblock]) + 1;

            // Setup the packet
            if ( (ret = serialize_sg_packet( sgLocalNodeId,    // Local ID
                            (files[fh].nodeID[destblock]),     // Remote ID
                            (files[fh].blocks[destblock]),     // Block ID
                                            SG_UPDATE_BLOCK,   // Operation
                                            sgLocalSeqno++,    // Sender sequence number
                                            remote,  // Receiver sequence number
                                            tmp, initPacket, &pktlen)) != SG_PACKT_OK ) {
                logMessage( LOG_ERROR_LEVEL, "sgInitEndpoint: failed serialization of packet [%d].", ret );
                return( -1 );
            }

            // Send the packet
            if ( sgServicePost(initPacket, &pktlen, recvPacket, &rpktlen) ) {
                logMessage( LOG_ERROR_LEVEL, "sgInitEndpoint: failed packet post" );
                return( -1 );
            } 

            // Unpack the recieived data
            if ( (ret = deserialize_sg_packet(&loc, &rem, &blkid, &op, &sloc, 
                                            &srem, buf, recvPacket, rpktlen)) != SG_PACKT_OK ) {
                logMessage( LOG_ERROR_LEVEL, "sgInitEndpoint: failed deserialization of packet [%d]", ret );
                return( -1 );
            }

            updateRseq(rem, srem);

        }
    }

    // Sanity check the return value
    if ( loc == SG_NODE_UNKNOWN ) {
        logMessage( LOG_ERROR_LEVEL, "sgInitEndpoint: bad local ID returned [%ul]", loc );
        return( -1 );
    }

    // Set the local node ID, log and return successfully
    sgLocalNodeId = rem;
    return ( 0 );
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : updateRseq
// Description  : Update the remote sequence #
//
// Inputs       : nid - nodeID
//                s - input sequence #
// Outputs      : 0

int updateRseq ( SG_Node_ID nid, SG_SeqNum s ){

    int x;

    for (x = 0; x < nodecount; x++){

        if (nArray[x].nodeID == nid){
            nArray[x].rseq = s;
            return 0;
        }

    }

    nArray[nodecount].nodeID = nid;
    nArray[nodecount].rseq = SG_INITIAL_SEQNO;
    nodecount++;
    return 0;
    
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : getLastRseq
// Description  : Update the remote sequence #
//
// Inputs       : nid - nodeID
// Outputs      : Remote sequence number stored for nid

SG_SeqNum getLastRseq ( SG_Node_ID nid ) {

    int x;

    for (x = 0; x < nodecount; x++){

        if (nArray[x].nodeID == nid){
            return nArray[x].rseq;
        }

    }
    return 0;
}
