#include <pspkernel.h>

#include <systemctrl.h>
#include <systemctrl_se.h>

#include <string.h>
#include <stdio.h>

#include "localize.h" //bin2c -> localizeruser.prx

#define JAL_OPCODE	0x0C000000
#define J_OPCODE    0x08000000
#define MAKE_CALL(a, f) _sw(JAL_OPCODE | (((u32)(f) >> 2)  & 0x03ffffff), a)
#define MAKE_JUMP(a, f) _sw(J_OPCODE | (((u32)(f) & 0x0ffffffc) >> 2), a)
#define PSP_FIRMWARE(f) ((((f >> 8) & 0xF) << 24) | (((f >> 4) & 0xF) << 16) | ((f & 0xF) << 8) | 0x10)

#include "nidpatch.h"
#include "../structs.h"

PSP_MODULE_INFO("LocalizerKernel", 0x1007, 1, 0);
PSP_MAIN_THREAD_ATTR(0);
PSP_HEAP_SIZE_KB(128);

int MainThread()
{    
    SceUID thid = sceKernelLoadModuleBuffer(size_localize, localize, 0, NULL);
    sceKernelStartModule(thid, 0, NULL, NULL, NULL);
	sceKernelExitDeleteThread(0);
    return 0;
}

int module_start(SceSize args, void *argp)
{  
    SceUID thid = sceKernelCreateThread("loc_kernel", MainThread, 0x18, 0xFA0, 0, NULL);
    sceKernelStartThread(thid, 0, NULL);
	return 0;
}
