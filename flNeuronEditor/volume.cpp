#include "vision.h"

#include <stdio.h>
#include <math.h>
#include <omp.h>
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <tiffio.h>

Volume::~Volume()
{
    if (_buffer != 0) delete[] _buffer;
    if (glIsTexture(_texture)) glDeleteTextures(1, &_texture);
    _buffer = 0;
    _texture = 0;
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
    if (photo > PHOTOMETRIC_MINISBLACK || channels != 1 || bits != 8) {
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
    if (_flip) glScalef(1.0f, -1.0f, 1.0f);
    float matrix[16];
    glGetFloatv(GL_MODELVIEW_MATRIX, matrix);
    glScalef(_width/_scale, _height/_scale, _depth*_thickness/_scale);
    glTranslatef(_offx*2.0f/_width, _offy*2.0f/_height, _offz*2.0f/_depth);

    static const glm::vec3 vertex[8] = {
        glm::vec3(-1.0, 1.0, 1.0),
        glm::vec3(1.0, 1.0, 1.0),
        glm::vec3(1.0, -1.0, 1.0),
        glm::vec3(-1.0, -1.0, 1.0),
        glm::vec3(-1.0, 1.0, -1.0),
        glm::vec3(1.0, 1.0, -1.0),
        glm::vec3(1.0, -1.0, -1.0),
        glm::vec3(-1.0, -1.0, -1.0),
    };

    if (_bound) {
        glPushAttrib(GL_ENABLE_BIT | GL_LINE_BIT);
        glDepthMask(GL_TRUE);
        glEnable(GL_DEPTH_TEST);
        glDisable(GL_BLEND);
        glDisable(GL_TEXTURE_3D);
        glLineWidth(4.0f);
        //glColor3f(0.75f, 0.75f, 0.75f);
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
        glLineWidth(4.0f);
        glColor3f(0.5f, 0.5f, 0.5f);
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
    glPopAttrib();
    glPopMatrix();
}

void Volume::Show() const
{
    printf("# statistics information of volume:\n");
    printf("#   width %d, height %d, depth %d, thickness %.2f, scale %.2f\n", _width, _height, _depth, _thickness, _scale);
}

void Volume::SetSample(int level) const
{
    if (_buffer == 0 || !glIsTexture(_texture) || level <= 0)  return;

    unsigned char *buffer = _buffer;
    size_t width = _width/level, height = _height/level, depth = _depth;
    if (level != 1) {
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
    }

    glBindTexture(GL_TEXTURE_3D, _texture);
    glTexImage3D(GL_TEXTURE_3D, 0, GL_INTENSITY, width, height, depth, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, buffer);
    if (buffer != _buffer) delete[] buffer;
}
