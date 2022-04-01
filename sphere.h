#ifndef SPHERE_H
#define SPHERE_H

#include <vector>
#include <cmath>
#include <iostream>
#include <iomanip>

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include "GraphicObject.h"
#include "config.h"

using namespace std;

class Sphere: public GraphicObject{
public:
    friend class Viewer;

    Sphere(int No=0, float radius=1.0f, int sectorCount=16, int stackCount=8, glm::vec3 position=glm::vec3( 0.0f,  0.0f,  0.0f), glm::vec3 color=glm::vec3( 0.0f,  0.0f,  0.0f), bool smooth=true);
    Sphere(const Sphere &other);

    ~Sphere() {}

    // getters/setters
    float getRadius() const                 { return radius; }
    int getSectorCount() const              { return sectorCount; }
    int getStackCount() const               { return stackCount; }
    void set(float radius, int sectorCount, int stackCount, bool smooth=true);
    void setRadius(float radius);
    void setSectorCount(int sectorCount);
    void setStackCount(int stackCount);
    void setSmooth(bool smooth);

    // for vertex data
    unsigned int getVertexCount() const     { return (unsigned int)vertices.size() / 3; }
    unsigned int getNormalCount() const     { return (unsigned int)normals.size() / 3; }
    unsigned int getTexCoordCount() const   { return (unsigned int)texCoords.size() / 2; }
    unsigned int getIndexCount() const      { return (unsigned int)indices.size(); }
    unsigned int getLineIndexCount() const  { return (unsigned int)lineIndices.size(); }
    unsigned int getTriangleCount() const   { return getIndexCount() / 3; }
    unsigned int getVertexSize() const      { return (unsigned int)vertices.size() * sizeof(float); }
    unsigned int getNormalSize() const      { return (unsigned int)normals.size() * sizeof(float); }
    unsigned int getTexCoordSize() const    { return (unsigned int)texCoords.size() * sizeof(float); }
    unsigned int getIndexSize() const       { return (unsigned int)indices.size() * sizeof(unsigned int); }
    unsigned int getLineIndexSize() const   { return (unsigned int)lineIndices.size() * sizeof(unsigned int); }
    const float* getVertices() const        { return vertices.data(); }
    const float* getNormals() const         { return normals.data(); }
    const float* getTexCoords() const       { return texCoords.data(); }
    const unsigned int* getIndices() const  { return indices.data(); }
    const unsigned int* getLineIndices() const  { return lineIndices.data(); }

    // for interleaved vertices: V/N/T
    unsigned int getInterleavedVertexCount() const  { return getVertexCount(); }    // # of vertices
    unsigned int getInterleavedVertexSize() const   { return (unsigned int)interleavedVertices.size() * sizeof(float); }    // # of bytes
    int getInterleavedStride() const                { return interleavedStride; }   // should be 32 bytes
    const float* getInterleavedVertices() const     { return interleavedVertices.data(); }

    // debug
    void printSelf() const;

    int getNo() const { return No; };
    void setNo(int new_no);

    glm::vec3 getColor() const { return color; };

    void setPosition(glm::vec3 new_position);
    glm::vec3 getPosition() const { return position; };

private:
    void buildVerticesSmooth();
    void buildVerticesFlat();
    void buildInterleavedVertices();
    void clearArrays();
    void addVertex(float x, float y, float z);
    void addNormal(float x, float y, float z);
    void addTexCoord(float s, float t);
    void addIndices(unsigned int i1, unsigned int i2, unsigned int i3);
    vector<float> computeFaceNormal(float x1, float y1, float z1,
                                    float x2, float y2, float z2,
                                    float x3, float y3, float z3);

    int No;
    float radius;
    int sectorCount;                        // longitude, # of slices
    int stackCount;                         // latitude, # of stacks
    bool smooth;
    vector<float> vertices;
    vector<float> normals;                  // 法线
    vector<float> texCoords;
    vector<unsigned int> indices;
    vector<unsigned int> lineIndices;

    // interleaved
    vector<float> interleavedVertices;
    int interleavedStride = 8;

    // position
    glm::vec3 position;
    glm::vec3 color;
};

#endif
