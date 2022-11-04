u32 ReleaseContextAfterAddingOption(u32 id)
{
	if (id)
	{
		u32 *buffer = (u32 *)id;

		if (FindModuleByName("sysconf_plugin_module"))
		{
			SceRcoEntry *src = (SceRcoEntry *)buffer[0];
			SceRcoEntry *plane = (SceRcoEntry *)(((u8 *)src)+src->first_child);
			SceRcoEntry *mlist = (SceRcoEntry *)(((u8 *)plane)+plane->first_child);
			u32 *mlist_param = (u32 *)(((u8 *)mlist)+mlist->param);
			SceRcoEntry *base = (SceRcoEntry *)(((u8 *)mlist)+mlist->first_child);

			mlist->first_child = buffer[1];
			mlist->child_count = buffer[2];
			mlist_param[16] = buffer[3];
			mlist_param[18] = buffer[4];
			
			SceRcoEntry* lastEntry = base;
	
	        //Find the last entry
            int i;
	        for (i = 0; i < mlist->child_count; i++)
	        {
                lastEntry = (SceRcoEntry *)(((u8 *)lastEntry)+base->next_entry);
            }
            
            lastEntry->next_entry = 0;
		}
		paf_free(buffer);
		sceKernelDcacheWritebackAll();
	}
	return 0;
}

u32 AddOptionToContext(void *resource, SceRcoEntry *src, char *option)
{
    //Pointers
    SceRcoEntry *plane = (SceRcoEntry *)(((u8 *)src)+src->first_child);
    SceRcoEntry *mlist = (SceRcoEntry *)(((u8 *)plane)+plane->first_child);
    u32 *mlist_param = (u32 *)(((u8 *)mlist)+mlist->param);
    SceRcoEntry *base = (SceRcoEntry *)(((u8 *)mlist)+mlist->first_child);
    
    
    //Alloc buffer for data backup & new SceRcoEntry
    u32 *buffer = paf_malloc(20+(base->next_entry));
	
	//backup some data
	buffer[0] = (u32)src;
	buffer[1] = mlist->first_child;
	buffer[2] = mlist->child_count;
	buffer[3] = mlist_param[16];
	buffer[4] = mlist_param[18];
	
	
	//Modify some data in mlist (add 1 option)
	int n = mlist->child_count + 1;
	mlist->child_count = n;
	mlist_param[16] = (n+1)+(n%2);
	mlist_param[18] = (n/2)+(n%2);
	
	SceRcoEntry *item = (SceRcoEntry *)(((u8 *)buffer)+20);
	u32 *item_param = (u32 *)(((u8 *)item)+base->param);
	
	SceRcoEntry* lastEntry = base;
	
	//Find the last entry
	int i;
	for (i = 0; i+1 < n; i++)
	{
        lastEntry = (SceRcoEntry *)(((u8 *)lastEntry)+base->next_entry);
    }
    
    //Patch it (add one entry after)
    lastEntry->next_entry = (u32)(((u8 *)item)-((u8 *)lastEntry));
    
    paf_memcpy(item, base, base->next_entry);
    item->next_entry = 0;
    item->prev_entry = base->next_entry;
	item_param[0] = 0xBEEF;
	item_param[1] = (u32)option;
	
	sceKernelDcacheWritebackAll();
	return (u32)buffer;
}

int vshGetRegistryValuePatched(u32 *option, char *name, void *arg2, int size, int *value)
{
	if (name)
	{
        if (strcmp(name, "system_lang_find_in_sysconf!!") == 0)
		{
            sysconf_page = 1;
            int ret = vshGetRegistryValue(option, name, arg2, size, value);
            if(isLocalizerEnabled)
            {
                 *value = 12;
                 ret = 0;
            }
			return ret;
		}
    }
	return vshGetRegistryValue(option, name, arg2, size, value);
}

int vshSetRegistryValuePatched(u32 *option, char *name, int size,  int *value)
{
	if (name)
	{
		if (strcmp(name, "system_lang_find_in_sysconf!!") == 0)
		{
            if(*value = 12)
            {
			   isLocalizerEnabled = 1;
			   return 0;
            }
            isLocalizerEnabled = 0;
			return vshSetRegistryValue(option, name, size, value);
		}
	}

	return vshSetRegistryValue(option, name, size, value);
}
