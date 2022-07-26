
#include <common/types.h>
#include <gdt.h>
#include <memorymanagement.h>
#include <hardwarecommunication/interrupts.h>
#include <syscalls.h>
#include <hardwarecommunication/pci.h>
#include <drivers/driver.h>
#include <drivers/keyboard.h>
#include <drivers/mouse.h>
#include <drivers/vga.h>
#include <drivers/ata.h>
#include <gui/desktop.h>
#include <gui/window.h>
#include <multitasking.h>

#include <drivers/amd_am79c973.h>
#include <net/etherframe.h>
#include <net/arp.h>
#include <net/ipv4.h>
#include <net/icmp.h>
#include <net/udp.h>
#include <net/tcp.h>


// #define GRAPHICSMODE


using namespace myos;
using namespace myos::common;
using namespace myos::drivers;
using namespace myos::hardwarecommunication;
using namespace myos::gui;
using namespace myos::net;




void printf(char* str)
{
    static uint16_t* VideoMemory = (uint16_t*)0xb8000;

    static uint8_t x=0,y=0;

    for(int i = 0; str[i] != '\0'; ++i)
    {
        switch(str[i])
        {
            case '\n':
                x = 0;
                y++;
                break;
            default:
                VideoMemory[80*y+x] = (VideoMemory[80*y+x] & 0xFF00) | str[i];
                x++;
                break;
        }

        if(x >= 80)
        {
            x = 0;
            y++;
        }

        if(y >= 25)
        {
            for(y = 0; y < 25; y++)
                for(x = 0; x < 80; x++)
                    VideoMemory[80*y+x] = (VideoMemory[80*y+x] & 0xFF00) | ' ';
            x = 0;
            y = 0;
        }
    }
}

void printfHex(uint8_t key)
{
    char* foo = "00";
    char* hex = "0123456789ABCDEF";
    foo[0] = hex[(key >> 4) & 0xF];
    foo[1] = hex[key & 0xF];
    printf(foo);
}
void printfHex16(uint16_t key)
{
    printfHex((key >> 8) & 0xFF);
    printfHex( key & 0xFF);
}
void printfHex32(uint32_t key)
{
    printfHex((key >> 24) & 0xFF);
    printfHex((key >> 16) & 0xFF);
    printfHex((key >> 8) & 0xFF);
    printfHex( key & 0xFF);
}
















/**
 * 
 * HOMEWORK HEADERS, DEFINITIONS AND SIMILAR!
 * 
 * */


// #define N 10000    //moves to interrupts.cpp
#define MAIN 10
#define PAGE 10

int disk[N];

int memory[MAIN];
int memory_count=0;//tracks amount of free memory, since there is no other memory management in this system.

// typedef enum{ false = 0, true = 1 } bool;

typedef enum{FIFO, SC_FIFO, LRU}PageReplacementAlgorithm;
int lru_counter = 0;

// typedef struct TableEntry{
//     int page_frame_number;//needed to access 
//     bool valid;
//     bool R;
//     bool M;
//     int counter;//needed for LRU, -1 indicates, LRU not used
// }TableEntry;

TableEntry pageTable[N];

//necessary for fifo structure
int fifo_queue[MAIN];//queue implementatino with array, tracks pages in main memory
int fifo_queue_count = 0;//0 elements in the begining
int removeFirst();
int removeAtPos(int index);
void addNewElement(int el);

PageReplacementAlgorithm pgrAlgo = FIFO;//set default page replacement to fifo, can be changed to any other

//task/process related
void executeTaskSorting(int* arr, int n ,void (*sorting_algorithm)(int*, int), PageReplacementAlgorithm prAlgo);
void executeTaskSortingWithFactor( void (*sorting_algorithm)(int*, int), int factor, PageReplacementAlgorithm prAlgo);//creates a random array with respect to the size of physical memory

int n_hits = 0;//The number of hits 
int clock_count = 0;

int hit_rate_pcc = 0;//hit rate per clock cycle
int n_hits_cycle = 0 ;

int n_misses = 0;//the number of misses
int miss_rate_pcc = 0;
int n_misses_cycle = 0 ;//The number of misses and miss rate per second

int n_page_loaded = 0;//The number of pages loaded
int n_page_written = 0;//The number of pages written back to the disk
void initAdditionalData();//sets all additional information to 0


//virtual memory related
void setUpPageTable(int* arr, int n);
void createPageTable(TableEntry* pageTable, int n);
TableEntry newPageTableEntry();
void writeToDisk(int* arr, int n);
void writeToDisk(int el, int index);//writes an integer to disk
void* getFromMemory(int virtual_page_number );//returns element in memory, performs page replacement if necessary
void* getPageFromDisk(int page_number);
void writeToDisk(TableEntry pageTableEntry, int virtual_page_number);
void getPage(int virtual_page_number);
void updateMemoryAndPageTable(int virtual_page_number, int el);//update memory with an integer
void writeModifiedPages();
void printMemory();
void printFifo();
void printDisk(int n);
void showData();

//page replacement algorithms
int pageReplacement_FIFO(int virtual_page_number);
int pageReplacement_SC_FIFO(int virtual_page_number);
int pageReplacement_LRU(int virtual_page_number);

//sorting related
void swap(int *x, int *y);
void bubbleSort(int* arr, int n);
void insertionSort(int* arr, int n);
int partition (int* arr, int low, int high);

void quickSortDefault(int* arr, int n);
void quickSort(int* arr, int low, int high);
void printArray(int* arr, int n);

//helper function for random numbers, I took this from the internet
unsigned short lfsr = 0xACE1u;
unsigned bit;

int rand(){
    bit  = ((lfsr >> 0) ^ (lfsr >> 2) ^ (lfsr >> 3) ^ (lfsr >> 5) ) & 1;
    return lfsr =  (lfsr >> 1) | (bit << 15);
}















/**
 * 
 * HOMEWORK IMPLEMENTATIONS OF DEFINED FUNCTIONS AND SIMILAR
 * 
 * */

//creates a page table with N entries
void createPageTable(TableEntry* pageTable, int n){
    for( int i = 0; i<n; ++ i ){
        pageTable[i] = newPageTableEntry();
    }
}

TableEntry newPageTableEntry(){
    TableEntry tableEntry = {-1, 0, 0, 0, -1};
    return tableEntry;
}

void bubbleSort(int* arr, int n){

   int i, j;
   bool swapped;
   for (i = 0; i < n-1; i++){
    
     swapped = false;
     for (j = 0; j < n-i-1; j++){

        int arr_j = *((int*)getFromMemory(j));//sizeof(int) for example, performs page replacement if not in memory
        int arr_j_1 = *((int*)getFromMemory(j+1));

        if (arr_j > arr_j_1){
           // swap(&arr[j], &arr[j+1]);
            // printf("IN HERE %d %d, %d %d\n",j, j+1, arr_j, arr_j_1);
            // printMemory();
            //performs swapping in here.
           updateMemoryAndPageTable(j, arr_j_1);
           updateMemoryAndPageTable(j+1, arr_j);
            // printMemory();
           // void updateMemoryAndPageTable(void* start_address, int index, index_offset, int el);//adding new int element to memory!
           // updateMemoryAndPageTable(arr, j+1, arr_j, sizeof(int));

           swapped = true;
        }
     }
 
     if (swapped == false)break;//break when sorted!
        
   }
}

void updateMemoryAndPageTable(int virtual_page_number, int el){//update memory with an integer

    pageTable[virtual_page_number].M = true;//update page table
    int page_frame = pageTable[virtual_page_number].page_frame_number;//get page frame
    // printf("PF: %d, %d, %d \n", page_frame, el, memory_count);
    memory[page_frame] = el;//update memory using page frame
}




void insertionSort(int* arr, int n){
    
    int i, key, j;
    for (i = 1; i < n; i++) {
        // key = arr[i];
        key = *((int*)getFromMemory(i));

        j = i - 1;

        int arr_j = *((int*)getFromMemory(j));
        while (j >= 0 && arr_j > key) {
            
            *((int*)getFromMemory(j+1));//before updating modified bit and similar, make sure page is inside the page table, memory and fifo's queue   
            updateMemoryAndPageTable(j+1, arr_j);
            j = j - 1;
            if(j>=0) arr_j = *((int*)getFromMemory(j));
        }

        *((int*)getFromMemory(j+1));//puts it in memory, manages the memory properly, get the entry before writing to it 
        // arr[j + 1] = key;
        updateMemoryAndPageTable(j+1, key);
        
    }
}
 
//helper for quick sort!
int partition (int* arr, int low, int high){

    // int pivot = arr[high]; // pivot
    int pivot = *((int*)getFromMemory(high));

    int i = (low - 1);//right position of pi
 
    for (int j = low; j <= high - 1; j++){// < pivot ! - smaller than pivot
        
        int arr_i;
        int arr_j = *((int*)getFromMemory(j));
        arr_i = *((int*)getFromMemory(i+1));//get it in advance        

        if (arr_j < pivot){
            i++;
            // arr_j = *((int*)getFromMemory(j));
            // printf("%d %d, %d %d\n", i, j, arr_i, arr_j);
            updateMemoryAndPageTable(j, arr_i);
            updateMemoryAndPageTable(i, arr_j);// swap(&arr[i], &arr[j]);
        }
    }


    int arr_i_1 = *((int*)getFromMemory(i+1));
    int arr_high = *((int*)getFromMemory(high));//swapping here
    updateMemoryAndPageTable(high, arr_i_1);
    updateMemoryAndPageTable(i+1, arr_high);

    return (i + 1);
}

void quickSort(int* arr, int low, int high){
    if (low < high){
        int partition_index = partition(arr, low, high);
 
        // sport one by one
        quickSort(arr, low, partition_index - 1);
        quickSort(arr, partition_index + 1, high);
    }
}

//helper for sort function
void quickSortDefault(int* arr, int n){
    quickSort(arr, 0, n-1);
}

void printDisk(int n){
    printf("\nDISK\n");
    for( int i = 0; i<n; ++i ){
        uint32_t atDiskPos = disk[i];
        printfHex32(atDiskPos);
        printf(" ");
    }
    printf("\n");
}

void executeTaskSortingWithFactor( void (*sorting_algorithm)(int*, int), int factor, PageReplacementAlgorithm prAlgo){//creates a random array with respect to the size of physical memory
    int arr_size = MAIN*factor;
    int arr[arr_size];

    for( int i = 0; i<arr_size; ++i ){//generating random array
        arr[i] = rand();
    }

    executeTaskSorting(arr, arr_size, sorting_algorithm, prAlgo);
}

void executeTaskSorting(int* arr, int n, void (*sorting_algorithm)(int*, int), PageReplacementAlgorithm prAlgo){
    initAdditionalData();
    writeToDisk(arr, n);//make sure things stored in memory first
    
    pgrAlgo = prAlgo;//set page replacement algorithm you want to use

    sorting_algorithm(arr, n);

    writeModifiedPages();//before task finished its execution, make sure all modified pages are written properly into the disk!
    printDisk(n);
    showData();
}

void writeModifiedPages(){

    for( int i = 0; i<N ; ++i){
        if( pageTable[i].M == true ){
            int modified_page_frame = pageTable[i].page_frame_number;
            int modified_page = memory[modified_page_frame];
            writeToDisk(modified_page, i);

            pageTable[i].M = false;
        }
    }
}

void writeToDisk(int* arr, int n){
    for( int i = 0; i<n; ++i )
        disk[i] = arr[i];

    for( int i = n; i<N; ++i )
        disk[i] = 0;
}

void* getFromMemory(int virtual_page_number ){

    //check if in memory
    TableEntry* pageTableEntry = pageTable + virtual_page_number;

    if( !(pageTableEntry->valid) ){//if not valid, perform page replacement, page is on the disk!
    
        ++n_misses;
        ++n_misses_cycle;

        getPage(virtual_page_number);//load the page into memory, performing replacement if necessary
        pageTableEntry->valid = true;
    }else{
        ++n_hits;
        ++n_hits_cycle;
    }

    pageTableEntry->R = true;//update referenced bit
    addNewElement(virtual_page_number);//update fifo!
    pageTableEntry->counter = ++lru_counter;//update the counter here also!
    //page is ready in memory!
    int page_frame_number = pageTableEntry->page_frame_number;
    return (void*)(memory + page_frame_number);
    
}


//load the page into memory, performing replacement if necessary
void getPage(int virtual_page_number){
    
    int new_page_entry = *((int*)getPageFromDisk(virtual_page_number));//get page from disk
    int new_page_frame_number = -1;//find new page's frame number, 2 cases, replacing or loading into free space

    ++n_page_loaded;
    if(memory_count < MAIN){//if enough memory, load a page
        new_page_frame_number = memory_count++;
    }else{//else, replace a page

        int removed_vn;
        switch(pgrAlgo){
        case FIFO:
            removed_vn = pageReplacement_FIFO(virtual_page_number);//manages its fifo queue, returns virtual page number of removed page
            break;
        case SC_FIFO:
            removed_vn = pageReplacement_SC_FIFO(virtual_page_number);
            break;
        case LRU:
            removed_vn = pageReplacement_LRU(virtual_page_number);
            break;
        }

        new_page_frame_number = pageTable[removed_vn].page_frame_number;//just swap them

        if(pageTable[removed_vn].M){//write page to disk if it is modified before updating memory
            int page_frame_removed = pageTable[removed_vn].page_frame_number;
            int old_entry = memory[page_frame_removed];//find entry to be removed in memory
            writeToDisk(old_entry, removed_vn);//write entry to the disk
        }

        pageTable[removed_vn].M = false;//after written, make sure to reset page table's modified bit
        pageTable[removed_vn].valid = false;//after written, make sure to reset page table's modified bit
        pageTable[removed_vn].R = false;//after written, make sure to reset page table's modified bit
    }

    //update memory
    memory[new_page_frame_number] = new_page_entry;
    
    //update the page table.
    pageTable[virtual_page_number].R = true;//on each reference in here, update R bit (these bits are cleared on each clock)
    pageTable[virtual_page_number].page_frame_number = new_page_frame_number;

}

int pageReplacement_FIFO(int virtual_page_number){
    int vpn_removed = removeFirst();//removes the head of the queue and returns its value
    return vpn_removed;
}


int pageReplacement_SC_FIFO(int virtual_page_number){
    int vpn_removed = -1;//removes the head of the queue and returns its value
    
    for( int i = 0; i<=fifo_queue_count; ++i ){
        vpn_removed = removeFirst();//removes the head of the queue and returns its value
        if( pageTable[vpn_removed].R )
            pageTable[vpn_removed].R = false;//reset the reference bit!
        else
            break;

        addNewElement(vpn_removed);//return it, give it a second chance
    }

    return vpn_removed;
}

int pageReplacement_LRU(int virtual_page_number){
    int vpn_removed = -1;

    int min_counter = -1;
    for( int i = 0; i<N; ++i){//find first valid page table
        if( pageTable[i].valid ){
            min_counter = pageTable[i].counter;
            vpn_removed = i;
        }
    } 


    for( int i = 0; i<N; ++i ){
        if( !(pageTable[i].counter <= -1) && pageTable[i].valid && pageTable[i].counter < min_counter ){
            min_counter = pageTable[i].counter;
            vpn_removed = i;
        }
    }
    pageTable[vpn_removed].counter = 0;
    //todo
    return vpn_removed;
}


void setupQueue(){
    for( int i = 0; i<MAIN; ++i )
        fifo_queue[i] = -1;
}
//fifo queue related functions
int removeFirst(){
    return removeAtPos(0);
}

int removeAtPos(int index){
    int to_ret = fifo_queue[index];
    for (int i =  index; i < fifo_queue_count-1; i++){
        fifo_queue[i]= fifo_queue[i+1];
    }

    fifo_queue[--fifo_queue_count] = -1;
    return to_ret;
}

int elementExistsFifo(int el){//-1 when it doesn't exist, 1 when it exists
    for( int i = 0; i<fifo_queue_count; ++i )
        if( fifo_queue[i] == el ) return i;

    return -1;
}

void addNewElement(int el){
    if( fifo_queue_count >= MAIN ) return;//doesn't add anything
    if( fifo_queue[fifo_queue_count-1] == el ) return;//first element already, no need to edit anything

    //if exists, shift everything to left at its position, and add it to end
    int exists_i = elementExistsFifo(el);
    if(exists_i != -1){//if there is a duplicate remove it and then add it at the begining
        removeAtPos(exists_i);
        // addNewElement(el);//removing duplicate disallows base condition infinite loop
    }


    fifo_queue[fifo_queue_count++] = el;
}

void* getPageFromDisk(int virtual_page_number){
    return (void*)(disk+virtual_page_number);
}


void initAdditionalData(){
    createPageTable(pageTable, N);
    clock_count = 0;
    fifo_queue_count = 0;
    lru_counter = 0;
    memory_count = 0;
    n_hits = 0;//The number of hits 

    hit_rate_pcc = 0;//hit rate per clock cycle
    n_hits_cycle = 0 ;

    n_misses = 0;//the number of misses
    miss_rate_pcc = 0;
    n_misses_cycle = 0 ;//The number of misses and miss rate per second

    n_page_loaded = 0;//The number of pages loaded
    n_page_written = 0;//The number of pages written back to the disk
}


void writeToDisk(TableEntry pageTableEntry, int virtual_page_number){
    ++n_page_written;//The number of pages written back to the disk
    int entry = memory[pageTableEntry.page_frame_number];//get entry from memory
    *(disk+virtual_page_number)=entry; //write entry to disk
}

void writeToDisk(int el, int index){//writes an integer to disk
    ++n_page_written;//The number of pages written back to the disk
    *(disk+index) = el;
}


void showData(){
    printf("\n");
    printf("Virtual pagging related information: \n\n");
    printf("Number of hits: ");
    printfHex32(n_hits);
    // printf("%d", n_hits);
    printf("\n");
    printf("Hit rate per clock cycle: ");
    if( clock_count == 0 )
        printfHex32(n_hits);
    else
        printfHex32(n_hits/clock_count);

    // printf("%d", hit_rate_pcc);
    printf("\n\n");

    printf("Number of misses: ");
    printfHex32(n_misses);
    printf("\n");
    printf("Miss rate per clock cycle: ");
    if( clock_count == 0 )
        printfHex32(n_misses);
    else
        printfHex32(n_misses/clock_count);
    // printf("%d", miss_rate_pcc);
    printf("\n\n");

    printf("Number of pages loaded: ");
    printfHex32(n_page_loaded);
    // printf("%d", n_page_loaded);
    printf("\n");
    printf("Number of pages written back to the disk: ");
    printfHex32(n_page_written);
    // printf("%d", n_page_written);
    printf("\n------------------------------------------\n\n");
}

























class PrintfKeyboardEventHandler : public KeyboardEventHandler
{
public:
    void OnKeyDown(char c)
    {
        char* foo = " ";
        foo[0] = c;
        printf(foo);
    }
};

class MouseToConsole : public MouseEventHandler
{
    int8_t x, y;
public:
    
    MouseToConsole()
    {
        uint16_t* VideoMemory = (uint16_t*)0xb8000;
        x = 40;
        y = 12;
        VideoMemory[80*y+x] = (VideoMemory[80*y+x] & 0x0F00) << 4
                            | (VideoMemory[80*y+x] & 0xF000) >> 4
                            | (VideoMemory[80*y+x] & 0x00FF);        
    }
    
    virtual void OnMouseMove(int xoffset, int yoffset)
    {
        static uint16_t* VideoMemory = (uint16_t*)0xb8000;
        VideoMemory[80*y+x] = (VideoMemory[80*y+x] & 0x0F00) << 4
                            | (VideoMemory[80*y+x] & 0xF000) >> 4
                            | (VideoMemory[80*y+x] & 0x00FF);

        x += xoffset;
        if(x >= 80) x = 79;
        if(x < 0) x = 0;
        y += yoffset;
        if(y >= 25) y = 24;
        if(y < 0) y = 0;

        VideoMemory[80*y+x] = (VideoMemory[80*y+x] & 0x0F00) << 4
                            | (VideoMemory[80*y+x] & 0xF000) >> 4
                            | (VideoMemory[80*y+x] & 0x00FF);
    }
    
};

class PrintfUDPHandler : public UserDatagramProtocolHandler
{
public:
    void HandleUserDatagramProtocolMessage(UserDatagramProtocolSocket* socket, common::uint8_t* data, common::uint16_t size)
    {
        char* foo = " ";
        for(int i = 0; i < size; i++)
        {
            foo[0] = data[i];
            printf(foo);
        }
    }
};


class PrintfTCPHandler : public TransmissionControlProtocolHandler
{
public:
    bool HandleTransmissionControlProtocolMessage(TransmissionControlProtocolSocket* socket, common::uint8_t* data, common::uint16_t size)
    {
        char* foo = " ";
        for(int i = 0; i < size; i++)
        {
            foo[0] = data[i];
            printf(foo);
        }
        
        
        
        if(size > 9
            && data[0] == 'G'
            && data[1] == 'E'
            && data[2] == 'T'
            && data[3] == ' '
            && data[4] == '/'
            && data[5] == ' '
            && data[6] == 'H'
            && data[7] == 'T'
            && data[8] == 'T'
            && data[9] == 'P'
        )
        {
            socket->Send((uint8_t*)"HTTP/1.1 200 OK\r\nServer: MyOS\r\nContent-Type: text/html\r\n\r\n<html><head><title>My Operating System</title></head><body><b>My Operating System</b> http://www.AlgorithMan.de</body></html>\r\n",184);
            socket->Disconnect();
        }
        
        
        return true;
    }
};


void sysprintf(char* str)
{
    asm("int $0x80" : : "a" (4), "b" (str));
}

void taskA()
{
    while(true)
        sysprintf("A");
}

void taskB()
{
    while(true)
        sysprintf("B");
}






typedef void (*constructor)();
extern "C" constructor start_ctors;
extern "C" constructor end_ctors;
extern "C" void callConstructors()
{
    for(constructor* i = &start_ctors; i != &end_ctors; i++)
        (*i)();
}



extern "C" void kernelMain(const void* multiboot_structure, uint32_t /*multiboot_magic*/)
{
    printf("Hello World! --- http://www.AlgorithMan.de\n");

    GlobalDescriptorTable gdt;
    
    
    uint32_t* memupper = (uint32_t*)(((size_t)multiboot_structure) + 8);
    size_t heap = 10*1024*1024;
    MemoryManager memoryManager(heap, (*memupper)*1024 - heap - 10*1024);
    
    printf("heap: 0x");
    printfHex((heap >> 24) & 0xFF);
    printfHex((heap >> 16) & 0xFF);
    printfHex((heap >> 8 ) & 0xFF);
    printfHex((heap      ) & 0xFF);
    
    void* allocated = memoryManager.malloc(1024);
    printf("\nallocated: 0x");
    printfHex(((size_t)allocated >> 24) & 0xFF);
    printfHex(((size_t)allocated >> 16) & 0xFF);
    printfHex(((size_t)allocated >> 8 ) & 0xFF);
    printfHex(((size_t)allocated      ) & 0xFF);
    printf("\n");
    
    TaskManager taskManager;
    /*
    Task task1(&gdt, taskA);
    Task task2(&gdt, taskB);
    taskManager.AddTask(&task1);
    taskManager.AddTask(&task2);
    */
    
    InterruptManager interrupts(0x20, &gdt, &taskManager, &clock_count, pageTable);
    SyscallHandler syscalls(&interrupts, 0x80);
    
    printf("Initializing Hardware, Stage 1\n");
    
    #ifdef GRAPHICSMODE
        Desktop desktop(320,200, 0x00,0x00,0xA8);
    #endif
    
    DriverManager drvManager;
    
        #ifdef GRAPHICSMODE
            KeyboardDriver keyboard(&interrupts, &desktop);
        #else
            PrintfKeyboardEventHandler kbhandler;
            KeyboardDriver keyboard(&interrupts, &kbhandler);
        #endif
        drvManager.AddDriver(&keyboard);
        
    
        #ifdef GRAPHICSMODE
            MouseDriver mouse(&interrupts, &desktop);
        #else
            MouseToConsole mousehandler;
            MouseDriver mouse(&interrupts, &mousehandler);
        #endif
        drvManager.AddDriver(&mouse);
        
        PeripheralComponentInterconnectController PCIController;
        PCIController.SelectDrivers(&drvManager, &interrupts);

        #ifdef GRAPHICSMODE
            VideoGraphicsArray vga;
        #endif
        
    printf("Initializing Hardware, Stage 2\n");
        drvManager.ActivateAll();
        
    printf("Initializing Hardware, Stage 3\n");

    #ifdef GRAPHICSMODE
        vga.SetMode(320,200,8);
        Window win1(&desktop, 10,10,20,20, 0xA8,0x00,0x00);
        desktop.AddChild(&win1);
        Window win2(&desktop, 40,15,30,30, 0x00,0xA8,0x00);
        desktop.AddChild(&win2);
    #endif


    
    printf("\nS-ATA primary master: ");
    AdvancedTechnologyAttachment ata0m(true, 0x1F0);
    ata0m.Identify();
    
    printf("\nS-ATA primary slave: ");
    AdvancedTechnologyAttachment ata0s(false, 0x1F0);
    ata0s.Identify();
    ata0s.Write28(0, (uint8_t*)"http://www.AlgorithMan.de", 25);
    ata0s.Flush();
    ata0s.Read28(0, 25);
    
    printf("\nS-ATA secondary master: ");
    AdvancedTechnologyAttachment ata1m(true, 0x170);
    ata1m.Identify();
    
    printf("\nS-ATA secondary slave: ");
    AdvancedTechnologyAttachment ata1s(false, 0x170);
    ata1s.Identify();
    // third: 0x1E8
    // fourth: 0x168
    
    

                 

                   
    amd_am79c973* eth0 = (amd_am79c973*)(drvManager.drivers[2]);

    
    // IP Address
    uint8_t ip1 = 10, ip2 = 0, ip3 = 2, ip4 = 15;
    uint32_t ip_be = ((uint32_t)ip4 << 24)
                | ((uint32_t)ip3 << 16)
                | ((uint32_t)ip2 << 8)
                | (uint32_t)ip1;
    eth0->SetIPAddress(ip_be);
    EtherFrameProvider etherframe(eth0);
    AddressResolutionProtocol arp(&etherframe);    

    
    // IP Address of the default gateway
    uint8_t gip1 = 10, gip2 = 0, gip3 = 2, gip4 = 2;
    uint32_t gip_be = ((uint32_t)gip4 << 24)
                   | ((uint32_t)gip3 << 16)
                   | ((uint32_t)gip2 << 8)
                   | (uint32_t)gip1;
    
    uint8_t subnet1 = 255, subnet2 = 255, subnet3 = 255, subnet4 = 0;
    uint32_t subnet_be = ((uint32_t)subnet4 << 24)
                   | ((uint32_t)subnet3 << 16)
                   | ((uint32_t)subnet2 << 8)
                   | (uint32_t)subnet1;
                   
    InternetProtocolProvider ipv4(&etherframe, &arp, gip_be, subnet_be);
    InternetControlMessageProtocol icmp(&ipv4);
    UserDatagramProtocolProvider udp(&ipv4);
    TransmissionControlProtocolProvider tcp(&ipv4);
    
    
    interrupts.Activate();

    printf("\n\n\n\n");
    
    arp.BroadcastMACAddress(gip_be);
    
    
    PrintfTCPHandler tcphandler;
    TransmissionControlProtocolSocket* tcpsocket = tcp.Listen(1234);
    tcp.Bind(tcpsocket, &tcphandler);
    //tcpsocket->Send((uint8_t*)"Hello TCP!", 10);

    
    //icmp.RequestEchoReply(gip_be);
    
    //PrintfUDPHandler udphandler;
    //UserDatagramProtocolSocket* udpsocket = udp.Connect(gip_be, 1234);
    //udp.Bind(udpsocket, &udphandler);
    //udpsocket->Send((uint8_t*)"Hello UDP!", 10);
    
    //UserDatagramProtocolSocket* udpsocket = udp.Listen(1234);
    //udp.Bind(udpsocket, &udphandler);


    createPageTable(pageTable, N);

    int arr[] = { 12, 11, 13, 5, 6, 1, 20, 0, 7, 15, 150 };
    int arr2[] = { 12, 11, 13, 5, 6, 1, 20, 0, 7, 15, 150 };
    int arr3[] = { 12, 11, 13, 5, 6, 1, 20, 0, 7, 15, 150, 1 };


    printf("\nBUBBLE SORT, factor 11/5, ---FIFO---\n");
    executeTaskSorting(arr, 11, &bubbleSort, FIFO);


    // printf("\nINSERTION SORT, factor 11/5, ---Second chance fifo---\n");
    // executeTaskSorting(arr2, 11, &insertionSort, SC_FIFO);

    // printf("\nQUICK SORT, factor 12/5, ---LRU--- \n");
    // executeTaskSorting(arr3, 12, &quickSortDefault, LRU);
    
    // printf("\nQuick sort, factor 4, generating random array, ---second chance fifo---\n");
    // executeTaskSortingWithFactor(&quickSortDefault, 4, SC_FIFO);

    // printf("\nInsertion sort, factor 2, generating random array, ---LRU---\n");
    // executeTaskSortingWithFactor(&insertionSort, 2, LRU);

    // printf("\nBubble sort, factor 10, generating random array, ---LRU---\n");
    // executeTaskSortingWithFactor(&bubbleSort, 10, LRU);


    
    // while(1)
    // {
    //     #ifdef GRAPHICSMODE
    //         desktop.Draw(&vga);
    //     #endif
    // }
}
