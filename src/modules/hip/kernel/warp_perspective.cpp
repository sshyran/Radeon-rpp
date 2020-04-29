#include <hip/hip_runtime.h>
extern "C" __global__ void warp_perspective_pln (   unsigned char* srcPtr,
                             unsigned char* dstPtr,
                              float* perspective,
                            const unsigned int source_height,
                            const unsigned int source_width,
                            const unsigned int dest_height,
                            const unsigned int dest_width,
                            const unsigned int channel
)
{
   int id_x = hipBlockIdx_x *hipBlockDim_x + hipThreadIdx_x;
   int id_y = hipBlockIdx_y *hipBlockDim_y + hipThreadIdx_y;
   int id_z = hipBlockIdx_z *hipBlockDim_z + hipThreadIdx_z;
   
   int xc = id_x - source_width/2;
   int yc = id_y - source_height/2;

   int k ;
   int l ;
   int m ;

   k = (int)((perspective[0] * xc) + (perspective[1] * yc)) + perspective[2];
   l = (int)((perspective[3] * xc) + (perspective[4] * yc)) + perspective[5];
   m = (int)((perspective[6] * xc) + (perspective[7] * yc)) + perspective[8];

   k = ((k / m) + source_width/2);
   l = ((l / m) + source_height/2);
    
   if (l < source_height && l >=0 && k < source_width && k >=0 )
   dstPtr[(id_z * dest_height * dest_width) + (id_y * dest_width) + id_x] =
                            srcPtr[(id_z * source_height * source_width) + (l * source_width) + k];
   else
   dstPtr[(id_z * dest_height * dest_width) + (id_y * dest_width) + id_x] = 0;

}


extern "C" __global__ void warp_perspective_pkd (   unsigned char* srcPtr,
                             unsigned char* dstPtr,
                              float* perspective,
                            const unsigned int source_height,
                            const unsigned int source_width,
                            const unsigned int dest_height,
                            const unsigned int dest_width,
                            const unsigned int channel
)
{

   int id_x = hipBlockIdx_x *hipBlockDim_x + hipThreadIdx_x;
   int id_y = hipBlockIdx_y *hipBlockDim_y + hipThreadIdx_y;
   int id_z = hipBlockIdx_z *hipBlockDim_z + hipThreadIdx_z;
   
   int xc = id_x - source_width/2;
   int yc = id_y - source_height/2;

   int k ;
   int l ;
   int m ;

   k = (int)((perspective[0] * xc) + (perspective[1] * yc)) + perspective[2];
   l = (int)((perspective[3] * xc) + (perspective[4] * yc)) + perspective[5];
   m = (int)((perspective[6] * xc) + (perspective[7] * yc)) + perspective[8];

   k = ((k / m) + source_width / 2);
   l = ((l / m) + source_height / 2);

   if (l < source_height && l >=0 && k < source_width && k >=0 )
   dstPtr[id_z + (channel * id_y * dest_width) + (channel * id_x)] =
                             srcPtr[id_z + (channel * l * source_width) + (channel * k)];
   else
   dstPtr[id_z + (channel * id_y * dest_width) + (channel * id_x)] = 0;

}

extern "C" __global__ void warp_perspective_batch(          unsigned char* srcPtr,
                                     unsigned char* dstPtr,
                                     float *perspective,
                                     unsigned int *source_height,
                                     unsigned int *source_width,
                                     unsigned int *dest_height,
                                     unsigned int *dest_width,
                                     unsigned int *max_source_width,
                                     unsigned int *max_dest_width,
                                     unsigned long *source_batch_index,
                                     unsigned long *dest_batch_index,
                                    const unsigned int channel,
                                     unsigned int *source_inc, // use width * height for pln and 1 for pkd
                                     unsigned int *dest_inc,
                                    const int plnpkdindex // use 1 pln 3 for pkd
                                    )
{
    int id_x = hipBlockIdx_x *hipBlockDim_x + hipThreadIdx_x, id_y = hipBlockIdx_y *hipBlockDim_y + hipThreadIdx_y, id_z = hipBlockIdx_z *hipBlockDim_z + hipThreadIdx_z;
    int indextmp=0;
    unsigned long src_pixIdx = 0, dst_pixIdx = 0;
    
    
    int xc = id_x - dest_width[id_z]/2;
    int yc = id_y - dest_height[id_z]/2;
    int perspective_index = id_z * 6;
   
    int k = (int)((perspective[perspective_index + 0] * xc )+ (perspective[ perspective_index + 1] * yc)) + perspective[perspective_index + 2];
    int l = (int)((perspective[perspective_index + 3] * xc )+ (perspective[ perspective_index + 4] * yc)) + perspective[perspective_index + 5];
    int m = (int)((perspective[perspective_index + 6] * xc )+ (perspective[ perspective_index + 7] * yc)) + perspective[perspective_index + 8];

    k = (k/m) + source_width[id_z]/2;
    l = (l/m) + source_height[id_z]/2;

    src_pixIdx = source_batch_index[id_z] + (id_x  + id_y * max_source_width[id_z] ) * plnpkdindex;
    dst_pixIdx = dest_batch_index[id_z]   + (id_x  + id_y * max_dest_width[id_z] ) * plnpkdindex;
    
    if(l < source_height[id_z] && l >=0 && k < source_width[id_z] && k >=0)
    {   
        for(indextmp = 0; indextmp < channel; indextmp++){
            dstPtr[dst_pixIdx] = srcPtr[src_pixIdx];
            src_pixIdx += source_inc[id_z];
            dst_pixIdx += dest_inc[id_z];
        }
    }
}