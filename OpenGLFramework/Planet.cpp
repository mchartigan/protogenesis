///////////////////////////////////////////////////////////////////////////////
// Planet.cpp
// ==========
// Planet for OpenGL with (radius, sectors, stacks)
// The min number of sectors is 3 and the min number of stacks are 2.
//
//  AUTHOR: Song Ho Ahn (song.ahn@gmail.com)
// CREATED: 2017-11-01
// UPDATED: 2020-05-20
///////////////////////////////////////////////////////////////////////////////

#ifdef _WIN32
#include <windows.h>    // include windows.h to avoid thousands of compile errors even though this class is not depending on Windows
#endif

#ifdef __APPLE__
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif

#include <iostream>
#include <iomanip>
#include <cmath>
#include "Planet.h"
#include "Noise.h"



// constants //////////////////////////////////////////////////////////////////
const int MIN_SECTOR_COUNT = 3;
const int MIN_STACK_COUNT  = 2;



///////////////////////////////////////////////////////////////////////////////
// ctor
///////////////////////////////////////////////////////////////////////////////
Planet::Planet(Params params, float radius, int sectors, int stacks) : interleavedStride(40)
{
    R = params.R;
    M = params.M;
    day = params.D;
    K = params.S;
    temp = params.T;
    water = params.W;
    terrestrial = params.terrestrial;
    red = params.red; green = params.green; blue = params.blue;
    set(radius, sectors, stacks);
}



///////////////////////////////////////////////////////////////////////////////
// setters
///////////////////////////////////////////////////////////////////////////////
void Planet::set(float radius, int sectors, int stacks)
{
    this->radius = radius;
    this->sectorCount = sectors;
    if(sectors < MIN_SECTOR_COUNT)
        this->sectorCount = MIN_SECTOR_COUNT;
    this->stackCount = stacks;
    if(sectors < MIN_STACK_COUNT)
        this->sectorCount = MIN_STACK_COUNT;
    setTexture(stacks, sectors);
    

    buildVertices();
}

void Planet::setRadius(float radius)
{
    if(radius != this->radius)
        set(radius, sectorCount, stackCount);
}

void Planet::setSectorCount(int sectors)
{
    if(sectors != this->sectorCount)
        set(radius, sectors, stackCount);
}

void Planet::setStackCount(int stacks)
{
    if(stacks != this->stackCount)
        set(radius, sectorCount, stacks);
}

float recnoise(float vec[3], float freq=1, float size=1) {
    if (freq > 32) return 0;
    else {
        float coord[3] = { vec[0] * freq, vec[1] * freq, vec[2] * freq };

        return noise3(coord) * size + recnoise(vec, freq * 2, size / 2);
    }
}

void Planet::setTexture(int stacks, int sectors)
{
    // texture goes from 0 - stacks and 0 - sectors (inclusive)
    tex = new float* [stacks + 1];
    for (int i = 0; i <= stacks; i++) {
        tex[i] = new float[sectors + 1];
    }

    const float PI = acos(-1);

    float sectorStep = 2 * PI / sectors;
    float stackStep = PI / stacks;
    float sectorAngle, stackAngle;

    // compute all vertices first, each vertex contains (x,y,z,s,t) except normal
    for (int i = 0; i <= stacks; ++i)
    {
        stackAngle = PI / 2 - i * stackStep;        // starting from pi/2 to -pi/2

        for (int j = 0; j <= sectors; ++j)
        {
            // std::cout << i << ", " << j << std::endl;
            sectorAngle = j * sectorStep;           // starting from 0 to 2pi

            float xy = radius * cosf(stackAngle);       // r * cos(u)
            float z = radius * sinf(stackAngle);        // r * sin(u)

            float x = xy * cosf(sectorAngle);      // x = r * cos(u) * cos(v)
            float y = xy * sinf(sectorAngle);      // y = r * cos(u) * sin(v)

            float c[3] = { x * res, y * res, z * res };
            tex[i][j] = recnoise(c);

            if (tex[i][j] < minHeight) minHeight = tex[i][j];
            else if (tex[i][j] > maxHeight) maxHeight = tex[i][j];

            //std::cout << tex[i][j] << ", ";
        }
        //std::cout << std::endl;
    }
    // std::cout << "Texture set." << std::endl;

    dH = maxHeight - minHeight;
}



///////////////////////////////////////////////////////////////////////////////
// print itself
///////////////////////////////////////////////////////////////////////////////
void Planet::printSelf() const
{
    std::cout << "===== Planet =====\n"
              << "        Radius: " << radius << "\n"
              << "  Sector Count: " << sectorCount << "\n"
              << "   Stack Count: " << stackCount << "\n"
              << "Triangle Count: " << getTriangleCount() << "\n"
              << "   Index Count: " << getIndexCount() << "\n"
              << "  Vertex Count: " << getVertexCount() << "\n"
              << "  Normal Count: " << getNormalCount() << "\n"
              << "   Color Count: " << getColorCount() << std::endl;
}



///////////////////////////////////////////////////////////////////////////////
// draw a Planet in VertexArray mode
// OpenGL RC must be set before calling it
///////////////////////////////////////////////////////////////////////////////
void Planet::draw() const
{
    // interleaved array
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_NORMAL_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);

    glVertexPointer(3, GL_FLOAT, interleavedStride, &interleavedVertices[0]);
    glNormalPointer(GL_FLOAT, interleavedStride, &interleavedVertices[3]);
    glColorPointer(4, GL_FLOAT, interleavedStride, &interleavedVertices[6]);

    glDrawElements(GL_TRIANGLES, (unsigned int)indices.size(), GL_UNSIGNED_INT, indices.data());

    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_NORMAL_ARRAY);
    glDisableClientState(GL_COLOR_ARRAY);
}



///////////////////////////////////////////////////////////////////////////////
// draw lines only
// the caller must set the line width before call this
///////////////////////////////////////////////////////////////////////////////
void Planet::drawLines(const float lineColor[4]) const
{
    // set line colour
    glColor4fv(lineColor);
    glMaterialfv(GL_FRONT, GL_DIFFUSE,   lineColor);

    // draw lines with VA
    glDisable(GL_LIGHTING);
    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(3, GL_FLOAT, 0, vertices.data());

    glDrawElements(GL_LINES, (unsigned int)lineIndices.size(), GL_UNSIGNED_INT, lineIndices.data());

    glDisableClientState(GL_VERTEX_ARRAY);
    glEnable(GL_LIGHTING);
}



///////////////////////////////////////////////////////////////////////////////
// draw a Planet surfaces and lines on top of it
// the caller must set the line width before call this
///////////////////////////////////////////////////////////////////////////////
void Planet::drawWithLines(const float lineColor[4]) const
{
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(1.0, 1.0f); // move polygon backward
    this->draw();
    glDisable(GL_POLYGON_OFFSET_FILL);

    // draw lines with VA
    drawLines(lineColor);
}



///////////////////////////////////////////////////////////////////////////////
// dealloc vectors
///////////////////////////////////////////////////////////////////////////////
void Planet::clearArrays()
{
    std::vector<float>().swap(vertices);
    std::vector<float>().swap(normals);
    std::vector<float>().swap(colors);
    std::vector<unsigned int>().swap(indices);
    std::vector<unsigned int>().swap(lineIndices);
}



///////////////////////////////////////////////////////////////////////////////
// generate vertices with flat shading
// each triangle is independent (no shared vertices)
///////////////////////////////////////////////////////////////////////////////
void Planet::buildVertices()
{
    std::vector<Vertex> tmpVertices;

    float sectorStep = 2 * PI / sectorCount;
    float stackStep = PI / stackCount;
    float sectorAngle, stackAngle;
    double omega = 2 * dPI / day;
    double h = pow(R, 4) * pow(omega, 2) / (G * M);
    h = h / R;  //normalize to 1

    // compute all vertices first, each vertex contains (x,y,z,s,t) except normal
    for(int i = 0; i <= stackCount; ++i)
    {
        stackAngle = PI / 2 - i * stackStep;        // starting from pi/2 to -pi/2

        // add (sectorCount+1) vertices per stack
        // the first and last vertices have same position and normal, but different tex coords
        for(int j = 0; j <= sectorCount; ++j)
        {
            // std::cout << i << ", " << j << std::endl;
            sectorAngle = j * sectorStep;           // starting from 0 to 2pi
            
            float adjRadius1 = radius + tex[i][j] * K;
            float adjRadius2;

            if (adjRadius1 < radius + (minHeight + dH * water) * K) {
                adjRadius2 = radius + (minHeight + dH * water) * K + tex[i][j] * pow(K, 2); // smooth out water
            }
            else adjRadius2 = adjRadius1;
            float xy = (adjRadius2 + h) * cosf(stackAngle); // r * cos(u); adjust for oblateness
            float z = adjRadius2 * sinf(stackAngle);        // r * sin(u)

            Vertex vertex;
            vertex.x = xy * cosf(sectorAngle);      // x = r * cos(u) * cos(v)
            vertex.y = xy * sinf(sectorAngle);      // y = r * cos(u) * sin(v)
            vertex.z = z;                           // z = r * sin(u)

            float vec[3] = { vertex.x, vertex.y, vertex.z };
            Vertex color = colorVertex('e', adjRadius1, stackAngle, vec);

            vertex.r = color.r;
            vertex.g = color.g;
            vertex.b = color.b;
            vertex.a = color.a;

            tmpVertices.push_back(vertex);
        }
    }

    // clear memory of prev arrays
    clearArrays();

    Vertex v1, v2, v3, v4;                          // 4 vertex positions and tex coords
    std::vector<float> n;                           // 1 face normal

    int i, j, k, vi1, vi2;
    int index = 0;                                  // index for vertex
    for(i = 0; i < stackCount; ++i)
    {
        vi1 = i * (sectorCount + 1);                // index of tmpVertices
        vi2 = (i + 1) * (sectorCount + 1);

        for(j = 0; j < sectorCount; ++j, ++vi1, ++vi2)
        {
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
            if(i == 0) // a triangle for first stack ==========================
            {
                // put a triangle
                addVertex(v1.x, v1.y, v1.z);
                addVertex(v2.x, v2.y, v2.z);
                addVertex(v4.x, v4.y, v4.z);

                // put color of triangle (temp red)
                addColor(v1.r, v1.g, v1.b, v1.a);
                addColor(v2.r, v2.g, v2.b, v2.a);
                addColor(v4.r, v4.g, v4.b, v4.a);

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
            }
            else if(i == (stackCount-1)) // a triangle for last stack =========
            {
                // put a triangle
                addVertex(v1.x, v1.y, v1.z);
                addVertex(v2.x, v2.y, v2.z);
                addVertex(v3.x, v3.y, v3.z);

                // put color of triangle (temp red)
                addColor(v1.r, v1.g, v1.b, v1.a);
                addColor(v2.r, v2.g, v2.b, v2.a);
                addColor(v3.r, v3.g, v3.b, v3.a);

                // put normal
                n = computeFaceNormal(v1.x,v1.y,v1.z, v2.x,v2.y,v2.z, v3.x,v3.y,v3.z);
                for(k = 0; k < 3; ++k)  // same normals for 3 vertices
                {
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
            }
            else // 2 triangles for others ====================================
            {
                // put quad vertices: v1-v2-v3-v4
                addVertex(v1.x, v1.y, v1.z);
                addVertex(v2.x, v2.y, v2.z);
                addVertex(v3.x, v3.y, v3.z);
                addVertex(v4.x, v4.y, v4.z);

                // put color of quad (temp red)
                addColor(v1.r, v1.g, v1.b, v1.a);
                addColor(v2.r, v2.g, v2.b, v2.a);
                addColor(v3.r, v3.g, v3.b, v3.a);
                addColor(v4.r, v4.g, v4.b, v4.a);

                // put normal
                n = computeFaceNormal(v1.x,v1.y,v1.z, v2.x,v2.y,v2.z, v3.x,v3.y,v3.z);
                for(k = 0; k < 4; ++k)  // same normals for 4 vertices
                {
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
// Color selected vertex based on a few parameters
///////////////////////////////////////////////////////////////////////////////
Vertex Planet::colorVertex(char c, float aR, float latitude, float vec[3])
{
    Vertex v;
    float absLat = sqrt(pow(latitude, 2));  // get magnitude of latitude
    float localTemp = (temp + 45) - absLat * 180 / PI;  // get temperature at absLat
    float coeff = 0.85 / 15 * localTemp;
    if (coeff > 0.91) coeff = 0.91;                     // cap snow to still appear at lower latitudes
    float snowHeight = (minHeight + coeff * dH) * K;    // snow is a function of temp + altitude
    float waterHeight = (minHeight + water * dH) * K;
    float sandHeight = waterHeight + (snowHeight - waterHeight) * 0.08;

    if ((absLat - PI / 4) * 180 / PI > temp &&
        rand() % 50 * 0.01 < pow(absLat - (PI / 4 + temp * PI / 180), 0.25) &&
        water > 0.0) {  // define planet arctic circle and add randomness
        if (aR > radius + waterHeight) {
            // snow
            v.r = 1.0;
            v.g = 0.98;
            v.b = 0.98;
        }
        else {
            if (rand() % 50 * 0.01 < pow(absLat - (PI / 4 + temp * PI / 180), 0.9)) {
                v.r = 180.0 / 255.0;
                v.g = 207.0 / 255.0;
                v.b = 250.0 / 255.0;
            }
            else {
                // water
                v.r = 0.0;
                v.g = 94.0 / 255.0;
                v.b = 184.0 / 255.0;
            }
        }
    }
    else if (aR <= radius + waterHeight &&
        water > 0.0) {
        // water
        v.r = 0.0;
        v.g = 94.0 / 255.0;
        v.b = 184.0 / 255.0;
    }
    else if (aR < radius + sandHeight && terrestrial) {
        v.r = 0.761;
        v.g = 0.698;
        v.b = 0.502;
    }
    else if (aR > radius + snowHeight &&
        water > 0.0) {  // lim x->inf, recnoise->2
        // snow
        v.r = 1.0;
        v.g = 0.98;
        v.b = 0.98;
    }
    else {
        if (terrestrial) {
            // grass
            v.r = 0.0;
            v.g = 154.0 / 255.0;
            v.b = 23.0 / 255.0;
        }
        else {
            float noise = noise1(latitude * 2);
            v.r = red + noise;
            v.g = green + noise;
            v.b = blue + noise;
        }
    }
        

    return v;
}

///////////////////////////////////////////////////////////////////////////////
// generate interleaved vertices: V/N/T
// stride must be 32 bytes
///////////////////////////////////////////////////////////////////////////////
void Planet::buildInterleavedVertices()
{
    std::vector<float>().swap(interleavedVertices);

    std::size_t i, j, k;
    std::size_t count = vertices.size();
    for(i = 0, j = 0, k = 0; i < count; i += 3, j += 4)
    {
        interleavedVertices.push_back(vertices[i]);
        interleavedVertices.push_back(vertices[i+1]);
        interleavedVertices.push_back(vertices[i+2]);

        interleavedVertices.push_back(normals[i]);
        interleavedVertices.push_back(normals[i+1]);
        interleavedVertices.push_back(normals[i+2]);

        interleavedVertices.push_back(colors[j]);
        interleavedVertices.push_back(colors[j+1]);
        interleavedVertices.push_back(colors[j+2]);
        interleavedVertices.push_back(colors[j+3]);
    }
}



///////////////////////////////////////////////////////////////////////////////
// add single vertex to array
///////////////////////////////////////////////////////////////////////////////
void Planet::addVertex(float x, float y, float z)
{
    vertices.push_back(x);
    vertices.push_back(y);
    vertices.push_back(z);
}



///////////////////////////////////////////////////////////////////////////////
// add single normal to array
///////////////////////////////////////////////////////////////////////////////
void Planet::addNormal(float nx, float ny, float nz)
{
    normals.push_back(nx);
    normals.push_back(ny);
    normals.push_back(nz);
}



///////////////////////////////////////////////////////////////////////////////
// add single color to array
///////////////////////////////////////////////////////////////////////////////
void Planet::addColor(float r, float g, float b, float a)
{
    colors.push_back(r);
    colors.push_back(g);
    colors.push_back(b);
    colors.push_back(a);
}



///////////////////////////////////////////////////////////////////////////////
// add 3 indices to array
///////////////////////////////////////////////////////////////////////////////
void Planet::addIndices(unsigned int i1, unsigned int i2, unsigned int i3)
{
    indices.push_back(i1);
    indices.push_back(i2);
    indices.push_back(i3);
}



///////////////////////////////////////////////////////////////////////////////
// return face normal of a triangle v1-v2-v3
// if a triangle has no surface (normal length = 0), then return a zero vector
///////////////////////////////////////////////////////////////////////////////
std::vector<float> Planet::computeFaceNormal(float x1, float y1, float z1,  // v1
                                             float x2, float y2, float z2,  // v2
                                             float x3, float y3, float z3)  // v3
{
    const float EPSILON = 0.000001f;

    std::vector<float> normal(3, 0.0f);     // default return value (0,0,0)
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
    if(length > EPSILON)
    {
        // normalize
        float lengthInv = 1.0f / length;
        normal[0] = nx * lengthInv;
        normal[1] = ny * lengthInv;
        normal[2] = nz * lengthInv;
    }

    return normal;
}
