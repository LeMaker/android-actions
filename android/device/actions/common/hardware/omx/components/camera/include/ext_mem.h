#ifndef _EXT_MEM_H_
#define _EXT_MEM_H_
#if 1
#include "actal_posix_dev.h"



static void *ext_get_vir_addr(void *phy_addr, int buflen)
{
    void *pvirAddr = NULL;//(unsigned char *)actal_get_virtaddr(phy_addr);
    printf("ext_get_vir_addr %x\n", pvirAddr);
    return pvirAddr;
}

static int ext_put_vir_addr(void *phy_addr, int buflen)
{
    //actal_free_virtaddr_mmap(phy_addr,buflen);
    return 0;
}


static void *ext_phycalloc_mem(unsigned n, unsigned nsize, void **VirPhy)
{
    void *nPhyAddr = 0;
    void *pVirAddr = NULL;
    pVirAddr = actal_malloc_uncache(n * nsize, &nPhyAddr);
    memset(pVirAddr, 0, n * nsize);
    printf("ext_phycalloc_mem %x,%x\n", pVirAddr, nPhyAddr);
    *VirPhy = pVirAddr;
    return nPhyAddr;
}

static void ext_phyfree_mem(void *Phy, void *Vir)
{
    if(Vir)
    {
        printf("ext_phyfree_mem %x,%x\n ", Phy, Vir);
        actal_free_uncache(Vir);
    }
}

static void *ext_phycalloc_mem_wt(unsigned n, unsigned nsize, void **VirPhy)
{
    void *nPhyAddr = 0;
    void *pVirAddr = NULL;
    pVirAddr = actal_malloc_wt(n * nsize, &nPhyAddr);
    memset(pVirAddr, 0, n * nsize);
    printf("ext_phycalloc_mem %x,%x\n", pVirAddr, nPhyAddr);
    *VirPhy = pVirAddr;
    return nPhyAddr;
}

static void ext_phyfree_mem_wt(void *Phy, void *Vir)
{
    if(Vir)
    {
        printf("ext_phyfree_mem %x,%x\n", Phy, Vir);
        actal_free_wt(Vir);
    }
}

static void *ext_phycalloc_mem_cm(unsigned n, unsigned nsize, void **VirPhy)
{
    void *nPhyAddr = 0;
    void *pVirAddr = NULL;
    pVirAddr = actal_malloc_cached_manual(n * nsize, &nPhyAddr);
    memset(pVirAddr, 0, n * nsize);
    printf("ext_phycalloc_mem %x,%x\n", pVirAddr, nPhyAddr);
    *VirPhy = pVirAddr;
    return nPhyAddr;
}

static void ext_phyfree_mem_cm(void *Phy, void *Vir)
{
    if(Vir)
    {
        printf("ext_phyfree_mem %x,%x\n", Phy, Vir);
        actal_free_cached_manual(Vir);
    }
}


#else
#include "dma_mem.h"
static unsigned int ext_get_vir_addr(unsigned int phy_addr, int buflen)
{
    //unsigned char *pvirAddr = (unsigned char *)actal_get_virtaddr_mmap(phy_addr,buflen);
    //printf("ext_get_vir_addr %x\n ",(unsigned int)pvirAddr);
    //return (unsigned int)pvirAddr;
    void *cpuphy;

    //codaphysical_to_cpuphysical
    if(((unsigned int)phy_addr & 0xF0000000) == 0x10000000)
    {
        cpuphy = (void *)((unsigned int)phy_addr & 0x0FFFFFFF | 0xc0000000);
    }
    else
    {
        cpuphy = phy_addr;
    }

    return sys_get_viraddr(cpuphy, MEM_CONTINUOUS | UNCACHE_MEM);
}
static int ext_put_vir_addr(unsigned int phy_addr, int buflen)
{
    //actal_free_virtaddr_mmap(phy_addr,buflen);
    return 0;
}
static unsigned int ext_phycalloc_mem(unsigned n, unsigned nsize, unsigned int *VirPhy)
{
    unsigned int nPhyAddr = 0;
    unsigned char *pVirAddr = NULL;
    pVirAddr = (unsigned char *)sys_mem_allocate(nsize * n, MEM_CONTINUOUS | UNCACHE_MEM);
    *VirPhy = (unsigned int)pVirAddr;
    nPhyAddr = sys_get_phyaddr(pVirAddr);

    //cpuphysical_to_codaphysical
    if(!((unsigned int)nPhyAddr & 0x80000000))
    {
        return nPhyAddr;
    }
    else
    {
        return (void *)((unsigned int)nPhyAddr & 0x0FFFFFFF | 0x10000000);
    }

    //pVirAddr = (unsigned char*)actal_malloc_uncache(n * nsize,(int32_t*)&nPhyAddr);
    //memset(pVirAddr,0,n * nsize);
    //printf("ext_phycalloc_mem %x,%x\n ",(unsigned int)pVirAddr,(unsigned int)nPhyAddr);
    //*VirPhy = (unsigned int)pVirAddr;
    //return (unsigned int)nPhyAddr;
}

static void ext_phyfree_mem(unsigned int Phy, unsigned int Vir)
{
    if(Vir)
    {
        printf("ext_phyfree_mem %x,%x\n ", (unsigned int)Phy, (unsigned int)Vir);
        sys_mem_free((void *)Vir);
    }
}
#endif
#endif
