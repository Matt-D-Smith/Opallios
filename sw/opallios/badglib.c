#include <raylib.h>
#include "badglib.h"
#include <math.h>
#include <stdlib.h>

void drawShape2d(Image* image, shape2d* shape, int xoffset, int yoffset, float rotationAngle, Color color) {
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

// Function to draw a 3d shape as a wireframe mesh
void drawShape3d(Image* image, shape3d* shape, int xoffset, int yoffset, float angleX, float angleY, float angleZ, Color color) {
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
            projectedVertices[index] = project(rotated, 200); // You can adjust the camera distance to your preference
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
void drawShape3dCulled(Image* image, shape3d* shape, int xoffset, int yoffset, float angleX, float angleY, float angleZ, Color color) {
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

shape3d rlMesh2Shape3d(Mesh mesh) {
    int numFaces = mesh.triangleCount;
    int totalVertices = mesh.vertexCount;

    Vector3* vertices = (Vector3*) malloc(totalVertices * sizeof(Vector3));
    for (int i = 0; i < totalVertices; i++) {
        vertices[i] = (Vector3){mesh.vertices[i * 3], mesh.vertices[i * 3 + 1], mesh.vertices[i * 3 + 2]};
    }

    int* faceVertices = NULL;
    int* numVerticesPerFace = NULL;

    if (mesh.indices != NULL) {
        faceVertices = (int*) malloc(totalVertices * sizeof(int));
        for (int i = 0; i < totalVertices; i++) {
            faceVertices[i] = (int)mesh.indices[i];
        }
    } else {
        faceVertices = (int*) malloc(totalVertices * sizeof(int));
        for (int i = 0; i < totalVertices; i++) {
            faceVertices[i] = i;
        }
    }

    numVerticesPerFace = (int*) malloc(numFaces * sizeof(int));
    for (int i = 0; i < numFaces; i++) {
        numVerticesPerFace[i] = 3; // Assuming the mesh consists of triangles
    }

    shape3d result = {
        .numFaces = numFaces,
        .vertices = vertices,
        .faceVertices = faceVertices,
        .numVerticesPerFace = numVerticesPerFace
    };

    return result;
}