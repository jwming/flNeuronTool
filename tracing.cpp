#include "filter.h"

#include <stdio.h>
#include <math.h>
#include <process.h>
#include <time.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

void Tracing::SetParam()
{
    if (_volume == 0 || _tree == 0) return;

    _tree->SetExtent(_volume->GetWidth(), _volume->GetHeight(), _volume->GetDepth(), _volume->GetThickness());

    float mean, low, high;
    _volume->GetValue(mean, low, high);
    _high = (mean+high)/2.0f;
    _low = (low+mean+high)/3.0f;
    _grads = 255.0f-_low;
    printf("[Tracing::SetParam] set value radius %.2f, high %.2f, low %.2f, grads %.2f\n", _radius, _high, _low, _grads);
}

void Tracing::SetParam(size_t x, size_t y, size_t z, size_t radius)
{
    float mean, low, high;
    _volume->GetValue(mean, low, high, x, y, z, radius);
    _high = (mean+high)/2.0f;
    _low = (low+mean+high)/3.0f;
    _grads = 255.0f-_low;
}

void Tracing::AddSeed(const Point &seed)
{
    if (_volume == 0 || _tree == 0) return;

    PNode point(seed);
    point.Value = _volume->GetVoxel(point);
    if (point.Value < _low) return;

    static const int udim = 9, vdim = 8, dim = 130;
    static const float pi = 3.14159265f;
    static const float bias = 2.0f, rs = 0.61803399f;

    static glm::vec3 dirs[dim];
    static float thickness = 0.0f;
    if (thickness != _volume->GetThickness()) {
        thickness = _volume->GetThickness();
        int vth = 1, ith = 0;
        for (int i=0; i<=udim/2; ++i) {
            vth = glm::max(i*vdim, 1);
            for (int j=0; j<vth; ++j) {
                dirs[ith].x = glm::sin(i*pi/(udim-1))*glm::cos(j*2.0f*pi/vth);
                dirs[ith].y = glm::sin(i*pi/(udim-1))*glm::sin(j*2.0f*pi/vth);
                dirs[ith].z = glm::cos(i*pi/(udim-1))/thickness;
                //dirs[ith] = glm::normalize(dirs[ith]);
                dirs[dim/2+ith] = -1.0f*dirs[ith];
                ++ith;
                if (ith >= dim/2) break;
            }
        }
    } 

    static PNode point0, point1, points[dim];
    for (int i=0; i<dim; ++i) {
        point1 = point;
        point1.Radius = 0.0f;
        do {
            point0 = point1;
            point1.Radius += 0.5f;
            point1.X = point.X + dirs[i].x*point1.Radius;
            point1.Y = point.Y + dirs[i].y*point1.Radius;
            point1.Z = point.Z + dirs[i].z*point1.Radius;
            point1.Value = _volume->GetVoxel(point1);
        } while (point1.Value >= _high || point1.Value >= _low && abs(point1.Value-point.Value) <= _grads);
        points[i] = point0;
    }
   
    point.X = point.Y = point.Z = point.Value = point.Radius = 0.0f;
    for (int i=0; i<dim; ++i) {
        point.X += points[i].X/dim;
        point.Y += points[i].Y/dim;
        point.Z += points[i].Z/dim;
        point.Radius += points[i].Radius/dim;
    }
    point.Value = _volume->GetVoxel(point);

    point0 = point1 = point;
    for (int i=0; i<dim/2; ++i) {
        points[i].Radius += points[dim/2+i].Radius;
        if (points[i].Radius < point.Radius) point.Radius = points[i].Radius;
        if (points[i].Radius > point0.Radius + point1.Radius) {
            point0 = points[i];
            point1 = points[dim/2+i];
            point0.Radius -= point1.Radius;
        }
    }
    point.Radius *= rs;
    if (point.Radius < 0.5f) point.Radius = 0.5f;

    if (point0.Radius >= bias*point.Radius) {
        glm::vec3 dir = glm::normalize(glm::vec3(point0.X-point.X, point0.Y-point.Y, point0.Z-point.Z));
        point.I = dir.x;
        point.J = dir.y;
        point.K = dir.z;
        point.Pid = -1;
        _seeds.push(point);
    }
    if (point1.Radius >= bias*point.Radius) {
        glm::vec3 dir = glm::normalize(glm::vec3(point1.X-point.X, point1.Y-point.Y, point1.Z-point.Z));
        point.I = dir.x;
        point.J = dir.y;
        point.K = dir.z;
        point.Pid = -1;
        _seeds.push(point);
    }
}

void Tracing::BeginUpdate()
{
    if (_volume==0 || _tree==0 || _seeds.empty() || _doing) return;
    
    _beginthread(Tracing::UpdateThread, 0, (void*)this);
    _cancel = false;
}

void Tracing::Update()
{
    if (_volume==0 || _tree==0 || _seeds.empty() || _doing) return;

    _doing = true;
    float params[4];
    if (_local) {
        params[0] = _radius;
        params[1] = _high;
        params[2] = _low;
        params[3] = _grads;
    }

    size_t len = _tree->GetSize();
    printf("[Tracing::Update] tracing starting, there are %d nodes in tree model\n", len);

    clock_t t = clock();
    std::vector<PNode> children;
    while (!_seeds.empty()) {
        PNode seed = _seeds.top();
        _seeds.pop();
        seed.Id = _tree->AddPoint(seed);
        if (_local) SetParam(seed, 5.0f);
        Advance(seed, children);
        //while (!children.empty()) {
        //    if (children.size() == 1) {
        //        seed = children[0];
        //        seed.Id = _tree->AddPoint(seed);
        //        if (_local) SetParam(seed, 5.0f);
        //        Advance(seed, children);
        //        continue;
        //    }
        //    if (children.size() > 1) {
        //        for (size_t i=0; i<children.size(); ++i)
        //            _seeds.push(children[i]);
        //        children.clear();
        //        continue;
        //    }
        //}
        if (!children.empty()) {
            for (size_t i=0; i<children.size(); ++i)
                _seeds.push(children[i]);
            children.clear();
        }
        printf("[Tracing::Update] there are %d seeds, %d nodes in tree model\r", _seeds.size(), _tree->GetSize());
        if (_cancel) {
            printf("[Tracing::Update] tracing canceled and return now\n");

            while (!_seeds.empty()) _seeds.pop();
        }
    }
    printf("[Tracing::Update] tracing finished, there are %d nodes in tree model (%ld ms)\n", _tree->GetSize(), clock()-t);

    //t = clock();
    //_tree->Reduce(len);
    //printf("[Tracing::Update] simplify tree model to %d nodes (%ld ms)\n", _tree->GetSize(), clock()-t);

    if (_local) {
        _radius = params[0];
        _high = params[1];
        _low = params[2];
        _grads = params[3];
    }
    _doing = false;
}

void Tracing::Advance(PNode &point, std::vector<PNode> &children) const
{
    static const int udim = 7, vdim = 8, dim = 2*udim-1; // 5 7
    static const float pi = 3.14159265f;
    static const float as = 0.80901699f; // 0.80901699f 0.92387953f 0.96592583f
    static const float bias = 1.0f, dist = 3.5f, step = 3.0f; // 1.41421356f 1.73205081f 2.23606798f 2.82842712f
    static const float ds =  0.98078528f; // 0.92387953f 0.98078528f

    // udim = 5, dim = 9
    //static const int sample[(dim+2)*(dim+2)] = {
    //    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    //    -1, 61, 60, 59, 58, 57, 56, 55, 54, 53, -1,
    //    -1, 62, 34, 33, 32, 31, 30, 29, 28, 52, -1,
    //    -1, 63, 35, 15, 14, 13, 12, 11, 27, 51, -1,
    //    -1, 64, 36, 16,  4,  3,  2, 10, 26, 50, -1,
    //    -1, 65, 37, 17,  5,  0,  1,  9, 25, 49, -1,
    //    -1, 66, 38, 18,  6,  7,  8, 24, 48, 80, -1,
    //    -1, 67, 39, 19, 20, 21, 22, 23, 47, 79, -1,
    //    -1, 68, 40, 41, 42, 43, 44, 45, 46, 78, -1,
    //    -1, 69, 70, 71, 72, 73, 74, 75, 76, 77, -1,
    //    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    //};
    //static const int ids[dim*dim] = {
    //    60,
    //    61, 50, 49, 48, 59, 70, 71, 72,
    //    62, 51, 40, 39, 38, 37, 36, 47, 58, 69, 80, 81, 82, 83, 84, 73,
    //    63, 52, 41, 30, 29, 28, 27, 26, 25, 24, 35, 46, 57, 68, 79, 90, 91, 92, 93, 94, 95, 96, 85, 74,
    //    64, 53, 42, 31, 20, 19, 18, 17, 16, 15, 14, 13, 12, 23, 34, 45, 56, 67, 78, 89, 100, 101, 102, 103, 104, 105, 106, 107, 108, 97, 86, 75,
    //};

    // udim = 7, dim = 13
    //static const int sample[(dim+2)*(dim+2)] = {
    //    -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1, -1,   -1,  -1,  -1,  -1,  -1, -1,        
    //    -1, 139, 138, 137, 136, 135, 134, 133, 132, 131, 130, 129, 128, 127, -1,
    //    -1, 140,  96,  95,  94,  93,  92,  91,  90,  89,  88,  87,  86, 126, -1,
    //    -1, 141,  97,  61,  60,  59,  58,  57,  56,  55,  54,  53,  85, 125, -1,
    //    -1, 142,  98,  62,  34,  33,  32,  31,  30,  29,  28,  52,  84, 124, -1,
    //    -1, 143,  99,  63,  35,  15,  14,  13,  12,  11,  27,  51,  83, 123, -1,
    //    -1, 144, 100,  64,  36,  16,   4,   3,   2,  10,  26,  50,  82, 122, -1,
    //    -1, 145, 101,  65,  37,  17,   5,   0,   1,   9,  25,  49,  81, 121, -1,
    //    -1, 146, 102,  66,  38,  18,   6,   7,   8,  24,  48,  80, 120, 168, -1,
    //    -1, 147, 103,  67,  39,  19,  20,  21,  22,  23,  47,  79, 119, 167, -1,
    //    -1, 148, 104,  68,  40,  41,  42,  43,  44,  45,  46,  78, 118, 166, -1,
    //    -1, 149, 105,  69,  70,  71,  72,  73,  74,  75,  76,  77, 117, 165, -1,
    //    -1, 150, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115, 116, 164, -1,
    //    -1, 151, 152, 153, 154, 155, 156, 157, 158, 159, 160, 161, 162, 163, -1,
    //    -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1, -1,
    //};
    static const int ids[dim*dim] = {
        112,
        113,  98,  97,  96, 111, 126, 127, 128,
        114,  99,  84,  83,  82,  81,  80,  95, 110, 125, 140, 141, 142, 143, 144, 129,
        115, 100,  85,  70,  69,  68,  67,  66,  65,  64,  79,  94, 109, 124, 139, 154, 155, 156, 157, 158, 159, 160, 145, 130,
        116, 101,  86,  71,  56,  55,  54,  53,  52,  51,  50,  49,  48,  63,  78,  93, 108, 123, 138, 153, 168, 169, 170, 171, 172, 173, 174, 175, 176, 161, 146, 131,
        117, 102,  87,  72,  57,  42,  41,  40,  39,  38,  37,  36,  35,  34,  33,  32,  47,  62,  77,  92, 107, 122, 137, 152, 167, 182, 183, 184, 185, 186, 187, 188, 189, 190, 191, 192, 177, 162, 147, 132,
        118, 103,  88,  73,  58,  43,  28,  27,  26,  25,  24,  23,  22,  21,  20,  19,  18,  17,  16,  31,  46,  61,  76,  91, 106, 121, 136, 151, 166, 181, 196, 197, 198, 199, 200, 201, 202, 203, 204, 205, 206, 207, 208, 193, 178, 163, 148, 133, 
    };

    static glm::vec3 dirs[dim*dim];
    static bool first = true;
    if (first) {
        first = false;
        int vth = 1, ith = 0;
        for (int i=0; i<udim; ++i) {
            vth = glm::max(i*vdim, 1);
            for (int j=0; j<vth; ++j) {
                dirs[ith].x = glm::sin(i*as*0.5f*pi/(udim-1))*glm::cos(j*2.0f*pi/vth);
                dirs[ith].y = glm::sin(i*as*0.5f*pi/(udim-1))*glm::sin(j*2.0f*pi/vth);
                dirs[ith].z = bias*glm::cos(i*as*0.5f*pi/(udim-1));
                dirs[ith] = glm::normalize(dirs[ith]);
                ++ith;
            }
        }
    }

    glm::vec3 line(point.I, point.J, point.K);
    glm::vec3 zaxis(0.0f, 0.0f, 1.0f);
    glm::vec3 axis = glm::cross(zaxis, line);
    float angle = glm::degrees(glm::acos(glm::dot(zaxis, line)));
    glm::mat4 matrix = glm::rotate(glm::mat4(), angle, axis);
    
    static PNode point0, point1, points[dim*dim];
    for (int i=0; i<dim*dim; ++i) {
        point1 = point;
        point1.Pid = point.Id;
        //point1.Radius *= 0.5f;
        point1.Radius = 0.5f;
        glm::vec4 dir = matrix*glm::vec4(dirs[i], 1.0f);
        point1.I = dir.x;
        point1.J = dir.y;
        point1.K = dir.z/_volume->GetThickness();
        do {
            point0 = point1;
            point1.Radius += 0.5f;
            point1.X = point.X + point1.I*point1.Radius;
            point1.Y = point.Y + point1.J*point1.Radius;
            point1.Z = point.Z + point1.K*point1.Radius;
            point1.Value = _volume->GetVoxel(point1);
        } while (point1.Radius <= _dist*_radius && point1.Value >= _low && abs(point1.Value-point.Value) <= _grads);
        points[i] = point0;
    }

    static unsigned char image[(dim+2)*(dim+2)];
    memset(image, 0, (dim+2)*(dim+2)*sizeof(unsigned char));
    for (int i=0; i<dim*dim; ++i) {
        if (points[i].Radius < dist*point.Radius) {
            image[ids[i]] = 0;
            continue;
        }
        if (points[i].Radius >= _dist*point.Radius) {
            points[i].Radius = _step*point.Radius;
            points[i].X = point.X + points[i].I*points[i].Radius;
            points[i].Y = point.Y + points[i].J*points[i].Radius;
            points[i].Z = point.Z + points[i].K*points[i].Radius;
            points[i].Value = _volume->GetVoxel(points[i]);
            image[ids[i]] = (unsigned char)points[i].Value;
        }
    }

    GetCenter(image, dim+2);
    //children.clear();
    for (int i=0; i<dim*dim; ++i) {
        if (image[ids[i]] > 0) {
            point0 = points[i];
            RefinePoint(point, point0);
            if (point.I*point0.I + point.J*point0.J + point.K*point0.K <= 0.0f) continue;
            if (children.empty()) {
                children.push_back(point0);
                continue;
            }
            bool repeat = false;
            for (size_t k=0; k<children.size(); ++k) {
                if (children[k].I*point0.I + children[k].J*point0.J + children[k].K*point0.K >= ds) {
                    repeat = true;
                    break;
                }
            }
            if (!repeat) {
                children.push_back(point0);
                continue;
            }
        }
    }
}

void Tracing::GetCenter(unsigned char *image, size_t dimension) const
{
    size_t width = dimension, height = dimension;
    unsigned char *value = new unsigned char[width*height];
    bool doing = true;
    do {
        doing = false;
        memset(value, 0, width*height*sizeof(unsigned char));
        for (size_t y=1; y<height-1; ++y) {
            for (size_t x=1; x<width-1; ++x) {
                size_t id = y*width+x;
                if (image[id] == 0)         continue;
                if (image[id-width-1] > 0)  ++value[id];
                if (image[id-width] > 0)    ++value[id]; ++value[id];
                if (image[id-width+1] > 0)  ++value[id];
                if (image[id-1] > 0)        ++value[id]; ++value[id];
                if (image[id+1] > 0)        ++value[id]; ++value[id];
                if (image[id+width-1] > 0)  ++value[id];
                if (image[id+width] > 0)    ++value[id]; ++value[id];
                if (image[id+width+1] > 0)  ++value[id];
            }
        }

        for (size_t y=1; y<height-1; ++y) {
            for (size_t x=1; x<width-1; ++x) {
                size_t id = y*width+x;
                unsigned char v = value[id];
                if (v == 0 || v >= 12)      continue;
                bool hit = false;
                if (value[id-width-1] >= v) hit = true;
                if (value[id-width] >= v)   hit = true;
                if (value[id-width+1] >= v) hit = true;
                if (value[id-1] >= v)       hit = true;
                if (value[id+1] > v)        hit = true;
                if (value[id+width-1] > v)  hit = true;
                if (value[id+width] > v)    hit = true;
                if (value[id+width+1] > v)  hit = true;
                if (hit) {
                    image[id] = 0;
                    doing = true;
                }
            }
        }
    } while (doing);
    delete[] value;
}

void Tracing::RefinePoint(PNode &parent, PNode &point) const
{
    static const size_t dim = 16;
    static const float pi = 3.14159265f;
    static const float ds = 0.98078528f; // cos(pi/16)
    static const float rs = 0.61803399f;

    static glm::vec3 dirs[dim];
    static bool first = true;
    if (first) {
        first = false;
        for (int i=0; i<dim/2; ++i) {
            dirs[i].x = cos(i*2.0f*pi/dim);
            dirs[i].y = sin(i*2.0f*pi/dim);
            dirs[i].z = 0.0f;
            dirs[dim/2+i] = -1.0f*dirs[i];
        }
    }

    static PNode point0, point1, points[dim];  
    do {
        glm::vec3 line(point.I, point.J, point.K);
        glm::vec3 zaxis(0.0f, 0.0f, 1.0f);
        glm::vec3 axis = glm::cross(zaxis, line);
        float angle = glm::degrees(glm::acos(glm::dot(zaxis, line)));
        glm::mat4 matrix = glm::rotate(glm::mat4(), angle, axis);

        for (int i=0; i<dim; ++i) {
            point1 = point;
            point1.Radius = 0.0f;
            glm::vec4 dir = matrix*glm::vec4(dirs[i], 1.0f);
            dir.z /= _volume->GetThickness();
            dir = glm::normalize(dir);
            point1.I = dir.x;
            point1.J = dir.y;
            point1.K = dir.z;
            do {
                point0 = point1;
                point1.Radius += 0.5f;
                point1.X = point.X + point1.I*point1.Radius;
                point1.Y = point.Y + point1.J*point1.Radius;
                point1.Z = point.Z + point1.K*point1.Radius;
                point1.Value = _volume->GetVoxel(point1);
            } while (point1.Radius <= _radius && point1.Value >= _low && abs(point1.Value-point.Value) <= _grads);
            points[i] = point0;
        }

        point0 = point;
        point.X = point.Y = point.Z = point.Radius = point.Value = 0.0f;
        for (int i=0; i<dim; ++i) {
            point.X += points[i].X/dim;
            point.Y += points[i].Y/dim;
            point.Z += points[i].Z/dim;
            point.Radius += points[i].Radius/dim;
        }
        point.Value = _volume->GetVoxel(point);

        glm::vec3 dir = glm::normalize(glm::vec3(point.X-parent.X, point.Y-parent.Y, point.Z-parent.Z));
        point.I = dir.x;
        point.J = dir.y;
        point.K = dir.z;
    } while (point.I*point0.I + point.J*point0.J + point.K*point0.K <= ds);

    for (int i=0; i<dim/2; ++i) {
        points[i].Radius += points[dim/2+i].Radius;
        if (points[i].Radius < point.Radius) point.Radius = points[i].Radius;
    }
    point.Radius *= rs;
    if (point.Radius < 1.0f) point.Radius = 1.0f;
}
