#define _XOPEN_SOURCE 500 // needed for nanosleep
#define __USE_POSIX199309 // needed for nanosleep
#include <stdio.h>
#include <getopt.h>
#include <stdlib.h>
#include <raylib.h>
#include <raymath.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <math.h>
#include "bw_bridge.h"
#include "badglib.h"
#include "fast_obj.h"

// Screen size
#define SCREEN_WIDTH 64
#define SCREEN_HEIGHT 64
#define NUMPIXELS SCREEN_WIDTH * SCREEN_HEIGHT
#define FPGA_MEM_OFFSET 0x4000

// Frame timer
#define FPS 100
#define FRAMETIME_US ((int)(1.0/FPS * 1e9)) // 10 ms / 100Hz

void *FrameTimerThread(void *vargp);

//Format data for gpmc
void loadMatrixData(uint16_t* matrixData, Image* fbuf, int FrameNum);

// Fire effect
static uint8_t fire[NUMPIXELS];

// Star field
#define NUM_STARS 100

typedef struct {
    float x, y, z;
    float velocity;
} Star;

Star stars[NUM_STARS];

void init_starfield();
void update_starfield();
void draw_starfield(uint16_t* matrixData);


int main(int argc, char *argv[])
{
    struct bridge br;

    int opt_i = 0;
    int opt;

    char filename[256];
    int numFrames;
    Image img;

    int mode = 0; // choose what function is being displayed
    bool printFrameTimes = false;

    // Handle input arguments
    static struct option long_opts[]= //parse arguments to read file name with -f
    {
        { "filename"    , required_argument, 0, 'f' }, // Filename
        { "mode"        , optional_argument, 0, 'm' }, // mode 0 = gif/image, 1 = software rendering
        { "frametimes"  , no_argument      , 0, 't' },
        { 0, 0, 0, 0 },
    };

    while((opt = getopt_long(argc, argv, "m:f:t", long_opts, &opt_i)) != -1)
    {
        switch(opt)
        {
        case 'f':
            strcpy(filename,optarg);
            mode = 0;
            break;
        case 'm':
            mode = atoi(optarg);
            break;
        case 't':
            printFrameTimes = true;
            break;
        }
    }

    //Handle image loading
    if (IsFileExtension(filename, ".png")) { // see if we are loading an image or an animation
        img = LoadImage(filename);
        numFrames = 1;
    }
    else if (IsFileExtension(filename, ".gif")) {
        img = LoadImageAnim(filename, &numFrames);
    }
    else {
        printf("ERROR: File type not supported");
        return 1;
    }

    if (bridge_init(&br, BW_BRIDGE_MEM_ADR, BW_BRIDGE_MEM_SIZE) < 0) { //after loading in file, initialize the GPMC interface
        printf("ERROR: GPMC Bridge Init failed");
        return 2;
    }

    int currentFrame = 0;

    printf("Number of Frames: %d\n", numFrames);

    // Set up a timer to have a controllable frame time
    volatile bool change_frame = 0;
    pthread_t frame_timer_thread_id;
    pthread_create(&frame_timer_thread_id, NULL, FrameTimerThread, (bool*)&change_frame);
    
    struct timespec ts;
    uint64_t us = 0;

    // 2d shape
    // for SW rendering make a frame buffer
    Image fbuf = GenImageColor(64, 64, BLACK); 
    // Other SW rendering objects
    //lets represent a wireframe shape as some vectors
    Vector2 vertices[] = {{-16.0, -8.0}, {16.0, -8.0}, {0.0, 8.0}};
    int lineIndices[] = {0, 1, 1, 2, 2, 0};
    shape2d triangle = {
        .numVertices = 3,
        .numLines = 3,
        .vertices = vertices,
        .lineIndices = lineIndices
    };

    float angle = 0;

    // 3d shape
    // Triangular prism
    shape3d triangularPrism = {
        .numFaces = 5,
        .vertices = (Vector3[]){{0, 16, 0}, {-16, -16, -16}, {16, -16, -16}, {16, -16, 16}, {-16, -16, 16}},
        .faceVertices = (int[]){
            0, 2, 1,   // Face 1
            0, 3, 2,   // Face 2
            0, 4, 3,   // Face 3
            0, 1, 4,   // Face 4
            4, 1, 2, 3 // Face 5 (square)
        },
        .numVerticesPerFace = (int[]){3, 3, 3, 3, 4}
    };

    Mesh sphereMesh = GenMeshSphere(30, 4, 8);
    shape3d sphere = rlMesh2Shape3d(sphereMesh);

    int heightMapX = 150;
    int heightMapY = 20;
    int heightMapZ = 150;
    Mesh heightMapMesh = GenMeshHeightmap(img, (Vector3) {heightMapX,heightMapY,heightMapZ});
    shape3d heightMap = rlMesh2Shape3d(heightMapMesh);
    for (int i = 0; i < heightMapMesh.vertexCount; i++) {
        heightMap.vertices[i].x = heightMap.vertices[i].x - heightMapX / 2;
        heightMap.vertices[i].y = heightMap.vertices[i].y - heightMapY / 2;
        heightMap.vertices[i].z = heightMap.vertices[i].z - heightMapZ / 2;
    }
    
    // Cube
    shape3d cube = {
        .numFaces = 6,
        .vertices = (Vector3[]){
            {-16,  16,  16}, // 0
            { 16,  16,  16}, // 1
            { 16, -16,  16}, // 2
            {-16, -16,  16}, // 3
            {-16,  16, -16}, // 4
            { 16,  16, -16}, // 5
            { 16, -16, -16}, // 6
            {-16, -16, -16}  // 7
        },
        .faceVertices = (int[]){
            0, 3, 2, 1, // Front face
            4, 5, 6, 7, // Back face
            0, 1, 5, 4, // Top face
            2, 3, 7, 6, // Bottom face
            0, 4, 7, 3, // Left face
            1, 2, 6, 5  // Right face
        },
        .numVerticesPerFace = (int[]){4, 4, 4, 4, 4, 4}
    };

    Vector3 rotationAngles = {0.0,0.0,0.0};
    Vector3 rotatedAngle;

    // Fire effect data
    Color colors[NUMPIXELS];

    for (int i = 0; i < 32; ++i) {
        /* black to mid red, 32 values*/
        // colors[i].r = i << 2; // make the last red section decay linearly
        // colors[i].r = (int)(4/32.0*pow(i,2)); //make the last red section decay exponentially
        colors[i].r = (int)(4/1024.0*pow(i,3)); //make the last red section decay as a 3rd order exponent

        /* mid red to orange, 32 values*/
        // colors[i + 32].r = 128 + (i << 2);

        colors[i + 32].r = 128 + (i << 2);
        colors[i + 32].g = (i << 2);

        /*yellow to orange, 32 values*/
        colors[i + 64].r = 255;
        colors[i + 64].g = 128 + (i << 2);

        /* yellow to white, 162 */
        colors[i + 96].r = 255;
        colors[i + 96].g = 255;
        colors[i + 96].b = i << 2;
        colors[i + 128].r = 255;
        colors[i + 128].g = 255;
        colors[i + 128].b = 64 + (i << 2);
        colors[i + 160].r = 255;
        colors[i + 160].g = 255;
        colors[i + 160].b = 128 + (i << 2);
        colors[i + 192].r = 255;
        colors[i + 192].g = 255;
        colors[i + 192].b = 192 + i;
        colors[i + 224].r = 255;
        colors[i + 224].g = 255;
        colors[i + 224].b = 224 + i;
    } 

    // Starfield effect
    init_starfield();	

    int i,j; 
    uint16_t temp;
    uint8_t index;

    fastObjMesh* objmesh = fast_obj_read("../media/fox.obj");
    shape3d obj = {
        .numFaces = objmesh->face_count,
        .vertices = malloc(objmesh->index_count*sizeof(Vector3)),
        .faceVertices = malloc(objmesh->index_count*sizeof(int)),
        .numVerticesPerFace = malloc(objmesh->face_count*sizeof(int))
    };
    for (unsigned int n = 0; n < objmesh->face_count; n++) {
        obj.numVerticesPerFace[n] = 3;
        for (unsigned int m = 0; m < 3; m++) {
            unsigned int vertex_index = objmesh->indices[n*3 + m].p;
            obj.vertices[n*3 + m] = Vector3Scale((Vector3){objmesh->positions[vertex_index*3], objmesh->positions[vertex_index*3+1], objmesh->positions[vertex_index*3+2]},0.4);
            obj.faceVertices[n*3 + m] = n*3 + m;
        }
    }
    
    // display our frames
    uint16_t matrixData[NUMPIXELS * 2];
    do {
        if (printFrameTimes) {
            clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
            us = (uint64_t)ts.tv_sec * 1000000 + (uint64_t)ts.tv_nsec / 1000;
        }

        switch (mode) {
            case 0: // display image/gif
                loadMatrixData(matrixData, &img, currentFrame);
                currentFrame++;
                if (currentFrame >= numFrames) currentFrame = 0;
                break;

            case 1: // 2d shape

                // make a 2d shape and draw it
                ImageClearBackground(&fbuf,BLACK);
                drawShape2d(&fbuf, &triangle, 31, 31, angle, BLUE);
                angle += 2;
                if (angle > 360) angle -= 360;

                loadMatrixData(matrixData, &fbuf, 0);
                break;

            case 2: // 3d prism
                // Draw the 3D shape
                ImageClearBackground(&fbuf, BLACK);
                drawShape3dCulled(&fbuf, &triangularPrism, 31, 31, rotationAngles, BLUE);

                // Update the rotation angles
                rotationAngles.x += 1.0;
                rotationAngles.y += 1.0;
                rotationAngles.z += 0.5;

                if (rotationAngles.x >= 360) rotationAngles.x -= 360;
                if (rotationAngles.y >= 360) rotationAngles.y -= 360;
                if (rotationAngles.z >= 360) rotationAngles.z -= 360;

                // Copy the pixels to matrixData
                loadMatrixData(matrixData, &fbuf, 0);
                break;

            case 3: // 3d cube
                // Draw the 3D shape
                ImageClearBackground(&fbuf, BLACK);
                drawShape3dCulled(&fbuf, &cube, 31, 31, rotationAngles, BLUE);

                // Update the rotation angles
                rotationAngles.x += 1.0;
                rotationAngles.y += 1.0;
                rotationAngles.z += 0.5;

                if (rotationAngles.x >= 360) rotationAngles.x -= 360;
                if (rotationAngles.y >= 360) rotationAngles.y -= 360;
                if (rotationAngles.z >= 360) rotationAngles.z -= 360;

                // Copy the pixels to matrixData
                loadMatrixData(matrixData, &fbuf, 0);
                break;

            case 4: // 3d sphere
                // Draw the 3D shape
                ImageClearBackground(&fbuf, BLACK);
                drawShape3dCulled(&fbuf, &sphere, 31, 31, rotationAngles, BLUE);

                // Update the rotation angles
                rotationAngles.x += 1.0;
                rotationAngles.y += 1.0;
                rotationAngles.z += 0.5;

                if (rotationAngles.x >= 360) rotationAngles.x -= 360;
                if (rotationAngles.y >= 360) rotationAngles.y -= 360;
                if (rotationAngles.z >= 360) rotationAngles.z -= 360;

                // Copy the pixels to matrixData
                loadMatrixData(matrixData, &fbuf, 0);
                break;

            case 5: // 3d heightMap
                // Draw the 3D shape
                ImageClearBackground(&fbuf, BLACK);
                rotatedAngle = QuaternionToEuler(QuaternionMultiply(QuaternionFromEuler(-25 * M_PI / 180,0,0),QuaternionFromEuler(rotationAngles.x * M_PI / 180, rotationAngles.y * M_PI / 180, rotationAngles.z * M_PI / 180)));
                drawShape3dCulled(&fbuf, &heightMap, 31, 20, Vector3Scale(rotatedAngle, 180 / M_PI), BLUE);

                // Update the rotation angles
                rotationAngles.x = 180;
                rotationAngles.y += 1.0;
                rotationAngles.z = 0.0;

                if (rotationAngles.x >= 360) rotationAngles.x -= 360;
                if (rotationAngles.y >= 360) rotationAngles.y -= 360;
                if (rotationAngles.z >= 360) rotationAngles.z -= 360;

                // Copy the pixels to matrixData
                loadMatrixData(matrixData, &fbuf, 0);
                break;

            case 6: // fire effect
                // credit to https://demo-effects.sourceforge.net/ for this algorithm, I just modified the color palette

                /* draw random bottom line in fire array*/
                j = SCREEN_WIDTH * (SCREEN_HEIGHT- 1);
                for (i = 0; i < SCREEN_WIDTH - 1; i++)
                {
                int random = 1 + (int)(16.0 * (rand()/(RAND_MAX+1.0)));
                if (random > 9) /* the lower the value, the intenser the fire, compensate a lower value with a higher decay value*/
                    fire[j + i] = 255; /*maximum heat*/
                else
                    fire[j + i] = 0;
                }  
                
                /* move fire upwards, start at bottom*/
                
                for (index = 0; index < 63 ; ++index) {
                    for (i = 0; i < SCREEN_WIDTH - 1; ++i) {
                        if (i == 0) { /* at the left border*/
                            temp = fire[j];
                            temp += fire[j + 1];
                            temp += fire[j - SCREEN_WIDTH];
                            temp /=3;
                        }
                        else if (i == SCREEN_WIDTH - 1) { /* at the right border*/
                            temp = fire[j + i];
                            temp += fire[j - SCREEN_WIDTH + i];
                            temp += fire[j + i - 1];
                            temp /= 3;
                        }
                        else {
                            temp = fire[j + i];
                            temp += fire[j + i + 1];
                            temp += fire[j + i - 1];
                            temp += fire[j - SCREEN_WIDTH + i];
                            temp >>= 2;
                        }
                        if (temp > 1) {
                            temp -= 1; /* decay */
                            if (temp%10 == 0) temp -= 1; // scale it down slightly so it doesn't hit the top edge
                        }
                        else temp = 0;

                        fire[j - SCREEN_WIDTH + i] = temp;
                    }
                    j -= SCREEN_WIDTH;
                }

                // not using an Image for drawing, load matrixData
                for (int i = 0; i < NUMPIXELS; i++) {
                    (matrixData)[i*2] = (colors[fire[i]].g) << 8 | (colors[fire[i]].r);
                    (matrixData)[i*2+1] = (colors[fire[i]].b);
                }
                break;

            case 7: // obj
                // Draw the obj
                ImageClearBackground(&fbuf, BLACK);
                rotatedAngle = QuaternionToEuler(QuaternionMultiply(QuaternionFromEuler(-25 * M_PI / 180,0,0),QuaternionFromEuler(rotationAngles.x * M_PI / 180, rotationAngles.y * M_PI / 180, rotationAngles.z * M_PI / 180)));
                drawShape3dCulled(&fbuf, &obj, 31, 50, Vector3Scale(rotatedAngle, 180 / M_PI), ORANGE);

                // Update the rotation angles
                rotationAngles.x = 180;
                rotationAngles.y += 1.0;
                rotationAngles.z = 0.0;

                if (rotationAngles.x >= 360) rotationAngles.x -= 360;
                if (rotationAngles.y >= 360) rotationAngles.y -= 360;
                if (rotationAngles.z >= 360) rotationAngles.z -= 360;

                // Copy the pixels to matrixData
                loadMatrixData(matrixData, &fbuf, 0);
                break;

            case 8: // Star field
                update_starfield();
                draw_starfield(matrixData);
                break;

        }

        if (printFrameTimes) {
            clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
            printf("%llu us\n",(uint64_t)ts.tv_sec * 1000000 + (uint64_t)ts.tv_nsec / 1000 - us);
        }

        // At this point we should have our data ready in matrixData
        if (change_frame) printf("Frame not ready!\n");
        while (!change_frame){
        };
        
        set_fpga_mem(&br, FPGA_MEM_OFFSET, matrixData, NUMPIXELS*2);
        change_frame = 0;

    } while (1);

    bridge_close(&br);
    UnloadImage(img);         // Unload CPU (RAM) image data (pixels)

    return 0;
}

void *FrameTimerThread(void *vargp) {
    const struct timespec frametimer = {0 , FRAMETIME_US};
    while(1) {
        nanosleep(&frametimer, NULL); //frame time
        *(bool*)(vargp) = 1; //new character ready
    }
    return NULL;
}

// Initialize starfield
void init_starfield() {
    for (int i = 0; i < NUM_STARS; i++) {
        stars[i].x = rand() % 64 - 32;
        stars[i].y = rand() % 64 - 32;
        stars[i].z = rand() % 64;
        stars[i].velocity = 1 + (rand() % 10) / 10.0;
    }
}

// Update starfield
void update_starfield() {
    for (int i = 0; i < NUM_STARS; i++) {
        stars[i].z -= stars[i].velocity;

        if (stars[i].z <= 0) {
            stars[i].x = rand() % 64 - 32;
            stars[i].y = rand() % 64 - 32;
            stars[i].z = 64;
            stars[i].velocity = 1 + (rand() % 10) / 10.0;
        }
    }
}

// Draw starfield
void draw_starfield(uint16_t* matrixData) {
    memset(matrixData, 0, 64 * 64 * 2 * sizeof(uint16_t)); // Clear matrixData

    for (int i = 0; i < NUM_STARS; i++) {
        int x = (int)((stars[i].x / stars[i].z) * 32 + 32);
        int y = (int)((stars[i].y / stars[i].z) * 32 + 32);

        if (x >= 0 && x < 64 && y >= 0 && y < 64) {
            int j = y * 64 + x;
            (matrixData)[j * 2] = (0xFF) << 8 | (0xFF); // G and R components
            (matrixData)[j * 2 + 1] = (0xFF); // B component
        }
    }
}

void loadMatrixData(uint16_t* matrixData, Image* fbuf, int FrameNum) {
    for (int i = 0; i < NUMPIXELS; i++)
    {
        (matrixData)[i*2] = (((uint8_t *)fbuf->data)[i*4+1+FrameNum*NUMPIXELS*4]) << 8 | (((uint8_t *)fbuf->data)[i*4+FrameNum*NUMPIXELS*4]);
        (matrixData)[i*2+1] = (((uint8_t *)fbuf->data)[i*4+2+FrameNum*NUMPIXELS*4]);
    }
}