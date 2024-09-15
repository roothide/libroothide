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

#define MACHO_VM_PAGE_SIZE  0x4000
#define MACHO_VM_PAGE_MASK  (MACHO_VM_PAGE_SIZE-1)

#define trunc_page(x)   ((x) & (~MACHO_VM_PAGE_MASK))
#define round_page(x)   trunc_page((x) + MACHO_VM_PAGE_MASK)

#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#pragma GCC diagnostic ignored "-Wunused-but-set-variable"


#define LOG(...)    //printf(__VA_ARGS__)

#include <string>
#include <vector>
#include <algorithm>

#define VROOT_SYMBOLE(NAME) "_"#NAME,

#define VROOT_API_DEF(RET,NAME,ARGTYPES) VROOT_SYMBOLE(NAME)
#define VROOTAT_API_DEF(RET,NAME,ARGTYPES) VROOT_SYMBOLE(NAME)
#define VROOT_API_WRAP(RET,NAME,ARGTYPES,ARGS,PATHARG) VROOT_SYMBOLE(NAME)
#define VROOTAT_API_WRAP(RET,NAME,ARGTYPES,ARGS,FD,PATHARG,ATFLAG) VROOT_SYMBOLE(NAME)

std::vector<std::string> g_shim_apis = {
#define VROOT_API_ALL
#define VROOT_INTERNAL
#include "vroot.h"
};

int g_slice_index = 0;

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

bool processSymbol(struct mach_header_64* header, int ordinal, const char* symbol)
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

static size_t write_uleb128(uint64_t val, uint8_t buf[10])
{
    uint8_t* start=buf;
    unsigned char c;
    bool flag;
    do {
        c = val & 0x7f;
        val >>= 7;
        flag = val != 0;
        *buf++ = c | (flag ? 0x80 : 0);
    } while (flag);
    return buf-start;
}
static size_t write_sleb128(uint64_t val, uint8_t buf[10])
{
    uint8_t* start=buf;
    unsigned char c;
    bool flag;
    do {
        c = val & 0x7f;
        val >>= 7;
        flag = c & 0x40 ? val != -1 : val != 0;
        *buf++ = c | (flag ? 0x80 : 0);
    } while (flag);
    return buf-start;
}
static uint64_t read_uleb128(uint8_t** pp, uint8_t* end)
{
    uint8_t* p = *pp;
    uint64_t result = 0;
    int         bit = 0;
    do {
        if ( p == end ) {
            abort();
            break;
        }
        uint64_t slice = *p & 0x7f;

        if ( bit > 63 ) {
            abort();
            break;
        }
        else {
            result |= (slice << bit);
            bit += 7;
        }
    }
    while (*p++ & 0x80);
    *pp = p;
    return result;
}
static int64_t read_sleb128(uint8_t** pp, uint8_t* end)
{
    uint8_t* p = *pp;
    int64_t  result = 0;
    int      bit = 0;
    uint8_t  byte = 0;
    do {
        if ( p == end ) {
            abort();
            break;
        }
        byte = *p++;
        result |= (((int64_t)(byte & 0x7f)) << bit);
        bit += 7;
    } while (byte & 0x80);
    *pp = p;
    // sign extend negative numbers
    if ( ((byte & 0x40) != 0) && (bit < 64) )
        result |= (~0ULL) << bit;
    return result;
}

char* BindCodeDesc[] = 
{
"BIND_OPCODE_DONE", //					0x00
"BIND_OPCODE_SET_DYLIB_ORDINAL_IMM", //			0x10
"BIND_OPCODE_SET_DYLIB_ORDINAL_ULEB", //			0x20
"BIND_OPCODE_SET_DYLIB_SPECIAL_IMM", //			0x30
"BIND_OPCODE_SET_SYMBOL_TRAILING_FLAGS_IMM", //		0x40
"BIND_OPCODE_SET_TYPE_IMM", //				0x50
"BIND_OPCODE_SET_ADDEND_SLEB", //				0x60
"BIND_OPCODE_SET_SEGMENT_AND_OFFSET_ULEB", //			0x70
"BIND_OPCODE_ADD_ADDR_ULEB", //				0x80
"BIND_OPCODE_DO_BIND", //					0x90
"BIND_OPCODE_DO_BIND_ADD_ADDR_ULEB", //			0xA0
"BIND_OPCODE_DO_BIND_ADD_ADDR_IMM_SCALED", //			0xB0
"BIND_OPCODE_DO_BIND_ULEB_TIMES_SKIPPING_ULEB", //		0xC0
"BIND_OPCODE_THREADED", //					0xD0
};

enum bindtype {
    bindtype_bind,
    bindtype_lazy,
    bindtype_weak
};

#define rebuild(p, copysize, _resized) {\
tmpcache = realloc(tmpcache, tmpsize+copysize);\
memcpy((void*)((uint64_t)tmpcache+tmpsize), p, copysize);\
tmpsize += copysize;\
copied=true;\
if(_resized) resized=true;\
}

void* rebind(int shimOrdinal, struct mach_header_64* header, enum bindtype type, void* data, uint32_t* size)
{
    bool rebuilt=false;
    void* newbind = NULL;
    size_t newsize = 0;

    bool resized=false;
    void* tmpcache = NULL;
    size_t tmpsize = 0;
    uint8_t* recordp = NULL;

    uint8_t*  p    = (uint8_t*)data;
    uint8_t*  end  = (uint8_t*)((uint64_t)data + *size);
    const char*     symbolName = NULL;
    int             libraryOrdinal = 0;
    bool            libraryOrdinalSet = false;
    bool            weakImport = false;
    bool stop=false;
    int i=0;
    recordp = p;
    while ( !stop && (p < end) ) 
    {
        bool copied=false;
        uint8_t immediate = *p & BIND_IMMEDIATE_MASK;
        uint8_t opcode = *p & BIND_OPCODE_MASK;
        uint8_t* curp = p++;
        static char const* bindtypedesc[] = {"bind","lazy","weak"};
        LOG("[%s][%d] %02X %s %02X\n", bindtypedesc[type], i++, opcode, BindCodeDesc[opcode>>4], immediate);
        switch (opcode) {
            case BIND_OPCODE_DONE:
                if(type!=bindtype_lazy) stop = true;
                break;
            case BIND_OPCODE_SET_DYLIB_ORDINAL_IMM:
            {
                assert(type != bindtype_weak);

                libraryOrdinal = immediate;
                libraryOrdinalSet = true;

                LOG("%d => %d\n", libraryOrdinal, shimOrdinal);

                if(shimOrdinal<BIND_IMMEDIATE_MASK) {
                    uint8_t newcode = opcode | shimOrdinal;
                    rebuild(&newcode, 1, false);
                } else {
                    uint8_t newop = BIND_OPCODE_SET_DYLIB_ORDINAL_ULEB;
                    rebuild(&newop, 1, false);
                    uint8_t ordinal[10];
                    rebuild(ordinal, write_uleb128(shimOrdinal, ordinal), true);
                }
            }
            break;

            case BIND_OPCODE_SET_DYLIB_ORDINAL_ULEB:
            {
                assert(type != bindtype_weak);

                uint8_t* oldp=p;
                ++i;

                libraryOrdinal = (int)read_uleb128(&p, end);
                libraryOrdinalSet = true;

                LOG("%d => %d\n", libraryOrdinal, shimOrdinal);

                uint8_t ordinal[10];
                int newlen = write_uleb128(shimOrdinal, ordinal);

                rebuild(curp, 1, false);
                rebuild(ordinal, newlen, newlen != (p-oldp));
            }
            break;

            case BIND_OPCODE_SET_DYLIB_SPECIAL_IMM:
                assert(type != bindtype_weak);
                // the special ordinals are negative numbers
                if ( immediate == 0 )
                    libraryOrdinal = 0;
                else {
                    int8_t signExtended = BIND_OPCODE_MASK | immediate;
                    libraryOrdinal = signExtended;
                }
                libraryOrdinalSet = true;
                break;

            case BIND_OPCODE_SET_SYMBOL_TRAILING_FLAGS_IMM:
            {
                ++i;
                weakImport = ( (immediate & BIND_SYMBOL_FLAGS_WEAK_IMPORT) != 0 );
                symbolName = (char*)p;
                while (*p != '\0')
                    ++p;
                ++p;
            }
            break;

            case BIND_OPCODE_SET_TYPE_IMM:
                assert(type != bindtype_lazy);
                break;
            case BIND_OPCODE_SET_SEGMENT_AND_OFFSET_ULEB:
                ++i;
                read_uleb128(&p, end);
                break;
            case BIND_OPCODE_SET_ADDEND_SLEB:
                ++i;
                read_sleb128(&p, end);
                break;


            case BIND_OPCODE_DO_BIND:
            {
                if(processSymbol(header, libraryOrdinal, symbolName)) 
                {
                    if(resized) rebuilt=true;
                    resized = false;

                    assert(tmpcache != NULL);

                    newbind = realloc(newbind, newsize+tmpsize);
                    memcpy((void*)((uint64_t)newbind+newsize), tmpcache, tmpsize);
                    newsize += tmpsize;

                }
                else
                {
                    int cursize = curp-recordp;
                    newbind = realloc(newbind, newsize+cursize);
                    memcpy((void*)((uint64_t)newbind+newsize), recordp, cursize);
                    newsize += cursize;
                }

                recordp = curp;

                if(tmpcache) free(tmpcache);
                tmpcache=NULL;
                tmpsize=0;
            }
            break;


            case BIND_OPCODE_THREADED:
                switch (immediate) {
                    case BIND_SUBOPCODE_THREADED_SET_BIND_ORDINAL_TABLE_SIZE_ULEB:
                    {
                        ++i;
                        uint64_t targetTableCount = read_uleb128(&p, end);
                        assert (!( targetTableCount > 65535 ));
                    }
                    break;
                    case BIND_SUBOPCODE_THREADED_APPLY:
                        break;
                    default:
                        abort();
                }
                break;
            case BIND_OPCODE_ADD_ADDR_ULEB:
            case BIND_OPCODE_DO_BIND_ADD_ADDR_ULEB:
                assert(type != bindtype_lazy);
                read_uleb128(&p, end);
                ++i;
                break;
            case BIND_OPCODE_DO_BIND_ADD_ADDR_IMM_SCALED:
                assert(type != bindtype_lazy);
                break;
            case BIND_OPCODE_DO_BIND_ULEB_TIMES_SKIPPING_ULEB:
                assert(type != bindtype_lazy);
                read_uleb128(&p, end);
                ++i;
                read_uleb128(&p, end);
                ++i;
                break;
            default:
                abort();
        }

        if(!copied) rebuild(curp, p-curp, false);
    }

    int leftsize = end - recordp;
    if(leftsize > 0) {
        newbind = realloc(newbind, newsize+leftsize);
        memcpy((void*)((uint64_t)newbind+newsize), recordp, leftsize);
        newsize += leftsize;
    }

    if(rebuilt) {
        *size = newsize;
        return newbind;
    }

    if(newbind) {
        assert(*size == newsize);
        memcpy(data, newbind, *size);
        free(newbind);
    }

    return NULL;
}

int processTarget(int fd, void* slice)
{
    uint32_t magic = *(uint32_t*)slice;
    if(magic != MH_MAGIC_64) {
        fprintf(stderr, "unsupport mach-o: %08x\n", magic);
        return -1;
    }
    
    struct mach_header_64* header = (struct mach_header_64*)slice;
    
    if((header->flags & MH_TWOLEVEL) == 0) {
        printf("mach-o has no MH_TWOLEVEL\n");
    }

    if((header->flags & MH_FORCE_FLAT) != 0) {
        fprintf(stderr, "mach-o has MH_FORCE_FLAT\n");
        abort();
    }

    int libOrdinal=1;
    int shimOrdinal=0;
    
    int first_sec_off = 0;
    struct segment_command_64* linkedit_seg = NULL;
    struct symtab_command* symtab = NULL;
    struct dysymtab_command* dysymtab = NULL;
    struct linkedit_data_command* fixup = NULL;
    struct dyld_info_command* dyld_info = NULL;
    struct linkedit_data_command* code_sign = NULL;
    
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
            
            case LC_DYLD_INFO:
            case LC_DYLD_INFO_ONLY:
                assert(dyld_info==NULL);
                dyld_info = (struct dyld_info_command*)lc;
                break;

            case LC_CODE_SIGNATURE:
                code_sign = (struct linkedit_data_command*)lc;
                break;
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
            //assert(ordinal > 0); ordinal=0:BIND_SPECIAL_DYLIB_SELF
            
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
    
    if(dyld_info)
    {
        // some machos only have export_offset, eg: padlock.dylib
        // assert(dyld_info->bind_off != 0); //follow dyld

        struct stat st;
        assert(fstat(fd, &st)==0);
        assert(st.st_size == (linkedit_seg->fileoff+linkedit_seg->filesize));
        
        size_t newfsize = st.st_size;

        if(dyld_info->bind_off && dyld_info->bind_size)
        {
            LOG("bind_off=%x size=%x\n", dyld_info->bind_off, dyld_info->bind_size);
            uint32_t size = dyld_info->bind_size;
            void* data = (void*)((uint64_t)header + dyld_info->bind_off);
            void* newbind = rebind(shimOrdinal, header, bindtype_bind, data, &size);
            LOG("new bind=%p size=%x\n", newbind, size);
            if(newbind)
            {
                assert(g_slice_index==0);

                assert(lseek(fd, 0, SEEK_END)==newfsize);
                assert(write(fd, newbind, size)==size);
                dyld_info->bind_off = newfsize;
                dyld_info->bind_size = size;
                linkedit_seg->filesize += size;
                linkedit_seg->vmsize += size;
                newfsize += size;

                free(newbind);
            }
        }
        if(dyld_info->weak_bind_off && dyld_info->weak_bind_size)
        {
            LOG("weak_bind_off=%x size=%x\n", dyld_info->weak_bind_off, dyld_info->weak_bind_size);
            uint32_t size = dyld_info->weak_bind_size;
            void* data = (void*)((uint64_t)header + dyld_info->weak_bind_off);
            void* newbind = rebind(shimOrdinal,header, bindtype_weak, data, &size);
            LOG("new weak bind=%p size=%x\n", newbind, size);
            assert(newbind == NULL); //weak bind, no ordinal
        }
        if(dyld_info->lazy_bind_off && dyld_info->lazy_bind_size)
        {
            LOG("lazy_bind_off=%x size=%x\n", dyld_info->lazy_bind_off, dyld_info->lazy_bind_size);
            uint32_t size = dyld_info->lazy_bind_size;
            void* data = (void*)((uint64_t)header + dyld_info->lazy_bind_off);
            void* newbind = rebind(shimOrdinal,header, bindtype_lazy, data, &size);
            LOG("new lazy bind=%p size=%x\n", newbind, size);
            if(newbind)
            {
                assert(g_slice_index==0);

                assert(lseek(fd, 0, SEEK_END)==newfsize);
                assert(write(fd, newbind, size)==size);
                dyld_info->lazy_bind_off = newfsize;
                dyld_info->lazy_bind_size = size;
                linkedit_seg->filesize += size;
                linkedit_seg->vmsize += size;
                newfsize += size;

                free(newbind);
            }
        }

        if(code_sign && newfsize!=st.st_size)
        {
            // some machos has padding data in the end
            // assert(st.st_size == (code_sign->dataoff+code_sign->datasize));
            
            assert(g_slice_index==0);
            
            void* data = (void*)((uint64_t)header + code_sign->dataoff);
            size_t size = code_sign->datasize;
            assert(lseek(fd, 0, SEEK_END)==newfsize);
            assert(write(fd, data, size)==size);
            code_sign->dataoff = newfsize;
            linkedit_seg->filesize += size;
            linkedit_seg->vmsize += size;
            newfsize += size;
        }

        // for(int i=0; i<(newfsize%0x10); i++) {
        //     int zero=0;
        //     linkedit_seg->filesize++;
        //     assert(write(fd, &zero, 1)==1);
        // }

        linkedit_seg->vmsize = round_page(linkedit_seg->vmsize);
    }
    
    return 0;
}

int processMachO(const char* file, int (*process)(int,void*))
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
    
    void* macho = mmap(NULL, st.st_size, PROT_READ|PROT_WRITE, MAP_PRIVATE, fd, 0);
    if(macho == MAP_FAILED) {
        fprintf(stderr, "map %s error:%d,%s\n", file, errno, strerror(errno));
        return -1;
    }
    
    uint32_t magic = *(uint32_t*)macho;
    LOG("macho magic=%08x\n", magic);
    if(magic==FAT_MAGIC || magic==FAT_CIGAM) {
        struct fat_header* fathdr = (struct fat_header*)macho;
        struct fat_arch* archdr = (struct fat_arch*)((uint64_t)fathdr + sizeof(*fathdr));
        int count = magic==FAT_MAGIC ? fathdr->nfat_arch : __builtin_bswap32(fathdr->nfat_arch);
        for(int i=0; i<count; i++) {
            uint32_t offset = magic==FAT_MAGIC ? archdr[i].offset : __builtin_bswap32(archdr[i].offset);
            if(process(fd, (void*)((uint64_t)macho + offset)) < 0)
               return -1;
            g_slice_index++;
        }
    } else if(magic==FAT_MAGIC_64 || magic==FAT_CIGAM_64) {
        struct fat_header* fathdr = (struct fat_header*)macho;
        struct fat_arch_64* archdr = (struct fat_arch_64*)((uint64_t)fathdr + sizeof(*fathdr));
        int count = magic==FAT_MAGIC_64 ? fathdr->nfat_arch : __builtin_bswap32(fathdr->nfat_arch);
        for(int i=0; i<count; i++) {
            uint64_t offset = magic==FAT_MAGIC_64 ? archdr[i].offset : __builtin_bswap64(archdr[i].offset);
            if(process(fd, (void*)((uint64_t)macho + offset)) < 0)
                return -1;
            g_slice_index++;
        }
    } else if(magic == MH_MAGIC_64) {
        if(process(fd, (void*)macho) < 0)
            return -1;
    } else {
        fprintf(stderr, "unknown magic: %08x\n", magic);
        return -1;
    }
    
    assert(lseek(fd, 0, SEEK_SET) == 0);
    if(write(fd, macho, st.st_size) != st.st_size) {
        fprintf(stderr, "write %lld error:%d,%s\n", st.st_size, errno, strerror(errno));
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
        printf("Usage: %s [target]\n", basename((char*)argv[0]));
        return 0;
    }
    
    const char* target = argv[1];
    printf("symredirect %s\n", target);
    printf("vroot api count = %d\n", g_shim_apis.size());
    if(processMachO(target, processTarget) < 0) {
        fprintf(stderr, "processTarget error!\n");
        return -1;
    }
    printf("finished.\n\n");

    return 0;
}
