	.set noreorder

#include "pspstub.s"

STUB_START "LocalizerLib",0x40090000,0x00040005
STUB_FUNC  0xB3169E4E, LLibClearCaches
STUB_FUNC  0x94F32CE0, LLibPatchExports
STUB_FUNC  0xEEA61CB7, LLibFindModuleByName
STUB_FUNC  0xBE2A6AC6, LLibFindProc
STUB_END
