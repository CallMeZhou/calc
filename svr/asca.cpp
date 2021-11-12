#include <cstdlib>
#include <cmath>
#include <fstream>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "stb_image_resize.h"
//#define STB_IMAGE_WRITE_IMPLEMENTATION
//#include "stb_image_write.h"

#include "libcom.hpp"
#include "fmt/core.h"

using namespace std;
using namespace http;
using namespace server_utils;
using namespace server_excepts;

static constexpr char LUM_CHARS[] = {
    ' ', '.', 'o', '+', '#'
};

static constexpr char N_LUM_CHARS = sizeof(LUM_CHARS) / sizeof(LUM_CHARS[0]);
static constexpr float LUM_DIV = (float) UINT8_MAX / (float) (N_LUM_CHARS - 1);

static void blur(uint8_t *destimgbuff, const uint8_t *srcimgbuff, int w, int h, int stride);
static void edge(uint8_t *destimgbuff, const uint8_t *srcimgbuff, int w, int h, int stride);

tuple<header_t, msgbuff_t> lambda_asca(const msgbuff_t &request, const args_t &args_url, const args_t &args_header) {

    try {

        auto form_data  = multpart_formdata::parse(args_header, request);
        auto form_entry = multpart_formdata::find(form_data, "Content-Disposition", "name", "asca-source-image");

        int w, h, ch;
        auto img = stbi_load_from_memory((const uint8_t*) &form_entry.body[0], form_entry.body.size(), &w, &h, &ch, 1);
        if (!img) {
            throw handle_request_failure(400, "unsupported image format");
        }
        server_utils::deferred free_stbi_buff( [img](void){ stbi_image_free(img); } );

        int requestedWidth = std::atoi(server_utils::find_arg(args_header, args_url, "requested-width", "80").c_str());
        int resultWidth = requestedWidth + 1; // resultWidth is stride
        int resultHeight = (int) ((float) h / (float) w * (float) requestedWidth + 0.5f);

        msgbuff_t response;
        response.resize(resultWidth * resultHeight + 1); // +1 for the last '\0'
        stbir_resize_uint8(img, w, h, 0, (uint8_t*) &response[0], requestedWidth, resultHeight, resultWidth, 1);

        std::vector<uint8_t> temp;
        temp.resize(response.size());
        blur(&temp[0], (uint8_t*) &response[0], requestedWidth, resultHeight, resultWidth);
        edge((uint8_t*) &response[0], &temp[0], requestedWidth, resultHeight, resultWidth);

        for (auto &i : response) {
            i = LUM_CHARS[(int) ((float) (uint8_t) i / LUM_DIV + 0.5f)];
        } 

        for (int x = requestedWidth, y = 0; y < resultHeight; y++, x += resultWidth) {
            response[x] = '\n';
        }

        *response.rbegin() = '\0';

        return {
            {
                {"Content-Type", "text/html; charset=UTF-8"}
            },

            move(response) // TODO is the move() benefitial here?
        };

    } catch (out_of_range &e) {
        throw handle_request_failure(400, e.what());
    }
}

REGISTER_CONTROLLER("POST", "/asca", lambda_asca);

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