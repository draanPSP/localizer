#include <pspkernel.h>

#include <systemctrl.h>
#include <systemctrl_se.h>

#include <string.h>
#include <stdio.h>

#include "../structs.h"
#include "genesis.h"

#define MAKE_CALL(a, f) _sw(0x0C000000 | (((u32)(f) >> 2) & 0x03ffffff), a);
#define MAKE_JUMP(a, f) _sw(0x08000000 | (((u32)(f)&0x0ffffffc) >> 2), a);
#define PSP_FIRMWARE(f) ((((f >> 8) & 0xF) << 24) | (((f >> 4) & 0xF) << 16) | ((f & 0xF) << 8) | 0x10)
#define U_EXTRACT_CALL(x) ((((u32)_lw((u32)x)) & ~0x0C000000) << 2)
#define U_EXTRACT_IMPORT(x) ((((u32)_lw((u32)x)) & ~0x08000000) << 2)

PSP_MODULE_INFO("LocalizerUser", 0, 2, 1);
PSP_HEAP_SIZE_KB(128);

// From RCOMage, thanks ZiNgA BuRgA
typedef struct
{
	uint32_t signature; // RCO_SIGNATURE
	uint32_t version;
	// 0x70 - UMD RCOs (video?), FW1.00
	// 0x71 - UMD RCOs (audio?), FW1.50, FW2.50
	// 0x90 - FW2.60
	// 0x95 - FW2.70, FW2.71
	// 0x96 - FW2.80, FW2.82, FW3.00, FW3.03, FW3.10, FW3.30, FW3.40
	// 0x100 - FW3.50, FW3.52, FW5.00, FW5.55
	uint32_t null;
	uint32_t compression;
	// upper nibble = compression,
	// lower nibble: 0=flash0 RCOs, 1=UMD RCOs????
	/*
	#define RCO_COMPRESSION_NONE 0x00 // entries for 0x01 ??
	#define RCO_COMPRESSION_ZLIB 0x10
	#define RCO_COMPRESSION_RLZ 0x20
	*/
	// main table pointers
	uint32_t pMainTable;	// type 1
	uint32_t pVSMXTable;	// type 2
	uint32_t pTextTable;	// type 3
	uint32_t pSoundTable; // type 5
	uint32_t pModelTable; // type 6
	uint32_t pImgTable;		// type 4
	uint32_t pUnknown;		// always 0xFFFFFFFF
	uint32_t pFontTable;	// type 7
	uint32_t pObjTable;		// type 8
	uint32_t pAnimTable;	// type 9
	// text stuff
	uint32_t pTextData; // NOTE: this may == pLabelData if lTextData == 0
	uint32_t lTextData;
	uint32_t pLabelData;
	uint32_t lLabelData;
	uint32_t pEventData;
	uint32_t lEventData;
	// pointer data
	uint32_t pTextPtrs;
	uint32_t lTextPtrs;
	uint32_t pImgPtrs;
	uint32_t lImgPtrs;
	uint32_t pModelPtrs;
	uint32_t lModelPtrs;
	uint32_t pSoundPtrs;
	uint32_t lSoundPtrs;
	uint32_t pObjPtrs;
	uint32_t lObjPtrs;
	uint32_t pAnimPtrs;
	uint32_t lAnimPtrs;
	// attached data
	uint32_t pImgData;
	uint32_t lImgData;
	uint32_t pSoundData;
	uint32_t lSoundData;
	uint32_t pModelData;
	uint32_t lModelData;
	// always 0xFFFFFFFF
	uint32_t unknown[3];
} PRFHeader;

typedef struct
{
	u32 labelOffset;
	u32 length;
	u32 offset;
} RCOTextIndex;
//

// Done by dumping args & reversing a bit
typedef struct
{
	int id;					// 0
	int unkFF;			// 4
	int unk2;				// 8
	void *rco_buf;	// 12
	int unk3;				// 16
	u16 sunk;				// 20
	u16 sunk2;			// 22
	int unk4;				// 24
	void *pointer;	// 28
	char *rco_path; // 32
	void *pointer2; // 36
	void *pointer3; // 40
	int unk5;				// 44
} RCO_STRUCT;

typedef struct
{
	u8 devkit_low;
	u8 devkit_high;
	u16 unk;
	u32 string_num;
} DATA_STRUCT;

//

// Function declarations
int paf_sprintf(char *, const char *, ...);
void LLibClearCaches();
void LLibPatchExports(const char *modname, const char *lib, u32 nid, void *patchfunction);
SceModule2 *LLibFindModuleByName(const char *name);
u32 LLibFindProc(const char *modname, const char *lib, u32 nid);

// Function pointers
wchar_t *(*scePafGetText)(void *arg, char *string);
int (*scePafGetPageString)(RCO_STRUCT *resource, DATA_STRUCT *data, int *arg2, wchar_t **string, int *temp0);
// int (*vshGetRegistryValue)(u32 *option, char *name, void *arg2, int size, int *value);
// int (*vshSetRegistryValue)(u32 *option, char *name, int size,  int *value);

// Global variables
char isLocalizerEnabled = 0;
char isGameCat = 0;
char isDatabasePresent = 0;
STMOD_HANDLER previous = NULL;
char *buffer = NULL;
u32 devkit_ver = 0;

// Strings
char *db = "ms0:/seplugins/localizer.dat";
char *db_ef = "ef0:/seplugins/localizer.dat";

void dump(char *path, void *buf, int size)
{
	SceUID fd = sceIoOpen(path, PSP_O_WRONLY | PSP_O_CREAT, 0777);
	sceIoLseek(fd, 0, SEEK_END);
	sceIoWrite(fd, buf, size);
	sceIoClose(fd);
}

void *m_alloc(int size)
{
	SceUID block_id = sceKernelAllocPartitionMemory(2, "", PSP_SMEM_Low, size + 4, NULL);
	void *buff = (void *)sceKernelGetBlockHeadAddr(block_id);
	memset(buff, 0, size);
	memcpy(buff, &block_id, 4);
	return buff + 4;
}

void m_free(void *buff)
{
	int *block_id = (int *)(buff - 4);
	sceKernelFreePartitionMemory(*block_id);
}

int checksize(wchar_t *string)
{
	char *str = (char *)string;
	int i = 0;
	while (str[i * 2] != 0)
	{
		i++;
	}
	return i * 2;
}

void MD5Hash(u8 *buff, int size, u8 *digest)
{
	SceKernelUtilsMd5Context ctx;
	sceKernelUtilsMd5BlockInit(&ctx);
	sceKernelUtilsMd5BlockUpdate(&ctx, buff, size);
	sceKernelUtilsMd5BlockResult(&ctx, digest);
}

RCOENTRY *locGetRCOEntryByHash(u8 *digest)
{
	LOCHEADER *header = (LOCHEADER *)buffer;
	RCOENTRY *rco_entries = (RCOENTRY *)((int)buffer + header->rco_entries_start);
	int i;
	for (i = 0; i < header->n_rco_entries; i++)
	{
		if (memcmp(rco_entries[i].rco_name_digest, digest, 16) == 0)
			return &rco_entries[i];
	}
	return NULL;
}

wchar_t *locGetTextByRCOandLabelHash(RCOENTRY *entry, u8 *digest)
{
	LOCHEADER *header = (LOCHEADER *)buffer;
	TEXTENTRY *entries = (TEXTENTRY *)((int)buffer + header->text_entries_start + entry->text_entries_start_offset);

	int i;
	for (i = 0; i < entry->n_text_entries; i++)
	{
		if (memcmp(digest, entries[i].label_digest, 16) == 0)
			return (wchar_t *)(buffer + header->text_start + entries[i].text_offset);
	}
	return NULL;
}

/*
wchar_t* locGetTextById(RCOENTRY* entry, u32 id, u32 ver)
{
		LOCHEADER* header = (LOCHEADER*)buffer;
		TEXTENTRY* entries = (TEXTENTRY*)((int)buffer + header->text_entries_start + entry->text_entries_start_offset);

		int i;
		for(i = 0; i < entry->n_text_entries; i++)
		{
		u32 search_id = 0;
		if(ver == PSP_FIRMWARE(0x500)) search_id = entries[i].id_500;
		else if(ver == PSP_FIRMWARE(0x550)) search_id = entries[i].id_550;
		else if(ver == PSP_FIRMWARE(0x620)) search_id = entries[i].id_620;
		if(id == search_id) return (wchar_t*)((int)buffer + header->text_start + entries[i].text_offset);
		}
		return NULL;
}
*/

wchar_t *locGetTextByLabelHash(u8 *digest)
{
	LOCHEADER *header = (LOCHEADER *)buffer;
	TEXTENTRY *entries = (TEXTENTRY *)(buffer + header->text_entries_start);
	int i;
	for (i = 0; i < header->n_text_entries; i++)
	{
		if (memcmp(digest, entries[i].label_digest, 16) == 0)
			return (wchar_t *)(buffer + header->text_start + entries[i].text_offset);
	}
	return NULL;
}

int string_number = 0;
char *label = NULL;

int (*internal_paf_func)(RCO_STRUCT *resource, u16 *short_unk, u8 **unk_pointer);

int internal_paf_funcPatched(RCO_STRUCT *resource, u16 *short_unk, u8 **unk_pointer)
{
	int ret = internal_paf_func(resource, short_unk, unk_pointer);

	if (ret == 0)
	{
		u8 *pointer = *unk_pointer;

		// Redo what scePafGetPageString does to get current RCOTextIndex
		int offset = (string_number << 1) + string_number;
		char *target = (char *)(pointer + (offset << 2));
		RCOTextIndex *index = (RCOTextIndex *)(target + 8);

		PRFHeader *header = (PRFHeader *)resource->rco_buf;
		if (header->pLabelData != 0xFFFFFFFF)
		{
			u8 *label_pointer = resource->rco_buf + header->pLabelData;
			if (index->labelOffset != 0xFFFFFFFF)
			{
				label = (char *)(label_pointer + index->labelOffset);
			}
		}
		else
		{
			label = NULL;
		}
	}
	else
	{
		label = NULL;
	}
	return ret;
}

int scePafGetPageStringPatched(RCO_STRUCT *resource, DATA_STRUCT *data, int *arg2, wchar_t **string, int *temp0)
{
	// Set string number for internal Paf patched func
	string_number = data->string_num;

	// Call orginal func to get label & fill unknown structs for valid return
	int ret = scePafGetPageString(resource, data, arg2, string, temp0);

	if (ret == 0)
	{
		if (label != NULL)
		{
			u8 digest[16];
			MD5Hash((u8 *)label, strlen(label), digest);
			wchar_t *text = locGetTextByLabelHash(digest);
			if (text != NULL)
			{
				string[0] = text;
				*temp0 = checksize(string[0]) + 2; // string size (with NULL at end)
			}
		}
	}
	return ret;
}

wchar_t *scePafGetTextPatched(void *arg, char *string)
{
	wchar_t *ret = scePafGetText(arg, string);
	if (isDatabasePresent && checksize(ret) != 0)
	{
		u8 digest[16];
		MD5Hash((u8 *)string, strlen(string), digest);

		wchar_t *text = locGetTextByLabelHash(digest);
		if (text != NULL)
			return text;
	}
	return ret;
}

static unsigned int size_fake_stat = 88;
static unsigned char fake_stat[] __attribute__((aligned(16))) = {
		0xff,
		0x21,
		0x00,
		0x00,
		0x20,
		0x00,
		0x00,
		0x00,
		0x10,
		0xe7,
		0x05,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0xda,
		0x07,
		0x0a,
		0x00,
		0x10,
		0x00,
		0x17,
		0x00,
		0x25,
		0x00,
		0x08,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0xdb,
		0x07,
		0x02,
		0x00,
		0x0a,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0xdb,
		0x07,
		0x02,
		0x00,
		0x0a,
		0x00,
		0x13,
		0x00,
		0x16,
		0x00,
		0x16,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
};

int sceIoGetstatPatched(const char *file, SceIoStat *stat)
{
	// I've actually getStated a file and grabbed the output
	memcpy(stat, fake_stat, size_fake_stat);
	stat->st_size = size_genesis;
	return 0;
}

SceUID sceIoOpenPatched(const char *file, int flags, SceMode mode)
{
	return 0x154; // fake UID
}

int sceIoReadPatched(SceUID fd, void *data, SceSize size)
{
	memcpy(data, genesis, size);
	return size_genesis;
}

int sceIoClosePatched(SceUID fd)
{
	return 0;
}

void PatchGameCategories(u32 text_addr)
{
	scePafGetText = (void *)U_EXTRACT_CALL(text_addr + 0x85C);
	MAKE_CALL(text_addr + 0x85C, scePafGetTextPatched);
	MAKE_JUMP(text_addr + 0xAA0, scePafGetTextPatched);

	scePafGetPageString = (void *)U_EXTRACT_IMPORT(text_addr + 0x1434);
	MAKE_JUMP(text_addr + 0x1434, scePafGetPageStringPatched);
	LLibClearCaches();
}

void PatchPaf(u32 text_addr, u32 ver)
{
	switch (ver)
	{
	case PSP_FIRMWARE(0x660):
		scePafGetText = (void *)(text_addr + 0x3C5EC);			 // NID 0x3874A5F8
		scePafGetPageString = (void *)(text_addr + 0x67894); // NID 0x4918DB1A

		_sw((int)scePafGetTextPatched, text_addr + 0x15D57C);
		_sw((int)scePafGetPageStringPatched, text_addr + 0x15D6B0);

		MAKE_CALL(text_addr + 0x3A67C, scePafGetPageStringPatched);
		MAKE_CALL(text_addr + 0x679F4, scePafGetPageStringPatched);

		internal_paf_func = (void *)(text_addr + 0x67D80);
		MAKE_CALL(text_addr + 0x67928, internal_paf_funcPatched);
		break;

	case PSP_FIRMWARE(0x639):
		scePafGetText = (void *)(text_addr + 0x3C3CC);			 // NID 0x70082F6F
		scePafGetPageString = (void *)(text_addr + 0x67674); // NID 0x9CFBB2D9

		_sw((int)scePafGetTextPatched, text_addr + 0x15D754);
		_sw((int)scePafGetPageStringPatched, text_addr + 0x15DA18);

		MAKE_CALL(text_addr + 0x3A45C, scePafGetPageStringPatched);
		MAKE_CALL(text_addr + 0x677D4, scePafGetPageStringPatched);

		internal_paf_func = (void *)(text_addr + 0x67B60);
		MAKE_CALL(text_addr + 0x67708, internal_paf_funcPatched);
		break;

	case PSP_FIRMWARE(0x635):
		// Untested, but the syscall bug is probably still present
		scePafGetText = (void *)(text_addr + 0x3C3CC);			 // NID 0x70082F6F
		scePafGetPageString = (void *)(text_addr + 0x67674); // NID 0x9CFBB2D9

		_sw((int)scePafGetTextPatched, text_addr + 0x15D664);
		_sw((int)scePafGetPageStringPatched, text_addr + 0x15D928);

		MAKE_CALL(text_addr + 0x3A45C, scePafGetPageStringPatched);
		MAKE_CALL(text_addr + 0x677D4, scePafGetPageStringPatched);

		internal_paf_func = (void *)(text_addr + 0x67B60);
		MAKE_CALL(text_addr + 0x67708, internal_paf_funcPatched);
		break;
	case PSP_FIRMWARE(0x620):
		// There is a strange bug in TN HEN with syscalls in kmode. We need to do stuff manually instead of using handy funcs.
		// Note that the clearCaches down there won't work too :(
		scePafGetText = (void *)(text_addr + 0x3C404);			 // NID 0xCB608DE5
		scePafGetPageString = (void *)(text_addr + 0x676AC); // NID 0x62D2266B

		_sw((int)scePafGetTextPatched, text_addr + 0x15D750);
		_sw((int)scePafGetPageStringPatched, text_addr + 0x15D074);

		MAKE_CALL(text_addr + 0x3A494, scePafGetPageStringPatched);
		MAKE_CALL(text_addr + 0x6780C, scePafGetPageStringPatched);

		internal_paf_func = (void *)(text_addr + 0x67B98);
		MAKE_CALL(text_addr + 0x67740, internal_paf_funcPatched);
		break;
	case PSP_FIRMWARE(0x550):
	case PSP_FIRMWARE(0x500):
	case PSP_FIRMWARE(0x503):
		if (isGameCat == 0) // only when Game Categories isn't there
		{
			scePafGetText = (void *)LLibFindProc("scePaf_Module", "scePaf", 0x970DC20D);
			scePafGetPageString = (void *)LLibFindProc("scePaf_Module", "scePaf", 0xDF056F33);
			LLibPatchExports("scePaf_Module", "scePaf", 0x970DC20D, scePafGetTextPatched);

			if (ver == PSP_FIRMWARE(0x550))
			{
				MAKE_CALL(text_addr + 0x3A434, scePafGetPageStringPatched);
				MAKE_CALL(text_addr + 0x67090, scePafGetPageStringPatched);

				internal_paf_func = (void *)(text_addr + 0x6741C);
				MAKE_CALL(text_addr + 0x66FC4, internal_paf_funcPatched);
			}
			else
			{
				// 5.00 or 5.03 fw
				MAKE_CALL(text_addr + 0x3A434, scePafGetPageStringPatched);
				MAKE_CALL(text_addr + 0x67090, scePafGetPageStringPatched);

				internal_paf_func = (void *)(text_addr + 0x6741C);
				MAKE_CALL(text_addr + 0x66FC4, internal_paf_funcPatched);
			}
		}
		LLibPatchExports("scePaf_Module", "scePaf", 0xDF056F33, scePafGetPageStringPatched);
		break;
	default:
		break;
	}
	LLibClearCaches();
}

void PatchSysconf(u32 text_addr, u32 ver)
{
	switch (ver)
	{
	case PSP_FIRMWARE(0x660):
		MAKE_CALL(text_addr + 0x1BDDC, sceIoGetstatPatched);
		MAKE_CALL(text_addr + 0x1BE1C, sceIoOpenPatched);
		MAKE_CALL(text_addr + 0x1BE34, sceIoReadPatched);
		MAKE_CALL(text_addr + 0x1BE54, sceIoClosePatched);
		break;
	case PSP_FIRMWARE(0x639):
	case PSP_FIRMWARE(0x635):
		MAKE_CALL(text_addr + 0x1B9E4, sceIoGetstatPatched);
		MAKE_CALL(text_addr + 0x1BA24, sceIoOpenPatched);
		MAKE_CALL(text_addr + 0x1BA3C, sceIoReadPatched);
		MAKE_CALL(text_addr + 0x1BA5C, sceIoClosePatched);
		break;
	case PSP_FIRMWARE(0x620):
		MAKE_CALL(text_addr + 0x1B398, sceIoGetstatPatched);
		MAKE_CALL(text_addr + 0x1B3D8, sceIoOpenPatched);
		MAKE_CALL(text_addr + 0x1B3F0, sceIoReadPatched);
		MAKE_CALL(text_addr + 0x1B410, sceIoClosePatched);
		break;
	case PSP_FIRMWARE(0x550):
		MAKE_CALL(text_addr + 0x18034, sceIoGetstatPatched);
		MAKE_CALL(text_addr + 0x18074, sceIoOpenPatched);
		MAKE_CALL(text_addr + 0x1808C, sceIoReadPatched);
		MAKE_CALL(text_addr + 0x180AC, sceIoClosePatched);
		break;
	case PSP_FIRMWARE(0x500):
	case PSP_FIRMWARE(0x503):
		MAKE_CALL(text_addr + 0x181FC, sceIoGetstatPatched);
		MAKE_CALL(text_addr + 0x1823C, sceIoOpenPatched);
		MAKE_CALL(text_addr + 0x18254, sceIoReadPatched);
		MAKE_CALL(text_addr + 0x18274, sceIoClosePatched);
		break;
	default:
		break;
	}
	LLibClearCaches();
}

void PatchHtmlViewer(u32 text_addr, u32 ver)
{
	LOCHEADER *header = (LOCHEADER *)buffer;
	/*
		 int i;
		 for(i = mod->text_addr + 0x14000; i < mod->text_addr + mod->text_size; i += 4)
		 {
			 if(*((int*)i) == 0x6D2F2F3A)   // "://m"
			 {
				char* patch = (char*)(i - 4); //go back to "http"
				strcpy(patch, site);
				LLibClearCaches();
			 }
		 }
	*/
	int patch;

	switch (ver)
	{
	case PSP_FIRMWARE(0x639):
	case PSP_FIRMWARE(0x635):
		patch = text_addr + 0x1C2B0;
		break;
	case PSP_FIRMWARE(0x620):
		patch = text_addr + 0x1C274;
		break;
	case PSP_FIRMWARE(0x550):
		patch = text_addr + 0x1AD78;
		break;
	case PSP_FIRMWARE(0x500):
	case PSP_FIRMWARE(0x503):
		patch = text_addr + 0x14AA0;
		break;
	default:
		patch = -1;
		break;
	}

	if (patch != -1)
	{
		strcpy((char *)patch, header->site);
	}
	LLibClearCaches();
}

void PatchVshmodule(u32 text_addr)
{
	/*
	vshGetRegistryValue = (void*)LLibFindProc("vsh_module", "vsh", 0x55DD305F);
	vshSetRegistryValue = (void*)LLibFindProc("vsh_module", "vsh", 0x0A9043D4);
	LLibPatchExports("vsh_module", "vsh", 0x55DD305F, vshGetRegistryValuePatched);
	LLibPatchExports("vsh_module", "vsh", 0x0A9043D4, vshSetRegistryValuePatched);
	LLibClearCaches();
	*/
}

int OnModuleStart(SceModule2 *mod)
{
	LOCHEADER *header = (LOCHEADER *)buffer;

	// These patches are designed for GC Revised v12, we don't need to patch the GC Light
	switch (devkit_ver)
	{
	case PSP_FIRMWARE(0x500):
	case PSP_FIRMWARE(0x503):
	case PSP_FIRMWARE(0x550):
		if (strcmp(mod->modname, "Game_Categories_Kernel") == 0)
			isGameCat = 1; // disable some patches on scePaf, patch GameCategories instead
		else if (strcmp(mod->modname, "Game_Categories_User") == 0)
			PatchGameCategories(mod->text_addr);
		break;
	default:
		break;
	}
	if (strcmp(mod->modname, "scePaf_Module") == 0)
	{
		PatchPaf(mod->text_addr, devkit_ver);
	}
	else if (strcmp(mod->modname, "vsh_module") == 0)
	{
		PatchVshmodule(mod->text_addr);
	}
	else if (strcmp(mod->modname, "htmlviewer_plugin_module") == 0 && strlen(header->site) > 0)
	{
		PatchHtmlViewer(mod->text_addr, devkit_ver);
	}
	else if (strcmp(mod->modname, "sysconf_plugin_module") == 0)
	{
		PatchSysconf(mod->text_addr, devkit_ver);
	}

	if (!previous)
		return 0;

	return previous(mod);
}

int module_start(SceSize args, void *argp)
{
	devkit_ver = sceKernelDevkitVersion();

	SceIoStat stat;
	memset(&stat, 0, sizeof(SceIoStat));

	if (sceIoGetstat(db_ef, &stat) >= 0)
	{
		buffer = (char *)m_alloc(stat.st_size);

		SceUID fd = sceIoOpen(db_ef, PSP_O_RDONLY, 0777);
		sceIoRead(fd, buffer, stat.st_size);
		sceIoClose(fd);
	}
	else if (sceIoGetstat(db, &stat) >= 0)
	{
		buffer = (char *)m_alloc(stat.st_size);

		SceUID fd = sceIoOpen(db, PSP_O_RDONLY, 0777);
		sceIoRead(fd, buffer, stat.st_size);
		sceIoClose(fd);
	}
	LOCHEADER *header = (LOCHEADER *)buffer;
	if (header->version == CURRENT_VERSION)
		isDatabasePresent = 1;

	SceModule2 *module = NULL;

	module = LLibFindModuleByName("Game_Categories");

	if (module)
		isGameCat = 1;

	previous = sctrlHENSetStartModuleHandler(OnModuleStart);
	return 0;
}

int module_stop()
{
	return 0;
}

/*

#define LANG_JP 0

int vshGetRegistryValuePatched(u32 *option, char *name, void *arg2, int size, int *value)
{
	if (name)
	{
	if(strcmp(name, "/CONFIG/SYSTEM/XMB/language") == 0)
	{
		if(isLocalizerEnabled)
		{
			*value = LANG_JP;
			return 0;
		}
	}
		}
		return vshGetRegistryValue(option, name, arg2, size, value);
}

int vshSetRegistryValuePatched(u32 *option, char *name, int size,  int *value)
{
	if (name)
	{
	if(strcmp(name, "/CONFIG/SYSTEM/XMB/language") == 0)
	{
		if(*value == LANG_JP)
		{
		 isLocalizerEnabled = !isLocalizerEnabled;
		}
		return 0;
	}
		}
		return vshSetRegistryValue(option, name, size, value);
}

int vshSetRegistryValuePatched(u32 *option, char *name, int size,  int *value)
{
	if (name)
	{
	if(strcmp(name, "/CONFIG/SYSTEM/XMB/language") == 0)
	{
		if(*value == LANG)
		{
			isLocalizerEnabled = 1;
		}else{
			isLocalizerEnabled = 0;
		}
		return 0;
	}
		}
		return vshSetRegistryValue(option, name, size, value);
}
*/
