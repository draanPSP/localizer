.set noreorder 

#include "pspstub.s" 

STUB_START "scePaf",0x00090011,0x00030005 
STUB_FUNC  0x970DC20D, scePafGetTextS
STUB_FUNC  0xDF056F33, scePafGetPageStringS
STUB_FUNC  0x11EFC5FD, paf_sprintf
STUB_END
