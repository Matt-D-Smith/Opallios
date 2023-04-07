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
void drawShape3d (Image* image, shape3d* shape, int xoffset, int yoffset, float angleX, float angleY, float angleZ, Color color);
void drawShape3dCulled(Image* image, shape3d* shape, int xoffset, int yoffset, float angleX, float angleY, float angleZ, Color color);
Vector3 subtract3d(Vector3 a, Vector3 b);
Vector3 cross_product(Vector3 a, Vector3 b);
float dot_product(Vector3 a, Vector3 b);
shape3d rlMesh2Shape3d(Mesh mesh);
Vector3 rotate3d(Vector3 point, float angleX, float angleY, float angleZ);
Vector2 project(Vector3 point, float cameraDistance);