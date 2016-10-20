#include "filter.h"

#include <stdio.h>
#include <math.h>
#include <process.h>
#include <time.h>
#include <omp.h>
#include <glm/glm.hpp>

void Probing::SetParam()
{
    if (_volume == 0 || _soma == 0) return;

    _soma->SetExtent(_volume->GetWidth(), _volume->GetHeight(), _volume->GetDepth(), _volume->GetThickness());

    float mean, low, high;
    _volume->GetValue(mean, low, high);
    _high = (mean+high)/2.0f;
    _low = (low+mean+high)/3.0f;
    _grads = 255.0f-_low;
    printf("[Probing::SetParam] set value radius %.2f, high %.2f, low %.2f, grads %.2f\n", _radius, _high, _low, _grads);
}

void Probing::AddPoint(const Point &seed)
{
    if (_volume == 0 || _soma == 0) return;

    PCell point(seed);
    point.Value = _volume->GetVoxel(point);
    if (point.Value < _low) return;

    size_t len = _soma->GetSize();
    RefinePoint(point);
    _soma->AddPoint(point);
    _soma->Reduce(len);
}

void Probing::BeginUpdate()
{
    if (_volume == 0 || _soma == 0 || _doing) return;

    _beginthread(Probing::UpdateThread, 0, (void*)this);
    _cancel = false;
}

void Probing::Update()
{
    if (_volume == 0 || _soma == 0 || _doing) return;

    static const float rs = 0.61803399f;

    _doing = true;
    float params[4];
    if (_local) {
        params[0] = _radius;
        params[1] = _high;
        params[2] = _low;
        params[3] = _grads;
    }

    size_t len = _soma->GetSize();
    printf("[Probing::Update] probing starting, there are %d cells in soma model\n", len);
    if (_local && len > 0) {
        _radius = _soma->GetPoint(0).Radius;
        for (size_t i=1; i<len; ++i) {
            float radius = _soma->GetPoint(i).Radius;
            if (radius < _radius) _radius = radius;
        }
    }
    if (_cancel) {
        printf("[Probing::Update] probing canceled and return now\n");
        _doing = false;
        return;
    }

    size_t width = _volume->GetWidth(), height = _volume->GetHeight(), depth = _volume->GetDepth();
    unsigned char *volume = new unsigned char[width*height*depth];
    memset(volume, 0, width*height*depth*sizeof(unsigned char));

    clock_t t = clock();
    size_t id, pid;
    unsigned char v;
    #pragma omp parallel for private(id, v)
    for (int z=1; z<(int)depth-1; ++z) {
        for (size_t y=1; y<height-1; ++y) {
            for (size_t x=1; x<width-1; ++x) {
                id = z*width*height + y*width + x;
                v = _volume->GetVoxel(x, y, z);
                if (v >= _high) volume[id] = v;
                else volume[id] = 0;
            }
        }
    }
    printf("[Probing::Update] volume binaryzation ok (%ld ms)\n", clock()-t);

    if (_cancel) {
        printf("[Probing::Update] probing canceled and return now\n");
        delete[] volume;
        _doing = false;
        return;
    }

    t = clock();
    unsigned char *value = new unsigned char[width*height*depth];
    bool doing = true;
    do {
        doing = false;
        memset(value, 0, width*height*depth*sizeof(unsigned char));
        #pragma omp parallel for private(id, pid)
        for (int z=1; z<(int)depth-1; ++z) {
            for (size_t y=1; y<height-1; ++y) {
                for (size_t x=1; x<width-1; ++x) {
                    id = z*width*height + y*width + x;
                    if (volume[id] == 0)            continue;
                    pid = id;
                    if (volume[pid-width-1] > 0)    value[id] += 2;
                    if (volume[pid-width] > 0)      value[id] += 3;
                    if (volume[pid-width+1] > 0)    value[id] += 2;
                    if (volume[pid-1] > 0)          value[id] += 3;
                    if (volume[pid+1] > 0)          value[id] += 3;
                    if (volume[pid+width-1] > 0)    value[id] += 2;
                    if (volume[pid+width] > 0)      value[id] += 3;
                    if (volume[pid+width+1] > 0)    value[id] += 2;

                    pid = id - width*height;
                    if (volume[pid] > 0)            value[id] += 3;
                    if (volume[pid-width-1] > 0)    value[id] += 1;
                    if (volume[pid-width] > 0)      value[id] += 2;
                    if (volume[pid-width+1] > 0)    value[id] += 1;
                    if (volume[pid-1] > 0)          value[id] += 2;
                    if (volume[pid+1] > 0)          value[id] += 2;
                    if (volume[pid+width-1] > 0)    value[id] += 1;
                    if (volume[pid+width] > 0)      value[id] += 2;
                    if (volume[pid+width+1] > 0)    value[id] += 1;

                    pid = id + width*height;
                    if (volume[pid] > 0)            value[id] += 3;
                    if (volume[pid-width-1] > 0)    value[id] += 1;
                    if (volume[pid-width] > 0)      value[id] += 2;
                    if (volume[pid-width+1] > 0)    value[id] += 1;
                    if (volume[pid-1] > 0)          value[id] += 2;
                    if (volume[pid+1] > 0)          value[id] += 2;
                    if (volume[pid+width-1] > 0)    value[id] += 1;
                    if (volume[pid+width] > 0)      value[id] += 2;
                    if (volume[pid+width+1] > 0)    value[id] += 1;
                }
            }
        }

        #pragma omp parallel for private(id, pid, v) reduction(|:doing)
        for (int z=1; z<(int)depth-1; ++z) {
            for (size_t y=1; y<height-1; ++y) {
                for (size_t x=1; x<width-1; ++x) {
                    id = z*width*height + y*width + x;
                    v = value[id];
                    if (v == 0 || v >= 60)          continue;
                    bool hit = false;
                    pid = id;
                    if (value[pid-width-1] >= v)    hit = true;
                    if (value[pid-width] >= v)      hit = true;
                    if (value[pid-width+1] >= v)    hit = true;
                    if (value[pid-1] >= v)          hit = true;
                    if (value[pid+1] > v)           hit = true;
                    if (value[pid+width-1] > v)     hit = true;
                    if (value[pid+width] > v)       hit = true;
                    if (value[pid+width+1] > v)     hit = true;

                    pid = id - width*height;
                    if (value[pid] >= v)            hit = true;
                    if (value[pid-width-1] >= v)    hit = true;
                    if (value[pid-width] >= v)      hit = true;
                    if (value[pid-width+1] >= v)    hit = true;
                    if (value[pid-1] >= v)          hit = true;
                    if (value[pid+1] >= v)          hit = true;
                    if (value[pid+width-1] >= v)    hit = true;
                    if (value[pid+width] >= v)      hit = true;
                    if (value[pid+width+1] >= v)    hit = true;

                    pid = id + width*height;
                    if (value[pid] > v)             hit = true;
                    if (value[pid-width-1] > v)     hit = true;
                    if (value[pid-width] > v)       hit = true;
                    if (value[pid-width+1] > v)     hit = true;
                    if (value[pid-1] > v)           hit = true;
                    if (value[pid+1] > v)           hit = true;
                    if (value[pid+width-1] > v)     hit = true;
                    if (value[pid+width] > v)       hit = true;
                    if (value[pid+width+1] > v)     hit = true;
                    if (hit) {
                        volume[id] = 0;
                        doing = true;
                    }
                }
            }
        }
    } while (doing);
    delete[] value;
    printf("[Probing::Update] center points probing ok (%ld ms)\n", clock()-t);
    if (_cancel) {
        printf("[Probing::Update] probing canceled and return now\n");
        delete[] volume;
        _doing = false;
        return;
    }

    PCell point;
    #pragma omp parallel for private(point)
    for (int z=1; z<(int)depth-1; ++z) {
        for (size_t y=1; y<height-1; ++y) {
            for (size_t x=1; x<width-1; ++x) {
                if (volume[z*width*height + y*width + x] > 0) {
                    point.X = x*1.0f;
                    point.Y = y*1.0f;
                    point.Z = z*1.0f;
                    point.Value = _volume->GetVoxel(point);
                    point.Radius = 0.0f;
                    point.Minor = 0.0f;
                    RefinePoint(point);
                    if (point.Radius >= rs*_radius) _soma->AddPoint(point);
                }
            }
        }
    }
    delete[] volume;
    printf("[Probing::Update] probing finished, there are %d cells in soma model (%ld ms)\n", _soma->GetSize(), clock()-t);

    t = clock();
    _soma->Reduce();
    printf("[Probing::Update] simplify soma model to %d cells (%ld ms)\n", _soma->GetSize(), clock()-t);

    if (_local) {
        _radius = params[0];
        _high = params[1];
        _low = params[2];
        _grads = params[3];
    }
    _doing = false;
}

void Probing::RefinePoint(PCell &point) const
{
    static const int udim = 9, vdim = 8, dim = 130;
    static const float pi = 3.14159265f;
    static const float bias = 2.0f; // 1.41421356f 1.73205081f 2.23606798f

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

    static PCell point0, point1, points[dim];
    do {    
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
            } while (point1.Value >= _low && abs(point1.Value-point.Value) <= _grads);
            points[i] = point0;
        }

        point0 = point;
        point.X = point.Y = point.Z = point.Value = point.Radius = point.Minor = 0.0f;
        float radius = 0.0f;
        for (int i=0; i<dim/2; ++i) {
            points[i].Radius += points[dim/2+i].Radius;
            radius += points[i].Radius/dim;
        }

        int rdim = 0;
        point.Minor = points[0].Radius;
        for (int i=0; i<dim/2; ++i) {
           if (points[i].Radius <= bias*2.0f*radius) {
                rdim += 2;
                point.X += points[i].X + points[dim/2+i].X;
                point.Y += points[i].Y + points[dim/2+i].Y;
                point.Z += points[i].Z + points[dim/2+i].Z;
                point.Radius += points[i].Radius;
                if (points[i].Radius < point.Minor) point.Minor = points[i].Radius;
           }
        }
        point.X /= rdim;
        point.Y /= rdim;
        point.Z /= rdim;
        point.Value = _volume->GetVoxel(point);
        point.Radius /= 2*rdim;
        if (point.Radius < 1.0f) point.Radius = 1.0f;
    } while (point.Minor > point0.Minor);
    point = point0;
}
