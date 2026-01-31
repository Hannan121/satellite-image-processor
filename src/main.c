#include "system_def.h"
#include<string.h>
#include<Windows.h>
#include<sys/timeb.h>


#include <windows.h>
    double get_time_ms() {
        LARGE_INTEGER frequency;
        LARGE_INTEGER start;
        QueryPerformanceFrequency(&frequency); // Ticks per second
        QueryPerformanceCounter(&start);       // Current tick
        return (double)start.QuadPart * 1000.0 / frequency.QuadPart;
    }

#define MAX_FRAMES 10

typedef struct {
double start_time;
double median_complete_time;
double stop_time;
}time_perf_t;
time_perf_t time_perf_list[MAX_FRAMES];

uint16_t start_frame_idx = 0;
uint16_t stop_frame_idx = 0;

img_ll_struct_t* write_ptr = NULL;
img_ll_struct_t* task1_ptr = NULL;
img_ll_struct_t* task2_ptr=NULL;

const char filename[] = "input_image.bin";



void image_acquisition_task()
{
    static double prev_millis=0;
    //Execute every 100ms 
    if(get_time_ms()-prev_millis>=100.0)//add subtraction with current time here
    {
        if(write_ptr!=NULL){
            if(write_ptr->status==WRITE_READY)
            {
                int ret = load_input_data(filename,write_ptr);
                if(ret==0)
                {

                    write_ptr= write_ptr->nxt_ptr;
                    time_perf_list[start_frame_idx].start_time = get_time_ms();
                    start_frame_idx+=1;
                }

            }
        }
        
    }

}

void task1()
{
    if(task1_ptr==NULL)
    {
        return;
    }
    if(task1_ptr->status==TASK1_READY)
    {
        median_filter_task(task1_ptr);
        if(task1_ptr->status==TASK2_READY)
        {
            time_perf_list[stop_frame_idx].median_complete_time = get_time_ms();
            task1_ptr = task1_ptr->nxt_ptr;
        }
    }

}

void task2()
{
    if(task2_ptr==NULL)
    {
        return;
    }
    if(task2_ptr->status==TASK2_READY)
    {
        edge_detection(task2_ptr);
        if(task2_ptr->status==TASK2_COMPLETE)
        {
            time_perf_list[stop_frame_idx].stop_time = get_time_ms();
            task2_ptr->status = WRITE_READY;
            task2_ptr = task2_ptr->nxt_ptr;
            stop_frame_idx+=1;
        }
    }

}

int main()
{
    printf("System initialization...\n");
    system_init(&write_ptr);
    task1_ptr = write_ptr;
    task2_ptr = write_ptr;
    printf("commencing single core simulation\n");
    while(stop_frame_idx<MAX_FRAMES)
    {
        image_acquisition_task();
        task1();
        task2();
    }

    printf("Simulation complete \n");
    printf("Execution Performance metrics:\n");
    printf("Frame id  , Total execution time, Median time, edge detection time \n");
    for(int i=0;i<MAX_FRAMES;i++)
    {
        printf(" %d        , %lf          , %lf       , %lf  \n ",i,(time_perf_list[i].stop_time-time_perf_list[i].start_time),(time_perf_list[i].median_complete_time-time_perf_list[i].start_time),(time_perf_list[i].stop_time-time_perf_list[i].median_complete_time));
    }
}