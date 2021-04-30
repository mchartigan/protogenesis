///////////////////////////////////////////////////////////////////////////////
// Planet.h
// ========
// Planet for OpenGL with (radius, sectors, stacks)
// The min number of sectors is 3 and The min number of stacks are 2.
//
//  AUTHOR: Song Ho Ahn (song.ahn@gmail.com)
// CREATED: 2017-11-01
// UPDATED: 2020-05-20
///////////////////////////////////////////////////////////////////////////////

#ifndef GEOMETRY_Planet_H
#define GEOMETRY_Planet_H

#include <vector>

struct Vertex
{
    float x, y, z;
    float r = 1.0, g = 0.0, b = 0.0, a = 1.0;
};

struct Params
{
    double R = 6357000, M = 5.9722e24, D = 86164.0;
    float S = 0.1, T = 15.0, W = 0.57;
    bool terrestrial = true;
    float red = 0.0, green = 0.0, blue = 0.0;
};

class Planet
{
public:
    // ctor/dtor
    Planet(Params params, float radius=1.0f, int sectorCount=36, int stackCount=18);
    Planet() {}
    ~Planet() {}

    // getters/setters
    float getRadius() const                 { return radius; }
    int getSectorCount() const              { return sectorCount; }
    int getStackCount() const               { return stackCount; }
    void set(float radius, int sectorCount, int stackCount);
    void setRadius(float radius);
    void setSectorCount(int sectorCount);
    void setStackCount(int stackCount);
    void setTexture(int, int);

    // for vertex data
    unsigned int getVertexCount() const     { return (unsigned int)vertices.size() / 3; }
    unsigned int getNormalCount() const     { return (unsigned int)normals.size() / 3; }
    unsigned int getColorCount() const      { return (unsigned int)colors.size() / 4; }
    unsigned int getIndexCount() const      { return (unsigned int)indices.size(); }
    unsigned int getLineIndexCount() const  { return (unsigned int)lineIndices.size(); }
    unsigned int getTriangleCount() const   { return getIndexCount() / 3; }
    unsigned int getVertexSize() const      { return (unsigned int)vertices.size() * sizeof(float); }
    unsigned int getNormalSize() const      { return (unsigned int)normals.size() * sizeof(float); }
    unsigned int getColorSize() const       { return (unsigned int)colors.size() * sizeof(float); }
    unsigned int getIndexSize() const       { return (unsigned int)indices.size() * sizeof(unsigned int); }
    unsigned int getLineIndexSize() const   { return (unsigned int)lineIndices.size() * sizeof(unsigned int); }
    const float* getVertices() const        { return vertices.data(); }
    const float* getNormals() const         { return normals.data(); }
    const float* getColors() const          { return colors.data(); }
    const unsigned int* getIndices() const  { return indices.data(); }
    const unsigned int* getLineIndices() const  { return lineIndices.data(); }

    // for interleaved vertices: V/N/T
    unsigned int getInterleavedVertexCount() const  { return getVertexCount(); }    // # of vertices
    unsigned int getInterleavedVertexSize() const   { return (unsigned int)interleavedVertices.size() * sizeof(float); }    // # of bytes
    int getInterleavedStride() const                { return interleavedStride; }   // should be 32 bytes
    const float* getInterleavedVertices() const     { return interleavedVertices.data(); }

    // draw in VertexArray mode
    void draw() const;                                  // draw surface
    void drawLines(const float lineColor[4]) const;     // draw lines only
    void drawWithLines(const float lineColor[4]) const; // draw surface and lines

    // debug
    void printSelf() const;

protected:

private:
    // member functions
    void buildVertices();
    Vertex colorVertex(char c, float aR, float latitude, float vec[3]);
    void buildInterleavedVertices();
    void clearArrays();
    void addVertex(float x, float y, float z);
    void addNormal(float x, float y, float z);
    void addColor(float r, float g, float b, float a);
    void addIndices(unsigned int i1, unsigned int i2, unsigned int i3);
    std::vector<float> computeFaceNormal(float x1, float y1, float z1,
                                         float x2, float y2, float z2,
                                         float x3, float y3, float z3);

    // member vars
    float radius;
    int sectorCount;                        // longitude, # of slices
    int stackCount;                         // latitude, # of stacks
    std::vector<float> vertices;
    std::vector<float> normals;
    std::vector<float> colors;
    std::vector<unsigned int> indices;
    std::vector<unsigned int> lineIndices;
    float** tex;
    float minHeight = 0.0;
    float maxHeight = 0.0;
    float dH;
    float res = 2.0;

    float PI = acos(-1);
    double dPI = acos(-1);

    // planet properties
    double G = 6.674e-11;    // gravitational constant (m^3 / kg*s^2)
    double M;       // mass (kg)
    double R;       // radius (m)
    double day;     // sidereal day (s)
    float water;    // water level as a %
    float K;
    float temp;
    bool terrestrial;
    float red, green, blue;

    // interleaved
    std::vector<float> interleavedVertices;
    int interleavedStride;                  // # of bytes to hop to the next vertex (should be 32 bytes)

};

#endif
