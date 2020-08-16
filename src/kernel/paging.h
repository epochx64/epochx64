//
// Created by Dennis on 2020-07-24.
//

#ifndef PAGING_H
#define PAGING_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "log.h"
#include "../include/mem.h"

namespace paging
{
    /* - - - - - - - - - - - - - - - - - - - - - - - - -
     *
     *                     Constants
     *
     - - - - - - - - - - - - - - - - - - - - - - - - - -*/
    #define PRESENT                 0b001
    #define WRITE                   0b010
    #define USER                    0b100
    #define PAGEMAP_ADDRESS_MASK    0xFFFFFFFFFFFFF000
    #define PAGEMAP_FLAG_MASK       0x0000000000000FFF

    //  Aligns address on 2MiB boundary
    #define PML1_ALIGN_MASK 0xFFFFFFFFFFE00000

    //  Size in the sense of how much memory it covers
    //  eg. 1 PML1 (PT) covers 512 pages of memory = 2MiB
    #define PML4_SIZE  0x0000F00000000000   // 0.25 EiB, max RAM size
    #define PML3_SIZE  0x0000008000000000   // 0.5  TiB
    #define PML2_SIZE  0x0000000040000000   // 1    GiB
    #define PML1_SIZE  0x0000000000200000   // 2    MiB
    #define PAGE_SIZE  0x0000000000001000   // 4    KiB

    #define PML4_ADDR 0x1000

    /* - - - - - - - - - - - - - - - - - - - - - - - - -
    *
    *                     Type definitions
    *
    - - - - - - - - - - - - - - - - - - - - - - - - - -*/

    /***
     * Can be used to work with page map structures
     * such as PT, PDPT, PML4...
     */
    typedef struct
    {
        /*
         * pml3, pml2, or pml1 entries
         */
        uint64_t    members[512];
    } __attribute__((packed))
    page_map_t;

    /***
     * Specify to the get_pml_info function to get
     * an address' corresponding PML4, PML3 .. PML1 index
     */
    typedef struct
    {
        //  Flags of the paging structure
        //  Location of the PML3 struct
        //  Index within the pml that points to the next struct

        //  Has members 0->4 for pml4,3,2,1 and the individual page
        uint16_t     flags_pml[5];
        uint64_t    addr_pml[5];
        uint16_t    index_pml[5];

    }__attribute__((packed))
    pml_info_t;

    /* - - - - - - - - - - - - - - - - - - - - - - - - -
    *
    *                     Functions
    *
    - - - - - - - - - - - - - - - - - - - - - - - - - -*/

    /***
     * Can only map one page at a time, and it does not
     * care if a page's flags is already PRESENT
     * @param u64addr
     * @param u8flags
     */
    void
    identity_map(uint64_t u64addr, uint16_t u16flags);

    /***
     * Get pointer to corresponding
     * @param pml
     * @param addr
     * @return
     */
    void
    get_pml_info(uint64_t addr, pml_info_t *pml_info);

    /***
     * Allocates one new page aligned page map structure
     * It starts at the end of the heap and grows downwards,
     * so...
     * TODO: Kernel should keep track of boundary of allocated dynamic memory and page map memory so they don't clash
     * TODO: Maybe also implement a way to deallocate
     * @return
     */
    page_map_t*
    page_map_alloc();
}

#endif
