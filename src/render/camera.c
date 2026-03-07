/*
 * camera.c — Camera implementation
 * C11, MIT License
 */
#include "camera.h"

void camera_init(Camera *cam, int screen_w, int screen_h,
                 int tile_w, int tile_h)
{
    cam->x        = 0.0f;
    cam->y        = 0.0f;
    cam->screen_w = screen_w;
    cam->screen_h = screen_h;
    cam->tile_w   = tile_w;
    cam->tile_h   = tile_h;
}

void camera_center_on(Camera *cam, float tx, float ty) {
    cam->x = tx;
    cam->y = ty;
}

void camera_move(Camera *cam, float dx, float dy) {
    cam->x += dx;
    cam->y += dy;
}

void camera_tile_to_screen(const Camera *cam, int tx, int ty,
                            int *sx, int *sy)
{
    /* Offset in tiles from camera center */
    float dtx = (float)tx - cam->x;
    float dty = (float)ty - cam->y;

    *sx = (int)(cam->screen_w / 2 + dtx * cam->tile_w);
    *sy = (int)(cam->screen_h / 2 + dty * cam->tile_h);
}

void camera_screen_to_tile(const Camera *cam, int sx, int sy,
                            int *tx, int *ty)
{
    float dtx = ((float)sx - cam->screen_w / 2.0f) / (float)cam->tile_w;
    float dty = ((float)sy - cam->screen_h / 2.0f) / (float)cam->tile_h;
    *tx = (int)(cam->x + dtx);
    *ty = (int)(cam->y + dty);
}

void camera_visible_rect(const Camera *cam,
                         int *x0, int *y0, int *x1, int *y1)
{
    int half_w = cam->screen_w / (cam->tile_w * 2) + 1;
    int half_h = cam->screen_h / (cam->tile_h * 2) + 1;
    *x0 = (int)cam->x - half_w;
    *y0 = (int)cam->y - half_h;
    *x1 = (int)cam->x + half_w;
    *y1 = (int)cam->y + half_h;
}
