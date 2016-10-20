#include "vision.h"

#include <stdio.h>
#include <math.h>
#include <GL/glew.h>

bool Soma::Read(const char *path)
{
    FILE *file = fopen(path, "r");
    if (file == 0) {
        printf("[Soma::Read] open APO file %s failed\n", path);
        return false;
    }

    size_t len = _list.size();
    char line[256];
    Cell cell;
    while (feof(file) == 0) {
        if (fgets(line, 256, file) == 0) continue;
        if (line[0]=='#' || line[0]==' ' || strlen(line)<9) continue;
        sscanf(line, "%f %f %f %f %f", &cell.X, &cell.Y, &cell.Z, &cell.Radius, &cell.Value);
        _list.push_back(cell);
    }
    fclose(file);
    printf("[Soma::Read] read APO file %s ok\n", path);

    for (size_t i=len; i<_list.size(); ++i) { // [0,S] -> [-1,1]
        _list[i].X = (2.0f*_list[i].X-_width)/_scale;
        _list[i].Y = (2.0f*_list[i].Y-_height)/_scale;
        _list[i].Z = (2.0f*_list[i].Z-_depth)*_thickness/_scale;
        _list[i].Radius = 2.0f*_list[i].Radius/_scale;
        _list[i].Value = _list[i].Value/255.0f;
    }
    printf("[Soma::Read] resolve multiple cells (#%d) ok\n", _list.size()-len);
    return true;
}

bool Soma::Write(const char *path) const
{
    FILE *file = fopen(path, "w");
    if (file == 0) {
        printf("[Soma::Write] open APO file %s failed\n", path);
        return false;
    }

    fprintf(file, "# neuron soma APO model file\n");
    fprintf(file, "# created or edited by flNeuronTool\n");
    fprintf(file, "# width %d, height %d, depth %d\n", _width, _height, _depth);
    fprintf(file, "# line: x y z radius value\n\n");
    Cell cell;
    for (size_t i=0; i<_list.size(); ++i) { // [-1,1] -> [0,S]
        cell = _list[i];
        cell.X = (_scale*cell.X+_width)/2.0f;
        cell.Y = (_scale*cell.Y+_height)/2.0f;
        cell.Z = (_scale/_thickness*cell.Z+_depth)/2.0f;
        cell.Radius = _scale*cell.Radius/2.0f;
        cell.Value = cell.Value*255.0f;
        fprintf(file, "%.2f %.2f %.2f %.2f %.2f\n", cell.X, cell.Y, cell.Z, cell.Radius, cell.Value);
    }
    fclose(file);
    printf("[Soma::Write] write APO file %s ok\n", path);
    return true;
}

void Soma::Draw() const
{
    if (_list.empty() || _style == APO_NONE) return;

    if (_style == APO_POINT) {
        glPushAttrib(GL_POINT_BIT);
        glPointSize(10.0f);
        glBegin(GL_POINTS);
        for (size_t i=0; i<_list.size(); ++i) {
            if (_list[i].Cur) glColor3f(1.0f, 0.0f, 0.0f);
            else glColor3f(0.0f, _list[i].Value, 1.0f-_list[i].Value);
            glVertex3f(_list[i].X, _list[i].Y, _list[i].Z);
        }
        glEnd();
        glPopAttrib();
        return;
    }

    if (_style == APO_WIRE) {
        glPushAttrib(GL_LINE_BIT | GL_POLYGON_BIT | GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_LIGHTING_BIT);
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
        glDisable(GL_LIGHTING);
        glDisable(GL_NORMALIZE);
        GLUquadricObj *quad = gluNewQuadric();
        gluQuadricDrawStyle(quad, GLU_LINE);
        for (size_t i=0; i<_list.size(); ++i) {
            glPushMatrix();
            glTranslatef(_list[i].X, _list[i].Y, _list[i].Z);
            if (_list[i].Cur) glColor3f(1.0f, 0.0f, 0.0f);
            else glColor3f(0.0f, _list[i].Value, 1.0f-_list[i].Value);
            gluSphere(quad, _list[i].Radius, 12, 12);
            glPopMatrix();
        } 
        gluDeleteQuadric(quad);
        glPopAttrib();
        return;
    }

    static const GLubyte color[8][3] = {
        {0,   200, 200},  // cyan
        {220, 200, 0  },  // yellow
        {0,   200, 20 },  // green
        {180, 90,  30 },  // coffee
        {180, 200, 120},  // asparagus
        {200, 100, 100},  // salmon
        {120, 200, 200},  // ice
        {100, 120, 200},  // orchid
    };

    if (_style == APO_SOLID) {
        glPushAttrib(GL_LINE_BIT | GL_POLYGON_BIT | GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_LIGHTING_BIT);
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
        glEnable(GL_LIGHTING);
        glEnable(GL_NORMALIZE);
        GLUquadricObj *quad = gluNewQuadric();
        gluQuadricDrawStyle(quad, GLU_FILL);
        for (size_t i=0; i<_list.size(); ++i) {
            glPushMatrix();
            glTranslatef(_list[i].X, _list[i].Y, _list[i].Z);
            if (_list[i].Cur) glColor3f(1.0f, 0.0f, 0.0f);
            //else glColor3f(0.0f, _list[i].Value, 1.0f-_list[i].Value);
            else glColor3ubv(color[i%8]);
            gluSphere(quad, _list[i].Radius*2.0, 32, 32);
            glPopMatrix();
        }
        gluDeleteQuadric(quad);
        glPopAttrib();
        return;
    }
}

void Soma::Show() const
{
    static const float pi = 3.14159265f;

    size_t count;
    float radius, mean, low, high, surface, volume;
    count = _list.size();
    radius = mean = high = surface = volume = 0.0f;
    low = _scale;
    for (size_t i=0; i<count; ++i) {
        radius = _scale*_list[i].Radius/2.0f;
        mean += radius;
        if (radius < low) low = radius;
        if (radius > high) high = radius;
        surface += radius*radius;
        volume += radius*radius*radius;
    }
    mean /= count;
    surface *= 4.0f*pi;
    volume *= 4.0f*pi/3.0f;

    printf("# statistics information of soma model:\n");
    printf("#   width %d, height %d, depth %d, thickness %.2f, scale %.2f\n", _width, _height, _depth, _thickness, _scale);
    printf("#   cells count %d\n", count);
    printf("#   average radius %.2f, minimum radius %.2f, maximum radius %.2f\n", mean, low, high);
    printf("#   surface %.2f, volume %.2f\n", surface, volume);
}

void Soma::DrawId() const
{
    glPushAttrib(GL_LINE_BIT | GL_POLYGON_BIT | GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_LIGHTING_BIT);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glDisable(GL_LIGHTING);
    glDisable(GL_NORMALIZE);
    GLUquadricObj *quad = gluNewQuadric();
    for (size_t i=0; i<_list.size(); ++i) {
        glPushMatrix();
        glPushName(i);
        glTranslatef(_list[i].X, _list[i].Y, _list[i].Z);
        gluSphere(quad, _list[i].Radius, 6, 6);
        glPopName();
        glPopMatrix();
    } 
    gluDeleteQuadric(quad);
    glPopAttrib();
}

void Soma::SetExtent(size_t width, size_t height, size_t depth, float thickness)
{
    for (size_t i=0; i<_list.size(); ++i) { // [-1,1] -> [0,S]
        _list[i].X = (_scale*_list[i].X+_width)/2.0f;
        _list[i].Y = (_scale*_list[i].Y+_height)/2.0f;
        _list[i].Z = (_scale/_thickness*_list[i].Z+_depth)/2.0f;
        _list[i].Radius = _scale*_list[i].Radius/2.0f;
    }

    _width = width;
    _height = height;
    _depth = depth;
    _thickness = thickness;
    _scale = std::max(std::max(_width, _height)*1.0f, _depth*_thickness);
    if (_scale < 1.0f) _scale = 1.0f;

    for (size_t i=0; i<_list.size(); ++i) { // [0,S] -> [-1,1]
        _list[i].X = (2.0f*_list[i].X-width)/_scale;
        _list[i].Y = (2.0f*_list[i].Y-height)/_scale;
        _list[i].Z = (2.0f*_list[i].Z-depth)*thickness/_scale;
        _list[i].Radius = 2.0f*_list[i].Radius/_scale;
    }
}

size_t Soma::Remove()
{
    std::vector<Cell> list;
    for (size_t i=0; i<_list.size(); ++i) {
        if (!_list[i].Cur) list.push_back(_list[i]);
    }
    _list.resize(list.size());
    if (!list.empty()) std::copy(list.begin(), list.end(), _list.begin());
    return _list.size();
}

size_t Soma::PruneSmall(float radius)
{
    if (_list.empty()) return 0;

    radius = 2.0f*radius/_scale;
    std::vector<Cell> list;
    for (size_t i=0; i<_list.size(); ++i) {
        if (_list[i].Radius >= radius) list.push_back(_list[i]);
    }
    _list.resize(list.size());
    if (!list.empty()) std::copy(list.begin(), list.end(), _list.begin());
    return _list.size();
}
