#ifndef ASHLANDS_MAT4_H
#define ASHLANDS_MAT4_H

#include <math.h>
#include <string.h>

typedef struct {
    float m[16];
} Mat4;

static inline void mat4_identity(Mat4 *out) {
    memset(out, 0, sizeof(*out));
    out->m[0] = 1.0f;
    out->m[5] = 1.0f;
    out->m[10] = 1.0f;
    out->m[15] = 1.0f;
}

static inline void mat4_multiply(Mat4 *out, const Mat4 *a, const Mat4 *b) {
    Mat4 tmp;

    for (int col = 0; col < 4; col++) {
        for (int row = 0; row < 4; row++) {
            tmp.m[col * 4 + row] =
                a->m[0 * 4 + row] * b->m[col * 4 + 0] +
                a->m[1 * 4 + row] * b->m[col * 4 + 1] +
                a->m[2 * 4 + row] * b->m[col * 4 + 2] +
                a->m[3 * 4 + row] * b->m[col * 4 + 3];
        }
    }

    *out = tmp;
}

static inline void mat4_perspective(Mat4 *out, float fovy_deg,
                                    float aspect, float z_near, float z_far) {
    float fovy_rad = fovy_deg * 0.01745329251994329577f;
    float f = 1.0f / tanf(fovy_rad * 0.5f);

    memset(out, 0, sizeof(*out));
    out->m[0] = f / aspect;
    out->m[5] = f;
    out->m[10] = (z_far + z_near) / (z_near - z_far);
    out->m[11] = -1.0f;
    out->m[14] = (2.0f * z_far * z_near) / (z_near - z_far);
}

static inline void mat4_lookat(Mat4 *out,
                               float eye_x, float eye_y, float eye_z,
                               float center_x, float center_y, float center_z,
                               float up_x, float up_y, float up_z) {
    float fx = center_x - eye_x;
    float fy = center_y - eye_y;
    float fz = center_z - eye_z;
    float flen = sqrtf(fx * fx + fy * fy + fz * fz);

    if (flen > 0.0f) {
        fx /= flen;
        fy /= flen;
        fz /= flen;
    }

    float sx = fy * up_z - fz * up_y;
    float sy = fz * up_x - fx * up_z;
    float sz = fx * up_y - fy * up_x;
    float slen = sqrtf(sx * sx + sy * sy + sz * sz);

    if (slen > 0.0f) {
        sx /= slen;
        sy /= slen;
        sz /= slen;
    }

    float ux = sy * fz - sz * fy;
    float uy = sz * fx - sx * fz;
    float uz = sx * fy - sy * fx;

    mat4_identity(out);
    out->m[0] = sx;
    out->m[1] = ux;
    out->m[2] = -fx;
    out->m[4] = sy;
    out->m[5] = uy;
    out->m[6] = -fy;
    out->m[8] = sz;
    out->m[9] = uz;
    out->m[10] = -fz;
    out->m[12] = -(sx * eye_x + sy * eye_y + sz * eye_z);
    out->m[13] = -(ux * eye_x + uy * eye_y + uz * eye_z);
    out->m[14] = fx * eye_x + fy * eye_y + fz * eye_z;
}

#endif
