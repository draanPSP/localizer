#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

#include "utils.h"
#include "../structs.h"

#define LEFT_ARROW 0x3C
#define RIGHT_ARROW 0x3E

#define SINGLE_QUOTE 0x27
#define DASH 0x23
#define AT 0x40
#define EQUAL 0x3D
#define BACKSLASH 0x5C
#define CHAR_N 0x6E
#define BIG_CHAR_N 4E

#define INCORRECT_START_VALUE -1
#define INCORRECT_END_VALUE -2

extern bool debug;

short int glob_act_as = 0;
char site[64];

typedef struct
{
        char rco_name_digest[16];
//        int value_500;
//        int value_550;
//        int value_620;
        char label_digest[16];
        wchar_t text[1024];
} INTERNALSTRUCT;

typedef struct
{
	char* label_buf;
	int label_size;
} LABEL_DESCRIPTOR;

typedef struct
{
	wchar_t* text_buf;
	int text_size;
} TEXT_DESCRIPTOR;

INTERNALSTRUCT internal_entries[8192]; //Yea, i know static buffers are bad. But it's working very good!
int rco_entries_num = 0;
int entries_num = 0;
int text_entries_in_rco_entry[50];

int getLabel(wchar_t *buf, LABEL_DESCRIPTOR *descr)
{
	int seek = 0;
    if(buf[seek] != SINGLE_QUOTE) return INCORRECT_START_VALUE;
	seek++; //omit '
	
	int label_size = 0;
	
	while(buf[seek] != SINGLE_QUOTE)
	{
        //if we have found " = ", that means user forgot about ending '
		if(buf[seek-1] == 0x20 && buf[seek] == EQUAL && buf[seek+1] == 0x20) return INCORRECT_END_VALUE;
		seek++;
		label_size++;
	}
	descr->label_buf = utftochar((char*)&buf[1], label_size*sizeof(wchar_t));
	descr->label_size = label_size;
	return label_size + 2; //label + 2 quotes
}

int getHex(wchar_t *buf, int *value)
{
	int seek = 0;
	if(buf[seek] != '0' && buf[seek+1] != 'x') return INCORRECT_START_VALUE;
	char* value_buf = utftochar((char*)&buf[seek], 5*sizeof(wchar_t)); //eg. 0x01C = 5 chars
	*value = strtol(value_buf, NULL, 16);
	free(value_buf);    
	return 5;
}

int getText(wchar_t *buf, TEXT_DESCRIPTOR *descr)
{
	int seek = 0;
	if(buf[seek] != SINGLE_QUOTE) return INCORRECT_START_VALUE;
	seek++;
	
	char isPreviousCharSlash = 0; //if yes, and current char is 'n', we'll replace them with 0xD and 0xA
	
	int text_size = 0;
	
	int seek_bckp = seek;
	
	while(buf[seek] != SINGLE_QUOTE)
	{
		if(buf[seek] == 0xD && buf[seek+1] == 0xA) return INCORRECT_END_VALUE;
		if(isPreviousCharSlash == 1 && buf[seek] == 'n')
		{
			buf[seek-1] = 0xD;
			buf[seek] = 0xA;
			isPreviousCharSlash = 0;
		}			
		if(buf[seek] == BACKSLASH)
		{
			isPreviousCharSlash = 1;
		}
		seek++;
		text_size++;
	}
	buf[seek] = 0;
	descr->text_buf = &buf[seek_bckp];
	descr->text_size = text_size;
	return text_size + 2; //text + 2 quotes
}

int parseLine(int line, char *rco_name, wchar_t *buf)
{
     int position = 0;
     int read = 0;
     LABEL_DESCRIPTOR lab_desc;
     TEXT_DESCRIPTOR  txt_desc;
     md5_byte_t digest[16];
     
     if(strcmp(rco_name, "localizer_internal") == 0)
     {
         int read = getLabel(&buf[position], &lab_desc);
         if(read == INCORRECT_START_VALUE)
         {
             printf("There is something wrong at line %d\n", line);
             printf("I can't understand Localizer's setting label, because there is an syntax error before it.\n");
             return -1;
         }
         else
         if(read == INCORRECT_END_VALUE)
         {
             printf("There is something wrong at line %d\n", line);
             printf("Localizer's setting label is not finished with '\n");
             return -1;
         }
                 
         position += read;
         /*
         if(strcmp(lab_desc.label_buf, "inCtfActAs") == 0)
         {
              free(lab_desc.label_buf);
              position += 3; //omit " = "
              int hex_value = 0;
              read = getHex(&buf[position], &hex_value);
              if(read == INCORRECT_START_VALUE)
              {
                  printf("There is something wrong at line %d\n", line);
                  printf("I can't understand inCtfActAs value!\n");
                  return -1;
              }
              position += read;
              glob_act_as = hex_value;
              return 0;
         }
         else 
         */
         if(strcmp(lab_desc.label_buf, "custom_site") == 0)
         {
              
              free(lab_desc.label_buf);
              position += 3; //omit " = "
              
              read = getLabel(&buf[position], &lab_desc);
              if(read == INCORRECT_START_VALUE)
              {
                  printf("There is something wrong at line %d\n", line);
                  printf("I can't understand custom site, because there is an syntax error before it.\n");
                  return -1;
              }
              if(read == INCORRECT_END_VALUE)
              {
                  printf("There is something wrong at line %d\n", line);
                  printf("Custom site is not finished with '\n");
                  return -1;
              }
              strcpy(site, lab_desc.label_buf);
              free(lab_desc.label_buf);
              return 0;
         }
     }
     if(debug)
     {
         printf("  ENTRY %d\n", entries_num);
     }
     //fill rco name
     //delete \n if it's there, delete all spaces after RCO name, etc.
     int rco_name_size = strlen(rco_name);
     if(rco_name[rco_name_size-1] == 0xA && rco_name[rco_name_size-2] != 0xD) rco_name_size -= 2;
     while(rco_name[rco_name_size-1] == 0x20) rco_name_size = rco_name_size - 1;
	 
	 MD5Hash(rco_name, rco_name_size, digest);
     memcpy(internal_entries[entries_num].rco_name_digest, digest, 16);
     
     if(debug)
     {
         printf("RCO NAME HASH: 0x");
         for(int x = 0; x < 16; x++)
         {
             char dig = digest[x];
             char hex1 = dig;
             char hex2 = dig >> 4;
             printf("%c%c", checkOneHex(hex1), checkOneHex(hex2));
         }
         printf("\n");
     }
     /*
     
     //fill data1's
     int hex_value = 0;
     read = getHex(&buf[position], &hex_value);
     if(read == INCORRECT_START_VALUE)
     {
          printf("There is something wrong at line %d\n", line);
          printf("I can't understand 5.00 value!\n");
          return -1;
     }
     position += read;
     internal_entries[entries_num].value_500 = hex_value;
     if(debug) printf("    5.00 value: 0x%X\n", internal_entries[entries_num].value_500);
     
     position += 1; //omit space
     
     read = getHex(&buf[position], &hex_value);
     if(read == INCORRECT_START_VALUE)
     {
          printf("There is something wrong at line %d\n", line);
          printf("I can't understand 5.50 value!\n");
          return -1;
     }
     position += read;
     internal_entries[entries_num].value_550 = hex_value;
     if(debug) printf("    5.50 value: 0x%X\n", internal_entries[entries_num].value_550);
     
     position += 1; //omit space
     
     read = getHex(&buf[position], &hex_value);
     if(read == INCORRECT_START_VALUE)
     {
          printf("There is something wrong at line %d\n", line);
          printf("I can't understand 6.20 value!\n");
          return -1;
     }
     position += read;
     internal_entries[entries_num].value_620 = hex_value;
     if(debug) printf("    6.20 value: 0x%X\n", internal_entries[entries_num].value_620);
     
     position += 1; //omit space
     */
     
     read = getLabel(&buf[position], &lab_desc);
     if(read == INCORRECT_START_VALUE)
     {
          printf("There is something wrong at line %d\n", line);
          printf("I can't understand label, because there is an syntax error before it.\n");
          return -1;
     }
     else
     if(read == INCORRECT_END_VALUE)
     {
          printf("There is something wrong at line %d\n", line);
          printf("Label is not finished with '\n");
          return -1;
     }
     
     position += read;
     
     MD5Hash(lab_desc.label_buf, lab_desc.label_size, digest);
     memcpy(internal_entries[entries_num].label_digest, digest, 16);
     
     free(lab_desc.label_buf);
     if(debug)
     {
         printf("    label_hash: 0x");
         for(int x = 0; x < 16; x++)
         {
             char dig = internal_entries[entries_num].label_digest[x];
             char hex1 = dig;
             char hex2 = dig >> 4;
             printf("%c%c", checkOneHex(hex1), checkOneHex(hex2));
         }
         printf("\n");
     }
     position += 3; //omit " = "
     read = getText(&buf[position], &txt_desc);
     if(read == INCORRECT_START_VALUE)
     {
          printf("There is something wrong at line %d\n", line);
          printf("I can't understand text, because there is an syntax error before it.\n");
          return -1;
     }
     else
     if(read == INCORRECT_END_VALUE)
     {
          printf("There is something wrong at line %d\n", line);
          printf("Text is not finished with '\n");
          return -1;
     }
     wstrncpy(internal_entries[entries_num].text, txt_desc.text_buf, txt_desc.text_size);
     internal_entries[entries_num].text[txt_desc.text_size] = 0;
     if(debug) wprintf(L"    string: '%s'\n", internal_entries[entries_num].text);
     
     return 0;
}

int ReadFile()
{
    FILE * input;
    input = fopen("strings.txt", "rb");
    wchar_t s;
    
    printf("Step 1 - Create Database from strings.txt\n");
    if(input == NULL) 
    {
        printf("Can't open strings.txt !\n");
        return -1;
    }
    //check if file is unicode, and skip BOM
    fread(&s, sizeof(wchar_t),1,input);
    if(s != 0xFEFF)
    {
         printf("Not UNICODE!\n");
          return -1;
    }
    
    int line = 0, name_size = 0;
    char *rco_name_buf = NULL;
    wchar_t buf[8192];
    memset(buf, 0, sizeof(buf));
    
    bool is_loc_internal = false;
    
    while(fgetws(buf, sizeof(buf), input) != NULL)
    {
        line++;
        if(buf[0] && buf[0] != '#') //omit comment lines
        {
            if(buf[0] != 0xD && buf[1] != 0xA) //omit blank lines
            {
                //parse rco_name
                if(buf[0] == '@')
                {
                    if(is_loc_internal) is_loc_internal = false;
                    if(rco_name_buf != NULL) free(rco_name_buf);
                    int name_size = getLen(&buf[1]);
                    rco_name_buf = utftochar((char*)&buf[1], name_size*2);
                    if(debug) printf("RCO ENTRY %d rco name: '%s'\n", rco_entries_num, rco_name_buf);
                    if(strcmp(rco_name_buf, "localizer_internal") == 0)
                    {
                        is_loc_internal = true;
                    }
                    else
                    {
                        rco_entries_num++;
                    }
                }
                else
                {
                    if(parseLine(line, rco_name_buf, buf) != -1)
                    {
                        if(!is_loc_internal)
                        {
                        text_entries_in_rco_entry[rco_entries_num-1] += 1;
                        entries_num++;
                        }
                    }
                    else
                    {
                        return -1;
                    }
                }
            }
        }
    }
    printf("ENTRIES %d\n", entries_num);
    return 0;
}

LOCHEADER header;

int WriteDatabase()
{
    if(debug) printf("\n\n");
    printf("Step 2 - Write Database to localizer.dat\n");
    //Set magic for header
    header.magic[0] = 0;
    header.magic[1] = 'D';
    header.magic[2] = 'A';
    header.magic[3] = 'T';
    
    header.version = CURRENT_VERSION;
    
    header.inCtfActAs = glob_act_as;
    strcpy(header.site, site);
    
    //Set RCO entries data
    header.n_rco_entries = rco_entries_num;
    header.rco_entries_start = sizeof(LOCHEADER);
    
    //Set TEXT entries data
    header.n_text_entries = entries_num;
    header.text_entries_start = header.rco_entries_start + rco_entries_num * sizeof(RCOENTRY);
    
    
    RCOENTRY *rco_entries = (RCOENTRY*)malloc(rco_entries_num*sizeof(RCOENTRY));
    
    int current_internal_entry = 0;
    int text_entries_pos = 0;
    int i;
    for(i = 0; i < header.n_rco_entries; i++)
    {
        memcpy(rco_entries[i].rco_name_digest, internal_entries[current_internal_entry].rco_name_digest, 16);
        
        rco_entries[i].n_text_entries = text_entries_in_rco_entry[i];
    
        rco_entries[i].text_entries_start_offset = text_entries_pos;
        
        if(debug)
        {
            printf("RCO ENTRY DUMP ");
            printf("rco_name_hash: 0x");
            for(int x = 0; x < 16; x++)
            {
                char dig = internal_entries[i].rco_name_digest[x];
                char hex1 = dig;
                char hex2 = dig >> 4;
                printf("%c%c", checkOneHex(hex1), checkOneHex(hex2));
            }
            printf(":\n");
            printf("  n_text_entries: %d, text_entries_start_offset: %d\n", rco_entries[i].n_text_entries, rco_entries[i].text_entries_start_offset);
        }
        
        current_internal_entry += text_entries_in_rco_entry[i];
        text_entries_pos += rco_entries[i].n_text_entries * sizeof(TEXTENTRY);
    }
    
    TEXTENTRY *text_entries = (TEXTENTRY*)malloc(entries_num * sizeof(TEXTENTRY));
    
    int current_text_pos = 0;
    
    for(i = 0; i < header.n_text_entries; i++)
    {
//          text_entries[i].id_500 = internal_entries[i].value_500;
//          text_entries[i].id_550 = internal_entries[i].value_550;
//          text_entries[i].id_620 = internal_entries[i].value_620;
          memcpy(text_entries[i].label_digest, internal_entries[i].label_digest, 16);
          text_entries[i].text_offset = current_text_pos;
          current_text_pos += (wstrlen(internal_entries[i].text) + 1) * sizeof(wchar_t);
          
          if(debug)
          {
              printf("TEXT ENTRY DUMP");
              printf("label_hash: 0x");
              for(int x = 0; x < 16; x++)
              {
                  char dig = internal_entries[i].label_digest[x];
                  char hex1 = dig;
                  char hex2 = dig >> 4;
                  printf("%c%c", checkOneHex(hex1), checkOneHex(hex2));
              }
              printf(":\n");
              //printf("  id_500: %d, id_550: %d, id_620: %d, text_offset: %d\n", text_entries[i].id_500, text_entries[i].id_550, text_entries[i].id_620, text_entries[i].text_offset);
              printf("  text_offset: %d\n", text_entries[i].text_offset);
          }
    }
    
    //Set texts
    header.text_start = header.text_entries_start + entries_num * sizeof(TEXTENTRY);
    
    //Save
    FILE * output;
    output = fopen("localizer.dat", "wb");
    
    fwrite(&header, sizeof(LOCHEADER), 1, output);
    
    fwrite(rco_entries, sizeof(RCOENTRY), rco_entries_num, output);
    
    fwrite(text_entries, sizeof(TEXTENTRY), entries_num, output);
    
    
    current_internal_entry = 0; 
    
    for(i = 0; i < entries_num; i++)
    {
          fwrite(internal_entries[i].text, (wstrlen(internal_entries[i].text) + 1) * sizeof(wchar_t), 1, output);
    }
    
    free(rco_entries);
    free(text_entries);
    fclose(output);
    return 0;
}

void ParseDb()
{
    FILE * input;
    input = fopen("localizer.dat", "rb");
    
    fseek(input, 0, SEEK_END);
    int size = ftell(input);
    rewind(input);
    
    char* buf = (char*)malloc(size);
    fread(buf, size, 1, input);
    
    LOCHEADER *header = (LOCHEADER*)buf;
    
    printf("Info: rcoentries %d textentries %d\n", header->n_rco_entries, header->n_text_entries);
    
    printf("\nRCO ENTRY INFORMATION\n\n");
    
    RCOENTRY *rco_entries = (RCOENTRY*)(buf + header->rco_entries_start);
    
    for(int i = 0; i < header->n_rco_entries; i++)
    {
            printf("RCO ENTRY ");
            printf("rco_name_hash 0x");
            for(int x = 0; x < 16; x++)
            {
                  char dig = rco_entries[i].rco_name_digest[x];
                  char hex1 = dig;
                  char hex2 = dig >> 4;
                  printf("%c%c", checkOneHex(hex1), checkOneHex(hex2));
            }
            printf(" ");
            printf("text entries %d\n", rco_entries->n_text_entries);
            
            TEXTENTRY *entries = (TEXTENTRY*)(buf + header->text_entries_start + rco_entries->text_entries_start_offset);
            for(int i = 0; i < rco_entries->n_text_entries; i++)
            {
                  //printf("  TEXT ENTRY id500 0x%X id550 0x%X id550 id620 0x%X ", entries[i].id_500, entries[i].id_550, entries[i].id_620);
                  printf("  TEXT ENTRY");
                  printf("label_hash 0x");
                  for(int x = 0; x < 16; x++)
                  {
                      char dig = entries[i].label_digest[x];
                      char hex1 = dig;
                      char hex2 = dig >> 4;
                      printf("%c%c", checkOneHex(hex1), checkOneHex(hex2));
                  }
                  printf(" ");
                  wchar_t *text = (wchar_t*)(buf+ header->text_start + entries[i].text_offset);
                  wprintf(L"text %s\n", text);
            }
    }
    
    printf("\nTEXT ENTRY INFORMATION\n\n");
    
    TEXTENTRY *entries = (TEXTENTRY*)(buf + header->text_entries_start);
    
    for(int i = 0; i < header->n_text_entries; i++)
    {
            //printf("TEXT ENTRY id500 0x%X id550 id0x%X ", entries[i].id_500, entries[i].id_550, entries[i].id_620);
            printf("  TEXT ENTRY");
            printf("label_hash 0x");
            for(int x = 0; x < 16; x++)
            {
                  char dig = entries[i].label_digest[x];
                  char hex1 = dig;
                  char hex2 = dig >> 4;
                  printf("%c%c", checkOneHex(hex1), checkOneHex(hex2));
            }
            printf(" ");
            wchar_t *text = (wchar_t*)(buf+ header->text_start + entries[i].text_offset);
            wprintf(L"text %s\n", text);
    }
    
    free(buf);
}
