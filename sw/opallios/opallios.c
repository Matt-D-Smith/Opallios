#define _XOPEN_SOURCE 500 // needed for nanosleep
#define __USE_POSIX199309 // needed for nanosleep
#include <stdio.h>
#include <getopt.h>
#include <stdlib.h>
#include <raylib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <math.h>
#include "bw_bridge.h"

// Frame timer
#define FPS 100
#define FRAMETIME_US ((int)(1.0/FPS * 1e9)) // 10 ms / 100Hz

void *FrameTimerThread(void *vargp);

// SW rendering shapes
typedef struct shape2d {
    int numVertices;
    int numLines;
    Vector2* vertices;
    int* lineIndices;
} shape2d;

typedef struct shape3d {
    int numFaces;
    Vector3* vertices;
    int* faceVertices;
    int* numVerticesPerFace;
} shape3d;

void drawShape2d (Image* Image, shape2d* shape, int xoffset, int yoffset, int rotationAngle, Color color);
void drawShape3d (Image* image, shape3d* shape, int xoffset, int yoffset, int angleX, int angleY, int angleZ, Color color);
void drawShape3dCulled(Image* image, shape3d* shape, int xoffset, int yoffset, int angleX, int angleY, int angleZ, Color color);
Vector3 rotate3d(Vector3 point, float angleX, float angleY, float angleZ);
Vector2 project(Vector3 point, float cameraDistance);

// Fire effect
#define SCREEN_WIDTH 64
#define SCREEN_HEIGHT 64
static uint8_t fire[SCREEN_WIDTH * SCREEN_HEIGHT];

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

    // Handle input arguments
    static struct option long_opts[]= //parse arguments to read file name with -f
    {
        { "filename", required_argument, 0, 'f' }, // Filename
        { "mode"    , optional_argument, 0, 'm' }, // mode 0 = gif/image, 1 = software rendering
        { 0, 0, 0, 0 },
    };

    while((opt = getopt_long(argc, argv, "m:f:", long_opts, &opt_i)) != -1)
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
    int numPixels = 4096;

    printf("Number of Frames: %d\n", numFrames);

    // Set up a timer to have a controllable frame time
    volatile bool change_frame = 0;
    pthread_t frame_timer_thread_id;
    pthread_create(&frame_timer_thread_id, NULL, FrameTimerThread, (bool*)&change_frame);

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

    int angle = 0;

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

    int angleX = 0;
    int angleY = 0;
    int angleZ = 0;

    // Fire effect data
    Color colors[numPixels];

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
    
    // display our frames
    uint16_t matrixData[8192];
    do {
        switch (mode) {
            case 0: // display image/gif
                for (int i = 0; i < numPixels; i++)
                {
                    (matrixData)[i*2] = (((uint8_t *)img.data)[i*4+1+currentFrame*numPixels*4]) << 8 | (((uint8_t *)img.data)[i*4+currentFrame*numPixels*4]);
                    (matrixData)[i*2+1] = (((uint8_t *)img.data)[i*4+2+currentFrame*numPixels*4]);
                }
                currentFrame++;
                if (currentFrame >= numFrames) currentFrame = 0;
                break;

            case 1: // 2d shape

                // make a 2d shape and draw it
                ImageClearBackground(&fbuf,BLACK);
                drawShape2d(&fbuf, &triangle, 31, 31, angle, BLUE);
                angle += 2;
                if (angle > 360) angle = 0;

                for (int i = 0; i < numPixels; i++)
                {
                    (matrixData)[i*2] = (((uint8_t *)fbuf.data)[i*4+1]) << 8 | (((uint8_t *)fbuf.data)[i*4]);
                    (matrixData)[i*2+1] = (((uint8_t *)fbuf.data)[i*4+2]);
                }
                break;

            case 2: // 3d Triangular prism
                // Draw the 3D shape
                ImageClearBackground(&fbuf, BLACK);
                drawShape3dCulled(&fbuf, &triangularPrism, 31, 31, angleX, angleY, angleZ, BLUE);

                // Update the rotation angles
                angleX += 2;
                angleY += 2;
                angleZ += 1;

                if (angleX >= 360) angleX = 0;
                if (angleY >= 360) angleY = 0;
                if (angleZ >= 360) angleZ = 0;

                // Copy the pixels to matrixData
                for (int i = 0; i < numPixels; i++) {
                    (matrixData)[i * 2] = (((uint8_t *)fbuf.data)[i * 4 + 1]) << 8 | (((uint8_t *)fbuf.data)[i * 4]);
                    (matrixData)[i * 2 + 1] = (((uint8_t *)fbuf.data)[i * 4 + 2]);
                }
                break;

            case 3: // 3d cube
                // Draw the 3D shape
                ImageClearBackground(&fbuf, BLACK);
                drawShape3dCulled(&fbuf, &cube, 31, 31, angleX, angleY, angleZ, BLUE);

                // Update the rotation angles
                angleX += 2;
                angleY += 2;
                angleZ += 1;

                if (angleX >= 360) angleX = 0;
                if (angleY >= 360) angleY = 0;
                if (angleZ >= 360) angleZ = 0;

                // Copy the pixels to matrixData
                for (int i = 0; i < numPixels; i++) {
                    (matrixData)[i * 2] = (((uint8_t *)fbuf.data)[i * 4 + 1]) << 8 | (((uint8_t *)fbuf.data)[i * 4]);
                    (matrixData)[i * 2 + 1] = (((uint8_t *)fbuf.data)[i * 4 + 2]);
                }
                break;

            case 4: // fire effect
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

                for (int i = 0; i < numPixels; i++) {
                    (matrixData)[i*2] = (colors[fire[i]].g) << 8 | (colors[fire[i]].r);
                    (matrixData)[i*2+1] = (colors[fire[i]].b);
                }
                break;

            case 5: // Star field
                update_starfield();
                draw_starfield(matrixData);
                break;

        }

        // printf("Frame %d\n", currentFrame);

        // At this point we should have our data ready in matrixData
        if (change_frame) printf("Frame not ready!\n");
        while (!change_frame){
        };
        
        set_fpga_mem(&br, 0x4000, matrixData, 8192);
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

void drawShape2d(Image* image, shape2d* shape, int xoffset, int yoffset, int rotationAngle, Color color) {
    float costheta = cos(rotationAngle * M_PI / 180);
    float sintheta = sin(rotationAngle * M_PI / 180);
    for (int i = 0; i < shape->numLines; i++) {
        int startIdx = shape->lineIndices[i * 2];
        int endIdx = shape->lineIndices[i * 2 + 1];

        Vector2 startPoint = shape->vertices[startIdx];
        Vector2 endPoint = shape->vertices[endIdx];

        int xStartRotated = startPoint.x * costheta - startPoint.y * sintheta;
        int yStartRotated = startPoint.y * costheta + startPoint.x * sintheta;
        int xEndRotated = endPoint.x * costheta - endPoint.y * sintheta;
        int yEndRotated = endPoint.y * costheta + endPoint.x * sintheta;

        ImageDrawLine(image, xoffset + xStartRotated, yoffset + yStartRotated, xoffset + xEndRotated, yoffset + yEndRotated, color);
    }
}

// Function to perform a 3D rotation around the X, Y, and Z axes
Vector3 rotate3d(Vector3 point, float angleX, float angleY, float angleZ) {
    // Convert degrees to radians
    float radX = angleX * (M_PI / 180.0f);
    float radY = angleY * (M_PI / 180.0f);
    float radZ = angleZ * (M_PI / 180.0f);

    // Rotate around X-axis
    float y1 = cos(radX) * point.y - sin(radX) * point.z;
    float z1 = sin(radX) * point.y + cos(radX) * point.z;

    // Rotate around Y-axis
    float x2 = cos(radY) * point.x + sin(radY) * z1;
    float z2 = -sin(radY) * point.x + cos(radY) * z1;

    // Rotate around Z-axis
    float x3 = cos(radZ) * x2 - sin(radZ) * y1;
    float y3 = sin(radZ) * x2 + cos(radZ) * y1;

    Vector3 result = {x3, y3, z2};
    return result;
}

// Function to perform perspective projection
Vector2 project(Vector3 point, float cameraDistance) {
    float x = point.x * (cameraDistance / (cameraDistance - point.z));
    float y = point.y * (cameraDistance / (cameraDistance - point.z));

    Vector2 result = {x, y};
    return result;
}

// Function tp draw a 3d shape as a wireframe mesh
void drawShape3d(Image* image, shape3d* shape, int xoffset, int yoffset, int angleX, int angleY, int angleZ, Color color) {
    int totalVertices = 0;
    for (int i = 0; i < shape->numFaces; i++) {
        totalVertices += shape->numVerticesPerFace[i];
    }

    Vector2* projectedVertices = (Vector2*) malloc(totalVertices * sizeof(Vector2));

    int baseIndex = 0;
    for (int i = 0; i < shape->numFaces; i++) {
        int numVertices = shape->numVerticesPerFace[i];

        for (int j = 0; j < numVertices; j++) {
            int index = baseIndex + j;
            Vector3 rotated = rotate3d(shape->vertices[shape->faceVertices[index]], angleX, angleY, angleZ);
            projectedVertices[index] = project(rotated, 100); // You can adjust the camera distance to your preference
        }

        int* lineIndices = (int*) malloc(numVertices * 2 * sizeof(int));
        for (int j = 0; j < numVertices; j++) {
            lineIndices[j * 2] = j;
            lineIndices[j * 2 + 1] = (j + 1) % numVertices;
        }

        shape2d face = {
            .numVertices = numVertices,
            .numLines = numVertices,
            .vertices = &projectedVertices[baseIndex],
            .lineIndices = lineIndices
        };

        drawShape2d(image, &face, xoffset, yoffset, 0, color);

        free(lineIndices);
        baseIndex += numVertices;
    }

    free(projectedVertices);
}

Vector3 subtract3d(Vector3 a, Vector3 b) {
    Vector3 result;
    result.x = a.x - b.x;
    result.y = a.y - b.y;
    result.z = a.z - b.z;
    return result;
}

Vector3 cross_product(Vector3 a, Vector3 b) {
    Vector3 result;
    result.x = a.y * b.z - a.z * b.y;
    result.y = a.z * b.x - a.x * b.z;
    result.z = a.x * b.y - a.y * b.x;
    return result;
}

float dot_product(Vector3 a, Vector3 b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

// Function to perform culling so only faces facing the camera are drawn
void drawShape3dCulled(Image* image, shape3d* shape, int xoffset, int yoffset, int angleX, int angleY, int angleZ, Color color) {
    // Calculate the total number of vertices in the shape
    int totalVertices = 0;
    for (int i = 0; i < shape->numFaces; i++) {
        totalVertices += shape->numVerticesPerFace[i];
    }

    // Allocate memory for the projected 2D vertices
    Vector2* projectedVertices = (Vector2*) malloc(totalVertices * sizeof(Vector2));
    
    // Define the camera position
    Vector3 cameraPosition = {0, 0, 100};

    int baseIndex = 0;
    for (int i = 0; i < shape->numFaces; i++) {
        int numVertices = shape->numVerticesPerFace[i];

        // Allocate memory for rotated vertices
        Vector3* rotatedVertices = (Vector3*) malloc(numVertices * sizeof(Vector3));

        // Rotate the vertices and project them onto the 2D plane
        for (int j = 0; j < numVertices; j++) {
            int index = baseIndex + j;
            rotatedVertices[j] = rotate3d(shape->vertices[shape->faceVertices[index]], angleX, angleY, angleZ);
            projectedVertices[index] = project(rotatedVertices[j], 100);
        }

        // Calculate the normal vector for the face
        Vector3 normal = cross_product(subtract3d(rotatedVertices[1], rotatedVertices[0]), subtract3d(rotatedVertices[2], rotatedVertices[0]));
        Vector3 cameraVector = subtract3d(rotatedVertices[0], cameraPosition);

        // Check if the face is visible (if the dot product is positive, the face is not visible)
        if (dot_product(normal, cameraVector) < 0) {
            int* lineIndices = (int*) malloc(numVertices * 2 * sizeof(int));
            for (int j = 0; j < numVertices; j++) {
                lineIndices[j * 2] = j;
                lineIndices[j * 2 + 1] = (j + 1) % numVertices;
            }

            // Create a 2D face with the projected vertices and draw it
            shape2d face = {
                .numVertices = numVertices,
                .numLines = numVertices,
                .vertices = &projectedVertices[baseIndex],
                .lineIndices = lineIndices
            };

            drawShape2d(image, &face, xoffset, yoffset, 0, color);

            // Free the memory allocated for lineIndices
            free(lineIndices);
        }

        // Free the memory allocated for rotatedVertices
        free(rotatedVertices);
        baseIndex += numVertices;
    }

    // Free the memory allocated for projectedVertices
    free(projectedVertices);
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