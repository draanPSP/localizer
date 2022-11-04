#ifndef HSTRUCTS
#define HSTRUCTS

#define CURRENT_VERSION 2

typedef struct
{
        char magic[4];
		char version;
		
		short int inCtfActAs;
		char site[64];
        
        //rco struct
        unsigned int n_rco_entries;
        unsigned int rco_entries_start;
        
        //text struct
        unsigned int n_text_entries;
        unsigned int text_entries_start;
        
        //strings
        unsigned int text_start;
} LOCHEADER;

typedef struct
{
        char rco_name_digest[16];
		unsigned int n_text_entries;
        unsigned int text_entries_start_offset;
} RCOENTRY;

typedef struct
{
//		int id_500;
//		int id_550;
//		int id_620;
        char label_digest[16];
        unsigned int text_offset;
} TEXTENTRY;

#endif
