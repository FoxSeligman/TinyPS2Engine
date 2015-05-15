/*
# _____     ___ ____     ___ ____
#  ____|   |    ____|   |        | |____|
# |     ___|   |____ ___|    ____| |    \    PS2DEV Open Source Project.
#-----------------------------------------------------------------------
# (c) 2005 Dan Peori <peori@oopo.net>
# Licenced under Academic Free License version 2.0
# Review ps2sdk README & LICENSE files for further details.
#
*/

#include <kernel.h>
#include <stdlib.h>
#include <tamtypes.h>
#include <math3d.h>

#include <packet.h>

#include <dma_tags.h>
#include <gif_tags.h>
#include <gs_psm.h>

#include <dma.h>

#include <graph.h>

#include <draw.h>
#include <draw3d.h>

#include "Utilities.c"
#include "square_data.c"



#include "bg.c"
#include "flower.c"
extern unsigned char bg[];
extern unsigned char flower[];



VECTOR object_position = { 0.00f, 0.00f, 0.00f, 1.00f };
VECTOR object_rotation = { 0.00f, 0.00f, 0.00f, 1.00f };

VECTOR camera_position = { 0.00f, 0.00f, 100.00f, 1.00f };
VECTOR camera_rotation = { 0.00f, 0.00f,   0.00f, 1.00f };

void init_dma()
{
    dma_channel_initialize(DMA_CHANNEL_GIF, NULL, 0);
    dma_channel_fast_waits(DMA_CHANNEL_GIF);
}

void create_canvas(canvas *c, int width, int height)
{

    framebuffer_t *frame = &c->frame;
    zbuffer_t *z = &c->z;
    memory *m = &c->memory;

    // Define a 32-bit 640x512 framebuffer.
    frame->width = width;
    frame->height = height;
    frame->mask = 0;
    frame->psm = GS_PSM_32;
    frame->address = graph_vram_allocate(frame->width, frame->height, frame->psm, GRAPH_ALIGN_PAGE);

    // Enable the zbuffer.
    z->enable = DRAW_ENABLE;
    z->mask = 0;
    z->method = ZTEST_METHOD_GREATER_EQUAL;
    z->zsm = GS_ZBUF_32;
    z->address = graph_vram_allocate(frame->width,frame->height,z->zsm, GRAPH_ALIGN_PAGE);

    // Create workspace
    //TODO: set the vertex_count to the largest geometry's vertex count
    m->temp_vertices = make_buffer(sizeof(VECTOR), vertex_count);
    m->verts  = make_buffer(sizeof(u64), vertex_count);
    m->colors = make_buffer(sizeof(u64), vertex_count);
    m->coordinates = make_buffer(sizeof(u64), vertex_count);
    //TODO: what does this do?
//    m->color.r = 0x80;
//    m->color.g = 0x80;
//    m->color.b = 0x80;
//    m->color.a = 0x80;
//    m->color.q = 1.0f;

    // Define the triangle primitive we want to use.
    prim_t *p = &c->prim;
    p->type = PRIM_TRIANGLE;
    p->shading = PRIM_SHADE_GOURAUD;
    p->mapping = DRAW_ENABLE;
    p->fogging = DRAW_DISABLE;
    p->blending = DRAW_ENABLE;
    p->antialiasing = DRAW_DISABLE;
    p->mapping_type = PRIM_MAP_ST;
    p->colorfix = PRIM_UNFIXED;

    // Set sprite geometry settings (pulled from square_data.c)
    geometry *g = &c->sprite_geometry;
    g->vertex_count = vertex_count;
    g->vertices = vertices;
    g->colors = colors;
    g->coordinates = coordinates;
    g->index_count = points_count;
    g->indices = points;

    // Create double-buffer
    c->buffers[0] = create_packet(3000);
    c->buffers[1] = create_packet(3000);

    // Initialize the screen and tie the first framebuffer to the read circuits.
    graph_initialize(frame->address, frame->width, frame->height, frame->psm, 0, 0);

    // Register canvas with the coprocessor
    register_canvas(c);

}

int render(canvas *c)
{

    // Matrices to setup the 3D environment and camera
    MATRIX P;
    MATRIX MV;
    MATRIX CAM;
    MATRIX FINAL;


    frustum(P, graph_aspect_ratio(), -3.00f, 3.00f, -3.00f, 3.00f, 1.00f, 2000.00f);

    wait();

    VECTOR scale;
    float size = 43.5f;
    scale[0] = size;
    scale[1] = size/1.33f;
    scale[2] = size;


    VECTOR scale2;
    size = 3.00f;
    scale2[0] = size;
    scale2[1] = size;
    scale2[2] = size;



    // Load textures
    sprite bg_sprite;
    load_sprite(&bg_sprite, bg, 512, 512, 0.17f, 0.83f);

    sprite flower_sprite;
    load_sprite(&flower_sprite, flower, 256, 256, 0.00f, 1.00f);

    // Create entities
    entity e_bg;
    e_bg.sprite = &bg_sprite;

    entity e_flower;
    e_flower.sprite = &flower_sprite;



    // The main loop...
    for (;;)
    {

        create_wand(c);

        clear(c);

        create_CAM(CAM, camera_position, camera_rotation);




        matrix_unit(MV);
        matrix_scale(MV, MV, scale);
        object_rotation[2] = 0.00f;
        matrix_rotate(MV, MV, object_rotation);
        matrix_translate(MV, MV, object_position);
        create_FINAL(FINAL, MV, CAM, P);
        drawObject(c, FINAL, &e_bg);

        matrix_unit(MV);
        matrix_scale(MV, MV, scale2);
        object_rotation[2] = e_flower.angle += 0.012f;
        matrix_rotate(MV, MV, object_rotation);
        create_FINAL(FINAL, MV, CAM, P);
        drawObject(c, FINAL, &e_flower);




        use_wand(c);

        c->current_buffer ^= 1;

        draw_wait_finish();
        graph_wait_vsync();

    }

    // ~~ Free if we're done with the for loop, but we never will be
//    packet_free(packets[0]);
//    packet_free(packets[1]);

    return 0;

}

void startup(int width, int height)
{

    init_dma();

    //TODO: can this be the one to produce the canvas?
    canvas c;
    create_canvas(&c, width, height);
    clear_color(&c, 0x80, 0, 0x80);

    // Render the cube
    render(&c);

    // Sleep
    SleepThread();

}
