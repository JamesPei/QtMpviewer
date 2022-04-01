#include "sphere.h"

const int MIN_SECTOR_COUNT = 3;
const int MIN_STACK_COUNT  = 2;

Sphere::Sphere(int No, float radius, int sectors, int stacks, glm::vec3 position, glm::vec3 color, bool smooth):position(position), color(color), No(No){
    set(radius, sectors, stacks, smooth);

    for(int i = 0; i < getVertexCount(); ++i){
        vertices[i*3] += position.x;
        vertices[i*3 + 1] += position.y;
        vertices[i*3 + 2] += position.z;
    }
    buildInterleavedVertices();
}

Sphere::Sphere(const Sphere &other){
    this->No = other.No;
    this->radius = other.radius;
    this->sectorCount = other.sectorCount;
    this->stackCount = other.stackCount;
    this->smooth = other.smooth;
    this->vertices = other.vertices;
    this->normals = other.normals;
    this->texCoords = other.texCoords;
    this->indices = other.indices;
    this->lineIndices = other.lineIndices;
    this->interleavedVertices = other.interleavedVertices;
    this->position = other.position;
    this->color = other.color;
};

void Sphere::set(float radius, int sectors, int stacks, bool smooth){
    this->radius = radius;
    this->sectorCount = sectors;
    if(sectors < MIN_SECTOR_COUNT)
        this->sectorCount = MIN_SECTOR_COUNT;
    this->stackCount = stacks;
    if(sectors < MIN_STACK_COUNT)
        this->sectorCount = MIN_STACK_COUNT;
    this->smooth = smooth;

    if(smooth) buildVerticesSmooth();
    else buildVerticesFlat();
}

void Sphere::setNo(int new_no){
    this->No = new_no;
}

void Sphere::setRadius(float radius){
    if(radius != this->radius)
        set(radius, sectorCount, stackCount, smooth);
}

void Sphere::setSectorCount(int sectors){
    if(sectors != this->sectorCount)
        set(radius, sectors, stackCount, smooth);
}

void Sphere::setStackCount(int stacks){
    if(stacks != this->stackCount)
        set(radius, sectorCount, stacks, smooth);
}

void Sphere::setPosition(glm::vec3 new_position){
    this->position = new_position;
    for(int i = 0; i < getVertexCount(); ++i){
        vertices[i*3] += new_position.x;
        vertices[i*3 + 1] += new_position.y;
        vertices[i*3 + 2] += new_position.z;
    }
    buildInterleavedVertices();
};

void Sphere::setSmooth(bool smooth){
    if(this->smooth == smooth)
        return;

    this->smooth = smooth;
    if(smooth)
        buildVerticesSmooth();
    else
        buildVerticesFlat();
}

///////////////////////////////////////////////////////////////////////////////
// print itself
///////////////////////////////////////////////////////////////////////////////
void Sphere::printSelf() const{
    cout << "===== Sphere =====\n"
         << "        Radius: " << radius << "\n"
         << "  Sector Count: " << sectorCount << "\n"
         << "   Stack Count: " << stackCount << "\n"
         << "Smooth Shading: " << (smooth ? "true" : "false") << "\n"
         << "Triangle Count: " << getTriangleCount() << "\n"
         << "   Index Count: " << getIndexCount() << "\n"
         << "  Vertex Count: " << getVertexCount() << "\n"
         << "  Normal Count: " << getNormalCount() << "\n"
         << "TexCoord Count: " << getTexCoordCount() << endl;
}

void Sphere::clearArrays(){
    vector<float>().swap(vertices);
    vector<float>().swap(normals);
    vector<float>().swap(texCoords);
    vector<unsigned int>().swap(indices);
    vector<unsigned int>().swap(lineIndices);
}

///////////////////////////////////////////////////////////////////////////////
// build vertices of sphere with smooth shading using parametric equation
// x = r * cos(u) * cos(v)
// y = r * cos(u) * sin(v)
// z = r * sin(u)
// where u: stack(latitude) angle (-90 <= u <= 90)
//       v: sector(longitude) angle (0 <= v <= 360)
///////////////////////////////////////////////////////////////////////////////
void Sphere::buildVerticesSmooth(){
    // clear memory of prev arrays
    clearArrays();

    float x, y, z, xy;                              // vertex position
    float nx, ny, nz, lengthInv = 1.0f / radius;    // normal
    float s, t;                                     // texCoord

    float sectorStep = 2 * PI / sectorCount;
    float stackStep = PI / stackCount;
    float sectorAngle, stackAngle;

    for(int i = 0; i <= stackCount; ++i){
        stackAngle = PI / 2 - i * stackStep;        // starting from pi/2 to -pi/2
        xy = radius * cosf(stackAngle);             // r * cos(u)
        z = fabs(radius * sinf(stackAngle))>0.01 ? radius * sinf(stackAngle): 0.0f;           // r * sin(u)

        // add (sectorCount+1) vertices per stack
        // the first and last vertices have same position and normal, but different tex coords
        for(int j = 0; j < sectorCount; ++j){
            sectorAngle = j * sectorStep;           // starting from 0 to 2pi

            // vertex position
            x = fabs(xy * cosf(sectorAngle))>0.01 ? xy * cosf(sectorAngle): 0.0f;             // r * cos(u) * cos(v)
            y = fabs(xy * sinf(sectorAngle))>0.01 ? xy * sinf(sectorAngle): 0.0f;             // r * cos(u) * sin(v)
            addVertex(x, y, z);

            // normalized vertex normal
            nx = x * lengthInv;
            ny = y * lengthInv;
            nz = z * lengthInv;
            addNormal(nx, ny, nz);

            // vertex tex coord between [0, 1]
            s = (float)j / sectorCount;
            t = (float)i / stackCount;
            addTexCoord(s, t);
        }
    }

    // generate CCW index list of sphere triangles
    // k1--k1+1
    // |  / |
    // | /  |
    // k2--k2+1
    int k1, k2;
    for(int i = 0; i < stackCount; ++i){
        k1 = i * sectorCount;     // beginning of current stack
        k2 = k1 + sectorCount;

        for(int j = 0; j < sectorCount; ++j, ++k1, ++k2){   //中间点
            // 2 triangles per sector excluding first and last stacks
            // k1 => k2 => k1+1
            // k1+1 => k2 => k2+1
            indices.push_back(k1);
            indices.push_back(k2);

            int k_1 = k1+1;

            if(k_1 < sectorCount*(i+1)){
                indices.push_back(k_1);
                indices.push_back(k_1);
            }else{
                k_1 = k_1 - sectorCount;
                indices.push_back(k_1);
                indices.push_back(k_1);
            }
            // cout << "k1:" << k1 << "  k2:" << k2 << "  k1+1:" << k_1 << "\n";

            indices.push_back(k2);
            int k_2 = k2+1;
            if(k_2 < sectorCount*(i+2)){
                indices.push_back(k_2);
            }else{
                k_2 = k_2 - sectorCount;
                indices.push_back(k_2);
            }
            // cout << "k1+1:" << k_1 << "  k2:" << k2 << "  k2+1:" << k_2 << "\n";

            // store indices for lines
            // vertical lines for all stacks, k1 => k2
            // lineIndices.push_back(k1);
            // lineIndices.push_back(k2);
            // if(i != 0) { // horizontal lines except 1st stack, k1 => k+1
            //     lineIndices.push_back(k1);
            //     lineIndices.push_back(k1 + 1);
            // }
        }
    }

    // generate interleaved vertex array as well
    buildInterleavedVertices();
}

///////////////////////////////////////////////////////////////////////////////
// generate vertices with flat shading
// each triangle is independent (no shared vertices)
///////////////////////////////////////////////////////////////////////////////
void Sphere::buildVerticesFlat(){
    // tmp vertex definition (x,y,z,s,t)
    struct Vertex{
        float x, y, z, s, t;
    };
    vector<Vertex> tmpVertices;

    float sectorStep = 2 * PI / sectorCount;
    float stackStep = PI / stackCount;
    float sectorAngle, stackAngle;

    // compute all vertices first, each vertex contains (x,y,z,s,t) except normal
    for(int i = 0; i <= stackCount; ++i){
        stackAngle = PI / 2 - i * stackStep;        // starting from pi/2 to -pi/2
        float xy = radius * cosf(stackAngle);       // r * cos(u)
        float z = radius * sinf(stackAngle);        // r * sin(u)

        // add (sectorCount+1) vertices per stack
        // the first and last vertices have same position and normal, but different tex coords
        for(int j = 0; j <= sectorCount; ++j){
            sectorAngle = j * sectorStep;           // starting from 0 to 2pi

            Vertex vertex;
            vertex.x = xy * cosf(sectorAngle);      // x = r * cos(u) * cos(v)
            vertex.y = xy * sinf(sectorAngle);      // y = r * cos(u) * sin(v)
            vertex.z = z;                           // z = r * sin(u)
            vertex.s = (float)j/sectorCount;        // s
            vertex.t = (float)i/stackCount;         // t
            tmpVertices.push_back(vertex);
        }
    }

    // clear memory of prev arrays
    clearArrays();

    Vertex v1, v2, v3, v4;                          // 4 vertex positions and tex coords
    vector<float> n;                           // 1 face normal

    int i, j, k, vi1, vi2;
    int index = 0;                                  // index for vertex
    for(i = 0; i < stackCount; ++i){
        vi1 = i * (sectorCount + 1);                // index of tmpVertices
        vi2 = (i + 1) * (sectorCount + 1);

        for(j = 0; j < sectorCount; ++j, ++vi1, ++vi2){
            // get 4 vertices per sector
            //  v1--v3
            //  |    |
            //  v2--v4
            v1 = tmpVertices[vi1];
            v2 = tmpVertices[vi2];
            v3 = tmpVertices[vi1 + 1];
            v4 = tmpVertices[vi2 + 1];

            // if 1st stack and last stack, store only 1 triangle per sector
            // otherwise, store 2 triangles (quad) per sector
            if(i == 0){ // a triangle for first stack ==========================
                // put a triangle
                addVertex(v1.x, v1.y, v1.z);
                addVertex(v2.x, v2.y, v2.z);
                addVertex(v4.x, v4.y, v4.z);

                // put tex coords of triangle
                addTexCoord(v1.s, v1.t);
                addTexCoord(v2.s, v2.t);
                addTexCoord(v4.s, v4.t);

                // put normal
                n = computeFaceNormal(v1.x,v1.y,v1.z, v2.x,v2.y,v2.z, v4.x,v4.y,v4.z);
                for(k = 0; k < 3; ++k)  // same normals for 3 vertices
                {
                    addNormal(n[0], n[1], n[2]);
                }

                // put indices of 1 triangle
                addIndices(index, index+1, index+2);

                // indices for line (first stack requires only vertical line)
                lineIndices.push_back(index);
                lineIndices.push_back(index+1);

                index += 3;     // for next
            } else if(i == (stackCount-1)) {// a triangle for last stack =========
                // put a triangle
                addVertex(v1.x, v1.y, v1.z);
                addVertex(v2.x, v2.y, v2.z);
                addVertex(v3.x, v3.y, v3.z);

                // put tex coords of triangle
                addTexCoord(v1.s, v1.t);
                addTexCoord(v2.s, v2.t);
                addTexCoord(v3.s, v3.t);

                // put normal
                n = computeFaceNormal(v1.x,v1.y,v1.z, v2.x,v2.y,v2.z, v3.x,v3.y,v3.z);
                for(k = 0; k < 3; ++k){  // same normals for 3 vertices
                    addNormal(n[0], n[1], n[2]);
                }

                // put indices of 1 triangle
                addIndices(index, index+1, index+2);

                // indices for lines (last stack requires both vert/hori lines)
                lineIndices.push_back(index);
                lineIndices.push_back(index+1);
                lineIndices.push_back(index);
                lineIndices.push_back(index+2);

                index += 3;     // for next
            }else{ // 2 triangles for others ====================================
                // put quad vertices: v1-v2-v3-v4
                addVertex(v1.x, v1.y, v1.z);
                addVertex(v2.x, v2.y, v2.z);
                addVertex(v3.x, v3.y, v3.z);
                addVertex(v4.x, v4.y, v4.z);

                // put tex coords of quad
                addTexCoord(v1.s, v1.t);
                addTexCoord(v2.s, v2.t);
                addTexCoord(v3.s, v3.t);
                addTexCoord(v4.s, v4.t);

                // put normal
                n = computeFaceNormal(v1.x,v1.y,v1.z, v2.x,v2.y,v2.z, v3.x,v3.y,v3.z);
                for(k = 0; k < 4; ++k){  // same normals for 4 vertices
                    addNormal(n[0], n[1], n[2]);
                }

                // put indices of quad (2 triangles)
                addIndices(index, index+1, index+2);
                addIndices(index+2, index+1, index+3);

                // indices for lines
                lineIndices.push_back(index);
                lineIndices.push_back(index+1);
                lineIndices.push_back(index);
                lineIndices.push_back(index+2);

                index += 4;     // for next
            }
        }
    }

    // generate interleaved vertex array as well
    buildInterleavedVertices();
}

///////////////////////////////////////////////////////////////////////////////
// generate interleaved vertices: V/N/T
// stride must be 32 bytes
///////////////////////////////////////////////////////////////////////////////
void Sphere::buildInterleavedVertices(){
    vector<float>().swap(interleavedVertices);

    size_t i, j;
    size_t count = vertices.size();
    for(i = 0, j = 0; i < count; i += 3, j += 2){
        interleavedVertices.push_back(vertices[i]);
        interleavedVertices.push_back(vertices[i+1]);
        interleavedVertices.push_back(vertices[i+2]);

        interleavedVertices.push_back(normals[i]);
        interleavedVertices.push_back(normals[i+1]);
        interleavedVertices.push_back(normals[i+2]);

        interleavedVertices.push_back(texCoords[j]);
        interleavedVertices.push_back(texCoords[j+1]);
    }
}

///////////////////////////////////////////////////////////////////////////////
// add single vertex to array
///////////////////////////////////////////////////////////////////////////////
void Sphere::addVertex(float x, float y, float z){
    // cout << "addVertex：" << x << "  " << y << "  " << z << "\n";
    if(fabs(x) < 0.01) x=0.0;
    if(fabs(y) < 0.01) y=0.0;
    if(fabs(z) < 0.01) z=0.0;
    for(auto v = vertices.begin(); v != vertices.end(); ++v){

    }
    vertices.push_back(x);
    vertices.push_back(y);
    vertices.push_back(z);
}



///////////////////////////////////////////////////////////////////////////////
// add single normal to array
///////////////////////////////////////////////////////////////////////////////
void Sphere::addNormal(float nx, float ny, float nz){
    if(fabs(nx) < 0.01) nx=0.0;
    if(fabs(ny) < 0.01) ny=0.0;
    if(fabs(nz) < 0.01) nz=0.0;
    normals.push_back(nx);
    normals.push_back(ny);
    normals.push_back(nz);
}

///////////////////////////////////////////////////////////////////////////////
// add single texture coord to array
///////////////////////////////////////////////////////////////////////////////
void Sphere::addTexCoord(float s, float t){
    if(fabs(s) < 0.01) s=0.0;
    if(fabs(t) < 0.01) t=0.0;
    texCoords.push_back(s);
    texCoords.push_back(t);
}



///////////////////////////////////////////////////////////////////////////////
// add 3 indices to array
///////////////////////////////////////////////////////////////////////////////
void Sphere::addIndices(unsigned int i1, unsigned int i2, unsigned int i3){
    // cout << "addIndices:" << i1 << "  " << i2 << "  " << i3 << "\n";
    indices.push_back(i1);
    indices.push_back(i2);
    indices.push_back(i3);
}

///////////////////////////////////////////////////////////////////////////////
// return face normal of a triangle v1-v2-v3
// if a triangle has no surface (normal length = 0), then return a zero vector
///////////////////////////////////////////////////////////////////////////////
std::vector<float> Sphere::computeFaceNormal(float x1, float y1, float z1,  // v1
                                             float x2, float y2, float z2,  // v2
                                             float x3, float y3, float z3)  // v3
{
    const float EPSILON = 0.000001f;

    vector<float> normal(3, 0.0f);     // default return value (0,0,0)
    float nx, ny, nz;

    // find 2 edge vectors: v1-v2, v1-v3
    float ex1 = x2 - x1;
    float ey1 = y2 - y1;
    float ez1 = z2 - z1;
    float ex2 = x3 - x1;
    float ey2 = y3 - y1;
    float ez2 = z3 - z1;

    // cross product: e1 x e2
    nx = ey1 * ez2 - ez1 * ey2;
    ny = ez1 * ex2 - ex1 * ez2;
    nz = ex1 * ey2 - ey1 * ex2;

    // normalize only if the length is > 0
    float length = sqrtf(nx * nx + ny * ny + nz * nz);
    if(length > EPSILON){
        // normalize
        float lengthInv = 1.0f / length;
        normal[0] = nx * lengthInv;
        normal[1] = ny * lengthInv;
        normal[2] = nz * lengthInv;
    }

    return normal;
}
