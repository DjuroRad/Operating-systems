 
#ifndef __MYOS__MULTITASKING_H
#define __MYOS__MULTITASKING_H
#define MAX_THREADS 256
#include <common/types.h>
#include <gdt.h>
namespace myos
{

    typedef enum{RUNNING, YIELDED, TERMINATED, WAITING}ThreadState;//not yet decided
    struct CPUState
    {
        common::uint32_t eax;
        common::uint32_t ebx;
        common::uint32_t ecx;
        common::uint32_t edx;

        common::uint32_t esi;
        common::uint32_t edi;
        common::uint32_t ebp;

        /*
        common::uint32_t gs;
        common::uint32_t fs;
        common::uint32_t es;
        common::uint32_t ds;
        */
        common::uint32_t error;

        common::uint32_t eip;
        common::uint32_t cs;
        common::uint32_t eflags;
        common::uint32_t esp;
        common::uint32_t ss;        
    } __attribute__((packed));
    

    class Task{
    friend class TaskManager;
    //friend class ThreadManager;
    public:
        // ---------------------------------------- THREAD, THREADTABLEENTRY, THREADMANAGER -------------------------------------------- //
        class Thread{
            
            public:
                friend class ThreadManager;
                friend class Task;
                CPUState* thread_cpustate;
                Thread(GlobalDescriptorTable *gdt, void entrypoint());
                Task* my_task;
                Thread(){ Thread(0,0);}
                ~Thread();
                Thread(const Thread &r);
                void thread_terminate();//removes this thread from the array of threads in ThreadManager class
                void thread_yield();
                void thread_join(Thread* thread_to_wait_for);
                // bool thread_terminate(void* retval);//removes this thread from the array of threads in ThreadManager class
                const CPUState* getCPUState(){return thread_cpustate;}
            
            //friend class ThreadManager;
            private:
                common::uint8_t thread_stack[4096]; // 4 KiB
                // typedef enum{RUNNING, WAITING, TERMINATED}ThreadState;//not yet decided
                // Task& parentTask;
                void setMyTask(Task* task){this->my_task = task;}
            };

        class ThreadTableEntry{

                public:
                    friend class ThreadManager;
                    Thread* thread;
                    ThreadTableEntry* waiting_threads[MAX_THREADS-1];//represents alive threads that are waiting for this thread
                    common::uint8_t numWaiting;
                    common::uint8_t numJoined;
                    ThreadState state_thread;
                    ThreadTableEntry(){ ThreadTableEntry(NULL); }
                    ThreadTableEntry(Thread* thread){ numWaiting = 0; numJoined = 0; state_thread = RUNNING; this->thread = thread;}
                    bool addJoinThread(){ ++numJoined; };
                    bool checkJoinedTerminated();
                private:
                    void removeJoinedThreads();
                };

        class ThreadManager{
                public:
                    bool currentTerminated; 
                    ThreadManager();
                    ThreadManager(Task* task){this->my_task = task;};
                    ~ThreadManager();
                    bool thread_create(Thread* thread);
                    void thread_exit_update();//search for a thread and remove it..

                    void thread_exit();//search for a thread and remove it..
                    void thread_exit_interrupt();
                    void thread_yield();//performs thread scheduling and changes the cpustate of the current task so that yield() is completed
                    void thread_join(Thread* thread);
                    CPUState* ScheduleThread(CPUState* thread_cpustate);
                    int getNumThreads(){return numThreads;};
                    int getCurrentThread(){return currentThread;};
                    bool currentThreadYielded(){ return threads[currentThread].state_thread == YIELDED; }
                private:
                    ThreadTableEntry threads[256];
                    Task* my_task;
                    int numThreads;
                    int currentThread;
                    ThreadState nextThreadState();
                    void setNextThreadStateRunning();
                    CPUState* skipTilNextRunningThread();
                    bool checkJoinedTerminated();
                    bool addCurrentThreadAsWaitingThread(Thread* thread);
            };
        // ---------------------------------------- THREAD, THREADTABLEENTRY, THREADMANAGER -------------------------------------------- //
        
        //public methods, variables and similar
        public:

        Task(GlobalDescriptorTable *gdt, void entrypoint());
        ~Task();
        void thread_create(Thread* thread){threadManager.thread_create(thread);};
        void thread_exit(){threadManager.thread_exit();};//exits the current thread!
        void thread_join(Thread* thread){threadManager.thread_join(thread);}
        void thread_yield(){threadManager.thread_yield();}

        int getNumThreads(){return threadManager.getNumThreads();}
        int getCurrentThread(){return threadManager.getCurrentThread();}
        void rescheduleCPUState(CPUState* cpustate);

        private:
            Thread mainThread;
            ThreadManager threadManager;
            void updateCPUState(GlobalDescriptorTable *gdt, Thread* thread);
            common::uint8_t stack[4096]; // 4 KiB
            CPUState* cpustate;
    };
    
    
    class TaskManager{
        public:
            TaskManager();
            ~TaskManager();
            bool AddTask(Task* task);
            CPUState* CurrentTaskFunctionTerminated(CPUState* cpustate);
            CPUState* Schedule(CPUState* cpustate);
            int getNumTasks(){return numTasks;}
            
        private:
            Task* tasks[256];
            int numTasks;
            int currentTask;
    };
    
    
   // ------------------------------------------ MUTEX && SEMAPHORE ------------------------------------------ //
    class Mutex{
        public:
            Mutex(){ for( common::uint16_t i = 0; i<MAX_THREADS; ++i ) interested[i] = false; turn = -1;}//initial setup
            void enter_critical_region(int thread_id);
            void leave_critical_region(int thread_id){ interested[thread_id] = false; }
        private:
            bool interested[MAX_THREADS];
            int turn;
            bool areOthersInterested(int thread_id);
    };

    class Semaphore{
        private:
            common::uint32_t count;
            Mutex countMutext;
            common::uint32_t max;
        public:
            Semaphore(common::uint32_t count){this->count = count; this->max = count;}
            void down(int thread_id);
            void up(int thread_id);
    };
}
#endif