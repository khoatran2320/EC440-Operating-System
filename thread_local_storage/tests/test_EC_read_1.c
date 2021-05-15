#include "tls.h"

//################################################################
//
//		helper functions here, tls_* implementations below
//
//################################################################

static bool is_fault_in_tls_space(unsigned long page_fault){
	//loop through all the pages int TLS and check to see if the page fault is in a TLS region
	for(int i = 0; i < MAX_THREAD_COUNT; ++i){
		if(tid_tls_arr[i] != 0){
			for(int j = 0; j < tid_tls_arr[i]->tls->num_pages; ++j){
				if(tid_tls_arr[i]->tls->pages[j]->address == page_fault){
					return true;
				}
			}
		}
	}
	return false;
}

void tls_handle_page_fault(int sig, siginfo_t *si, void*context)
{
	//address of page
	unsigned long page_fault = ((unsigned long)si->si_addr) & ~(page_size - 1);

	if(is_fault_in_tls_space(page_fault)){
		pthread_exit(NULL);
	}
	signal(SIGBUS, SIG_DFL);
	signal(SIGSEGV, SIG_DFL);
	raise(sig);
}

static int tls_protect(struct page *page)
{
	if(pthread_mutex_unlock(&page->mutex) != 0){
		return -1;
	}
	return mprotect((void *)page->address, page_size, PROT_NONE);
}

static int tls_unprotect(struct page *page)
{
	if(pthread_mutex_lock(&page->mutex) != 0){
		return -1;
	}
	return mprotect((void *)page->address, page_size, (PROT_READ | PROT_WRITE));
	
}

static int tls_init()
{
	struct sigaction sigact;
	page_size = getpagesize();

	if(sigemptyset(&sigact.sa_mask) == -1){
		return -1;
	}
	sigact.sa_flags = SA_SIGINFO;
	sigact.sa_sigaction = tls_handle_page_fault;
	if(sigaction(SIGBUS, &sigact, NULL) == -1){
		return -1;
	}
	if(sigaction(SIGSEGV, &sigact, NULL) == -1){
		return -1;
	}

	return 0;
}
//append thread id - tls pair to global array
static int append_to_arr(tid_tls *tt)
{
	for(int i = 0; i < MAX_THREAD_COUNT;++i){
		if(tid_tls_arr[i] == 0){
			tid_tls_arr[i] = tt;
			return 0;
		}
	}
	return -1;
}


static TLS *get_tls(pthread_t tid)
{
	for(int i = 0; i < MAX_THREAD_COUNT; ++i){
		if(tid_tls_arr[i] != 0){
			if(tid_tls_arr[i]->tid == tid){
				return tid_tls_arr[i]->tls;
			}
		}
	} 
	return NULL;
}

static int create_new_tls(unsigned int size, bool is_clone, pthread_t clone_tid)
{
	//create tls struct
	TLS *tls = (TLS *)malloc(sizeof(TLS));
	if(tls == NULL){
		return -1;
	}

	TLS *tls_to_clone;
	if(is_clone){
		tls_to_clone = get_tls(clone_tid);
		if(tls_to_clone == NULL){
			return -1;
		}
	}

	//set tls info
	tls->tid = pthread_self();
	
	if(is_clone){
		tls->size = tls_to_clone->size;
		tls->num_pages = tls_to_clone->num_pages;
	}
	else{
		tls->size = size;
		tls->num_pages = size < page_size ? 1 : (((size % page_size) == 0) ? size/page_size : size/page_size + 1);
	}

	//create array of pages
	struct page **pp = (struct page**)calloc(tls->num_pages, sizeof(struct page*));
	if(pp == NULL){
		return -1;
	}

	//set each page
	for(int i = 0; i < tls->num_pages; ++i){
		if(is_clone){
			(*(tls_to_clone->pages+i))->ref_count++;
			*(pp+i) = *(tls_to_clone->pages+i);
		}
		else{
			struct page *p = (struct page*)malloc(sizeof(struct page));
			if(p == NULL){
				return -1;
			}
			p->address = (unsigned long)mmap(0, page_size, PROT_NONE, MAP_ANON | MAP_PRIVATE, 0, 0);
			p->ref_count = 1;
			if(pthread_mutex_init(&p->mutex, NULL) != 0){
				return -1;
			}
			*(pp+i) = p;
		}
	}

	tls->pages = pp;
	//create new tid - tls pair and append to array
	tid_tls *tt = (tid_tls *)malloc(sizeof(tid_tls));
	if(tt == NULL){
		return -1;
	}
	tt->tid = tls->tid;
	tt->tls = tls;

	return append_to_arr(tt);
}


//loop through and check to see if the thread has an associated tls
static bool check_tid_has_lsa(pthread_t tid)
{
	for(int i = 0; i < MAX_THREAD_COUNT;++i){
		if(tid_tls_arr[i] != 0){
			if(tid_tls_arr[i]->tid == tid){
				return true;
			}
		}
	}
	return false;
}


static int copy_page(TLS *tls, struct page *page, unsigned page_index){

	//create new page copy
	struct page *copy = (struct page *)malloc(sizeof(struct page));
	copy->address = (unsigned long)mmap(0, page_size, PROT_NONE, MAP_ANON | MAP_PRIVATE, 0, 0);
	copy->ref_count = 1;
	if(pthread_mutex_init(&copy->mutex, NULL) != 0){
		return -1;
	}

	//copy contents of page over to the new copy
	if(tls_unprotect(page) == -1){
		return -1;
	}
	if(tls_unprotect(copy) == -1){
		return -1;
	}
	memcpy((void*)copy->address, (void*)page->address, page_size);
	if(tls_protect(page) == -1){
		return -1;
	}
	if(tls_protect(copy) == -1){
		return -1;
	}
	//decrement the reference count on the page
	page->ref_count--;

	//replace page in the tls
	tls->pages[page_index] = copy;
	return 0;

}


//#################################################
//
//		tls_* implementations
//
//#################################################


///// Creates new LSA with size of size
int tls_create(unsigned int size)
{
	//error checking
	if (size <= 0)
	{
		return -1;
	}
	if(check_tid_has_lsa(pthread_self()) == true){
		return -1;
	}

	//init once
	static bool is_first_time = true;
	if(is_first_time){
		is_first_time = false;
		if(tls_init() == -1){
			return -1;
		}
	}

	//create new tls
	return create_new_tls(size, false, 0);
}

///// Destroys LSA /////
int tls_destroy()
{
	pthread_t tid = pthread_self();
	tid_tls * tt = NULL;

	int i;

	//find tid-tls pair
	for(i = 0; i < MAX_THREAD_COUNT; ++i){
		if(tid_tls_arr[i] != 0){
			if(tid_tls_arr[i]->tid == tid){
				tt = tid_tls_arr[i];
			}
		}
	}
	if(tt == NULL){
		return -1;
	}

	for(int j = 0; j < tt->tls->num_pages; ++j){

		//release mapped space
		if(tt->tls->pages[j]->ref_count < 2){
			if(munmap((void *)(tt->tls->pages[j]->address), page_size) == -1){
				return -1;
			}
			pthread_mutex_destroy(&tt->tls->pages[j]->mutex);
			//free allocated memory for page pointer
			free(tt->tls->pages[j]);
		}
		else{
			tt->tls->pages[j]->ref_count--;
		}
		
	}

	//free allocated memory for list of page pointers
	free(tt->tls->pages);

	//free tls
	free(tt->tls);

	//free tid-tls pair
	free(tt);

	//remove tid-tls pair from global array
	tid_tls_arr[i] = 0;

	return 0;
}

///// Reads length data from LSA onto buffer starting at offset /////
int tls_read(unsigned int offset, unsigned int length, char *buffer)
{	
	page_size = getpagesize();
	TLS *tls = get_tls(pthread_self());
	if(tls == NULL){
		return -1;
	}
	if(offset + length > tls->size){
		return -1;
	}
	unsigned int start_page = offset / page_size;
	unsigned int start_page_offset = offset % page_size;
	
	unsigned int read = 0;
	unsigned int page_index = 0;
	struct page *curr_page = tls->pages[start_page];
	unsigned int bytes_avail_on_page = page_size - start_page_offset;

	//read the beginning segment
	if(tls_unprotect(curr_page) == -1){
		return -1;
	}
	if(length > bytes_avail_on_page){
		memcpy(buffer, (void *)(curr_page->address + start_page_offset), bytes_avail_on_page);
		if(tls_protect(curr_page) == -1){
			return -1;
		}
		page_index++;
		read += bytes_avail_on_page;
	}
	else{
		memcpy(buffer, (void*)(curr_page->address+start_page_offset), length);
		if(tls_protect(curr_page) == -1){
			return -1;
		}
		return 0;
	}

	while(read < length){
		curr_page = tls->pages[start_page + page_index];
		bytes_avail_on_page = page_size;
		
		if((length - read) > bytes_avail_on_page){

			//read the full page
			if(tls_unprotect(curr_page) == -1){
				return -1;
			}

			memcpy(buffer + read, (void *)curr_page->address, bytes_avail_on_page);

			if(tls_protect(curr_page) == -1){
				return -1;
			}
			page_index++;
			read += bytes_avail_on_page;
		}
		else{

			//read the last segment
			if(tls_unprotect(curr_page) == -1){
				return -1;
			}

			memcpy(buffer + read, (void *)curr_page->address, length-read);

			if(tls_protect(curr_page) == -1){
				return -1;
			}
			break;
		}
	}
	return 0;
}


int tls_write(unsigned int offset, unsigned int length, const char *buffer)
{
	TLS *tls = get_tls(pthread_self());
	if(tls == NULL){
		return -1;
	}
	if(offset + length > tls->size){
		return -1;
	}
	unsigned int start_page = offset / page_size;
	unsigned int start_page_offset = offset % page_size;
	
	unsigned int written = 0;
	unsigned int page_index = 0;
	struct page *curr_page = tls->pages[start_page];
	unsigned int curr_avail_space = page_size - start_page_offset;

	//write the beginning segment
	if(curr_page->ref_count > 1){
		if(copy_page(tls, curr_page, start_page) == -1){
			return -1;
		}
		curr_page = tls->pages[start_page];
	}

	if(tls_unprotect(curr_page) == -1){
		return -1;
	}
	if(length > curr_avail_space){
		memcpy((void *)(curr_page->address + start_page_offset), buffer, curr_avail_space);
		if(tls_protect(curr_page) == -1){
			return -1;
		}
		page_index++;
		written += curr_avail_space;
	}
	else{
		memcpy((void*)(curr_page->address+start_page_offset), buffer, length);
		if(tls_protect(curr_page) == -1){
			return -1;
		}
		return 0;
	}

	while(written < length){
		curr_page = tls->pages[start_page + page_index];

		//copy the page if it is a shared page
		if(curr_page->ref_count > 1){
			if(copy_page(tls, curr_page, start_page + page_index) == -1){
				return -1;
			}
			curr_page = tls->pages[start_page + page_index];
		}
		
		curr_avail_space = page_size;
		
		if((length - written) > curr_avail_space){

			//write the full page
			if(tls_unprotect(curr_page) == -1){
				return -1;
			}

			memcpy((void *)curr_page->address, buffer+written, curr_avail_space);

			if(tls_protect(curr_page) == -1){
				return -1;
			}
			page_index++;
			written += curr_avail_space;
		}
		else{

			//write the last segment
			if(tls_unprotect(curr_page) == -1){
				return -1;
			}

			memcpy((void *)curr_page->address, buffer+written, length-written);

			if(tls_protect(curr_page) == -1){
				return -1;
			}
			break;
		}
	}

	return 0;
}

int tls_clone(pthread_t tid)
{
	if(check_tid_has_lsa(pthread_self()) == true){
		return -1;
	}
	if(check_tid_has_lsa(tid) == false){
		return -1;
	}
	
	return create_new_tls(0, true, tid);
}
