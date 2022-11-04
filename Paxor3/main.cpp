#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

bool debug = false;  
 
#include "parse.h"

int main () 
{
    printf("Paxor3 - database utility for Localizer\n");
    printf("Created by Draan, 2011 - all rights reserved\n\n");
    if(ReadFile() != -1)
    {
        WriteDatabase();
        printf("Done.\n");
        if(debug) ParseDb();
    }
    else
    {
        printf("There was an error. Operation aborted.\n");
    }
    system("PAUSE");
    return 0;
}
