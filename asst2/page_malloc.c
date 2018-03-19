#include "page_malloc.h"

#define PAGE_SIZE 4092
#define NUM_PAGES 8

static char memory[PAGE_SIZE*NUM_PAGES];	//allocated memory
static page_meta_t page_data[NUM_PAGES];	//memory for metadata
struct sigaction act;

void vmem_sig_handler(int signo, siginfo_t *info, void *context) {
	/*	
	void* index;
	int i, swapout, swapin;     

	// finds the beginning of the page that contains address accessed
	index = (void*)((intptr_t)info->si_addr/PAGE_SIZE);

	// finds the struct associated with illegal page accessed
	for(i = 0; i < NUM_PAGES; i++) {
                if(page_data[i].page == index) {
                        swapout = i;
                        break;
                }
        }
	*/
}

void page_malloc_init() {

	// set up signal handler
	act.sa_sigaction = vmem_sig_handler;
        act.sa_flags = SA_SIGINFO;
        sigaction(SIGSEGV, &act, NULL);

	int i;
	for(i = 0; i < NUM_PAGES; i++) {	
		page_data[i].page = (void*)memory + i*PAGE_SIZE;
		page_data[i].next = NULL;
		page_data[i].prev = NULL;
		page_data[i].free = 1;
		page_data[i].TID = -1;
	}
}

void * page_malloc(size_t size, char * file, int line, int request) {

	int pages_needed, pages_remaining, i, pos;
	void* ret = NULL;

	// determine number of pages needed to fulfill request
	if(size % PAGE_SIZE == 0) {
		pages_needed = size/PAGE_SIZE;
	} else {
		pages_needed = (int)(size/PAGE_SIZE) + 1;
	}

	// list of free pages that will possibly be used for allocation
	int free_pages[pages_needed];
	pos = 0;

	// find free pages, and add them to a list
	pages_remaining = pages_needed;
	for(i = 0; i < NUM_PAGES; i++) {
		if(page_data[i].free)	{
			free_pages[pos] = i;
			pos++;
			pages_remaining--;
			if(pages_remaining == 0) {
				break;
			}
		}
	}

	// reached end of the list and pages still needed -> insufficient memory
	if(pages_remaining != 0) {
		errno = ENOMEM;
		return NULL;
	}

	// link up pages
	for(i = 0; i < pages_needed; i++) {
		page_data[ free_pages[i] ].free = 0;

		// foreward links	
		if(i == pages_needed - 1) {	// last page
			page_data[ free_pages[i] ].next = NULL;
		} else {
			page_data[ free_pages[i] ].next = &page_data[ free_pages[i+1] ];	
		} 

		// backward links
		if(i == 0) {	// first page
			page_data[ free_pages[i] ].prev = NULL;
		} else {
			page_data[ free_pages[i] ].prev = &page_data[ free_pages[i-1] ];
		}

	}

	return  page_data[ free_pages[0] ].page;
}

void page_free(void * index, char * file, int line, int request) {

	int i, pos = -1;
	page_meta_t *ptr, *temp;

	for(i = 0; i < NUM_PAGES; i++) {
		if(page_data[i].page == index) {
			pos = i;
			break;
		}
	}

	if(pos < 0) {	// invalid address
		raise(SIGSEGV);
	}

	// reset all metadata structs to free state
	ptr = &page_data[pos];
	while(ptr != NULL) {
		temp = ptr->next;	// save reference to next metadata
		ptr->next = NULL;
		ptr->free = 1;
		ptr->TID = -1;
		ptr = temp;
	}
}

void page_swap(int P1, int P2) {

	// swap space
	char temp_page[PAGE_SIZE];				
	void* temp_ptr;

	// swap pages
	memcpy(temp_page, page_data[P1].page, PAGE_SIZE);		// temp <- P1
	memcpy(page_data[P1].page, page_data[P2].page, PAGE_SIZE); 	// P1 <- P2
	memcpy(page_data[P2].page, temp_page, PAGE_SIZE);		// P2 <- temp (<- P1)
	
	// swap page links
	temp_ptr = page_data[P1].page;				
	page_data[P1].page = page_data[P2].page;
	page_data[P2].page = temp_ptr; 

	return;	
}

int main(int argc, char** argv) {

	page_malloc_init();
	page_data[0].free = 0; 
	page_data[2].free = 0;
	page_data[4].free = 0;
	
	void* ptr = page_malloc(PAGE_SIZE*3, __FILE__, __LINE__, 0);	
	if(ptr == NULL) {
		perror("ERROR");
	}

	page_swap(1, 4);

	page_free(ptr, __FILE__, __LINE__, 0);
	
	return 0;
}
