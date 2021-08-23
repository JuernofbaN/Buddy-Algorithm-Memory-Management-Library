#include <stdlib.h>
#include "sbmem.h"

//name of the shared memory
#define MEMNAME "/share memory"

//semaphore name
sem_t mutex;

//Struct for linklist
struct Node {
	void*  start_pos;
	void* end_pos;
	int  proc_id;
	struct Node* next;
};

//start
void* start_pos;

struct Node* head;
void showNode();
int segSize;

/*
	allocation of the shared memory
	by the specific size which is power two
*/
int sbmem_init (int segsize){
	
	//wheather or not it is power 2
	if ((segsize != 0) && ((segsize & (segsize - 1)) == 0)){
	
	
	
		//-----------------------------------------should be delete before submition--------------------------------
		shm_unlink(MEMNAME);
		//-----------------------------------------should be delete before submition--------------------------------
		
		//shm_open function
		int fd = shm_open(MEMNAME, O_CREAT | O_EXCL | O_RDWR, S_IRUSR | S_IWUSR);
				
		//error for shm_open function
		if (fd == -1) {
			perror("shm_open");
			return -1;
		}
					
		//function of ftruncate and if it has any problem
		if (ftruncate(fd, segsize) == -1){
			perror("ftruncate");
			return -1;
		}
		
		// mmap the memory to the 
		start_pos = mmap(NULL, segsize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
					
		//if it faced witth a error
		if (start_pos == MAP_FAILED) {
			perror("mmap");
			return -1;
		}

		//store the information of the memory in the memory
		memcpy(start_pos,  &segsize, sizeof(int));
		
		return 0;
	}
	else
		return -1;

}

/*
	removing the shared memory which 
	is created
*/
sbmem_remove (){
	
	//remove semaphore
	sem_destroy(&mutex);
	
	//remove shared memory
	shm_unlink(MEMNAME);
	
	return (0); 
}

/*
open the shared memory based on the size
which is pecified by the initializiation
of the shared memory
*/
int sbmem_open() {
	
	//shm_open function read &write
	int fd = shm_open(MEMNAME, O_RDWR, S_IRUSR | S_IWUSR);
	
	//error for shm_open function
	if (fd == -1) {
		perror("shm_open");
		return -1;
	}
	
	// mmap the memory into the minimum size 
	start_pos = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

	//if mmap function faced witth a error
	if (start_pos == MAP_FAILED) {
		perror("mmap");
		return -1;
	}

	//getting the memory information
	memcpy(&segSize, start_pos,  sizeof(int));
	
	//unmap the memory in order to map a acurate segmentation size 
	munmap(start_pos, sizeof(int));
	
	// mmap the memory into the correct Segmentation size
	start_pos = mmap(NULL, segSize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

    	//if mmap function faced witth a error
	if (start_pos == MAP_FAILED) {
		perror("mmap");
		return -1;
	}
	
	fd = sem_init(&mutex, 1, 1);
	if (fd == -1) {
		perror("sem_init");
		return -1;
	}
	
    	return (0); 
}

/*
allocated share memory based on the size 
which is specified by the parameter
*/
void *sbmem_alloc (int size)
{
    // lock the process
    sem_wait(&mutex);
    
    void* realStartPos = (start_pos + sizeof (segSize)) + sizeof (struct Node);
    
    void* headAdr = start_pos + sizeof (segSize);
    
    head = headAdr;

    int spaceToAllocate = 1;
    while (spaceToAllocate < (size + sizeof (struct Node))){
        spaceToAllocate = spaceToAllocate * 2;
    }
    
    //if there was not any node in the linklist
    if(head->start_pos == 0){
    	
    	//address of head in shared memory
    	head = start_pos + sizeof(int);
    	
        //First Allocation
        head->start_pos = realStartPos ;
        head->end_pos = head->start_pos + spaceToAllocate;
        head->next = NULL;
        head->proc_id = getpid();
        
        //Store First Node in Memory
        memcpy(realStartPos, head, sizeof (struct Node));
        memcpy(headAdr, head, sizeof (struct Node));
        
        //releae the singal        
        sem_post(&mutex);
        
        return head->start_pos + sizeof (struct Node);
        
    }else{
    
    	// if there was the place in the bigining of the memory for allocation
        if (head->start_pos - realStartPos >= spaceToAllocate) {
            
            
             struct Node nwNode;
             
             //allocation of the Node
             nwNode.next = head;
             nwNode.start_pos = realStartPos ;
             head = nwNode.start_pos;
             nwNode.end_pos = nwNode.start_pos + spaceToAllocate;
             nwNode.proc_id = getpid();
             
             //store in the shered memory
             memcpy(headAdr, (const)head, sizeof (struct Node));
             
             //releae the singal          
             sem_post(&mutex);
             
             return nwNode.start_pos + sizeof (struct Node);
        }
        struct Node* cur = head;
        while(cur->next != NULL){
            
            //if there was place between the nodes 
            int avSpace = ((cur->next)->start_pos) - ((cur->end_pos));
            if( avSpace >= spaceToAllocate){
            
                struct Node nwNode;
                
                //allocation of the Node
                nwNode.next = cur->next;
                nwNode.start_pos = cur->end_pos;
                cur->next = nwNode.start_pos;
                nwNode.end_pos = nwNode.start_pos + spaceToAllocate;
                nwNode.proc_id = getpid();
                
                //store in the shered memory
                memcpy(nwNode.start_pos, &nwNode, sizeof (struct Node));
                
		//releae the singal		
		sem_post(&mutex);
		
                return nwNode.start_pos + sizeof (struct Node);
            }
            cur = cur->next;

        }
        
        //Last Node
        cur = head;
        while(cur->next != NULL){
            cur = cur->next;
        }
        
        if(spaceToAllocate <= (int)(((int)realStartPos+ segSize) - (int)cur->end_pos)){

            struct Node nwNode;
	    
	    //allocation of the Node
            nwNode.next = NULL;
            nwNode.start_pos = cur->end_pos;
            cur->next = nwNode.start_pos;
            nwNode.end_pos = nwNode.start_pos + spaceToAllocate;
	    nwNode.proc_id = getpid();
	    
	    //store in the shered memory
            memcpy(nwNode.start_pos, &nwNode, sizeof (struct Node));
            
            //releae the singal
            sem_post(&mutex);
            
            return nwNode.start_pos + sizeof (struct Node);
        }

    }
    return (NULL);
}

/*
Remove NOde from the linkList
and upldate the memory share 
and head
*/
void sbmem_free (void *p) {

	sem_wait(&mutex);
	
	void* headAdr = (start_pos + sizeof (segSize));
        head = headAdr;

        if(head->next == NULL){
            head = NULL;
            
            memcpy(headAdr, &head, sizeof (struct Node));
            return;
        }
	if (head != NULL){
    		
		void* realStartPos = start_pos + (sizeof (segSize));
		    
		void* pNode = p - sizeof (struct Node);
		struct Node* cur = pNode;
		
		struct Node* traveller = head;
		
		if(cur->start_pos == head->start_pos){
		
			//DELETED HEAD
            		head = head->next;
            		
			//store in the shered memory
			memcpy(headAdr, head, sizeof (struct Node));
		}else{

			while(traveller->next != cur){
				traveller = traveller->next;
			}
			traveller->next = cur->next;
			cur->start_pos = NULL;
			cur->end_pos = NULL;
			cur->proc_id = NULL;
			cur = NULL;
			
		}
		
	}		
     	else
     		printf("head is empty");
    	sem_post(&mutex);
}


int sbmem_close()
{
    munmap(start_pos,segSize);
    return (0); 
}

