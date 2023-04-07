// Bad Graphics Library

#include <raylib.h>

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

void drawShape2d (Image* Image, shape2d* shape, int xoffset, int yoffset, float rotationAngle, Color color);
void drawShape3d (Image* image, shape3d* shape, int xoffset, int yoffset, Vector3 rotationAngles, Color color);
void drawShape3dCulled(Image* image, shape3d* shape, int xoffset, int yoffset, Vector3 rotationAngles, Color color);
shape3d rlMesh2Shape3d(Mesh mesh);
Vector3 rotate3d(Vector3 point, Vector3 rotationAngles);
Vector2 project(Vector3 point, float cameraDistance);