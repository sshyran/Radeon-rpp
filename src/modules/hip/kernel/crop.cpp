#include <hip/hip_runtime.h>
#define saturate_8u(value) ((value) > 255 ? 255 : ((value) < 0 ? 0 : (value)))

extern "C" __global__ void crop_batch(
    unsigned char *input,
    unsigned char *output, 
    unsigned int *dst_height, unsigned int *dst_width,
    unsigned int *src_width, unsigned int *start_x,
    unsigned int *start_y, unsigned int *max_src_width,
    unsigned int *max_dst_width,
    unsigned long *src_batch_index,
    unsigned long *dst_batch_index, const unsigned int channel,
    // const unsigned int batch_size,
    unsigned int *src_inc, 
    unsigned int *dst_inc,
    const int in_plnpkdind, const int out_plnpkdind 
) {
  int id_x = hipBlockIdx_x * hipBlockDim_x + hipThreadIdx_x;
  int id_y = hipBlockIdx_y * hipBlockDim_y + hipThreadIdx_y;
  int id_z = hipBlockIdx_z * hipBlockDim_z + hipThreadIdx_z;
  int indextmp = 0;
  unsigned long src_pixIdx =
      src_batch_index[id_z] +
      (id_x + start_x[id_z] + (id_y + start_y[id_z]) * max_src_width[id_z]) *
          in_plnpkdind; 
  unsigned long dst_pixIdx =
      dst_batch_index[id_z] + (id_x + id_y * max_dst_width[id_z]) * out_plnpkdind; 
  if ((id_x < dst_width[id_z]) && (id_y < dst_height[id_z])) {
    for (indextmp = 0; indextmp < channel; indextmp++) {
      output[dst_pixIdx] = input[src_pixIdx];
      src_pixIdx += src_inc[id_z];
      dst_pixIdx += dst_inc[id_z];
    }
  } else {
    for (indextmp = 0; indextmp < channel; indextmp++) {
      output[dst_pixIdx] = 0;
      dst_pixIdx += dst_inc[id_z];
    }
  }
}

extern "C" __global__ void crop_batch_fp32(
    float *input,
    float *output, 
    unsigned int *dst_height, unsigned int *dst_width,
    unsigned int *src_width, unsigned int *start_x,
    unsigned int *start_y, unsigned int *max_src_width,
    unsigned int *max_dst_width,
    unsigned long *src_batch_index,
    unsigned long *dst_batch_index, const unsigned int channel,
    // const unsigned int batch_size,
    unsigned int *src_inc, // use width * height for pln and 1 for pkd
    unsigned int *dst_inc,
    const int in_plnpkdind, const int out_plnpkdind
) {
  int id_x = hipBlockIdx_x * hipBlockDim_x + hipThreadIdx_x;
  int id_y = hipBlockIdx_y * hipBlockDim_y + hipThreadIdx_y;
  int id_z = hipBlockIdx_z * hipBlockDim_z + hipThreadIdx_z;
  int indextmp = 0;
  unsigned long src_pixIdx =
      src_batch_index[id_z] +
      (id_x + start_x[id_z] + (id_y + start_y[id_z]) * max_src_width[id_z]) *
          in_plnpkdind; 
  unsigned long dst_pixIdx =
      dst_batch_index[id_z] +
      (id_x + id_y * max_dst_width[id_z]) *
          out_plnpkdind; 
  if ((id_x < dst_width[id_z]) && (id_y < dst_height[id_z])) {
    for (indextmp = 0; indextmp < channel; indextmp++) {
      output[dst_pixIdx] = input[src_pixIdx];
      src_pixIdx += src_inc[id_z];
      dst_pixIdx += dst_inc[id_z];
    }
  } else {
    for (indextmp = 0; indextmp < channel; indextmp++) {
      output[dst_pixIdx] = 0;
      dst_pixIdx += dst_inc[id_z];
    }
  }
}

// extern "C" __global__ void crop_batch_fp16(
//     half *input,
//     half *output, 
//     unsigned int *dst_height, unsigned int *dst_width,
//     unsigned int *src_width, unsigned int *start_x,
//     unsigned int *start_y, unsigned int *max_src_width,
//     unsigned int *max_dst_width,
//     unsigned long *src_batch_index,
//     unsigned long *dst_batch_index, const unsigned int channel,
//     // const unsigned int batch_size,
//     unsigned int *src_inc, // use width * height for pln and 1 for pkd
//     unsigned int *dst_inc,
//     const int in_plnpkdind, const int out_plnpkdind
// ) {
//   int id_x = hipBlockIdx_x * hipBlockDim_x + hipThreadIdx_x;
//   int id_y = hipBlockIdx_y * hipBlockDim_y + hipThreadIdx_y;
//   int id_z = hipBlockIdx_z * hipBlockDim_z + hipThreadIdx_z;
//   int indextmp = 0;
//   unsigned long src_pixIdx =
//       src_batch_index[id_z] +
//       (id_x + start_x[id_z] + (id_y + start_y[id_z]) * max_src_width[id_z]) *
//           in_plnpkdind; 
//   unsigned long dst_pixIdx =
//       dst_batch_index[id_z] +
//       (id_x + id_y * max_dst_width[id_z]) *
//           out_plnpkdind; 
//   if ((id_x < dst_width[id_z]) && (id_y < dst_height[id_z])) {
//     for (indextmp = 0; indextmp < channel; indextmp++) {
//       output[dst_pixIdx] = input[src_pixIdx];
//       src_pixIdx += src_inc[id_z];
//       dst_pixIdx += dst_inc[id_z];
//     }
//   } else {
//     for (indextmp = 0; indextmp < channel; indextmp++) {
//       output[dst_pixIdx] = 0;
//       dst_pixIdx += dst_inc[id_z];
//     }
//   }
// }

extern "C" __global__ void crop_batch_int8(
    char *input,
    char *output, 
    unsigned int *dst_height, unsigned int *dst_width,
    unsigned int *src_width, unsigned int *start_x,
    unsigned int *start_y, unsigned int *max_src_width,
    unsigned int *max_dst_width,
    unsigned long *src_batch_index,
    unsigned long *dst_batch_index, const unsigned int channel,
    // const unsigned int batch_size,
    unsigned int *src_inc, // use width * height for pln and 1 for pkd
    unsigned int *dst_inc,
    const int in_plnpkdind, const int out_plnpkdind
) {
  int id_x = hipBlockIdx_x * hipBlockDim_x + hipThreadIdx_x;
  int id_y = hipBlockIdx_y * hipBlockDim_y + hipThreadIdx_y;
  int id_z = hipBlockIdx_z * hipBlockDim_z + hipThreadIdx_z;
  int indextmp = 0;
  unsigned long src_pixIdx =
      src_batch_index[id_z] +
      (id_x + start_x[id_z] + (id_y + start_y[id_z]) * max_src_width[id_z]) *
          in_plnpkdind; 
  unsigned long dst_pixIdx = dst_batch_index[id_z] +
      (id_x + id_y * max_dst_width[id_z]) * out_plnpkdind; 
  if ((id_x < dst_width[id_z]) && (id_y < dst_height[id_z])) {
    for (indextmp = 0; indextmp < channel; indextmp++) {
      output[dst_pixIdx] = input[src_pixIdx];
      src_pixIdx += src_inc[id_z];
      dst_pixIdx += dst_inc[id_z];
    }
  } else {
    for (indextmp = 0; indextmp < channel; indextmp++) {
      output[dst_pixIdx] = -128;
      dst_pixIdx += dst_inc[id_z];
    }
  }
}

extern "C" __global__ void crop_batch_u8_fp32(
    unsigned char *input, float *output,
    unsigned int *dst_height, unsigned int *dst_width,
    unsigned int *src_width, unsigned int *start_x,
    unsigned int *start_y, unsigned int *max_src_width,
    unsigned int *max_dst_width,
    unsigned long *src_batch_index,
    unsigned long *dst_batch_index, const unsigned int channel,
    // const unsigned int batch_size,
    unsigned int *src_inc, unsigned int *dst_inc,
    const int in_plnpkdind, const int out_plnpkdind) {
  int id_x = hipBlockIdx_x * hipBlockDim_x + hipThreadIdx_x;
  int id_y = hipBlockIdx_y * hipBlockDim_y + hipThreadIdx_y;
  int id_z = hipBlockIdx_z * hipBlockDim_z + hipThreadIdx_z;
  int indextmp = 0;
  unsigned long src_pixIdx =
      src_batch_index[id_z] +
      (id_x + start_x[id_z] + (id_y + start_y[id_z]) * max_src_width[id_z]) *
          in_plnpkdind;
  unsigned long dst_pixIdx =
      dst_batch_index[id_z] + (id_x + id_y * max_dst_width[id_z]) * out_plnpkdind;
  if ((id_x < dst_width[id_z]) && (id_y < dst_height[id_z])) {
    for (indextmp = 0; indextmp < channel; indextmp++) {
      output[dst_pixIdx] = input[src_pixIdx] / 255.0;
      src_pixIdx += src_inc[id_z];
      dst_pixIdx += dst_inc[id_z];
    }
  } else {
    for (indextmp = 0; indextmp < channel; indextmp++) {
      output[dst_pixIdx] = 0.0;
      dst_pixIdx += dst_inc[id_z];
    }
  }
}

// extern "C" __global__ void crop_batch_u8_fp16(
//     unsigned char *input, half *output,
//     unsigned int *dst_height, unsigned int *dst_width,
//     unsigned int *src_width, unsigned int *start_x,
//     unsigned int *start_y, unsigned int *max_src_width,
//     unsigned int *max_dst_width,
//     unsigned long *src_batch_index,
//     unsigned long *dst_batch_index, const unsigned int channel,
//     // const unsigned int batch_size,
//     unsigned int *src_inc, unsigned int *dst_inc,
//     const int in_plnpkdind, const int out_plnpkdind) {
//   int id_x = hipBlockIdx_x * hipBlockDim_x + hipThreadIdx_x;
//   int id_y = hipBlockIdx_y * hipBlockDim_y + hipThreadIdx_y;
//   int id_z = hipBlockIdx_z * hipBlockDim_z + hipThreadIdx_z;
//   int indextmp = 0;
//   unsigned long src_pixIdx =
//       src_batch_index[id_z] +
//       (id_x + start_x[id_z] + (id_y + start_y[id_z]) * max_src_width[id_z]) *
//           in_plnpkdind;
//   unsigned long dst_pixIdx =
//       dst_batch_index[id_z] + (id_x + id_y * max_dst_width[id_z]) * out_plnpkdind;
//   if ((id_x < dst_width[id_z]) && (id_y < dst_height[id_z])) {
//     for (indextmp = 0; indextmp < channel; indextmp++) {
//       output[dst_pixIdx] = (half)(input[src_pixIdx] / 255.0);
//       src_pixIdx += src_inc[id_z];
//       dst_pixIdx += dst_inc[id_z];
//     }
//   } else {
//     for (indextmp = 0; indextmp < channel; indextmp++) {
//       output[dst_pixIdx] = (half)0.0;
//       dst_pixIdx += dst_inc[id_z];
//     }
//   }
// }

extern "C" __global__ void crop_batch_u8_int8(
    unsigned char *input, char *output,
    unsigned int *dst_height, unsigned int *dst_width,
    unsigned int *src_width, unsigned int *start_x,
    unsigned int *start_y, unsigned int *max_src_width,
    unsigned int *max_dst_width,
    unsigned long *src_batch_index,
    unsigned long *dst_batch_index, const unsigned int channel,
    // const unsigned int batch_size,
    unsigned int *src_inc, unsigned int *dst_inc,
    const int in_plnpkdind, const int out_plnpkdind) {
  int id_x = hipBlockIdx_x * hipBlockDim_x + hipThreadIdx_x;
  int id_y = hipBlockIdx_y * hipBlockDim_y + hipThreadIdx_y;
  int id_z = hipBlockIdx_z * hipBlockDim_z + hipThreadIdx_z;
  int indextmp = 0;
  unsigned long src_pixIdx =
      src_batch_index[id_z] +
      (id_x + start_x[id_z] + (id_y + start_y[id_z]) * max_src_width[id_z]) *
          in_plnpkdind;
  unsigned long dst_pixIdx =
      dst_batch_index[id_z] + (id_x + id_y * max_dst_width[id_z]) * out_plnpkdind;
  if ((id_x < dst_width[id_z]) && (id_y < dst_height[id_z])) {
    for (indextmp = 0; indextmp < channel; indextmp++) {
      output[dst_pixIdx] = input[src_pixIdx] - 128;
      src_pixIdx += src_inc[id_z];
      dst_pixIdx += dst_inc[id_z];
    }
  } else {
    for (indextmp = 0; indextmp < channel; indextmp++) {
      output[dst_pixIdx] = -128;
      dst_pixIdx += dst_inc[id_z];
    }
  }
}