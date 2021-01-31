## Scatter-Gather Cloud Storage Simulation

You can use the [editor on GitHub](https://github.com/langyinan/scatter-gather/edit/main/docs/index.md) to maintain and preview the content for your website in Markdown files.

Whenever you commit to this repository, GitHub Pages will run [Jekyll](https://jekyllrb.com/) to rebuild the pages in your site, from the content in your Markdown files.

### Project Concept

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


### I/O Bus

In a cloud storage system, the local client communicate with the cloud storage system via

[Link](url) and ![Image](src)

For more details see [GitHub Flavored Markdown](https://guides.github.com/features/mastering-markdown/).

### Jekyll Themes

Your Pages site will use the layout and styles from the Jekyll theme you have selected in your [repository settings](https://github.com/langyinan/scatter-gather/settings). The name of this theme is saved in the Jekyll `_config.yml` configuration file.

### Support or Contact

Having trouble with Pages? Check out our [documentation](https://docs.github.com/categories/github-pages-basics/) or [contact support](https://support.github.com/contact) and we’ll help you sort it out.
