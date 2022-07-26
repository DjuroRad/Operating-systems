
#include <common/types.h>
#include <gdt.h>
#include <hardwarecommunication/interrupts.h>
#include <hardwarecommunication/pci.h>
#include <drivers/driver.h>
#include <drivers/keyboard.h>
#include <drivers/mouse.h>
#include <drivers/vga.h>
#include <gui/desktop.h>
#include <gui/window.h>
#include <multitasking.h>


// #define GRAPHICSMODE


using namespace myos;
using namespace myos::common;
using namespace myos::drivers;
using namespace myos::hardwarecommunication;
using namespace myos::gui;


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


char* itoa(int value, char* result, int base) {
    // check that the base if valid
    if (base < 2 || base > 36) { *result = '\0'; return result; }

    char* ptr = result, *ptr1 = result, tmp_char;
    int tmp_value;

    do {
        tmp_value = value;
        value /= base;
        *ptr++ = "zyxwvutsrqponmlkjihgfedcba9876543210123456789abcdefghijklmnopqrstuvwxyz" [35 + (tmp_value - value * base)];
    } while ( value );

    // Apply negative sign
    if (tmp_value < 0) *ptr++ = '-';
    *ptr-- = '\0';
    while(ptr1 < ptr) {
        tmp_char = *ptr;
        *ptr--= *ptr1;
        *ptr1++ = tmp_char;
    }
    return result;
}

// int i1c = 0;
void taskA()
{
    int i = 0;
    while(true){
        if( i%5000==0 )
            printf("A");
        ++i;
    }
    // while(i1c < 20000){
        // printf("- KMU -");

        // i1c++;
        // }
}

// int i1b = 0;
void taskB()
{
    int i = 0;
    while(true){
        if( i%5000 == 0 )
            printf("B");
        ++i;
    }
    // while(i1b < 20000 ){
        // printf("- KMU -");

        // i1b++;
        // }
}

//uncomment to test thread yield
// void task2_Yield(){
//     int i = 0;
//     while( true ){
//         // if( i < 1000000 )
//         // task1.thread_yield();
//         // else{
//         if( i%5000==0 ){
//             printf("2-");
//             char str[10];
//             itoa(i, str, 10);
//             printf(str);
//             printf(" ");
//         }

//         ++i;
//     }
// }
void empty_foo(){}

GlobalDescriptorTable gdt;
    Task task1(&gdt, taskA);
    //comment to test thread yield
    Task task2(&gdt, taskB);
    //uncomment to test thread yield
    // Task task2(&gdt, task2_Yield);
    Task task_consumer_producer_test(&gdt, empty_foo);

    TaskManager taskManager;


void printfHex(uint8_t key)
{
    char* foo = "00";
    char* hex = "0123456789ABCDEF";
    foo[0] = hex[(key >> 4) & 0xF];
    foo[1] = hex[key & 0xF];
    printf(foo);
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





void taskC(){
    while(true)
        printf("C");
}





void threadA(){
    int iA = 0;
    while(true){
        if( iA % 5000 == 0 )
            printf("1");
        ++iA;
    }
}

void threadA_Exit(){
    int iA = 0;
    while(true){
        
        if( iA == 2000000 )
            task1.thread_exit();

        if( iA %5000 == 0 ){
            char str[10];
            itoa(iA, str, 10);
            printf(str); printf(" ");
        }
        ++iA;
    }
}

    // char str[400];
    // while( iA < 3000000 ){
    //     iA = iA + 1;
    //     task1.threadManager.thread_yield();
    // }


    // char str[100];
    // while(true){
    //     // str[0] = str[0] + 1;
    //     // str[0] = str[0] + 1;
    //     itoa(iA, str,10);
    //     printf(str);
    //     printf(" ");
    //     if( iA == 100000){
    //         task1.thread_exit();
    //     }
    //     if( iA > 100000 ){
    //         printf(" +++++++++++++++++++++++++++++++++++++++++++++++++++ ");
    //     }
    //     int num = task1.getNumThreads();
        
    //     char str2[10];
    //     itoa(num, str2, 10);
    //     printf(" - ");
    //     printf(str2);
    //     printf(" - ");
    //     itoa(task1.getCurrentThread(), str2, 10);
    //     printf(" - ");
    //     printf(str2);
    //     printf(" - ");
            
        
        
    //     ++iA;
    // }


void threadB(){
    int i = 0;
    // task1.threadManager.thread_join(&thread1);
    while(true){
        if( i%5000 == 0 )
            printf("2");
        ++i;
    }
}
void threadC(){
    int i = 0;
    while(true){
        if( i%5000 == 0 )
            printf("3");
        ++i;
    }  
}

void threadD(){
    int i = 0;
    while( true ){
        if( i%5000==0 )
            printf("4");
        
        ++i;
    }
}

int ch = 0;
char produce_item(){
    char new_char = 'a' + ch%20;
    ++ch;
    if( ch > 25 ) ch = 0;
    return new_char;
}

const int N_buffer = 10000;

char buffer[N_buffer];

int numElBuffer = 0;
Semaphore full(0);
Semaphore empty(N_buffer);
Mutex mutex_example;//critical region for consumer and producer

void consumer(){
    while( true ){

        char to_consume = '\0';
        char str[2];
        str[1] = '\0';
        
        full.down( task1.getCurrentThread() );
            mutex_example.enter_critical_region( task_consumer_producer_test.getCurrentThread() );
                if(numElBuffer > 0)
                    to_consume = buffer[--numElBuffer];
            mutex_example.leave_critical_region( task_consumer_producer_test.getCurrentThread() );
        empty.up( task1.getCurrentThread() );
        
        if( to_consume != '\0'){
            printf("CONSUMER IS CONSUMING: ");
            str[0] = to_consume;
            printf(str);
            printf("\n");
        }
    }
}





// void consumer(){
//     while( true ){

//         char to_consume;
//         char str[2];
//         str[1] = '\0';
        
//         full.down( task1.getCurrentThread() );
//             mutex_example.enter_critical_region( task1.getCurrentThread() );
//                 to_consume = buffer[--numElBuffer];
//                 printf("CONSUMER IS CONSUMING: ");
//                 str[0] = to_consume;
//                 printf(str);
//                 printf("\n");
//             mutex_example.leave_critical_region( task1.getCurrentThread() );
//         empty.up( task1.getCurrentThread() );

        
//     }
// }

void producer(){

    while( true ){
        char produced_item = produce_item();
        char str[2];
        str[0] = produced_item;
        str[1] = '\0';

        empty.down( task1.getCurrentThread() );
            mutex_example.enter_critical_region( task_consumer_producer_test.getCurrentThread() );
                if( numElBuffer < N_buffer ){
                    buffer[numElBuffer++] = produced_item;
                    printf("PRODUCER PRODUCED: ");
                    printf(str);
                    printf("\n");
                }
                
            mutex_example.leave_critical_region( task_consumer_producer_test.getCurrentThread() );
        full.up( task1.getCurrentThread() );
    }
    
}




void test_thread_create(){
    

    // Task::Thread thread3(&gdt, threadC);
    // Task::Thread thread2(&gdt, consumer);
    // Task::Thread thread3_helper(&gdt, producer);
    
    // task1.thread_create(&thread3_helper);


  

    // task1.thread_create(&thread3);

}

typedef void (*constructor)();
extern "C" constructor start_ctors;
extern "C" constructor end_ctors;
extern "C" void callConstructors()
{
    for(constructor* i = &start_ctors; i != &end_ctors; i++)
        (*i)();
}

//Uncomment to test thread_JOIN

// Task::Thread thread1(&gdt, threadA_Exit);
// void threadC_Joined(){
//     task1.thread_join(&thread1);
//     int i = 0;
//     while(true){
//         if( i%5000 == 0 )
//             printf("3");
//         ++i;
//     }  
// }

// void threadB_Joined(){
//     task1.thread_join(&thread1);
//     int i = 0;
//     while( true ){
//         if( i%5000==0 )
//             printf("2");
        
//         ++i;
//     }
// }



//Uncomment to test thread_YIELD

// void threadD_Yield(){
//     int i = 0;
//     while( true ){

//         task1.thread_yield();

//         if( i%5000 == 0 ){
//             char str[10];
//             printf("D-");
//             itoa(i, str, 10);
//             printf(str);
//             printf(" ");
//         }
//         ++i;
//     }
// }


extern "C" void kernelMain(const void* multiboot_structure, uint32_t /*multiboot_magic*/)
{
    printf("Hello World! --- http://www.AlgorithMan.de\n");


    //  --- thread_CREATE_test (UNCOMMENT SECTION TO TEST) ---  //
    
    // Task::Thread thread1(&gdt, threadA);
    // Task::Thread thread2(&gdt, threadB);
    // Task::Thread thread3(&gdt, threadC);
    // Task::Thread thread4(&gdt, threadD);

    // task1.thread_create(&thread3);
    // task1.thread_create(&thread1);
    // task1.thread_create(&thread2);
    // task2.thread_create(&thread4);

    // taskManager.AddTask(&task1);
    // taskManager.AddTask(&task2);

    //  --- thread_create_test ---  //



    //  --- thread_EXIT_test (UNCOMMENT SECTION TO TEST) ---  //
    
    Task::Thread thread1(&gdt, threadA_Exit);
    Task::Thread thread2(&gdt, threadB);
    Task::Thread thread3(&gdt, threadC);
    Task::Thread thread4(&gdt, threadD);

    task1.thread_create(&thread3);
    task1.thread_create(&thread1);
    task1.thread_create(&thread2);
    task2.thread_create(&thread4);

    taskManager.AddTask(&task1);
    taskManager.AddTask(&task2);

    //  --- thread_create_test ---  //
    



    //  --- thread_JOIN_test (UNCOMMENT SECTION TO TEST) ---  //
    //  --- thread_JOIN_test (ALSO UNCOMMENT SECTION FOR FUNCTIONS AFTER LINE 378 til kernelMain) ---  //

    
    // Task::Thread thread4(&gdt, threadD);
    // Task::Thread thread2(&gdt, threadB_Joined);
    // Task::Thread thread3(&gdt, threadC_Joined);
    
    // task1.thread_create(&thread3);
    // task1.thread_create(&thread1);
    // task1.thread_create(&thread2);
    // task2.thread_create(&thread4);

    // taskManager.AddTask(&task1);
    // taskManager.AddTask(&task2);

    //  --- thread_join_test ---  //


    //  --- thread_YIELD_test (UNCOMMENT SECTION TO TEST) ---  //
    //  --- UNCOMMENT threadD_Yield to be able to see the function ---  //    
    // Task::Thread thread1(&gdt, threadA);
    // Task::Thread thread2(&gdt, threadB);
    // Task::Thread thread3(&gdt, threadC);
    // Task::Thread thread4(&gdt, threadD_Yield);

    // task1.thread_create(&thread3);
    // task1.thread_create(&thread1);
    // task1.thread_create(&thread2);
    // task2.thread_create(&thread4);

    // taskManager.AddTask(&task1);
    // taskManager.AddTask(&task2);

    //  --- thread_yield_test ---  //


    
    //  --- Consumer/Producer (mutext) test (UNCOMMENT SECTION TO TEST) ---  //
    // Task task_consumer_producer_test(&gdt, empty_foo);
    // Task::Thread thread1(&gdt, consumer);
    // Task::Thread thread1a(&gdt, consumer);
    // Task::Thread thread1b(&gdt, consumer);
    
    // Task::Thread thread2(&gdt, producer);
    // Task::Thread thread3(&gdt, producer);


    // taskManager.AddTask(&task_consumer_producer_test);

    // task_consumer_producer_test.thread_create(&thread1);
    // task_consumer_producer_test.thread_create(&thread1a);
    // task_consumer_producer_test.thread_create(&thread1b);
    // task_consumer_producer_test.thread_create(&thread3);
    // task_consumer_producer_test.thread_create(&thread2);
    

    

    //  --- Consumer/Producer (mutext) test (UNCOMMENT SECTION TO TEST) ---  //
    InterruptManager interrupts(0x20, &gdt, &taskManager);
    
    
    
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

        VideoGraphicsArray vga;
        
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


    interrupts.Activate();
    
    while(1)
    {
        #ifdef GRAPHICSMODE
            desktop.Draw(&vga);
        #endif
    }
}
