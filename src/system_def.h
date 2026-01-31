#ifndef SYSTEM_DEF_H
#define SYSTEM_DEF_H

#include <stdio.h>
#include <stdint.h>

//image size and linked list buffer size config
#define IMG_WIDTH 3124
#define IMG_HEIGHT 3030
#define BUFF_SIZE 4

//Median filtering config
#define HIST_SIZE 256 
#define MEDIAN_SUBFRAME_SIZE 100



typedef enum {
WRITE_READY = 0,
TASK1_READY,
TASK2_READY,
TASK2_COMPLETE
}buffer_state_t;


#define MAX_EDGES 50000
#define EDGE_THRESHOLD 40
typedef struct{
    uint32_t pixel_address;
    uint8_t pixel_value;
}inference_t;
typedef struct{
    uint32_t count;
    inference_t inference[MAX_EDGES];
}inference_list_t;
//ring buffer linked list memeber struct definition
typedef struct img_ll_struct{
    struct img_ll_struct* nxt_ptr;
    uint8_t* raw_img_ptr;
    uint8_t* processed_img_ptr;
    buffer_state_t status;
    inference_list_t* inference_ptr;
    // uint16_t rows;
    // uint16_t columns;
}img_ll_struct_t;

void system_init(img_ll_struct_t** head_ref);
int load_input_data(const char* filename, img_ll_struct_t* node);
void median_filter_task(img_ll_struct_t* item);
void edge_detection(img_ll_struct_t* item);

#endif