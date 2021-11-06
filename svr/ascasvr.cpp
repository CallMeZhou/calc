#include <stdio.h>
#include <stdint.h>
#include <math.h>
#include "ascasvr.hpp"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "stb_image_resize.h"

static constexpr uint8_t LUM_CHARS[] = {
    ' ', '.', 'o', '+', '#'
};

static constexpr uint8_t N_LUM_CHARS = sizeof(LUM_CHARS) / sizeof(LUM_CHARS[0]);
static constexpr float LUM_DIV = (float) UINT8_MAX / (float) (N_LUM_CHARS - 1);

static void blur(uint8_t *destimgbuff, const uint8_t *srcimgbuff, int w, int h, int stride);
static void edge(uint8_t *destimgbuff, const uint8_t *srcimgbuff, int w, int h, int stride);

ServiceAsciiart::ServiceAsciiart(int peer) : SimpleHandler(sizeof(AsciiartHeader), peer) {
}

ReturnCode ServiceAsciiart::handleRequest(const std::vector<uint8_t> &request, BaseHeader *header, std::vector<uint8_t> &response) const {
    AsciiartHeader *ascHeader = (AsciiartHeader*)header;

    int w, h, ch;
    auto img = stbi_load_from_memory(&request[0], request.size(), &w, &h, &ch, 1);
    if (!img) {
        return ReturnCode::FailedProcess;
    }

    int resultWidth = ascHeader->requestedWidth + 1; // resultWidth is stride
    int resultHeight = (int) ((float) h / (float) w * (float) ascHeader->requestedWidth + 0.5f);

    response.resize(resultWidth * resultHeight + 1);
    stbir_resize_uint8(img, w, h, 0, &response[0], ascHeader->requestedWidth, resultHeight, resultWidth, 1);
    stbi_image_free(img);

    {
        std::vector<uint8_t> temp;
        temp.resize(response.size());
        blur(&temp[0], &response[0], ascHeader->requestedWidth, resultHeight, resultWidth);
        edge(&response[0], &temp[0], ascHeader->requestedWidth, resultHeight, resultWidth);
    }

    for (auto &i : response) {
        i = LUM_CHARS[(int) ((float) i / LUM_DIV + 0.5f)];
    } 

    for (int x = ascHeader->requestedWidth, y = 0; y < resultHeight; y++, x += resultWidth) {
        response[x] = '\n';
    }

    *response.rbegin() = '\0';

    return ReturnCode::Succeeded;
}

ServiceAsciiart::~ServiceAsciiart() {
    printf("ServiceAsciiart-%p done.\n", this);
}

StreamServerHandler* ServiceAsciiart::factory(int peer) {
    return new ServiceAsciiart(peer);
}

static float conv_kernel(const uint8_t *pixelInImgBuff, int stride, const float *mat33) {
    const uint8_t *imgbuff = pixelInImgBuff - stride - 1;
    float result = 0;
    for (int i = 0; i < 3; i++) {
        float temp[] = {(float) imgbuff[0], (float) imgbuff[1], (float) imgbuff[2]};
        result += temp[0] * mat33[0] + temp[1] * mat33[1] + temp[2] * mat33[2];
        imgbuff += stride; mat33 += 3;
    }
    return result;
}

static void blur(uint8_t *destimgbuff, const uint8_t *srcimgbuff, int w, int h, int stride) {
    static constexpr float gaussian[] = {
        0.0625f, 0.125f, 0.0625f,
         0.125f,  0.25f,  0.125f,
        0.0625f, 0.125f, 0.0625f,
    };

    auto src = srcimgbuff + stride + 1;
    auto dest = destimgbuff + stride + 1;
    for (int y = 1; y < h - 1; y++) {
        auto srcptr  = src;
        auto destptr = dest;
        for (int x = 1; x < w - 1; x++) {
            *destptr++ = (uint8_t) conv_kernel(srcptr++, stride, gaussian);
        }
        dest[-1] = dest[0];
        destptr[0] = destptr[-1];
        src  += stride;
        dest += stride;
    }
    // dest is now pointing to the 2nd pixel of the last line
    dest--; // move back to the start of the last line

    memcpy(destimgbuff, destimgbuff + stride, w);
    memcpy(dest, dest - stride, w);
}

static void edge(uint8_t *destimgbuff, const uint8_t *srcimgbuff, int w, int h, int stride) {
    static constexpr float sobel_x[] = {
        1, 0, -1,
        2, 0, -2,
        1, 0, -1,
    };

    static constexpr float sobel_y[] = {
         1,  2,  1,
         0,  0,  0,
        -1, -2, -1,
    };

    auto src = srcimgbuff + stride + 1;
    auto dest = destimgbuff + stride + 1;
    for (int y = 1; y < h - 1; y++) {
        auto srcptr  = src;
        auto destptr = dest;
        for (int x = 1; x < w - 1; x++) {
            float accum_x = conv_kernel(srcptr, stride, sobel_x);
            float accum_y = conv_kernel(srcptr, stride, sobel_y);
            srcptr++;
            float gradient = sqrt(accum_x * accum_x + accum_y * accum_y);
            if (gradient < 0) gradient = 0; else if (gradient > 255.0f) gradient = 255.0f;
            *destptr++ = (uint8_t) gradient;
        }
        dest[-1] = dest[0];
        destptr[0] = destptr[-1];
        src  += stride;
        dest += stride;
    }
    // dest is now pointing to the 2nd pixel of the last line
    dest--; // move back to the start of the last line

    memcpy(destimgbuff, destimgbuff + stride, w);
    memcpy(dest, dest - stride, w);
}