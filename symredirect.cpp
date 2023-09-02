#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/fcntl.h>
#include <mach-o/fat.h>
#include <mach-o/nlist.h>
#include <mach-o/loader.h>
#include <mach-o/fixup-chains.h>
#include <assert.h>
#include <libgen.h>

#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#pragma GCC diagnostic ignored "-Wunused-but-set-variable"


#define LOG(...)    printf(__VA_ARGS__)

#include <string>
#include <vector>

#define VROOT_SYMBOLE(NAME) "_"#NAME,

#define VROOT_API_DEF(RET,NAME,ARGTYPES) VROOT_SYMBOLE(NAME)
#define VROOTAT_API_DEF(RET,NAME,ARGTYPES) VROOT_SYMBOLE(NAME)
#define VROOT_API_WRAP(RET,NAME,ARGTYPES,ARGS,PATHARG) VROOT_SYMBOLE(NAME)
#define VROOTAT_API_WRAP(RET,NAME,ARGTYPES,ARGS,FD,PATHARG,ATFLAG) VROOT_SYMBOLE(NAME)

std::vector<std::string> g_shim_apis = {
#define VROOT_INTERNAL
#include "vroot.h"
};

const char* g_shim_install_name = "@loader_path/.jbroot/usr/lib/libvrootapi.dylib";

char* getLibraryByOrdinal(struct mach_header_64* header, int ordinal)
{
    int libOrdinal=1;
    struct load_command* lc = (struct load_command*)((uint64_t)header + sizeof(*header));
    for (int i = 0; i < header->ncmds; i++) {
        
        switch(lc->cmd) {
                
            case LC_LOAD_DYLIB:
            case LC_LOAD_WEAK_DYLIB:
            case LC_REEXPORT_DYLIB:
            case LC_LOAD_UPWARD_DYLIB: {
                
                struct dylib_command* idcmd = (struct dylib_command*)lc;
                char* name = (char*)((uint64_t)idcmd + idcmd->dylib.name.offset);
                
                if(libOrdinal++ == ordinal)
                    return name;
                
                break;
            }
        }
        
        lc = (struct load_command *) ((char *)lc + lc->cmdsize);
    }
    return NULL;
}

bool processSymbol(struct mach_header_64* header, int ordinal, char* symbol)
{
    char* library = getLibraryByOrdinal(header, ordinal);
    LOG("import %s from %x:%s\n", symbol, ordinal, library);
    
    if(std::find(g_shim_apis.begin(),g_shim_apis.end(),symbol)!=g_shim_apis.end())
    {
        LOG("shim => %s\n", symbol);
        
        if(!library) {
            fprintf(stderr, "unexplicit shim symbol %s @ %x!\n", symbol, ordinal);
            abort();
        }
        
        //only redirect symbols which comes from system library
        if(strstr(library,"/usr/lib/")==library || strstr(library,"/System/")==library) {

            if((header->flags & MH_TWOLEVEL) == 0) {
                fprintf(stderr, "mach-o has no MH_TWOLEVEL\n");
                abort();
            }
            if((header->flags & MH_FORCE_FLAT) != 0) {
                fprintf(stderr, "mach-o has MH_FORCE_FLAT\n");
                abort();
            }
            
            return true;
        }
    }

    return false;
}

int processTarget(void* slice)
{
    uint32_t magic = *(uint32_t*)slice;
    if(magic != MH_MAGIC_64) {
        fprintf(stderr, "unsupport mach-o: %08x\n", magic);
        return -1;
    }
    
    struct mach_header_64* header = (struct mach_header_64*)slice;
    
    int libOrdinal=1;
    int shimOrdinal=0;
    
    int first_sec_off = 0;
    struct segment_command_64* linkedit_seg = NULL;
    struct symtab_command* symtab = NULL;
    struct dysymtab_command* dysymtab = NULL;
    struct linkedit_data_command* fixup = NULL;
    
    struct load_command* lc = (struct load_command*)((uint64_t)header + sizeof(*header));
    for (int i = 0; i < header->ncmds; i++) {
        
        //LOG("load command = %llx\n", lc->cmd);
        
        switch(lc->cmd) {
                
            case LC_LOAD_DYLIB:
            case LC_LOAD_WEAK_DYLIB:
            case LC_REEXPORT_DYLIB:
            case LC_LOAD_UPWARD_DYLIB: {
                
                struct dylib_command* idcmd = (struct dylib_command*)lc;
                char* name = (char*)((uint64_t)idcmd + idcmd->dylib.name.offset);
                LOG("libOrdinal=%d, %s\n", libOrdinal, name);
                
                if(strcmp(name, g_shim_install_name)==0) {
                    LOG("shim library exists %d\n", libOrdinal);
                    shimOrdinal = libOrdinal;
                }
                
                libOrdinal++;
                break;
            }
                
            case LC_SYMTAB: {
                symtab = (struct symtab_command*)lc;
                LOG("symtab offset=%x count=%d strtab offset=%x size=%x\n", symtab->symoff, symtab->nsyms, symtab->stroff, symtab->strsize);
                
                break;
            }
                
            case LC_DYSYMTAB: {
                dysymtab = (struct dysymtab_command*)lc;
                LOG("dysymtab export_index=%d count=%d, import_index=%d count=%d, \n", dysymtab->iextdefsym, dysymtab->nextdefsym, dysymtab->iundefsym, dysymtab->nundefsym);
                
                break;
            }
                
            case LC_SEGMENT_64: {
                struct segment_command_64 * seg = (struct segment_command_64 *) lc;
                
                LOG("segment: %s file=%llx:%llx vm=%16llx:%16llx\n", seg->segname, seg->fileoff, seg->filesize, seg->vmaddr, seg->vmsize);
                
                if(strcmp(seg->segname,SEG_LINKEDIT)==0)
                    linkedit_seg = seg;
                
                struct section_64* sec = (struct section_64*)((uint64_t)seg+sizeof(*seg));
                for(int j=0; j<seg->nsects; j++)
                {
                    LOG("section[%d] = %s/%s offset=%x vm=%16llx:%16llx\n", j, sec[j].segname, sec[j].sectname,
                          sec[j].offset, sec[j].addr, sec[j].size);
                    
                    if(sec[j].offset && (first_sec_off==0 || first_sec_off>sec[j].offset)) {
                        LOG("first_sec_off %x => %x\n", first_sec_off, sec[j].offset);
                        first_sec_off = sec[j].offset;
                    }
                }
                break;
            }
                
            case LC_DYLD_CHAINED_FIXUPS: {
                fixup = (struct linkedit_data_command*)lc;
                break;
            }
                
        }
        
        /////////
        lc = (struct load_command *) ((char *)lc + lc->cmdsize);
    }
    
    if(shimOrdinal == 0)
    {
        int addsize = sizeof(struct dylib_command) + strlen(g_shim_install_name) + 1;
        if(addsize%sizeof(void*)) addsize = (addsize/sizeof(void*) + 1) * sizeof(void*); //align
        if(first_sec_off < (sizeof(*header)+header->sizeofcmds+addsize))
        {
            fprintf(stderr, "mach-o header has no enough space!\n");
            return -1;
        }
        
        struct dylib_command* shimlib = (struct dylib_command*)((uint64_t)header + sizeof(*header) + header->sizeofcmds);
        shimlib->cmd = LC_LOAD_DYLIB;
        shimlib->cmdsize = addsize;
        shimlib->dylib.timestamp = 0;
        shimlib->dylib.current_version = 0;
        shimlib->dylib.compatibility_version = 0;
        shimlib->dylib.name.offset = sizeof(*shimlib);
        strcpy((char*)shimlib+sizeof(*shimlib), g_shim_install_name);
        
        header->sizeofcmds += addsize;
        header->ncmds++;
        
        shimOrdinal = libOrdinal;
        LOG("insert shim library @%d\n", shimOrdinal);
    }
    
//    uint32_t* itab = (uint32_t*)( (uint64_t)header + dysymtab->indirectsymoff);
//    for(int i=0; i<dysymtab->nindirectsyms; i++) {
//        LOG("indirect sym[%d] %d\n", i, itab[i]);
//    }
//
    struct nlist_64* symbols64 = (struct nlist_64*)( (uint64_t)header + symtab->symoff);
    for(int i=0; i<symtab->nsyms; i++) {
        char* symstr = (char*)( (uint64_t)header + symtab->stroff + symbols64[i].n_un.n_strx);
        LOG("sym[%d] type:%02x sect:%02x desc:%04x value:%llx \tstr:%x\t%s\n", i, symbols64[i].n_type, symbols64[i].n_sect, symbols64[i].n_desc, symbols64[i].n_value, symbols64[i].n_un.n_strx, symstr);
        
        if(i>=dysymtab->iundefsym && i<(dysymtab->iundefsym+dysymtab->nundefsym))
        {
            uint16_t n_desc = symbols64[i].n_desc;
            uint8_t  n_type = symbols64[i].n_type;
            
            int ordinal= GET_LIBRARY_ORDINAL(n_desc);
            assert(ordinal > 0);
            
            // Handle defined weak def symbols which need to get a special ordinal
            if ( ((n_type & N_TYPE) == N_SECT) && ((n_type & N_EXT) != 0) && ((n_desc & N_WEAK_DEF) != 0) )
                ordinal = BIND_SPECIAL_DYLIB_WEAK_LOOKUP;
            
            if(processSymbol(header, ordinal, symstr))
               SET_LIBRARY_ORDINAL(symbols64[i].n_desc, shimOrdinal);
        }
    }
    
    if(fixup)
    {
        struct dyld_chained_fixups_header* chainsHeader = (struct dyld_chained_fixups_header*)((uint64_t)header + fixup->dataoff);
        assert(chainsHeader->fixups_version == 0);
        assert(chainsHeader->symbols_format == 0);
        LOG("import format=%d offset=%x count=%d\n", chainsHeader->imports_format, chainsHeader->imports_offset,
               chainsHeader->imports_count);
        
        dyld_chained_import*          imports;
        dyld_chained_import_addend*   importsA32;
        dyld_chained_import_addend64* importsA64;
        char*                         symbolsPool     = (char*)chainsHeader + chainsHeader->symbols_offset;
        int                                 libOrdinal;
        switch (chainsHeader->imports_format) {
            case DYLD_CHAINED_IMPORT:
                imports = (dyld_chained_import*)((uint8_t*)chainsHeader + chainsHeader->imports_offset);
                for (uint32_t i=0; i < chainsHeader->imports_count; ++i) {
                     char* symbolName = &symbolsPool[imports[i].name_offset];
                    uint8_t libVal = imports[i].lib_ordinal;
                    if ( libVal > 0xF0 )
                        libOrdinal = (int8_t)libVal;
                    else
                        libOrdinal = libVal;
                    
                    /* c++ template may link as weak symbols:BIND_SPECIAL_DYLIB_WEAK_LOOKUP
                     https://stackoverflow.com/questions/58241873/why-are-functions-generated-on-use-of-template-have-weak-as-symbol-type
                     https://stackoverflow.com/questions/44335046/how-does-the-linker-handle-identical-template-instantiations-across-translation
                     */
                    
                    LOG("import[%d] ordinal=%x weak=%d name=%s\n", i, imports[i].lib_ordinal, imports[i].weak_import, symbolName);
                    if(processSymbol(header, libOrdinal, symbolName))
                        imports[i].lib_ordinal = shimOrdinal;
                }
                break;
            case DYLD_CHAINED_IMPORT_ADDEND:
                importsA32 = (dyld_chained_import_addend*)((uint8_t*)chainsHeader + chainsHeader->imports_offset);
                for (uint32_t i=0; i < chainsHeader->imports_count; ++i) {
                     char* symbolName = &symbolsPool[importsA32[i].name_offset];
                    uint8_t libVal = importsA32[i].lib_ordinal;
                    if ( libVal > 0xF0 )
                        libOrdinal = (int8_t)libVal;
                    else
                        libOrdinal = libVal;
                    
                    LOG("importA32[%d] ordinal=%x weak=%d name=%s\n", i, importsA32[i].lib_ordinal, importsA32[i].weak_import, symbolName);
                    if(processSymbol(header, libOrdinal, symbolName))
                        importsA32[i].lib_ordinal = shimOrdinal;
                }
                break;
            case DYLD_CHAINED_IMPORT_ADDEND64:
                importsA64 = (dyld_chained_import_addend64*)((uint8_t*)chainsHeader + chainsHeader->imports_offset);
                for (uint32_t i=0; i < chainsHeader->imports_count; ++i) {
                     char* symbolName = &symbolsPool[importsA64[i].name_offset];
                    uint16_t libVal = importsA64[i].lib_ordinal;
                    if ( libVal > 0xFFF0 )
                        libOrdinal = (int16_t)libVal;
                    else
                        libOrdinal = libVal;
                    
                    LOG("importA64[%d] ordinal=%x weak=%d name=%s\n", i, importsA64[i].lib_ordinal, importsA64[i].weak_import, symbolName);
                    if(processSymbol(header, libOrdinal, symbolName))
                        importsA64[i].lib_ordinal = shimOrdinal;
                }
                break;
           default:
                fprintf(stderr, "unknown imports format: %d\n", chainsHeader->imports_format);
                return -1;
        }
    }

    return 0;
}

int processMachO(const char* file, int (*process)(void*))
{
    int fd = open(file, O_RDWR);
    if(fd < 0) {
        fprintf(stderr, "open %s error:%d,%s\n", file, errno, strerror(errno));
        return -1;
    }
    
    struct stat st;
    if(stat(file, &st) < 0) {
        fprintf(stderr, "stat %s error:%d,%s\n", file, errno, strerror(errno));
        return -1;
    }
    
    LOG("file size = %lld\n", st.st_size);
    
    void* macho = mmap(NULL, st.st_size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
    if(macho == MAP_FAILED) {
        fprintf(stderr, "map %s error:%d,%s\n", file, errno, strerror(errno));
        return -1;
    }
    
    uint32_t magic = *(uint32_t*)macho;
    LOG("macho magic=%08x\n", magic);
    if(magic==FAT_MAGIC || magic==FAT_CIGAM) {
        struct fat_header* fathdr = (struct fat_header*)macho;
        struct fat_arch* archdr = (struct fat_arch*)((uint64_t)fathdr + sizeof(*fathdr));
        int count = magic==FAT_MAGIC ? fathdr->nfat_arch : NXSwapInt(fathdr->nfat_arch);
        for(int i=0; i<count; i++) {
            uint32_t offset = magic==FAT_MAGIC ? archdr[i].offset : NXSwapInt(archdr[i].offset);
            if(process((void*)((uint64_t)macho + offset)) < 0)
               return -1;
        }
    } else if(magic==FAT_MAGIC_64 || magic==FAT_CIGAM_64) {
        struct fat_header* fathdr = (struct fat_header*)macho;
        struct fat_arch_64* archdr = (struct fat_arch_64*)((uint64_t)fathdr + sizeof(*fathdr));
        int count = magic==FAT_MAGIC_64 ? fathdr->nfat_arch : NXSwapInt(fathdr->nfat_arch);
        for(int i=0; i<count; i++) {
            uint64_t offset = magic==FAT_MAGIC_64 ? archdr[i].offset : NXSwapLongLong(archdr[i].offset);
            if(process((void*)((uint64_t)macho + offset)) < 0)
                return -1;
        }
    } else if(magic == MH_MAGIC_64) {
        return process((void*)macho);
    } else {
        fprintf(stderr, "unknown magic: %08x\n", magic);
        return -1;
    }
    
    //msync(macho, st.st_size, MS_SYNC);
    munmap(macho, st.st_size);

    close(fd);
    
    return 0;
}

int main(int argc, const char * argv[])
{
    if(argc != 2) {
        printf("Usage: %s [target]\n", getprogname());
        return 0;
    }
    
    const char* shimfile = argv[1];
    if(processMachO(shimfile, processTarget) < 0) {
        fprintf(stderr, "processTarget error!\n");
        return -1;
    }

    return 0;
}
