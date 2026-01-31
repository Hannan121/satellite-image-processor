#include "system_def.h"
#include<string.h>

static uint8_t raw_img[BUFF_SIZE][IMG_HEIGHT][IMG_WIDTH];
static uint8_t processed_img[BUFF_SIZE][IMG_HEIGHT][IMG_WIDTH];
static img_ll_struct_t descriptor_queue[BUFF_SIZE];

static inference_list_t inference_pool[BUFF_SIZE];

void system_init(img_ll_struct_t** head_ref)
{
    for(int i = 0;i<BUFF_SIZE;i++)
    {
        descriptor_queue[i].raw_img_ptr = &raw_img[i][0][0];
        descriptor_queue[i].processed_img_ptr = &processed_img[i][0][0];
        descriptor_queue[i].status = WRITE_READY;
        descriptor_queue[i].inference_ptr = &inference_pool[i];
        if(i==(BUFF_SIZE-1))
        {
            descriptor_queue[i].nxt_ptr = &descriptor_queue[0];
        }
        else
        {
            descriptor_queue[i].nxt_ptr = &descriptor_queue[i+1];
        }
    }
    *head_ref = &descriptor_queue[0];

}

//load image
int load_input_data(const char* filename, img_ll_struct_t* node) {
    FILE* f = fopen(filename, "rb"); // "rb" is critical for binary
    if (!f) {
        printf("[ERROR] Could not open %s. Run the Python conversion script first.\n", filename);
        return -1;
    }

    // Read exactly 3124 * 3030 bytes directly into the raw buffer
    // This simulates a DMA transfer from the camera
    size_t pixels_read = fread(node->raw_img_ptr, 1, IMG_WIDTH * IMG_HEIGHT, f);
    memcpy(node->processed_img_ptr, node->raw_img_ptr, IMG_HEIGHT * IMG_WIDTH);
    
    fclose(f);

    if (pixels_read != (IMG_WIDTH * IMG_HEIGHT)) {
        printf("[WARN] File size mismatch! Read %zu pixels, expected %d.\n", 
               pixels_read, IMG_WIDTH * IMG_HEIGHT);
        return -2;
    }

    printf("[SYS] Loaded test vector: %s (%zu bytes)\n", filename, pixels_read);
    node->status = TASK1_READY;
    return 0;
}




//Median filters for subregion of 100x100, smaller roi for corner cases
//Using histogram for faster compute

void median_filter_task(img_ll_struct_t* item) {
    uint8_t* raw_ptr = item->raw_img_ptr;
    uint8_t* out_ptr = item->processed_img_ptr;
    
    // Histogram for finding median 
    uint32_t histogram[HIST_SIZE]; 

    // Loop through the image in 100x100 region
    for (int y_start = 0; y_start < IMG_HEIGHT; y_start += MEDIAN_SUBFRAME_SIZE) {
        for (int x_start = 0; x_start < IMG_WIDTH; x_start += MEDIAN_SUBFRAME_SIZE) {
            
            //Block boundary conditions handling, either 100x100 or lower
            int y_end = (y_start + MEDIAN_SUBFRAME_SIZE < IMG_HEIGHT) ? y_start + MEDIAN_SUBFRAME_SIZE : IMG_HEIGHT;
            int x_end = (x_start + MEDIAN_SUBFRAME_SIZE < IMG_WIDTH) ? x_start + MEDIAN_SUBFRAME_SIZE : IMG_WIDTH;
            
            // Clear/Reset Histogram
            memset(histogram, 0, sizeof(histogram));
            int total_pixels_in_block = 0;

            //Build Histogram for subframe
            for (int y = y_start; y < y_end; y++) {
                int row_offset = y * IMG_WIDTH;
                for (int x = x_start; x < x_end; x++) {
                    histogram[raw_ptr[row_offset + x]]++;
                    total_pixels_in_block++;
                }
            }

            //Find Median Value for subframe
            int count = 0;
            int median_val = 0;
            int threshold = total_pixels_in_block / 2;//middle value for the region
            
            for (int i = 0; i < 256; i++) {
                count += histogram[i];
                if (count >= threshold) {
                    median_val = i;
                    break;
                }
            }

            // Subtract median value from every pixel of subframe
            for (int y = y_start; y < y_end; y++) {
                int row_offset = y * IMG_WIDTH;
                for (int x = x_start; x < x_end; x++) {
                    uint8_t pixel = raw_ptr[row_offset + x];
                    // Subtract & Clamp to 0
                    out_ptr[row_offset + x] = (pixel > median_val) ? (pixel - median_val) : 0;
                }
            }
        }
    }
    
    // Task Complete - Hand off to Edge Detection
    item->status = TASK2_READY;
}


/* Task 2 
*Gaussian kernel- Binomial filter pascal traingle approximation to give a sum of 256 makes division 
*computationally less expensive with the use of bit shifts
*2d Gaussian kernel further sub divided into two 1d kernels for lesser operations
*Horizontal and vertical pass
*
*Sobel and inference plotted in same loop , static edge threshold used due to computational limitations
*/

static const uint8_t GAUSS_1D_KERNEL[5] = {
    1 ,4, 6, 4, 1
};

static uint16_t gauss_hor_out[IMG_HEIGHT][IMG_WIDTH];
static uint8_t gauss_out[IMG_HEIGHT][IMG_WIDTH];

//math helper absolute value function
static inline int abs(int val)
{
    return (val>0)? val:-1*val;
}


void edge_detection(img_ll_struct_t* item)
{
if(item->status!=TASK2_READY)
{
    printf("[Error] Buffer not ready for task 2!\n ");
}

int gauss_margin =2;
uint8_t* img = item->processed_img_ptr;

//horizontal pass
for(int y = 0;y < IMG_HEIGHT;y++)
{
    
    for(int x=gauss_margin;x<IMG_WIDTH-gauss_margin;x++)
    {
        uint16_t sum = img[y*IMG_WIDTH + x-2]*GAUSS_1D_KERNEL[0]
        + img[y*IMG_WIDTH + x-1]*GAUSS_1D_KERNEL[1]
        + img[y*IMG_WIDTH + x]*GAUSS_1D_KERNEL[2]
        + img[y*IMG_WIDTH + x+1]*GAUSS_1D_KERNEL[3]
        + img[y*IMG_WIDTH + x+2]*GAUSS_1D_KERNEL[4];
        gauss_hor_out[y][x] = sum;
    }
}

//vertical pass
for(int y = gauss_margin;y < IMG_HEIGHT-gauss_margin;y++)
{
    
    for(int x=gauss_margin;x<IMG_WIDTH-gauss_margin;x++)
    {
        uint16_t sum = gauss_hor_out[(y-2)][x]*GAUSS_1D_KERNEL[0]
        + gauss_hor_out[(y-1)][x]*GAUSS_1D_KERNEL[1]
        + gauss_hor_out[(y)][x]*GAUSS_1D_KERNEL[2]
        + gauss_hor_out[(y+1)][x]*GAUSS_1D_KERNEL[3]
        + gauss_hor_out[(y+2)][x]*GAUSS_1D_KERNEL[4];
        gauss_out[y][x] = (uint8_t)((sum+128)>>8);//avoids rounding down
    }
}

//sobel

int s_margin = gauss_margin + 1;
    
    // Reset Telemetry Count for this frame
    inference_list_t* infer = item->inference_ptr; 
    infer->count = 0;
    for (int y = s_margin; y < IMG_HEIGHT - s_margin; y++) {
        uint8_t* row_m1 = &gauss_out[y - 1][0];
        uint8_t* row_0  = &gauss_out[y][0];
        uint8_t* row_p1 = &gauss_out[y + 1][0];
        for (int x = s_margin; x < IMG_WIDTH - s_margin; x++) {
            
            

            // horizontal gradient gx(detect vertical edges)
            int16_t gx = (int16_t)(row_m1[x+1]) - (int16_t)(row_m1[x-1])
                        +2*((int16_t)(row_0[x+1]) - (int16_t)(row_0[x-1]))
                        +(int16_t)(row_p1[x+1]) - (int16_t)(row_p1[x-1]);
            
            //vertical gradient gy
            
            int16_t gy = (int16_t)(row_p1[x-1])+2*(int16_t)(row_p1[x])+(int16_t)(row_p1[x+1]) 
                        -((int16_t)(row_m1[x-1])+2*(int16_t)(row_m1[x])+(int16_t)(row_m1[x+1])) ;

            // Compute Magnitude
            int mag = abs(gx) + abs(gy);
            
            // Clamp to 8-bit
            uint8_t final_val = (mag > 255) ? 255 : (uint8_t)mag;
            
            // STORE IMAGE (Visual Output)
            uint32_t offset = y * IMG_WIDTH + x;
            item->processed_img_ptr[offset] = final_val;

            // 6. INFERENCE GENERATION (Telemetry Output)
            // "Fuse" this logic here to save memory reads.
            // Threshold > 20 is just an example; tune as needed.
            if (final_val >= EDGE_THRESHOLD) {
                if (infer->count < MAX_EDGES) {
                    infer->inference[infer->count].pixel_address = offset;
                    infer->inference[infer->count].pixel_value = final_val;
                    infer->count++;
                }
            }
        }
    }
    
    // Task Complete
    item->status = TASK2_COMPLETE;

    //Dump data out once
    static int frame0_dumped = 0;
    if (!frame0_dumped) {
        printf("[DEBUG] Dumping Frame 0 inference & Image...\n");
        
        // Save Image
        FILE* f = fopen("output_frame0.pgm", "wb");
        if(f) {
            fprintf(f, "P5\n%d %d\n255\n", IMG_WIDTH, IMG_HEIGHT);
            fwrite(item->processed_img_ptr, 1, IMG_WIDTH*IMG_HEIGHT, f);
            fclose(f);
        }
        
        // Save Inference
        FILE* fcsv = fopen("output_inference0.csv", "w");
        if(fcsv) {
            fprintf(fcsv, "Address,Value\n");
            for(int k=0; k<infer->count; k++) {
                fprintf(fcsv, "%u,%u\n", infer->inference[k].pixel_address, 
                                         infer->inference[k].pixel_value);
            }
            fclose(fcsv);
        }
        frame0_dumped = 1;
    }


}