# Scatter-Gather Cloud Storage Simulation (C)

This is a breif explanation of this project. Only a part of the code is presented. For access to the full project, please contact me:


Email:          langyina88@gmail.com

LinkedIn:       [langyinan](https://www.linkedin.com/in/langyinan)

GitHub:         [langyinan](https://github.com/langyinan)



##### The core functions of the cloud storage system is stored in [sg_driver.c](https://github.com/langyinan/scatter-gather/blob/main/sg_driver.c)

##### The core functions of the cache is stored in [sg_cache.c](https://github.com/langyinan/scatter-gather/blob/main/sg_cache.c)

##### The key definitions of the system is stored in [sg_defs.h](https://github.com/langyinan/scatter-gather/blob/main/sg_defs.h)

## Project Concept and Structure


This project utilizes object-oriented programming to abstract the concept of an "Archive" as below:
```markdown
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
```
- **fhandle** is a number that represents the document, similar to a file name or file path.
- ***address** is used for memory-level operations
- **Nodes** and **Blocks** are simulations of a cloud storage system:A segment of data are stored in different blocks located in different nodes. If a fileexceeds one block size, a different block will be allocated to this file.
- Operations are performed on blocks. That means, if a data is shorter than the block,the system grabs the data from the block, modify it, and re-upload it to the storage system.
- Local **Node IDs** and **Block IDs** are **different** from the remote ones. Remote IDs are stored in the cloud storage system and locals are stored in local memories. Since local and remote communicate with each other via I/O Bus, they don't have the same reference of object. 
- Further more, the remote node IDs store additional information called **remote sequence number**. Everytime the remote node got accessed, the sequence number will increase by one for that node only.
- Different from **remote sequence number**, the **local sequence number** increases everytime a operation is performed. That means, a bus is sent or received.


## I/O Bus

In a cloud storage system, the local client communicate with the cloud storage system via **I/O Bus**. Every bus is a number that encodes with various information.

The process of pack and unpacking the information is called **serialization** and **deserialization**. Here is the component of the function called **serialize_sg_packet**

```markdown
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
```

## Cache

The cloud storage system supports  **LFU cache**. It is currently of size 128, but can be changed in the future if needed.

The cache is initialized with the cloud storage system and everytime that a blockID needs to be retrived from the system, the cache will be called and see if it the block that we wanted is already stored locally. 

```markdown
struct datacache{

    char *buf;                // Data
    SG_Block_ID blockID;      // Block ID
    SG_Node_ID nodeID;        // Node ID
    int timer;                // A timer

};
```




##### _Updated 1/31/2021 by Yinan Lang_



