#include <hip/hip_runtime.h>
#define saturate_8u(value) ( (value) > 255 ? 255 : ((value) < 0 ? 0 : (value) ))
__device__ unsigned int xorshift(int pixid) {
    unsigned int x = 123456789;
    unsigned int w = 88675123;
    unsigned int seed = x + pixid;
    unsigned int t = seed ^ (seed << 11);
    unsigned int res = w ^ (w >> 19) ^ (t ^(t >> 8));
	return res;
}
extern "C" __global__ void gaussian(   unsigned char* input1,
                     unsigned char* input2,
                     unsigned char* output,
                    const unsigned int height,
                    const unsigned int width,
                    const float mean,
                    const float sigma,
                    const unsigned int channel
)
{
    int id_x = hipBlockIdx_x *hipBlockDim_x + hipThreadIdx_x;
    int id_y = hipBlockIdx_y *hipBlockDim_y + hipThreadIdx_y;
    int id_z = hipBlockIdx_z *hipBlockDim_z + hipThreadIdx_z;
    if (id_x >= width || id_y >= height || id_z >= channel) return;

    int pixIdx = id_x + id_y * width + id_z * width * height;

    float res = input1[pixIdx] + input2[pixIdx];
    output[pixIdx] = saturate_8u(res);
}

extern "C" __global__ void snp_pkd(   unsigned char* input,
                     unsigned char* output,
                    const unsigned int height,
                    const unsigned int width,
                    const unsigned int channel,
                    const unsigned int pixelDistance
)
{
    int id_x = hipBlockIdx_x *hipBlockDim_x + hipThreadIdx_x;
    int id_y = hipBlockIdx_y *hipBlockDim_y + hipThreadIdx_y;
    int id_z = hipBlockIdx_z *hipBlockDim_z + hipThreadIdx_z;
    if (id_x >= width || id_y >= height) return;

    int pixIdx = id_y * width * channel + id_x * channel;
    int rand;
    
    if(pixIdx % pixelDistance == 0 )
    {
        int rand_id = xorshift(pixIdx) % (60 * pixelDistance);
        rand_id -= rand_id % 3;
        rand = (rand_id % 2) ? 0 : 255;
        for(int i = 0 ; i < channel ; i++)
            output[pixIdx + i + rand_id] = rand;
    }
}




extern "C" __global__ void snp_pln(   unsigned char* input,
                     unsigned char* output,
                    const unsigned int height,
                    const unsigned int width,
                    const unsigned int channel,
                    const unsigned int pixelDistance
)
{
    int id_x = hipBlockIdx_x *hipBlockDim_x + hipThreadIdx_x;
    int id_y = hipBlockIdx_y *hipBlockDim_y + hipThreadIdx_y;
    int id_z = hipBlockIdx_z *hipBlockDim_z + hipThreadIdx_z;
    if (id_x >= width || id_y >= height) return;

    int pixIdx = id_y * width + id_x;
    int channelSize = width * height;
    
    int rand;
    
    if(pixIdx % pixelDistance == 0 )
    {
        int rand_id = xorshift(pixIdx) % (60 * pixelDistance);
        rand_id -= rand_id % 3;
        rand = (rand_id % 2) ? 0 : 255;
        for(int i = 0 ; i < channel ; i++)
            output[pixIdx + channelSize * i + rand_id] = rand;
    }
}

extern "C" __global__ void noise_batch( unsigned char* input,
                                     unsigned char* output,
                                     float *noiseProbability,
                                     int *xroi_begin,
                                     int *xroi_end,
                                     int *yroi_begin,
                                     int *yroi_end,
                                     unsigned int *height,
                                     unsigned int *width,
                                     unsigned int *max_width,
                                     unsigned long *batch_index,
                                    const unsigned int channel,
                                     unsigned int *inc, // use width * height for pln and 1 for pkd
                                    const int plnpkdindex // use 1 pln 3 for pkd
                                    )
{
    int id_x = hipBlockIdx_x * hipBlockDim_x + hipThreadIdx_x;
    int id_y = hipBlockIdx_y * hipBlockDim_y + hipThreadIdx_y;
    int id_z = hipBlockIdx_z * hipBlockDim_z + hipThreadIdx_z;   
    float probTemp = noiseProbability[id_z];
    int indextmp=0;
    long pixIdx = 0;
    int rand;
    pixIdx = batch_index[id_z] + (id_x  + id_y * max_width[id_z] ) * plnpkdindex ;
    for(indextmp = 0; indextmp < channel; indextmp++)
    {
        output[pixIdx] = input[pixIdx];
        pixIdx += inc[id_z];
    }
    if(id_x < width[id_z] && id_y < height[id_z])
    {
        pixIdx = batch_index[id_z] + (id_x  + id_y * max_width[id_z] ) * plnpkdindex ;
        if((id_y >= yroi_begin[id_z] ) && (id_y <= yroi_end[id_z]) && (id_x >= xroi_begin[id_z]) && (id_x <= xroi_end[id_z]))
        {   
            float noisePixel = probTemp * (float)(width[id_z] * height[id_z]);
            float pixelDistance = 1.0;
            pixelDistance /=  probTemp;
            if(((pixIdx - batch_index[id_z]) % (int)pixelDistance) == 0)
            {
                int rand_id = xorshift(pixIdx) % (int)(60 * pixelDistance);
                rand = (rand_id % 2) ? 0 : 255;
                rand_id = rand_id % (int)pixelDistance;
                rand_id -= rand_id % 3;
                rand_id = rand_id * plnpkdindex;
                for(indextmp = 0; indextmp < channel; indextmp++)
                {
                    output[pixIdx + rand_id] = rand;
                    pixIdx += inc[id_z];
                }
            }
            else
            {
                for(indextmp = 0; indextmp < channel; indextmp++)
                {
                    output[pixIdx] = input[pixIdx];
                    pixIdx += inc[id_z];
                }

            }
        }
    }
    else {
        pixIdx = batch_index[id_z]   + (id_x  + id_y * max_width[id_z] ) * plnpkdindex;
        for(indextmp = 0; indextmp < channel; indextmp++){
            output[pixIdx] = 0;
            pixIdx +=  inc[id_z];
        }
    }
}