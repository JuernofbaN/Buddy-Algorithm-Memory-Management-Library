

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>32768

#include "sbmem.h"

int main()
{
    
    if (sbmem_init(32768) != -1){ 

    	printf ("memory segment is created and initialized \n");
    }
    


    return (0); 
}
