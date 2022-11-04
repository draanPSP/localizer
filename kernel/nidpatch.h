void LLibClearCaches()
{
     sceKernelDcacheWritebackAll();
     sceKernelIcacheClearAll();
}

u32 LLibFindProc(const char* szMod, const char* szLib, u32 nid)
{
    return sctrlHENFindFunction(szMod, szLib, nid);
}

void LLibPatchExports(const char *modname, const char *lib, u32 nid, u32 patchfunction)
{
	int i = 0, u;
	SceModule2 *mod = (SceModule2 *)sceKernelFindModuleByName(modname);
		
	int entry_size = mod->ent_size;
	int entry_start = (int)mod->ent_top;
	
	while (i < entry_size)
	{
		SceLibraryEntryTable *entry = (SceLibraryEntryTable *)(entry_start + i);
		
		if (entry->libname && (strcmp(entry->libname, lib) == 0))
		{
			u32 *table = entry->entrytable;
			int total = entry->stubcount + entry->vstubcount;
			
			if (total > 0)
			{ 
				for (u = 0; u < total; u++)
				{ 
					if (table[u] == nid)
					{						
						if (patchfunction)
							table[u + total] = patchfunction;
						 
					    return;
					}
				} 
			} 	
		}
		
		i += (entry->len * 4);
	}
}

//BEWARE! Works only for usermode -> usermode!

void PatchImports(const char *modname, const char *lib, u32 nid, u32 patchfunction)
{
	int i = 0, u;
	SceModule2 *mod = (SceModule2 *)sceKernelFindModuleByName(modname);
		
	int stub_size = mod->stub_size;
	int stub_start = (int)mod->stub_top;
	
	while (i < stub_size)
	{
		SceLibraryStubTable *entry = (SceLibraryStubTable *)(stub_start + i);
		
		if (entry->libname && (strcmp(entry->libname, lib) == 0))
		{
			u32 *table = (u32*)entry->nidtable;
			int total = entry->stubcount + entry->vstubcount;
			
			if (total > 0)
			{ 
				for (u = 0; u < total; u++)
				{ 
					if (table[u] == nid)
					{						
						if (patchfunction)
						{
						   u32 addr = (u32)&((u32 *)entry->stubtable)[u << 1];
						   MAKE_JUMP(addr, patchfunction);
						 
					       return;
                        }
					}
				} 
			} 	
		}
		
		i += (entry->len * 4);
	}
}

SceModule2* LLibFindModuleByName(const char* name)
{
    SceModule2* mod;
    u32 k1 = pspSdkSetK1(0);
    mod = (SceModule2 *)sceKernelFindModuleByName(name);
    pspSdkSetK1(k1);
    return mod;
}
            
