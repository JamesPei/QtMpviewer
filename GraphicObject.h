#ifndef GRAPHICOBJECT_H
#define GRAPHICOBJECT_H
#include <vector>

#include <glm/glm.hpp>

using namespace std;

class GraphicObject{
public:
    GraphicObject(){};

    virtual ~GraphicObject() = 0;

    virtual unsigned int getVertexCount() const = 0;

    virtual unsigned int getIndexCount() const = 0;
    virtual unsigned int getTriangleCount() const = 0;

    virtual const unsigned int* getIndices() const = 0;

    virtual const float* getInterleavedVertices() const = 0;

    virtual unsigned int getInterleavedVertexCount() const = 0;
    virtual unsigned int getInterleavedVertexSize() const = 0;

    virtual int getInterleavedStride() const = 0;

    virtual glm::vec3 getColor() const = 0;

    void setColor(glm::vec3 new_color) { color = new_color; };

private:
    vector<float> vertices;
    vector<unsigned int> indices;
    vector<float> interleavedVertices;
    int interleavedStride = 8;
    glm::vec3 color;
};

#endif // GRAPHICOBJECT_H
