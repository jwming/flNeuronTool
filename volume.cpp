#include "vision.h"

#include <stdio.h>
#include <math.h>
#include <time.h>
#include <omp.h>
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <tiffio.h>

Volume::~Volume()
{
    if (_buffer != 0) delete[] _buffer;
    if (glIsTexture(_texture)) glDeleteTextures(1, &_texture);
    if (glIsTexture(_color)) glDeleteTextures(1, &_color);
    if (glIsProgram(_program)) glDeleteProgram(_program);
    _buffer = 0;
    _texture = _color = _program = 0;
}

bool Volume::Read(const char *path)
{   
    TIFFSetWarningHandler(0);
    TIFF *tif = TIFFOpen(path, "rb");
    if (tif == 0) {
        printf("[Volume::Read] open TIFF file %s failed\n", path);
        return false;
    }

    uint16 photo=1, channels=1, bits=8;
    TIFFGetField(tif, TIFFTAG_PHOTOMETRIC, &photo); // PHOTOMETRIC_MINISWHITE=0, PHOTOMETRIC_MINISBLACK=1
    TIFFGetField(tif, TIFFTAG_SAMPLESPERPIXEL, &channels);
    TIFFGetField(tif, TIFFTAG_BITSPERSAMPLE, &bits);
    if (photo > 1 || channels != 1 || bits != 8) {
        printf("[Volume::Read] TIFF image has an unsupported bits, channels or photometric\n");
        TIFFClose(tif);
        return false;
    }

    uint32 width=0, height=0;
    TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &width);
    TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &height);
    uint16 depth = TIFFNumberOfDirectories(tif);
    uint8 *buffer = new uint8[width*height*depth];
    uint32 *slice = new uint32[width*height]; 
    for (uint16 i=0; i<depth; ++i) {
        // assert bits, channels, photometric & width, height
        TIFFReadRGBAImageOriented(tif, width, height, slice, ORIENTATION_TOPLEFT);
        for (uint32 j=0; j<width*height; ++j)
            buffer[i*width*height+j] = TIFFGetR(slice[j]);
        TIFFReadDirectory(tif);
    }
    delete[] slice;
    TIFFClose(tif);
    printf("[Volume::Read] read TIFF file %s ok\n", path);

    if (_buffer != 0) delete[] _buffer;
    _buffer = buffer;
    _width = width;
    _height = height;
    _depth = depth;
    _thickness = 1.0f;
    _scale = max(max(_width, _height)*1.0f, _depth*_thickness);
    if (_scale < 1.0f) _scale = 1.0f;

    clock_t t = clock();
    GetValue(_mean, _low, _high, 0, 0, 0, (size_t)(_scale+0.5f));
    printf("[Volume::Read] calculate volume voxel values ok (%ld ms)\n", clock()-t);

    if (!glIsTexture(_texture)) {
        glGenTextures(1, &_texture);
        glBindTexture(GL_TEXTURE_3D, _texture);
        glTexParameterf(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP);
        glTexParameterf(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP);
        glTexParameterf(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP);
        glTexParameterf(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameterf(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

        float xcoeff[] = { 0.5f, 0.0f, 0.0f, 0.5f };
        glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
        glTexGenfv(GL_S, GL_OBJECT_PLANE, xcoeff);
        float ycoeff[] = { 0.0f, 0.5f, 0.0f, 0.5f };
        glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
        glTexGenfv(GL_T, GL_OBJECT_PLANE, ycoeff);
        float zcoeff[] = { 0.0f, 0.0f, 0.5f, 0.5f };
        glTexGeni(GL_R, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
        glTexGenfv(GL_R, GL_OBJECT_PLANE, zcoeff);
    }
    glBindTexture(GL_TEXTURE_3D, _texture);
    glTexImage3D(GL_TEXTURE_3D, 0, GL_INTENSITY, _width, _height, _depth, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, _buffer);

    unsigned char color[256];
    for (size_t i=0; i<256; ++i) color[i] = i;
    if (!glIsTexture(_color)) {
        glGenTextures(1, &_color);
        glBindTexture(GL_TEXTURE_1D, _color);
        glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP);
        glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    }
    glBindTexture(GL_TEXTURE_1D, _color);
    glTexImage1D(GL_TEXTURE_1D, 0, GL_INTENSITY, 256, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, color);

    if (!glIsProgram(_program)) {
        char const *fsource  = "\
            uniform sampler3D Index;\
            uniform sampler1D Color;\
            void main() { gl_FragColor = texture1D(Color, texture3D(Index, gl_TexCoord[0].xyz).a); }";
        _program = glCreateProgram();
        unsigned int fshader = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fshader, 1, &fsource, 0);
        glCompileShader(fshader);
        glAttachShader(_program, fshader);
        glLinkProgram(_program);
    }
    return true;
}

bool Volume::Write(const char *path) const
{
    if (_buffer == 0) return false;

    TIFF *tif = TIFFOpen(path, "wb");
    if (tif == 0) {
        printf("[Volume::Write] open TIFF file %s failed\n", path);
        return false;
    }

    for (size_t i=0; i<_depth; ++i) {
        TIFFSetField(tif, TIFFTAG_IMAGEWIDTH, _width);
        TIFFSetField(tif, TIFFTAG_IMAGELENGTH, _height);
        TIFFSetField(tif, TIFFTAG_ROWSPERSTRIP, _height);
        TIFFSetField(tif, TIFFTAG_SAMPLESPERPIXEL, 1);
        TIFFSetField(tif, TIFFTAG_BITSPERSAMPLE, 8);
        TIFFSetField(tif, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_MINISBLACK); 
        TIFFSetField(tif, TIFFTAG_COMPRESSION, COMPRESSION_PACKBITS);
        TIFFSetField(tif, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);      
        TIFFWriteEncodedStrip(tif, 0, _buffer+i*_width*_height, _width*_height);
        TIFFWriteDirectory(tif);
    }
    TIFFClose(tif);
    printf("[Volume::Write] write TIFF file %s ok\n", path);
    return true;
}

void Volume::Draw() const
{
    if (!glIsTexture(_texture)) return;

    glPushMatrix();
    float matrix[16];
    glGetFloatv(GL_MODELVIEW_MATRIX, matrix);
    glScalef(_width/_scale, _height/_scale, _depth*_thickness/_scale);

    static const glm::vec3 vertex[8] = {
        glm::vec3(-1.0, -1.0, -1.0),
        glm::vec3(1.0, -1.0, -1.0),
        glm::vec3(1.0, 1.0, -1.0),
        glm::vec3(-1.0, 1.0, -1.0),
        glm::vec3(-1.0, -1.0, 1.0),
        glm::vec3(1.0, -1.0, 1.0),
        glm::vec3(1.0, 1.0, 1.0),
        glm::vec3(-1.0, 1.0, 1.0),
    };

    if (_bound) {
        glPushAttrib(GL_ENABLE_BIT | GL_LINE_BIT);
        glDepthMask(GL_TRUE);
        glEnable(GL_DEPTH_TEST);
        glDisable(GL_BLEND);
        glDisable(GL_TEXTURE_3D);
        glLineWidth(2.0f);
        glBegin(GL_LINES);
        glColor3f(0.5f, 0.25f, 0.25f);
        glVertex3fv(&vertex[0][0]);
        glVertex3fv(&vertex[1][0]);
        glColor3f(0.25f, 0.5f, 0.25f);
        glVertex3fv(&vertex[0][0]);
        glVertex3fv(&vertex[3][0]);
        glColor3f(0.25f, 0.25f, 0.5f);
        glVertex3fv(&vertex[0][0]);
        glVertex3fv(&vertex[4][0]);
        glEnd();
        glLineWidth(1.0f);
        glColor3f(0.25f, 0.25f, 0.25f);
        glBegin(GL_LINE_LOOP);
        glVertex3fv(&vertex[1][0]);
        glVertex3fv(&vertex[2][0]);
        glVertex3fv(&vertex[6][0]);
        glVertex3fv(&vertex[5][0]);
        glEnd();
        glBegin(GL_LINE_LOOP);
        glVertex3fv(&vertex[3][0]);
        glVertex3fv(&vertex[7][0]);
        glVertex3fv(&vertex[6][0]);
        glVertex3fv(&vertex[2][0]);
        glEnd();
        glBegin(GL_LINE_LOOP);
        glVertex3fv(&vertex[4][0]);
        glVertex3fv(&vertex[5][0]);
        glVertex3fv(&vertex[6][0]);
        glVertex3fv(&vertex[7][0]);
        glEnd();
        glPopAttrib();
    }

    switch (_style) {
    case VOL_MIP:
        glBlendFunc(GL_ONE, GL_ONE);
        glBlendEquation(GL_MAX);
        break;
    case VOL_ACCUM:
        glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
        glBlendEquation(GL_FUNC_ADD);  
        break;
    case VOL_BLEND:
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glBlendEquation(GL_FUNC_ADD); 
        break;
    default: // case VOL_NONE:
        glPopMatrix();
        glPopAttrib();
        return;
    }

    glPushAttrib(GL_CURRENT_BIT | GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_LINE_BIT | GL_POLYGON_BIT | GL_TEXTURE_BIT);
    glColor3f(1.0f, 1.0f, 1.0f);
    glDepthMask(GL_FALSE);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glEnable(GL_TEXTURE_3D);
    glEnable(GL_TEXTURE_GEN_S);
    glEnable(GL_TEXTURE_GEN_T);
    glEnable(GL_TEXTURE_GEN_R);
    glUseProgram(_program);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_3D, _texture);
    glUniform1i(glGetUniformLocation(_program, "Index"), 0);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_1D, _color);
    glUniform1i(glGetUniformLocation(_program, "Color"), 1);

#define VOL_RENDERING_VIEW // slower speed but higher quality
#ifdef VOL_RENDERING_VIEW
    typedef std::pair<size_t, size_t> edge_t;
    static const edge_t edge[12] = {
        edge_t(0, 1), edge_t(1, 2), edge_t(2, 3), edge_t(3, 0),
        edge_t(0, 4), edge_t(1, 5), edge_t(2, 6), edge_t(3, 7),
        edge_t(4, 5), edge_t(5, 6), edge_t(6, 7), edge_t(7, 4),
    };

    glm::vec3 view(-matrix[2], -matrix[6], -matrix[10]);
    size_t step = (size_t)(abs(view.x)*_width + abs(view.y)*_height + abs(view.z)*_depth*_thickness + 0.5f);
    float dist0 = glm::dot(view, vertex[0]), dist1 = dist0;
    for(int v=1; v<8; ++v) {
        float d = glm::dot(view, vertex[v]);
        if (d < dist0) dist0 = d;
        if (d > dist1) dist1 = d;
    }
    float ddist = (dist1-dist0)/step;

    std::vector<glm::vec4> points;
    for (size_t n=0; n<=step; ++n) {
        points.clear();
        for (int e=0; e<12; ++e) {
            float f0 = glm::dot(view, vertex[edge[e].first]) - dist1 + n*ddist;
            float f1 = glm::dot(view, vertex[edge[e].second]) - dist1 + n*ddist;
            if (f0*f1 < 0 && f0 != f1)
                points.push_back(glm::vec4((vertex[edge[e].first]*f1-vertex[edge[e].second]*f0)/(f1-f0), 1.0f));
        }
        if (points.size() <= 2) continue;

        if (points.size() >= 4) {
            for (size_t i=0; i<points.size(); ++i)
                points[i].w = glm::atan(points[i].y, points[i].x);
            for (size_t i=0; i<points.size()-1; ++i) {
                size_t t = i;
                for (size_t j=i+1; j<points.size(); ++j)
                    if (points[j].w < points[t].w) t = j;
                if (t != i) std::swap(points[t], points[i]);
            }
        }

        glBegin(GL_POLYGON);
        for (size_t i=0; i<points.size(); ++i)
            glVertex3fv(&points[i][0]);
        glEnd();
    }

#else
    glBegin(GL_QUADS);
    float view[3] = { abs(matrix[2]), abs(matrix[6]), abs(matrix[10]) }; // vec4 view = mat4(matrix) * vec4(0,0,-1,0);
    int maxis = (view[0]>view[1]) ? (view[0]>view[2] ? 0 : 2) : (view[1]>view[2] ? 1 : 2);
    if (maxis == 0) {
        size_t stepx = (size_t)(_width/abs(matrix[2]) + 0.5f);
        if (matrix[2] > 0.0f) {
            for (size_t i=0; i<=stepx; ++i) {
                glVertex3f(2.0f*i/stepx-1.0f, -1.0f, -1.0f);
                glVertex3f(2.0f*i/stepx-1.0f, 1.0f, -1.0f);
                glVertex3f(2.0f*i/stepx-1.0f, 1.0f, 1.0f);
                glVertex3f(2.0f*i/stepx-1.0f, -1.0f, 1.0f);
            }
        }
        else if (matrix[2] < 0.0f) {
            for (size_t i=0; i<=stepx; ++i) {
                glVertex3f(1.0f-2.0f*i/stepx, -1.0f, -1.0f);
                glVertex3f(1.0f-2.0f*i/stepx, 1.0f, -1.0f);
                glVertex3f(1.0f-2.0f*i/stepx, 1.0f, 1.0f);
                glVertex3f(1.0f-2.0f*i/stepx, -1.0f, 1.0f);
            }
        }
    }
    else if (maxis == 1) {
        size_t stepy = (size_t)(_height/abs(matrix[6]) + 0.5f);
        if (matrix[6] > 0.0f) {
            for (size_t i=0; i<=stepy; ++i) {
                glVertex3f(-1.0f, 2.0f*i/stepy-1.0f, -1.0f);
                glVertex3f(1.0f, 2.0f*i/stepy-1.0f, -1.0f);
                glVertex3f(1.0f, 2.0f*i/stepy-1.0f, 1.0f);
                glVertex3f(-1.0f, 2.0f*i/stepy-1.0f, 1.0f);
            }
        }
        else if (matrix[6] < 0.0f) {
            for (size_t i=0; i<=stepy; ++i) {
                glVertex3f(-1.0f, 1.0f-2.0f*i/stepy, -1.0f);
                glVertex3f(1.0f, 1.0f-2.0f*i/stepy, -1.0f);
                glVertex3f(1.0f, 1.0f-2.0f*i/stepy, 1.0f);
                glVertex3f(-1.0f, 1.0f-2.0f*i/stepy, 1.0f);
            }
        }
    }
    else if (maxis == 2) {
        size_t stepz = (size_t)(_depth*_thickness/abs(matrix[10]) + 0.5f);
        if (matrix[10] > 0.0f) {
            for (size_t i=0; i<=stepz; ++i) {
                glVertex3f(-1.0f, -1.0f, 2.0f*i/stepz-1.0f);
                glVertex3f(1.0f, -1.0f, 2.0f*i/stepz-1.0f);
                glVertex3f(1.0f, 1.0f, 2.0f*i/stepz-1.0f);
                glVertex3f(-1.0f, 1.0f, 2.0f*i/stepz-1.0f);
            }
        }
        else if (matrix[10] < 0.0f) {
            for (size_t i=0; i<=stepz; ++i) {
                glVertex3f(-1.0f, -1.0f, 1.0f-2.0f*i/stepz);
                glVertex3f(1.0f, -1.0f, 1.0f-2.0f*i/stepz);
                glVertex3f(1.0f, 1.0f, 1.0f-2.0f*i/stepz);
                glVertex3f(-1.0f, 1.0f, 1.0f-2.0f*i/stepz);
            }
        }
    }
    glEnd();
#endif

    glUseProgram(0);
    glPopAttrib();
    glPopMatrix();
}

void Volume::Show() const
{
    printf("# statistics information of volume:\n");
    printf("#   width %d, height %d, depth %d, thickness %.2f, scale %.2f\n", _width, _height, _depth, _thickness, _scale);
    printf("#   voxels mean %.2f, low %.2f, high %.2f\n", _mean, _low, _high);
}

void Volume::SetSample(int level) const
{
    if (_buffer == 0 || !glIsTexture(_texture) || level <= 0)  return;

    unsigned char *buffer = _buffer;
    size_t width = _width/level, height = _height/level, depth = _depth;
    if (level != 1) {
        clock_t t = clock();
        buffer = new unsigned char[width*height*depth];
        float value = 0.0f;
        #pragma omp parallel for private(value)
        for (int z=0; z<(int)depth; ++z) {
            for (size_t y=0; y<height; ++y) {
                for (size_t x=0; x<width; ++x) {
                    value = 0.0f;
                    for (int i=1-level; i<=level-1; ++i) {
                        for (int j=1-level; j<=level-1; ++j) {
                            value += GetVoxel(level*x+i, level*y+j, z);
                        }
                    }
                    value /= ((2*level-1)*(2*level-1));
                    buffer[z*height*width + y*width + x] = (unsigned char)value;
                }
            }
        }
        printf("[Volume::SetSample] downsampling volume ok (%ld ms)\n", clock()-t);
    }

    glBindTexture(GL_TEXTURE_3D, _texture);
    glTexImage3D(GL_TEXTURE_3D, 0, GL_INTENSITY, width, height, depth, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, buffer);
    if (buffer != _buffer) delete[] buffer;
}

void Volume::SetColor(const unsigned char *color) const
{
    if (!glIsTexture(_color)) return;
    glBindTexture(GL_TEXTURE_1D, _color);
    glTexImage1D(GL_TEXTURE_1D, 0, GL_INTENSITY, 256, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, color);
}

float Volume::GetVoxel(float x, float y, float z) const
{
    if (_buffer == 0 || x < 0.0f || y < 0.0f || z < 0.0f)  return 0.0f;

    size_t x0 = (size_t)x;
    size_t x1 = x0+1;
    size_t y0 = (size_t)y;
    size_t y1 = y0+1;
    size_t z0 = (size_t)z;
    size_t z1 = z0+1;
    float xiy0z0 = GetVoxel(x0,y0,z0)*(x1-x) + GetVoxel(x1,y0,z0)*(x-x0);
    float xiy1z0 = GetVoxel(x0,y1,z0)*(x1-x) + GetVoxel(x1,y1,z0)*(x-x0);
    float xiyiz0 = xiy0z0*(y1-y) + xiy1z0*(y-y0);
    float xiy0z1 = GetVoxel(x0,y0,z1)*(x1-x) + GetVoxel(x1,y0,z1)*(x-x0);
    float xiy1z1 = GetVoxel(x0,y1,z1)*(x1-x) + GetVoxel(x1,y1,z1)*(x-x0);
    float xiyiz1 = xiy0z1*(y1-y) + xiy1z1*(y-y0);
    return xiyiz0*(z1-z) + xiyiz1*(z-z0);
}

Point Volume::GetPoint(float x, float y, float z) const
{
    if (_buffer == 0) return Point();

    Point point;
    point.X = (_scale*x+_width)/2.0f;
    point.Y = (_scale*y+_height)/2.0f;
    point.Z = (_scale/_thickness*z+_depth)/2.0f;
    return point;
}

Point Volume::GetPoint(const Point &point0, const Point &point1) const
{
    if (_buffer == 0)  return Point();

    int dx = (int)abs(point0.X-point1.X), dy = (int)abs(point0.Y-point1.Y), dz = (int)abs(point0.Z-point1.Z);
    int ds = (dx>dy) ? (dx>dz ? dx+1 : dz+1) : (dy>dz ? dy+1 : dz+1);
    float t;
    Point pt, point = point0;
    for (int i=0; i<ds; ++i) {
        t = i/(ds-1.0f);
        pt.X = t*point0.X + (1-t)*point1.X;
        pt.Y = t*point0.Y + (1-t)*point1.Y;
        pt.Z = t*point0.Z + (1-t)*point1.Z;
        pt.Value = GetVoxel(pt);
        if (pt.Value > point.Value) point = pt;
    }
    return point;
}

void Volume::GetValue(float &mean, float &low, float &high, size_t x, size_t y, size_t z, size_t radius) const
{
    if (_buffer == 0 || x >= _width || y >= _height || z >= _depth) {
        mean = low = high = 0.0f;
        return;
    }

    size_t x0 = (x<radius) ? 0 : (x-radius);
    size_t x1 = (x+radius>=_width) ? (_width-1) : (x+radius);
    size_t y0 = (y<radius) ? 0 : (y-radius);
    size_t y1 = (y+radius>=_height) ? (_height-1) : (y+radius);
    size_t z0 = (z<radius) ? 0 : (z-radius);
    size_t z1 = (z+radius>=_depth) ? (_depth-1) : (z+radius);
    
    size_t nb, nf;
    double b, f, t0, t1; // OVERFLOW
    nb = nf = 0;
    b = f = t0 = t1 = 0.0;

    unsigned char v;
    #pragma omp parallel for private(v) reduction(+:nf,f)
    for (int i=z0; i<=(int)z1; ++i) {
        for (size_t j=y0; j<=y1; ++j) {
            for (size_t k=x0; k<=x1; ++k) {
                v = _buffer[i*_height*_width+j*_width+k];
                if (v >= 0 && v <= 255) {
                    ++nf;
                    f += v/255.0;
                }
            }
        }
    }
    if (nf > 0) t1 = f/nf*255.0;
    mean = (float)t1;

    do {
        nb = nf = 0;
        b = f = 0.0;
        t0 = t1;
        #pragma omp parallel for private(v) reduction(+:nb,b,nf,f)
        for (int i=z0; i<=(int)z1; ++i) {
            for (size_t j=y0; j<=y1; ++j) {
                for (size_t k=x0; k<=x1; ++k) {
                    v = _buffer[i*_height*_width+j*_width+k];
                    if (v >= 0 && v < t0) {
                        ++nb;
                        b += v/255.0;
                    }
                    else if (v >= t0 && v <= 255) {
                        ++nf;
                        f += v/255.0;
                    }
                }
            }
        }
        if (nb > 0) b = b/nb*255.0;
        if (nf > 0) f = f/nf*255.0;
        t1 = (b+f)/2.0f;
    } while (abs(t1-t0) > 0.5);
    low = (float)b;
    high = (float)f;
}

void Volume::SetValue(int low, int high, size_t x, size_t y, size_t z, size_t radius)
{
    if (_buffer == 0 || x >= _width || y >= _height || z >= _depth) return;

    size_t x0 = (x<radius) ? 0 : (x-radius);
    size_t x1 = (x+radius>=_width) ? (_width-1) : (x+radius);
    size_t y0 = (y<radius) ? 0 : (y-radius);
    size_t y1 = (y+radius>=_height) ? (_height-1) : (y+radius);
    size_t z0 = (z<radius) ? 0 : (z-radius);
    size_t z1 = (z+radius>=_depth) ? (_depth-1) : (z+radius);

    if (low < 0) low = 0;
    if (high > 255) high = 255;
    float diff = 255.0f/(high-low);
    unsigned char *ptr = _buffer;
    #pragma omp parallel for private(ptr)
    for (int i=z0; i<=(int)z1; ++i) {
        for (size_t j=y0; j<=y1; ++j) {
            for (size_t k=x0; k<=x1; ++k) {
                ptr = _buffer + i*_height*_width + j*_width + k;
                if(*ptr < low) *ptr = low;
                else if(*ptr > high) *ptr = high;
                *ptr = (unsigned char)((*ptr-low)*diff);
            }
        }
    }

    if (!glIsTexture(_texture)) return;
    glBindTexture(GL_TEXTURE_3D, _texture);
    glTexSubImage3D(GL_TEXTURE_3D, 0, 0, 0, z0, _width, _height, z1-z0+1, GL_LUMINANCE, GL_UNSIGNED_BYTE, _buffer+z0*_height*_width);
}

void Volume::SetValue(int low, int high)
{
    SetValue(low, high, 0, 0, 0, (size_t)(_scale+0.5f));
    clock_t t = clock();
    GetValue(_mean, _low, _high, 0, 0, 0, (size_t)(_scale+0.5f));
    printf("[Volume::SetValue] calculate volume voxel values ok (%ld ms)\n", clock()-t);
}

void Volume::GetIndex(double *index, size_t x, size_t y, size_t z, size_t radius) const
{
    if (_buffer == 0 || index == 0 || x >= _width || y >= _height || z >= _depth) return;

    size_t x0 = (x<radius) ? 0 : (x-radius);
    size_t x1 = (x+radius>=_width) ? (_width-1) : (x+radius);
    size_t y0 = (y<radius) ? 0 : (y-radius);
    size_t y1 = (y+radius>=_height) ? (_height-1) : (y+radius);
    size_t z0 = (z<radius) ? 0 : (z-radius);
    size_t z1 = (z+radius>=_depth) ? (_depth-1) : (z+radius);

    for (int i=0; i<256; ++i) index[i] = 0.0; // OVERFLOW
    for (size_t i=z0; i<=z1; ++i) {
        for (size_t j=y0; j<=y1; ++j) {
            for (size_t k=x0; k<=x1; ++k) {
                index[_buffer[i*_height*_width+j*_width+k]] += 1.0f;
            }
        }
    }
}

void Volume::SetValue(const unsigned char *value, size_t x, size_t y, size_t z, size_t radius)
{
    if (_buffer == 0 || value == 0 || x >= _width || y >= _height || z >= _depth) return;

    size_t x0 = (x<radius) ? 0 : (x-radius);
    size_t x1 = (x+radius>=_width) ? (_width-1) : (x+radius);
    size_t y0 = (y<radius) ? 0 : (y-radius);
    size_t y1 = (y+radius>=_height) ? (_height-1) : (y+radius);
    size_t z0 = (z<radius) ? 0 : (z-radius);
    size_t z1 = (z+radius>=_depth) ? (_depth-1) : (z+radius);

    unsigned char *ptr = _buffer;
    #pragma omp parallel for private(ptr)
    for (int i=z0; i<=(int)z1; ++i) {
        for (size_t j=y0; j<=y1; ++j) {
            for (size_t k=x0; k<=x1; ++k) {
                ptr = _buffer + i*_height*_width + j*_width + k;
                *ptr = value[*ptr];
            }
        }
    }

    if (!glIsTexture(_texture)) return;
    glBindTexture(GL_TEXTURE_3D, _texture);
    glTexSubImage3D(GL_TEXTURE_3D, 0, 0, 0, z0, _width, _height, z1-z0+1, GL_LUMINANCE, GL_UNSIGNED_BYTE, _buffer+z0*_height*_width);
}

void Volume::SetValue(const unsigned char *value)
{
    SetValue(value, 0, 0, 0, (size_t)(_scale+0.5f));
    clock_t t = clock();
    GetValue(_mean, _low, _high, 0, 0, 0, (size_t)(_scale+0.5f));
    printf("[Volume::SetValue] calculate volume voxel values ok (%ld ms)\n", clock()-t);
}
