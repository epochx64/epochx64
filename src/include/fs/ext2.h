#ifndef _EXT2_H
#define _EXT2_H

#include <epstring.h>
#include <typedef.h>

/*
 * This fs is heavily influenced by ext2, but not compatible in the slightest
 * with ext2. It's written to work as the bare minimum for the epoch kernel
 * Because this is only supposed to be a bare minimum implementation, expect some
 * spaghetti
 *
 * TODO: This whole lib is in need of optimization
 * TODO: Implement locking (multicore)
 */

namespace ext2
{
    #define MAX_PATH 260
    #define BLOCK_SIZE 0x1000
    #define SUPERBLOCK_SIZE 1024

    #define BLOCKS_PER_BLOCK_GROUP 0x8000
    #define INODES_PER_BLOCK_GROUP 0x8000

    //  128 MiB
    #define BYTES_PER_BLOCK_GROUP 0x8000000

    #define BLOCK_BITMAP_SIZE (BLOCKS_PER_BLOCK_GROUP/8)/BLOCK_SIZE
    #define INODE_BITMAP_SIZE (INODES_PER_BLOCK_GROUP/8)/BLOCK_SIZE

    #define DATA_BLOCKS_PER_GROUP\
    (BLOCKS_PER_BLOCK_GROUP\
    - 2\
    - BLOCK_BITMAP_SIZE\
    - INODE_BITMAP_SIZE\
    - (INODES_PER_BLOCK_GROUP*sizeof(INODE)/BLOCK_SIZE))

    //  Single/double/triple Index Block Pointer used in inodes
    #define BLOCK_SPAN 1
    #define SIBP_SPAN (BLOCK_SIZE/4)
    #define DIBP_SPAN ((BLOCK_SIZE/4)*SIBP_SPAN)
    #define TIBP_SPAN ((BLOCK_SIZE/4)*DIBP_SPAN)

    typedef UINT64 BLOCK_ID;
    typedef UINT64 INODE_ID;
    typedef UINT64 ENTRY_ID;

    typedef UINT64 STATUS;
    #define STATUS_FAIL 0
    #define STATUS_OK 1

    #define FILETYPE_DIR 1
    #define FILETYPE_REG 0

    /*
     * All information about filesystem
     * Superblock is stored 1024 bytes from start of image
     * Backup copies are stored in block groups throughout the fs
     * https://www.nongnu.org/ext2-doc/ext2.html
     */
    typedef struct
    {
        UINT32  s_inodes_count;
        UINT32  s_blocks_count;
        UINT32  s_r_blocks_count;
        UINT32  s_free_blocks_count;
        UINT32  s_free_inodes_count;
        UINT32  s_first_data_block;
        UINT32  s_log_block_size;
        UINT32  s_log_frag_size;
        UINT32  s_blocks_per_group;
        UINT32  s_frags_per_group;
        UINT32  s_inodes_per_group;
        UINT32  s_mtime;
        UINT32  s_wtime;
        UINT16	s_mnt_count;
        UINT16	s_max_mnt_count;
        UINT16	s_magic;
        UINT16	s_state;
        UINT16	s_errors;
        UINT16	s_minor_rev_level;
        UINT32	s_lastcheck;
        UINT32	s_checkinterval;
        UINT32	s_creator_os;
        UINT32	s_rev_level;
        UINT16	s_def_resuid;
        UINT16	s_def_resgid;

        //  -- EXT2_DYNAMIC_REV Specific --
        UINT32	s_first_ino;
        UINT16	s_inode_size;
        UINT16	s_block_group_nr;
        UINT32	s_feature_compat;
        UINT32	s_feature_incompat;
        UINT32	s_feature_ro_compat;
        UINT64	s_uuid[2];
        UINT64	s_volume_name[2];
        UINT64	s_last_mounted[8];
        UINT32	s_algo_bitmap;

        //  -- Performance Hints --
        UINT8	s_prealloc_blocks;
        UINT8	s_prealloc_dir_blocks;
        UINT16	(alignment);

        //  -- Journaling Support --
        UINT64	s_journal_uuid[2];
        UINT32	s_journal_inum;
        UINT32	s_journal_dev;
        UINT32	s_last_orphan;

        //  -- Directory Indexing Support --
        UINT32	s_hash_seed[4];
        UINT8	s_def_hash_version;
        UINT8	padding[3];  // - reserved for future expansion

        //  -- Other options --
        UINT32	s_default_mount_options;
        UINT32	s_first_meta_bg;
        UINT8	Unused[760 + (BLOCK_SIZE-0x400)]; // - reserved for future revisions
    } __attribute__((packed)) SUPERBLOCK;

    typedef struct
    {
        UINT32	bg_block_bitmap;
        UINT32	bg_inode_bitmap;
        UINT32	bg_inode_table;
        UINT16	bg_free_blocks_count;
        UINT16	bg_free_inodes_count;
        UINT16	bg_used_dirs_count;
        UINT16	bg_pad;
        UINT8	bg_reserved[12];
        UINT8   Padding[BLOCK_SIZE - 0x20];
    } __attribute__((packed)) BLOCK_GROUP_DESCRIPTOR_TABLE;

    typedef struct
    {
        UINT16	i_mode;
        UINT16	i_uid;
        UINT32	i_size;
        UINT32	i_atime;
        UINT32	i_ctime;
        UINT32	i_mtime;
        UINT32	i_dtime;
        UINT16	i_gid;
        UINT16	i_links_count;
        UINT32	i_blocks;
        UINT32	i_flags;
        UINT32	i_osd1;
        UINT32	i_block[15];
        UINT32	i_generation;
        UINT32	i_file_acl;
        UINT32	i_dir_acl;
        UINT32	i_faddr;
        UINT8	i_osd2[12];
    } __attribute__((packed)) INODE;

    typedef struct
    {
        UINT8 Data[BLOCK_SIZE];
    }__attribute__((packed)) BLOCK;

    typedef struct
    {
        SUPERBLOCK SuperBlock;
        BLOCK_GROUP_DESCRIPTOR_TABLE BlockGroupDescriptorTable;
        BLOCK BlockBitMap[BLOCK_BITMAP_SIZE];
        BLOCK InodeBitMap[INODE_BITMAP_SIZE];
        BLOCK InodeTable[INODES_PER_BLOCK_GROUP*sizeof(INODE)/BLOCK_SIZE];
        BLOCK DataBlocks[DATA_BLOCKS_PER_GROUP];
    } __attribute__((packed)) BLOCK_GROUP;

    typedef struct
    {
        UINT64 Size;
        UINT8 Type;
        UINT8 Path[MAX_PATH];
    } FILE;

    typedef struct
    {
        UINT64 INodeID;
        UINT64 Size;
        UINT8 Type;
        UINT8 Name[MAX_PATH];
    } __attribute__((packed, aligned(4))) DIRECTORY_ENTRY;

    STATUS GetFilenameFromPath(UINT8 Path[MAX_PATH], UINT8* Filename);

    class RAMDisk;

    class File
    {
    private:
        FILE data;
    public:
        File(RAMDisk *ramDisk = nullptr, UINT8 type = 0, const char *path = nullptr);

        FILE *Data();

        inline UINT64 Size() { return data.Size; }
        inline UINT8 Type() { return data.Type; }
        inline UINT8 *Path() { return data.Path; }
    };

    class RAMDisk
    {
    public:
        RAMDisk(UINT64 pStart, UINT64 Size, bool New = true);

        DIRECTORY_ENTRY *GetFile(UINT8 *Path);
        STATUS CreateFile(FILE *File);

        STATUS ReadFile(FILE *File, UINT8 *Buffer);

        STATUS WriteFile(FILE *File, UINT8 *Buffer);

        STATUS MakeDir(UINT8 Path[MAX_PATH]);

        UINT64 GetFileSize(UINT8 Path[MAX_PATH]);

    private:
        UINT64 pLBA0;
        UINT64 DiskSize;

        SUPERBLOCK *pSuperBlock;

        BLOCK_GROUP *BlockGroups;

        /*
         * Takes a block group ID and returns pointer to it in memory
         * Block groups start at 0
         */
        BLOCK_GROUP *GetBlockGroup(UINT64 ID);

        /*
         * Takes an INode ID and returns pointer to it in memory
         * INodes start at 1
         */
        INODE *GetINode(INODE_ID ID);

        /*
         * Takes a block ID and returns pointer to it in memory
         * Blocks start at 1
         */
        BLOCK *GetBlock(BLOCK_ID ID);

        /*
         * Used for file allocation
         */
        void AllocateBlocks(INODE* INode, UINT64 Size);

        INODE_ID AllocateINode();
        /*
         * Finds a free block in the filesystem and returns its ID
         */
        BLOCK_ID AllocateBlock();

        /*
         * Sets a block as occupied in the corresponding bitmap
         */
        STATUS OccupyBlock(BLOCK_ID ID);

        BLOCK_ID GetTIBPEntry(INODE* INode, UINT64 Index);
        void SetTIBPEntry(INODE* INode, UINT64 Index, BLOCK_ID Value);

        /*
         * Uses an inode's Triply Indirect Block Pointer to locate
         * DIRECTORY ENTRY pointer
         */
        DIRECTORY_ENTRY *GetINodeDirectoryEntry(INODE *INode, UINT64 ID);
        void SetINodeDirectoryEntry(INODE *INode, UINT64 EntryID, DIRECTORY_ENTRY *DirectoryEntry);
    };
}

#endif
