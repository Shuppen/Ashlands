/*
 * camera.h — Camera state (works across all render modes)
 * C11, MIT License
 */
#ifndef ASHLANDS_CAMERA_H
#define ASHLANDS_CAMERA_H

/* Camera tracks where the view is centered in tile-space */
typedef struct Camera {
    float x, y;          /* Center of view in tile coordinates */
    int   screen_w;      /* Screen pixel width  */
    int   screen_h;      /* Screen pixel height */
    int   tile_w;        /* Tile width  in pixels */
    int   tile_h;        /* Tile height in pixels */
} Camera;

void  camera_init(Camera *cam, int screen_w, int screen_h,
                  int tile_w, int tile_h);
void  camera_center_on(Camera *cam, float tx, float ty);
void  camera_move(Camera *cam, float dx, float dy);

/* Convert tile → screen pixel (top-left of tile cell) */
void  camera_tile_to_screen(const Camera *cam, int tx, int ty,
                             int *sx, int *sy);

/* Convert screen pixel → tile */
void  camera_screen_to_tile(const Camera *cam, int sx, int sy,
                             int *tx, int *ty);

/* Visible tile rectangle */
void  camera_visible_rect(const Camera *cam,
                          int *x0, int *y0, int *x1, int *y1);

#endif /* ASHLANDS_CAMERA_H */
