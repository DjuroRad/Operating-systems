#include <multitasking.h>

using namespace myos;
using namespace myos::common;


Task::Task(GlobalDescriptorTable *gdt, void entrypoint())
{
    //make thread (entry point), 
    Thread mainThreadNew(gdt, entrypoint);
    this->mainThread = mainThreadNew;
    threadManager.thread_create(&this->mainThread);
    //threadManager.create ( thread );
    cpustate = (CPUState*)(stack + 4096 - sizeof(CPUState));
    
    //set the cpustate of the process to be the cpu state of the main thread !!!
    updateCPUState(gdt, &(this->mainThread));
}
void Task::updateCPUState(GlobalDescriptorTable *gdt, Thread* thread){
    cpustate -> eax = thread->getCPUState()->eax;
    cpustate -> ebx = thread->getCPUState()->ebx;
    cpustate -> ecx = thread->getCPUState()->ecx;
    cpustate -> edx = thread->getCPUState()->edx;

    cpustate -> esi = thread->getCPUState()->esi;
    cpustate -> edi = thread->getCPUState()->edi;
    cpustate -> ebp = thread->getCPUState()->ebp;
    
    /*
    cpustate -> gs = 0;
    cpustate -> fs = 0;
    cpustate -> es = 0;
    cpustate -> ds = 0;
    */
    
    cpustate -> eip = (uint32_t)thread->getCPUState()->eip;
    
    if( (uint32_t)gdt != 0 )
        cpustate -> cs = gdt->CodeSegmentSelector();
    
    cpustate -> eflags = thread->getCPUState()->eflags;
}


Task::~Task()
{
}

        
TaskManager::TaskManager()
{
    numTasks = 0;
    currentTask = -1;
}

TaskManager::~TaskManager()
{
}

bool TaskManager::AddTask(Task* task)
{
    if(numTasks >= 256)
        return false;
    tasks[numTasks++] = task;
    
    return true;
}

CPUState* TaskManager::CurrentTaskFunctionTerminated(CPUState* cpustate){
    //only one thread -> remove the task itself
    if( tasks[currentTask]->getNumThreads() == 1 ){
        
        //last element needs to be set to 0, or an empty process what ever, 0...
        --numTasks;
        for( uint32_t i = currentTask; i<numTasks; ++i ){
            tasks[i] = tasks[i+1];// no need for %numTasks .. even if it was full now it can't cause an overflow since it won't be a full array...
        }
        
        if( currentTask >= numTasks )
            currentTask = 0;

        return tasks[currentTask]->cpustate;
    }
    else if(tasks[currentTask]->getNumThreads() > 1){//otherwise remove the current thread
        tasks[currentTask]->threadManager.thread_exit_interrupt();
        return Schedule(cpustate);
    }
    else
        return cpustate;
}

CPUState* TaskManager::Schedule(CPUState* cpustate)
{
    if(numTasks <= 0)
        return cpustate;
    
    if(currentTask >= 0){        
        tasks[currentTask]->rescheduleCPUState(cpustate);//after task's cpustate is updated, cpustate of the thread it is executing should also be updated
    }
    if(++currentTask >= numTasks)
        currentTask %= numTasks;

    // tasks[currentTask] -> cpustate -> eip = tasks[currentTask] -> threadManager.ScheduleThread();
    return tasks[currentTask]->cpustate;
}

//UPDATES:
//1. thread's cpustate ( thread which is currently in execution )
//2. currentThread number
//3. task's cpu state to new currentThread!
void Task::rescheduleCPUState(CPUState* cpustate){
    CPUState* new_cpustate = this->threadManager.ScheduleThread(cpustate);//1. + 2.
    this -> cpustate = new_cpustate;
}
// ------------------------------------     MULTITHREADING      ----------------------------------------------  //

Task::Thread::Thread(GlobalDescriptorTable *gdt, void entrypoint()){
    

    thread_cpustate = (CPUState*)(thread_stack + 4096 - sizeof(CPUState));
    
    thread_cpustate -> eax = 0;
    thread_cpustate -> ebx = 0;
    thread_cpustate -> ecx = 0;
    thread_cpustate -> edx = 0;

    thread_cpustate -> esi = 0;
    thread_cpustate -> edi = 0;
    thread_cpustate -> ebp = 0;
    
    /*
    cpustate -> gs = 0;
    cpustate -> fs = 0;
    cpustate -> es = 0;
    cpustate -> ds = 0;
    */
    
    // cpustate -> error = 0;    
   
    // cpustate -> esp = ;
    thread_cpustate -> eip = (uint32_t)entrypoint;
    thread_cpustate -> cs = gdt->CodeSegmentSelector();
    // cpustate -> ss = ;
    thread_cpustate -> eflags = 0x202;
}
Task::Thread::~Thread(){}


void Task::Thread::thread_terminate(){
    my_task->threadManager.thread_exit();
}//removes this thread from the array of threads in ThreadManager class

void Task::Thread::thread_yield(){
    // yield_foo( this->thread_cpustate );//or just change esp/ip to this yield_this
    
    my_task->threadManager.thread_yield();
    while( my_task->threadManager.currentThreadYielded() );

    //again busy wait til timer interrupt -> after this ThreadSchedule will make sure
    //to let another thread run instead of this one in the next round
}//invokes the scheduler of the thread manager


bool Task::ThreadTableEntry::checkJoinedTerminated(){
    if( numJoined < 0 ) numJoined = 0;//it's impossible for it to be less than 0, anyways just some precautions
    return numJoined == 0;
}



Task::ThreadManager::ThreadManager(){
    numThreads = 0;
    currentThread = -1;
    currentTerminated = false;
}

Task::ThreadManager::~ThreadManager(){}

//adds a thread to the queue of threads
bool Task::ThreadManager::thread_create(Thread* thread){
    //set the terminate and yield for the thread so that it can call them
    // thread->setYieldFoo(&(this->thread_yield_threadManager));
    // thread->setTerminateFoo(this->thread_exit);
    thread->setMyTask(my_task);
    if(numThreads >= 256)
        return false;
    threads[numThreads++].thread = thread;
    if( currentThread == -1 ) currentThread = 0;
    return true;

}

//performs scheduling for threads
//whenever OS gives CPU to a task it performs scheduling of threads and than executes next thread
CPUState* Task::ThreadManager::ScheduleThread(CPUState* thread_cpustate){

    if(numThreads <= 0)
        return thread_cpustate;
    
    if(currentThread >= 0)
        threads[currentThread].thread->thread_cpustate = thread_cpustate;//store the new cpu state
    
    switch( threads[currentThread].state_thread){
        case TERMINATED:
            thread_exit_update();//updates the currentThread and exits the current one since it is TERMINATED
            return threads[currentThread].thread->thread_cpustate;
            break; 
        case YIELDED: //means that during this cycle the thread was yielded
        case WAITING: //means that during this cycle the thread called join()
        case RUNNING: //means no change during this round, but we have to check state of the thread after current one in order to schedule the next one
            switch( nextThreadState() ){
                case YIELDED:
                    setNextThreadStateRunning();//this pauses it for one cycle allowing others to execute their job
                    return skipTilNextRunningThread();//skips next thread since it was yielded and sets cpustate to some other thread freeing resources this way
                    break;
                case WAITING:
                    if( !checkJoinedTerminated() )
                        return skipTilNextRunningThread();
                    else
                        setNextThreadStateRunning();//release it so that it can run now since all the threads it waited for finished their execution
                    break;
            }
    }

    if(++currentThread >= numThreads)
        currentThread %= numThreads;
    return threads[currentThread].thread->thread_cpustate;
}

bool Task::ThreadManager::checkJoinedTerminated(){
    uint16_t next_thread = (currentThread + 1) % numThreads;
    return threads[next_thread].checkJoinedTerminated();
}


CPUState* Task::ThreadManager::skipTilNextRunningThread(){
    uint16_t curr_i  = (currentThread+2)%numThreads;//skips checking next thread which has been set to RUNNING beforehand, but is skipped since we now it yields, we set it to RUNNING because we want it executed now, not in the next round.

    for( uint16_t i = 1; i<numThreads-1 && threads[curr_i].state_thread != RUNNING; ++i ){
        // curr_i = (currentThread+1) + i;
        curr_i = (curr_i+1)%numThreads;
        // if( curr_i >= numThreads ) curr_i %= numThreads;
    }

    currentThread = curr_i;
    return threads[curr_i].thread->thread_cpustate;
}

ThreadState Task::ThreadManager::nextThreadState(){
    return threads[ (currentThread + 1)%numThreads ].state_thread;
}

void Task::ThreadManager::setNextThreadStateRunning(){
    threads[ (currentThread + 1)%numThreads ].state_thread = RUNNING;
}

void Task::ThreadManager::thread_exit_interrupt(){
    threads[currentThread].state_thread = TERMINATED;
    //also free all the threads that have been waiting for this thread to terminate
    for( uint16_t i = 0; i < threads[currentThread].numWaiting; ++i ){
        threads[currentThread].waiting_threads[i]->numJoined = threads[currentThread].waiting_threads[i]->numJoined - 1;
    }
    //no busy waiting here
}
//removing the current thread from the threadQueue.
void Task::ThreadManager::thread_exit(){
    threads[currentThread].state_thread = TERMINATED;
    //also free all the threads that have been waiting for this thread to terminate
    for( uint16_t i = 0; i < threads[currentThread].numWaiting; ++i ){
        threads[currentThread].waiting_threads[i]->numJoined = threads[currentThread].waiting_threads[i]->numJoined - 1;
    }
    while(1);//busy wait afterwards
}

void Task::ThreadManager::thread_exit_update(){
    if( numThreads <= 0 ) return;
    //last element needs to be set to 0, or an empty process what ever, 0...
    --numThreads;
    for( uint32_t i = currentThread; i<numThreads; ++i ){
        threads[i] = threads[i+1];// no need for %numTasks .. even if it was full now it can't cause an overflow since it won't be a full array...
    }
    
    if( currentThread >= numThreads )//in case last task was removed!
        currentThread = 0;
}

void Task::ThreadManager::thread_yield(){
    if( numThreads <= 1 ) return;//if there is just one thread don't yield!

    bool all_yielded = true;
    for( uint16_t i = 0; i<numThreads && all_yielded; ++i )
        if( i!=currentThread && threads[i].state_thread != YIELDED )
            all_yielded = false;
    if(!all_yielded){
        threads[currentThread].state_thread = YIELDED;        
        // while( threads[currentThread].state_thread == YIELDED );
    }
    //otherwise do nothing, thread will continue to run.
    //one can busy wait this and still make a deadlock.
    //but this way it will make sure that this function will not cause it.    
}

void Task::ThreadManager::thread_join(Thread* thread){
    threads[currentThread].state_thread = WAITING;
    threads[currentThread].addJoinThread();
    //now add waiting thread to the *thread it is pointing to, gotta iterate through them
    addCurrentThreadAsWaitingThread(thread);
    // thread->addWaitingThread();
    //i don't want it to continue execution after calling threadJoin() !
    while(threads[currentThread].state_thread == WAITING);
}

bool Task::ThreadManager::addCurrentThreadAsWaitingThread(Thread* thread){
    ThreadTableEntry* thread_to_add = &threads[currentThread];

    ThreadTableEntry* joined_thread = NULL; 
    uint16_t i = 0;
    for(; i<numThreads && threads[i].thread != thread; ++i );//this will find the joined thread's entry in the table!
    
    joined_thread = &threads[i];

    bool notAlreadyAdded = true;
    for( i = 0; i< joined_thread->numWaiting && notAlreadyAdded; ++i )
        if( joined_thread->waiting_threads[i]->thread == thread_to_add->thread )
            notAlreadyAdded = false;

    if( notAlreadyAdded ){
        (joined_thread->waiting_threads)[ joined_thread->numWaiting ] = thread_to_add;
        ++(joined_thread->numWaiting);
    }
    
    return notAlreadyAdded;
}

void Mutex::enter_critical_region(int thread_id){
    interested[thread_id] = true;
    turn = thread_id;
    while( turn == thread_id && areOthersInterested(thread_id) );
}

bool Mutex::areOthersInterested(int thread_id){
    for( uint16_t i=0; i<MAX_THREADS; ++i )
        if( i!=thread_id && interested[i] ) return true;
    
    return false;
}

void Semaphore::down(int thread_id){

    int wait_flag = 1;

    countMutext.enter_critical_region(thread_id);
        if( count <= 0 ){
            count = 0;
            while( count == 0 ){
                if( wait_flag ){//first time in here just release the critical region
                    wait_flag = 0;
                    countMutext.leave_critical_region(thread_id);
                }
            }; //so just wait here until it is changed
        }
        else{
            count = count - 1;
            countMutext.leave_critical_region(thread_id);    
        }
    
}

void Semaphore::up(int thread_id){
    countMutext.enter_critical_region(thread_id);
        count = count + 1;//increase the count producing one more
    countMutext.leave_critical_region(thread_id);
}