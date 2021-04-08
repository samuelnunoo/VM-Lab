#include <stdlib.h>
#include <stdio.h>

typedef int physical_addr;
typedef int virtual_addr;
typedef char byte;
enum strategies{RANDOM, LRU};
typedef struct page_table_entry{
  int frame_number;
  int valid;
  long lru_counter;
} page_table_entry_t;



// global constants
// Note: I recommend testing with 16, 64, 128
//       final evaluation with 4096, 32768, 1048576  
const int PAGE_SIZE = 4096; // 4 KB = 2^12 bytes
const int PHYSICAL_MEMORY_SIZE = 32768; // 32 KB = 2^15 bytes 
const int VIRTUAL_MEMORY_SIZE = 1048576; // 1 MB = 2^20 bytes

int INT_SIZE;
int INTS_PER_PAGE;
int NUM_FRAMES;
int NUM_PAGES;

// global variables
page_table_entry_t * page_table;
byte * physical_memory;
long num_faults;
long num_accesses;
int strategy;


int * gen_array(int n){
  // allocate space for array
  int * array = (int *) malloc(n*sizeof(int));

  // initialize array
  int i;
  for(i = 0; i < n; i++){
    *(array + i) = rand() % 100000;
  }

  // return pointer to beginning of array
  return array;
}

void print_simulation_state(){
  int i,j;

  // print page table
  printf("\npage table: [");
  for(i = 0; i < NUM_PAGES; i++){
    page_table_entry_t * entry_ptr = page_table + i;
    printf(" %d(%d:%ld),",entry_ptr->frame_number,entry_ptr->valid,entry_ptr->lru_counter);
  }
  printf("]\n");

  // print physical memory
  printf("num_frames = %d, ints_per_page= %d\n",NUM_FRAMES,INTS_PER_PAGE);
  printf("physical memory: [");
  for(i = 0; i < NUM_FRAMES; i++){
    for(j = 0; j < INTS_PER_PAGE; j++){
      printf(" %d,",*((int *) (physical_memory+i*PAGE_SIZE+j*INT_SIZE)));
    }
  }
  printf("]\n\n");
}


void initialize(int * data, int n){
  int i,page_num;

  // initialize physical frames
  int num_pages_n = (n*INT_SIZE+PAGE_SIZE-1)/PAGE_SIZE;
  for(page_num = 0; page_num < NUM_FRAMES; page_num++){

    // initialize physical frame from data array
    for(i=0; i < INTS_PER_PAGE; i++){
      int val = *(data+page_num*INTS_PER_PAGE+i);
      *((int *) (physical_memory+page_num*PAGE_SIZE+i*INT_SIZE)) = val;
    }
    
    // initialize page table entry
    page_table_entry_t * entry_ptr = page_table + page_num;
    if(page_num < num_pages_n){
      // add valid page table entry
      entry_ptr->frame_number = page_num;
      entry_ptr->valid = 1;

    } else {
      // add invalid page table entry
      entry_ptr->valid = 0;
    }

  }

  // initialize paged-out virtual pages in files
  for(page_num = NUM_FRAMES; page_num < NUM_PAGES; page_num++){

    // add invalid page table entry
    page_table_entry_t * entry_ptr = page_table + page_num;
    entry_ptr->valid = 0;

    if(page_num < num_pages_n){
      // write virtual page to file
      char filename[42];
      sprintf(filename,"files/%030d.pg",page_num);
      FILE * file_ptr = fopen(filename, "w");    
      int read = fwrite(((const void *) data)+page_num*PAGE_SIZE, PAGE_SIZE, 1,
			file_ptr);
      fclose(file_ptr);

    }

  }

}


physical_addr translate_addr(virtual_addr va){
  // TODO #1: implement this function
  return 0;
}

void handle_page_fault(virtual_addr va){
  int i;

  // select page to evict
  int evicted_page;
  page_table_entry_t * evicted_entry_ptr = NULL;
  if(strategy == RANDOM){
    do{
      evicted_page = rand() % NUM_PAGES; 
      evicted_entry_ptr = page_table + evicted_page;
    } while(evicted_entry_ptr->valid == 0);
    
  } else if (strategy == LRU){
    // TODO #2: implement this page eviction strategy
    
  } else {
    printf("INVALID PAGE REPLACEMENT STRATEGY\n");
    exit(1);
  }
  int frame_number = evicted_entry_ptr->frame_number;

  // write evicted page to disk
  char filename[42];
  sprintf(filename,"files/%030d.pg", evicted_page);
  FILE * file_ptr = fopen(filename, "w");
  int read = fwrite(physical_memory+frame_number*PAGE_SIZE, PAGE_SIZE, 1,
		    file_ptr);
  fclose(file_ptr);

  // load new page into physical frame
  int page_number = va/PAGE_SIZE;
  page_table_entry_t * curr_entry_ptr = page_table + page_number;
  sprintf(filename,"files/%030d.pg", page_number);
  file_ptr = fopen(filename, "r");
  read = fread(physical_memory+frame_number*PAGE_SIZE, PAGE_SIZE, 1,
	       file_ptr);
  fclose(file_ptr);
    
  // update page table
  evicted_entry_ptr->valid = 0;
  curr_entry_ptr->frame_number = frame_number;
  curr_entry_ptr->valid = 1;
    
}

int load(virtual_addr va){
  num_accesses++;

  physical_addr pa= translate_addr(va);
  if(pa == -1) { // page fault
    num_faults++;

    handle_page_fault(va);
    pa = translate_addr(va);
  }

  // load value from physical memory
  return *((int *) (physical_memory+pa));
}

void store(virtual_addr va, int val){
  num_accesses++;

  physical_addr pa= translate_addr(va);
  if(pa == -1) { // page fault
    num_faults++;

    handle_page_fault(va);
    pa = translate_addr(va);
  }

  // store value in physical memory
  *((int *) (physical_memory+pa)) = val;  
}

void print_array(int n){
  int i;
  for(i=0;i < n; i++){
    virtual_addr va = INT_SIZE*i;
    int val = load(va);
    printf("%d, ",val);
  }
  printf("\n");
}

void swap(int va, int vb){
  int temp = load(va);
  store(va,load(vb));
  store(vb, temp);
}


void selection_sort(int n){
  int i,j, min_pos;
  int int_size = (int) sizeof(int);
  int array_size = n * int_size;

  // iterate through positions in the array
  for(i = 0; i < array_size; i=i+int_size){

    // find int that belongs in that position
    min_pos = i;
    for(j = i; j < array_size; j=j+int_size){
      if(load(j) < load(min_pos)){
	min_pos = j;
      }
    }

    // swap that int into current position
    swap(i, min_pos);
  }
}


void insertion_sort(int n){
  int i, curr_pos;
  int int_size = (int) sizeof(int);
  int array_size = n * int_size;

  // iterate through positions in the array
  for(i = 0; i < array_size; i+= int_size){

    // swap int in that position into the right place
    curr_pos = i;
    while(curr_pos > 0 && load(curr_pos-int_size) > load(curr_pos)){
      swap(curr_pos-int_size, curr_pos);
      curr_pos -= int_size;
    }
  }
}

int parent(int i){
  return (i-1)/2;
}

int leftChild(int i){
  return 2*i + 1;
}

int rightChild(int i){
  return 2*i + 2;
}

int biggestChild(int i, int n){
  int c1 = leftChild(i);
  int c2 = rightChild(i);
  if(c2 > n || load(c1*INT_SIZE) > load(c2*INT_SIZE)){
    return c1;
  } else {
    return c2;
  }
}

void heapify(int n){
  int i = 1;
  while(i < n){ // while more elements to add to heap
    int curr = i;
    int p = parent(curr);
    while(curr > 0 && load(curr*INT_SIZE) > load(p*INT_SIZE)){
      swap(curr*INT_SIZE,p*INT_SIZE);
      curr = p;
      p = parent(curr);
    }
    
    i = i+1;
  }
}

void heap_sort(int n){
  heapify(n);

  n = n-1;
  while(n > 0){
    // move root of heap to end
    swap(0,n*INT_SIZE);

    // treat the beginning as a smaller heap and fix the heap
    n--;
    int curr = 0;
    int child = biggestChild(curr,n);

    // while curr has a bigger child, swap with bigger child
    while(child <= n && load(curr*INT_SIZE) < load(child*INT_SIZE)){
      swap(curr*INT_SIZE, biggestChild(curr,n)*INT_SIZE);

      curr = child;
      child = biggestChild(curr, n);
    }
  }
}


int partition(int start, int end){
  int pivot;
  int v1 = load(start*INT_SIZE);
  int v2 = load(((end-start)/2)*INT_SIZE);
  int v3 = load(end*INT_SIZE);
  if((v2 >= v1 && v1 >= v3) || (v2 <= v1 && v1 <= v3)){
    pivot = v1;
  } else if((v1 >= v2 && v2 >= v3) || (v1 <= v2 && v2 <= v3)){
    pivot = v2;
    swap(start*INT_SIZE, (end-start)/2*INT_SIZE);
  } else {
    pivot = v3;
    swap(start*INT_SIZE,end*INT_SIZE);
  }
  
  int i = start;
  int j = end;
  while(i < j){ // loop invariant: < i => < pivot, i = pivot, > t => > pivot 
    int temp1 = load((i+1)*INT_SIZE);
    int temp2 = load(i*INT_SIZE);
    if(load((i+1)*INT_SIZE) <= load(i*INT_SIZE)){
      swap((i+1)*INT_SIZE, i*INT_SIZE); // move before pivot
      i++;
    } else {
      swap((i+1)*INT_SIZE, j*INT_SIZE); // move after pivot
      j--;
    }
  }

  return i;
}

void quick_sort(int start, int end){
  if(start < end){
    int middle = partition(start, end);
    quick_sort(start, middle-1);
    quick_sort(middle+1, end);
  }
}


void evaluate(){

  int max_ints= (int) VIRTUAL_MEMORY_SIZE/INT_SIZE;

  // generate array of random numbers
  int * data = gen_array(max_ints);


  int len;
  for(len = 2; len <= max_ints; len*=2){

    strategy = RANDOM;
    initialize(data, len);
    num_faults = 0;
    num_accesses = 0;
    heap_sort(len);
    long heap_misses = num_faults;
    long heap_accesses = num_accesses;

    strategy = LRU;
    initialize(data, len);
    num_faults = 0;
    num_accesses = 0;
    heap_sort(len);
    long heap_misses_lru = num_faults;
    long heap_accesses_lru = num_accesses;

    strategy = RANDOM;
    initialize(data, len);
    num_faults = 0;
    num_accesses = 0;
    quick_sort(0,len-1);
    long quick_misses = num_faults;
    long quick_accesses = num_accesses;

    strategy = LRU;
    initialize(data, len);
    num_faults = 0;
    num_accesses = 0;
    quick_sort(0,len-1); 
    long quick_misses_lru = num_faults;
    long quick_accesses_lru = num_accesses;
    
    printf("%d,%ld,%ld,%f,%ld,%ld,%f,%ld,%ld,%f,%ld,%ld,%f\n", len,
    	   heap_misses, heap_accesses, (double) heap_misses/heap_accesses,
    	   heap_misses_lru, heap_accesses_lru, (double) heap_misses_lru/heap_accesses_lru,
    	   quick_misses, quick_accesses, (double) quick_misses/quick_accesses,
    	   quick_misses_lru, quick_accesses_lru, (double) quick_misses_lru/quick_accesses_lru);

  }

  free(data);

}

int main(int argc, char ** argv){
  // initialize simulation constants
  INT_SIZE = (int) sizeof(int);
  INTS_PER_PAGE = (int) PAGE_SIZE/INT_SIZE;
  NUM_FRAMES = (int) (PHYSICAL_MEMORY_SIZE/PAGE_SIZE); 
  NUM_PAGES = (int) (VIRTUAL_MEMORY_SIZE/PAGE_SIZE); 

  // initialize simulation global variables
  page_table = (page_table_entry_t *) malloc(NUM_PAGES*sizeof(page_table_entry_t));
  physical_memory = (byte *) malloc(PHYSICAL_MEMORY_SIZE);

  evaluate();

  free(page_table);
  free(physical_memory);

}
