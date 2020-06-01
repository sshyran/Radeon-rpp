#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/opencv.hpp>
#include <iostream>
#include "/opt/rocm/rpp/include/rppi.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>
#include <CL/cl.hpp>
#include <half.hpp>

using namespace cv;
using namespace std;
using half_float::half;

typedef half Rpp16f;

#define RPPPIXELCHECK(pixel) (pixel < (Rpp32f)0) ? ((Rpp32f)0) : ((pixel < (Rpp32f)255) ? pixel : ((Rpp32f)255))

int main(int argc, char **argv)
{
    const int MIN_ARG_COUNT = 6;
    printf("\nUsage: ./BatchPD_ocl <src1 folder> <src2 folder (place same as src1 folder for single image functionalities)> <dst folder> <u8 = 0 / f16 = 1 / f32 = 2> <case number = 1:7>\n");
    if (argc < MIN_ARG_COUNT)
    {
        printf("\nImproper Usage! Needs all arguments!\n");
        return -1;
    }

    printf("\nsrc1 = %s", argv[1]);
    printf("\nsrc2 = %s", argv[2]);
    printf("\ndst = %s", argv[3]);
    printf("\nu8/f16/f32 (0/1/2) = %s", argv[4]);
    printf("\ncase number (1:7) = %s", argv[5]);

    char *src = argv[1];
    char *src_second = argv[2];
    char *dst = argv[3];
    int ip_bitDepth = atoi(argv[4]);
    int test_case = atoi(argv[5]);

    int ip_channel = 3;

    char funcType[1000] = {"BatchPD_OCL_PKD3"};

    char funcName[1000];
    switch (test_case)
    {
    case 1:
        strcpy(funcName, "rotate");
        break;
    case 2:
        strcpy(funcName, "resize");
        break;
    case 3:
        strcpy(funcName, "resize_crop");
        break;
    case 4:
        strcpy(funcName, "resize_crop_mirror");
        break;
    case 5:
        strcpy(funcName, "crop");
        break;
    case 6:
        strcpy(funcName, "crop_mirror_normalize");
        break;
    case 7:
        strcpy(funcName, "color_twist");
        break;
    }

    if (ip_bitDepth == 0)
    {
        strcat(funcName, "_u8_");
    }
    else if (ip_bitDepth == 1)
    {
        strcat(funcName, "_f16_");
    }
    else if (ip_bitDepth == 2)
    {
        strcat(funcName, "_f32_");
    }

    char func[1000];
    strcpy(func, funcName);
    strcat(func, funcType);
    printf("\n\nRunning %s...", func);

    int missingFuncFlag = 0;

    int i = 0, j = 0;
    int minHeight = 30000, minWidth = 30000, maxHeight = 0, maxWidth = 0;
    int minDstHeight = 30000, minDstWidth = 30000, maxDstHeight = 0, maxDstWidth = 0;
    unsigned long long count = 0;
    unsigned long long ioBufferSize = 0;
    unsigned long long oBufferSize = 0;
    static int noOfImages = 0;

    Mat image, image_second;

    struct dirent *de;
    char src1[1000];
    strcpy(src1, src);
    strcat(src1, "/");
    char src1_second[1000];
    strcpy(src1_second, src_second);
    strcat(src1_second, "/");
    strcat(funcName, funcType);
    strcat(dst, "/");
    strcat(dst, funcName);

    DIR *dr = opendir(src);
    while ((de = readdir(dr)) != NULL)
    {
        if (strcmp(de->d_name, ".") == 0 || strcmp(de->d_name, "..") == 0)
            continue;
        noOfImages += 1;
    }
    closedir(dr);

    RppiSize *srcSize = (RppiSize *)calloc(noOfImages, sizeof(RppiSize));
    RppiSize *dstSize = (RppiSize *)calloc(noOfImages, sizeof(RppiSize));
    const int images = noOfImages;
    char imageNames[images][1000];

    DIR *dr1 = opendir(src);
    while ((de = readdir(dr1)) != NULL)
    {
        if (strcmp(de->d_name, ".") == 0 || strcmp(de->d_name, "..") == 0)
            continue;
        strcpy(imageNames[count], de->d_name);
        char temp[1000];
        strcpy(temp, src1);
        strcat(temp, imageNames[count]);
        if (ip_channel == 3)
        {
            image = imread(temp, 1);
        }
        else
        {
            image = imread(temp, 0);
        }
        srcSize[count].height = image.rows;
        srcSize[count].width = image.cols;
        if (maxHeight < srcSize[count].height)
            maxHeight = srcSize[count].height;
        if (maxWidth < srcSize[count].width)
            maxWidth = srcSize[count].width;
        if (minHeight > srcSize[count].height)
            minHeight = srcSize[count].height;
        if (minWidth > srcSize[count].width)
            minWidth = srcSize[count].width;

        dstSize[count].height = image.rows;
        dstSize[count].width = image.cols;
        if (maxDstHeight < dstSize[count].height)
            maxDstHeight = dstSize[count].height;
        if (maxDstWidth < dstSize[count].width)
            maxDstWidth = dstSize[count].width;
        if (minDstHeight > dstSize[count].height)
            minDstHeight = dstSize[count].height;
        if (minDstWidth > dstSize[count].width)
            minDstWidth = dstSize[count].width;

        count++;
    }
    closedir(dr1);

    ioBufferSize = (unsigned long long)maxHeight * (unsigned long long)maxWidth * (unsigned long long)ip_channel * (unsigned long long)noOfImages;
    oBufferSize = (unsigned long long)maxDstHeight * (unsigned long long)maxDstWidth * (unsigned long long)ip_channel * (unsigned long long)noOfImages;

    Rpp8u *input = (Rpp8u *)calloc(ioBufferSize, sizeof(Rpp8u));
    Rpp8u *input_second = (Rpp8u *)calloc(ioBufferSize, sizeof(Rpp8u));
    Rpp8u *output = (Rpp8u *)calloc(oBufferSize, sizeof(Rpp8u));

    Rpp16f *inputf16 = (Rpp16f *)calloc(ioBufferSize, sizeof(Rpp16f));
    Rpp16f *inputf16_second = (Rpp16f *)calloc(ioBufferSize, sizeof(Rpp16f));
    Rpp16f *outputf16 = (Rpp16f *)calloc(oBufferSize, sizeof(Rpp16f));

    Rpp32f *inputf32 = (Rpp32f *)calloc(ioBufferSize, sizeof(Rpp32f));
    Rpp32f *inputf32_second = (Rpp32f *)calloc(ioBufferSize, sizeof(Rpp32f));
    Rpp32f *outputf32 = (Rpp32f *)calloc(oBufferSize, sizeof(Rpp32f));

    RppiSize maxSize, maxDstSize;
    maxSize.height = maxHeight;
    maxSize.width = maxWidth;
    maxDstSize.height = maxDstHeight;
    maxDstSize.width = maxDstWidth;

    DIR *dr2 = opendir(src);
    DIR *dr2_second = opendir(src_second);
    count = 0;
    i = 0;
    while ((de = readdir(dr2)) != NULL)
    {
        if (strcmp(de->d_name, ".") == 0 || strcmp(de->d_name, "..") == 0)
            continue;

        count = (unsigned long long)i * (unsigned long long)maxHeight * (unsigned long long)maxWidth * (unsigned long long)ip_channel;

        char temp[1000];
        strcpy(temp, src1);
        strcat(temp, de->d_name);

        char temp_second[1000];
        strcpy(temp_second, src1_second);
        strcat(temp_second, de->d_name);

        if (ip_channel == 3)
        {
            image = imread(temp, 1);
            image_second = imread(temp_second, 1);
        }
        else
        {
            image = imread(temp, 0);
            image_second = imread(temp_second, 0);
        }

        Rpp8u *ip_image = image.data;
        Rpp8u *ip_image_second = image_second.data;
        for (j = 0; j < srcSize[i].height; j++)
        {
            for (int x = 0; x < srcSize[i].width; x++)
            {
                for (int y = 0; y < ip_channel; y++)
                {
                    input[count + ((j * maxWidth * ip_channel) + (x * ip_channel) + y)] = ip_image[(j * srcSize[i].width * ip_channel) + (x * ip_channel) + y];
                    input_second[count + ((j * maxWidth * ip_channel) + (x * ip_channel) + y)] = ip_image_second[(j * srcSize[i].width * ip_channel) + (x * ip_channel) + y];
                }
            }
        }
        i++;
    }
    closedir(dr2);

    if (ip_bitDepth == 1)
    {
        Rpp8u *inputTemp, *input_secondTemp;
        Rpp16f *inputf16Temp, *inputf16_secondTemp;

        inputTemp = input;
        input_secondTemp = input_second;

        inputf16Temp = inputf16;
        inputf16_secondTemp = inputf16_second;

        for (int i = 0; i < ioBufferSize; i++)
        {
            *inputf16Temp = (Rpp16f)*inputTemp / 255.0;
            *inputf16_secondTemp = (Rpp16f)*input_secondTemp / 255.0;
            inputTemp++;
            inputf16Temp++;
            input_secondTemp++;
            inputf16_secondTemp++;
        }
    }
    else if (ip_bitDepth == 2)
    {
        Rpp8u *inputTemp, *input_secondTemp;
        Rpp32f *inputf32Temp, *inputf32_secondTemp;

        inputTemp = input;
        input_secondTemp = input_second;

        inputf32Temp = inputf32;
        inputf32_secondTemp = inputf32_second;

        for (int i = 0; i < ioBufferSize; i++)
        {
            *inputf32Temp = (Rpp32f)*inputTemp / 255.0;
            *inputf32_secondTemp = (Rpp32f)*input_secondTemp / 255.0;
            inputTemp++;
            inputf32Temp++;
            input_secondTemp++;
            inputf32_secondTemp++;
        }
    }

    cl_mem d_input, d_inputf16, d_inputf32, d_input_second, d_output, d_outputf16, d_outputf32;
    cl_platform_id platform_id;
    cl_device_id device_id;
    cl_context theContext;
    cl_command_queue theQueue;
    cl_int err;
    err = clGetPlatformIDs(1, &platform_id, NULL);
    err = clGetDeviceIDs(platform_id, CL_DEVICE_TYPE_GPU, 1, &device_id, NULL);
    theContext = clCreateContext(0, 1, &device_id, NULL, NULL, &err);
    theQueue = clCreateCommandQueue(theContext, device_id, 0, &err);
    d_input = clCreateBuffer(theContext, CL_MEM_READ_ONLY, ioBufferSize * sizeof(Rpp8u), NULL, NULL);
    d_inputf16 = clCreateBuffer(theContext, CL_MEM_READ_ONLY, ioBufferSize * sizeof(Rpp16f), NULL, NULL);
    d_inputf32 = clCreateBuffer(theContext, CL_MEM_READ_ONLY, ioBufferSize * sizeof(Rpp32f), NULL, NULL);
    d_input_second = clCreateBuffer(theContext, CL_MEM_READ_ONLY, ioBufferSize * sizeof(Rpp8u), NULL, NULL);
    d_output = clCreateBuffer(theContext, CL_MEM_READ_ONLY, oBufferSize * sizeof(Rpp8u), NULL, NULL);
    d_outputf16 = clCreateBuffer(theContext, CL_MEM_READ_ONLY, oBufferSize * sizeof(Rpp16f), NULL, NULL);
    d_outputf32 = clCreateBuffer(theContext, CL_MEM_READ_ONLY, oBufferSize * sizeof(Rpp32f), NULL, NULL);
    err = clEnqueueWriteBuffer(theQueue, d_input, CL_TRUE, 0, ioBufferSize * sizeof(Rpp8u), input, 0, NULL, NULL);
    err = clEnqueueWriteBuffer(theQueue, d_inputf16, CL_TRUE, 0, ioBufferSize * sizeof(Rpp16f), inputf16, 0, NULL, NULL);
    err = clEnqueueWriteBuffer(theQueue, d_inputf32, CL_TRUE, 0, ioBufferSize * sizeof(Rpp32f), inputf32, 0, NULL, NULL);
    err = clEnqueueWriteBuffer(theQueue, d_input_second, CL_TRUE, 0, ioBufferSize * sizeof(Rpp8u), input_second, 0, NULL, NULL);
    rppHandle_t handle;

    rppCreateWithStreamAndBatchSize(&handle, theQueue, noOfImages);

    clock_t start, end;
    double cpu_time_used;

    string test_case_name;

    switch (test_case)
    {
    case 1:
    {
        test_case_name = "rotate";

        Rpp32f angle[images];
        for (i = 0; i < images; i++)
        {
            angle[i] = 50;
        }

        start = clock();
        if (ip_bitDepth == 0)
            rppi_rotate_u8_pkd3_batchPD_gpu(d_input, srcSize, maxSize, d_output, dstSize, maxDstSize, angle, noOfImages, handle);
        else if (ip_bitDepth == 1)
            rppi_rotate_f16_pkd3_batchPD_gpu(d_inputf16, srcSize, maxSize, d_outputf16, dstSize, maxDstSize, angle, noOfImages, handle);
        else if (ip_bitDepth == 2)
            rppi_rotate_f32_pkd3_batchPD_gpu(d_inputf32, srcSize, maxSize, d_outputf32, dstSize, maxDstSize, angle, noOfImages, handle);
        end = clock();

        break;
    }

    case 2:
    {
        test_case_name = "resize";

        for (i = 0; i < images; i++)
        {
            dstSize[i].height = image.rows / 3;
            dstSize[i].width = image.cols / 1.1;
            if (maxDstHeight < dstSize[i].height)
                maxDstHeight = dstSize[i].height;
            if (maxDstWidth < dstSize[i].width)
                maxDstWidth = dstSize[i].width;
            if (minDstHeight > dstSize[i].height)
                minDstHeight = dstSize[i].height;
            if (minDstWidth > dstSize[i].width)
                minDstWidth = dstSize[i].width;
        }
        maxDstSize.height = maxDstHeight;
        maxDstSize.width = maxDstWidth;

        start = clock();
        if (ip_bitDepth == 0)
            rppi_resize_u8_pkd3_batchPD_gpu(d_input, srcSize, maxSize, d_output, dstSize, maxDstSize, noOfImages, handle);
        else if (ip_bitDepth == 1)
            rppi_resize_f16_pkd3_batchPD_gpu(d_inputf16, srcSize, maxSize, d_outputf16, dstSize, maxDstSize, noOfImages, handle);
        else if (ip_bitDepth == 2)
            rppi_resize_f32_pkd3_batchPD_gpu(d_inputf32, srcSize, maxSize, d_outputf32, dstSize, maxDstSize, noOfImages, handle);
        end = clock();

        break;
    }
    case 3:
    {
        test_case_name = "resize_crop";

        Rpp32u x1[images];
        Rpp32u y1[images];
        Rpp32u x2[images];
        Rpp32u y2[images];
        for (i = 0; i < images; i++)
        {
            x1[i] = 0;
            y1[i] = 0;
            x2[i] = 50;
            y2[i] = 50;
            dstSize[i].height = image.rows / 3;
            dstSize[i].width = image.cols / 1.1;
            if (maxDstHeight < dstSize[i].height)
                maxDstHeight = dstSize[i].height;
            if (maxDstWidth < dstSize[i].width)
                maxDstWidth = dstSize[i].width;
            if (minDstHeight > dstSize[i].height)
                minDstHeight = dstSize[i].height;
            if (minDstWidth > dstSize[i].width)
                minDstWidth = dstSize[i].width;
        }
        maxDstSize.height = maxDstHeight;
        maxDstSize.width = maxDstWidth;

        start = clock();
        if (ip_bitDepth == 0)
            rppi_resize_crop_u8_pkd3_batchPD_gpu(d_input, srcSize, maxSize, d_output, dstSize, maxDstSize, x1, x2, y1, y2, noOfImages, handle);
        else if (ip_bitDepth == 1)
            rppi_resize_crop_f16_pkd3_batchPD_gpu(d_inputf16, srcSize, maxSize, d_outputf16, dstSize, maxDstSize, x1, x2, y1, y2, noOfImages, handle);
        else if (ip_bitDepth == 2)
            rppi_resize_crop_f32_pkd3_batchPD_gpu(d_inputf32, srcSize, maxSize, d_outputf32, dstSize, maxDstSize, x1, x2, y1, y2, noOfImages, handle);
        end = clock();

        break;
    }
    case 4:
    {
        test_case_name = "resize_crop_mirror";

        Rpp32u x1[images];
        Rpp32u y1[images];
        Rpp32u x2[images];
        Rpp32u y2[images];
        Rpp32u mirrorFlag[images];
        for (i = 0; i < images; i++)
        {
            x1[i] = 0;
            y1[i] = 0;
            x2[i] = 50;
            y2[i] = 50;
            dstSize[i].height = image.rows / 3;
            dstSize[i].width = image.cols / 1.1;
            if (maxDstHeight < dstSize[i].height)
                maxDstHeight = dstSize[i].height;
            if (maxDstWidth < dstSize[i].width)
                maxDstWidth = dstSize[i].width;
            if (minDstHeight > dstSize[i].height)
                minDstHeight = dstSize[i].height;
            if (minDstWidth > dstSize[i].width)
                minDstWidth = dstSize[i].width;
            mirrorFlag[i] = 1;
        }
        maxDstSize.height = maxDstHeight;
        maxDstSize.width = maxDstWidth;

        start = clock();
        if (ip_bitDepth == 0)
            rppi_resize_crop_mirror_u8_pkd3_batchPD_gpu(d_input, srcSize, maxSize, d_output, dstSize, maxDstSize, x1, x2, y1, y2, mirrorFlag, noOfImages, handle);
        else if (ip_bitDepth == 1)
            rppi_resize_crop_mirror_f16_pkd3_batchPD_gpu(d_inputf16, srcSize, maxSize, d_outputf16, dstSize, maxDstSize, x1, x2, y1, y2, mirrorFlag, noOfImages, handle);
        else if (ip_bitDepth == 2)
            rppi_resize_crop_mirror_f32_pkd3_batchPD_gpu(d_inputf32, srcSize, maxSize, d_outputf32, dstSize, maxDstSize, x1, x2, y1, y2, mirrorFlag, noOfImages, handle);
        end = clock();

        break;
    }
    case 5:
    {
        test_case_name = "crop";

        Rpp32u crop_pos_x[images];
        Rpp32u crop_pos_y[images];
        for (i = 0; i < images; i++)
        {
            dstSize[i].height = 100;
            dstSize[i].width = 120;
            if (maxDstHeight < dstSize[i].height)
                maxDstHeight = dstSize[i].height;
            if (maxDstWidth < dstSize[i].width)
                maxDstWidth = dstSize[i].width;
            if (minDstHeight > dstSize[i].height)
                minDstHeight = dstSize[i].height;
            if (minDstWidth > dstSize[i].width)
                minDstWidth = dstSize[i].width;
            crop_pos_x[i] = 50;
            crop_pos_y[i] = 50;
        }

        start = clock();
        if (ip_bitDepth == 0)
            rppi_crop_u8_pkd3_batchPD_gpu(d_input, srcSize, maxSize, d_output, dstSize, maxDstSize, crop_pos_x, crop_pos_y, noOfImages, handle);
        else if (ip_bitDepth == 1)
            rppi_crop_f16_pkd3_batchPD_gpu(d_inputf16, srcSize, maxSize, d_outputf16, dstSize, maxDstSize, crop_pos_x, crop_pos_y, noOfImages, handle);
        else if (ip_bitDepth == 2)
            rppi_crop_f32_pkd3_batchPD_gpu(d_inputf32, srcSize, maxSize, d_outputf32, dstSize, maxDstSize, crop_pos_x, crop_pos_y, noOfImages, handle);
        end = clock();

        break;
    }
    case 6:
    {
        test_case_name = "crop_mirror_normalize";

        Rpp32u crop_pos_x[images];
        Rpp32u crop_pos_y[images];
        Rpp32f mean[images];
        Rpp32f stdDev[images];
        Rpp32u mirrorFlag[images];
        for (i = 0; i < images; i++)
        {
            dstSize[i].height = 100;
            dstSize[i].width = 100;
            if (maxDstHeight < dstSize[i].height)
                maxDstHeight = dstSize[i].height;
            if (maxDstWidth < dstSize[i].width)
                maxDstWidth = dstSize[i].width;
            if (minDstHeight > dstSize[i].height)
                minDstHeight = dstSize[i].height;
            if (minDstWidth > dstSize[i].width)
                minDstWidth = dstSize[i].width;
            crop_pos_x[i] = 50;
            crop_pos_y[i] = 50;
            mean[i] = 0.0;
            stdDev[i] = 1.0;
            mirrorFlag[i] = 1;
        }
        Rpp32u outputFormatToggle = 0;

        start = clock();
        if (ip_bitDepth == 0)
            rppi_crop_mirror_normalize_u8_pkd3_batchPD_gpu(d_input, srcSize, maxSize, d_output, dstSize, maxDstSize, crop_pos_x, crop_pos_y, mean, stdDev, mirrorFlag, outputFormatToggle, noOfImages, handle);
        else if (ip_bitDepth == 1)
            rppi_crop_mirror_normalize_f16_pkd3_batchPD_gpu(d_inputf16, srcSize, maxSize, d_outputf16, dstSize, maxDstSize, crop_pos_x, crop_pos_y, mean, stdDev, mirrorFlag, outputFormatToggle, noOfImages, handle);
        else if (ip_bitDepth == 2)
            rppi_crop_mirror_normalize_f32_pkd3_batchPD_gpu(d_inputf32, srcSize, maxSize, d_outputf32, dstSize, maxDstSize, crop_pos_x, crop_pos_y, mean, stdDev, mirrorFlag, outputFormatToggle, noOfImages, handle);
        end = clock();

        break;
    }
    case 7:
    {
        test_case_name = "color_twist";

        Rpp32f alpha[images];
        Rpp32f beta[images];
        Rpp32f hueShift[images];
        Rpp32f saturationFactor[images];
        for (i = 0; i < images; i++)
        {
            alpha[i] = 1.4;
            beta[i] = 0;
            hueShift[i] = 60;
            saturationFactor[i] = 1.9;
        }

        start = clock();
        if (ip_bitDepth == 0)
            rppi_color_twist_u8_pkd3_batchPD_gpu(d_input, srcSize, maxSize, d_output, alpha, beta, hueShift, saturationFactor, noOfImages, handle);
        else if (ip_bitDepth == 1)
            rppi_color_twist_f16_pkd3_batchPD_gpu(d_inputf16, srcSize, maxSize, d_outputf16, alpha, beta, hueShift, saturationFactor, noOfImages, handle);
        else if (ip_bitDepth == 2)
            rppi_color_twist_f32_pkd3_batchPD_gpu(d_inputf32, srcSize, maxSize, d_outputf32, alpha, beta, hueShift, saturationFactor, noOfImages, handle);
        end = clock();

        break;
    }
    default:
        missingFuncFlag = 1;
        break;
    }

    if (missingFuncFlag == 1)
    {
        printf("\nThe functionality %s doesn't yet exist in RPP\n", func);
        return -1;
    }

    cpu_time_used = ((double)(end - start)) / CLOCKS_PER_SEC;
    cout << "\nGPU Time - BatchPD : " << cpu_time_used << endl;

    clEnqueueReadBuffer(theQueue, d_output, CL_TRUE, 0, oBufferSize * sizeof(Rpp8u), output, 0, NULL, NULL);
    clEnqueueReadBuffer(theQueue, d_outputf16, CL_TRUE, 0, oBufferSize * sizeof(Rpp16f), outputf16, 0, NULL, NULL);
    clEnqueueReadBuffer(theQueue, d_outputf32, CL_TRUE, 0, oBufferSize * sizeof(Rpp32f), outputf32, 0, NULL, NULL);

    rppDestroyGPU(handle);

    if (ip_bitDepth == 1)
    {
        int valCount = 0;
        Rpp8u *outputTemp;
        outputTemp = output;
        Rpp16f *outputf16Temp;
        outputf16Temp = outputf16;
        for (int i = 0; i < oBufferSize; i++)
        {
            *outputTemp = (Rpp8u)RPPPIXELCHECK(*outputf16Temp * 255.0);
            outputf16Temp++;
            outputTemp++;
        }
    }
    else if (ip_bitDepth == 2)
    {
        int valCount = 0;
        Rpp8u *outputTemp;
        outputTemp = output;
        Rpp32f *outputf32Temp;
        outputf32Temp = outputf32;
        for (int i = 0; i < oBufferSize; i++)
        {
            *outputTemp = (Rpp8u)RPPPIXELCHECK(*outputf32Temp * 255.0);
            outputf32Temp++;
            outputTemp++;
        }
    }

    mkdir(dst, 0700);
    strcat(dst, "/");
    count = 0;
    for (j = 0; j < noOfImages; j++)
    {
        int op_size = maxHeight * maxWidth * ip_channel;
        Rpp8u *temp_output = (Rpp8u *)calloc(op_size, sizeof(Rpp8u));
        for (i = 0; i < op_size; i++)
        {
            temp_output[i] = output[count];
            count++;
        }
        char temp[1000];
        strcpy(temp, dst);
        strcat(temp, imageNames[j]);
        Mat mat_op_image;
        if (ip_channel == 3)
        {
            mat_op_image = Mat(maxHeight, maxWidth, CV_8UC3, temp_output);
            imwrite(temp, mat_op_image);
        }
        if (ip_channel == 1)
        {
            mat_op_image = Mat(maxHeight, maxWidth, CV_8UC1, temp_output);
            imwrite(temp, mat_op_image);
        }
        free(temp_output);
    }

    free(srcSize);
    free(dstSize);
    free(input);
    free(input_second);
    free(output);
    free(inputf16);
    free(inputf16_second);
    free(outputf16);
    free(inputf32);
    free(inputf32_second);
    free(outputf32);
    clReleaseMemObject(d_input);
    clReleaseMemObject(d_output);
    clReleaseMemObject(d_input_second);
    clReleaseMemObject(d_inputf16);
    clReleaseMemObject(d_outputf16);
    clReleaseMemObject(d_inputf32);
    clReleaseMemObject(d_outputf32);
    return 0;
}
