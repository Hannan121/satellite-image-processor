#include "system_def.h"
#include<string.h>
#include<Windows.h>
#include<sys/timeb.h>
#include <process.h>

#include <windows.h>
    double get_time_ms() {
        LARGE_INTEGER frequency;
        LARGE_INTEGER start;
        QueryPerformanceFrequency(&frequency); // Ticks per second
        QueryPerformanceCounter(&start);       // Current tick
        return (double)start.QuadPart * 1000.0 / frequency.QuadPart;
    }

#define MAX_FRAMES 20


double core0_exec_times[MAX_FRAMES];
double core1_exec_times[MAX_FRAMES];

img_ll_struct_t* write_ptr = NULL;
img_ll_struct_t* task1_ptr = NULL;
img_ll_struct_t* task2_ptr=NULL;

const char filename[] = "input_image.bin";


volatile int simulation_running = 1;


unsigned __stdcall Core0_Thread(void* arg) {
    
    // Force this thread to run ONLY on Logic Processor 1 (Mask = 0x01)
    SetThreadAffinityMask(GetCurrentThread(), 0x01);
    int Frames_processed = 0;
    static double prev_millis=0;
    double start=0;
    double stop=0;
    while(Frames_processed<MAX_FRAMES && simulation_running)
    {
        if(get_time_ms()-prev_millis>=100.0)
        {
            prev_millis = get_time_ms();
            if(task1_ptr->status==WRITE_READY)
            {
                task1_ptr->status=TASK1_READY;
                start = get_time_ms();

                median_filter_task(task1_ptr);
                stop = get_time_ms();
                core0_exec_times[Frames_processed] = stop-start;
                Frames_processed++;
                task1_ptr = task1_ptr->nxt_ptr;

            }
            else {
                // Buffer Starvation: Core 1 is too slow!
                printf("[ERROR] Buffer not free\n");
            }
        }
        else
        {
            Sleep(1);
        }
    }
    return 0 ;
}

unsigned __stdcall Core1_Thread(void* arg) {
 
    // Force this thread to run ONLY on Logic Processor 2 (Mask = 0x01)
    SetThreadAffinityMask(GetCurrentThread(), 0x02);
    int Frames_processed = 0;
    double start=0;
    double stop=0;
    while(Frames_processed<MAX_FRAMES)
    {
        if(task2_ptr->status==TASK2_READY)
        {
            start = get_time_ms();
            edge_detection(task2_ptr);
            stop = get_time_ms();
            core1_exec_times[Frames_processed] = stop-start;
            //Process inference here if needed

            //handover
            task2_ptr->status =WRITE_READY;
            task2_ptr = task2_ptr->nxt_ptr;
            Frames_processed++;
        }
        else
        {
            Sleep(0);
        }
    }
    simulation_running = 0;
    return 0;
}


int main()
{
    printf("System initialization...\n");
    system_init(&write_ptr);
    task1_ptr = write_ptr;
    task2_ptr = write_ptr;

    //preloading image buffers
    for(int i=0;i< BUFF_SIZE;i++)
    {
        if(write_ptr==NULL)
        {
            printf("[Error] ring descriptor not initialized\n");
            return -2;
        }
        load_input_data(filename,write_ptr);
        write_ptr->status=WRITE_READY;
        write_ptr=write_ptr->nxt_ptr;
    }
    write_ptr = task1_ptr;

    printf("commencing dual core simulation\n");
    //Initializing cores
    HANDLE hCore0 = (HANDLE)_beginthreadex(NULL, 0, Core0_Thread, NULL, 0, NULL);
    HANDLE hCore1 = (HANDLE)_beginthreadex(NULL, 0, Core1_Thread, NULL, 0, NULL);

    WaitForSingleObject(hCore1, INFINITE);
    


    printf("Simulation complete \n");
    printf("Execution Performance metrics:\n");
    printf("Frame id  , Core0_execution time, Core1_execution time \n");
    for(int i=0;i<MAX_FRAMES;i++)
    {
        printf(" %d        , %lf          , %lf \n ",i,core0_exec_times[i],core1_exec_times[i]);
    }
}