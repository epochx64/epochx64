#include "paging.h"

namespace paging
{
    void
    identity_map(uint64_t u64addr, uint16_t u16flags)
    {
        pml_info_t pml_info;
        mem::set((byte*)&pml_info, 0x00, sizeof(pml_info_t));
        get_pml_info(u64addr, &pml_info);

        auto *page_map_iter     = (page_map_t*)PML4_ADDR;

        /*
         * Drill down the page map level structures
         * until we reach a null PML pointer
         */
        for ( uint8_t i = 4; i > 0; i-- )
        {
            //  We are in a PML1 struct
            if(i == 1)
            {
                page_map_iter->members[pml_info.index_pml[i]] = (uint64_t)( (u64addr&PAGEMAP_ADDRESS_MASK) | u16flags);
                return;
            }

            auto pml_next_addr = page_map_iter->members[pml_info.index_pml[i]] & PAGEMAP_ADDRESS_MASK;

            if( pml_next_addr == 0 )
            {
                pml_next_addr = (uint64_t)page_map_alloc();
                page_map_iter->members[pml_info.index_pml[i]] = (uint64_t)( pml_next_addr | u16flags);
            }

            page_map_iter   = (page_map_t*)pml_next_addr;
        }
    }

    void
    get_pml_info(uint64_t addr, pml_info_t *pml_info)
    {
        auto *page_map_iter     = (page_map_t*)PML4_ADDR;
        auto addr_resolver      = addr;
        uint64_t pml_sizes[]    = {PAGE_SIZE, PML1_SIZE, PML2_SIZE, PML3_SIZE};

        //  Drill down the page map structures and grab
        //  relevant info
        for ( uint8_t i = 4; i > 0; i-- )
        {
            //  We can fill out the indexes even if the PML structures don't exist
            //  the address and flags of the next PML info structure we cannot
            uint16_t pml_i_index    = addr_resolver / pml_sizes[i-1];
            pml_info->index_pml[i]  = pml_i_index;

            addr_resolver -= pml_i_index*pml_sizes[i-1];

            //  We can only grab the flags and address of the
            //  i-th pml if it exists
            if(page_map_iter != nullptr)
            {
                pml_info->flags_pml[i-1]  = page_map_iter->members[pml_i_index] & (uint64_t)PAGEMAP_FLAG_MASK;
                pml_info->addr_pml[i-1]   = page_map_iter->members[pml_i_index] & (uint64_t)PAGEMAP_ADDRESS_MASK;

                page_map_iter = (page_map_t*)(pml_info->addr_pml[i-1]);
            }
        }
    }
}
