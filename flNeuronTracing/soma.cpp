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

    _list.clear();
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
   
    if (_scale < 1.0f) _scale = 1.0f;
    for (size_t i=0; i<_list.size(); ++i) { // [0,S] -> [-1,1]
        _list[i].X = (2.0f*_list[i].X-_width)/_scale;
        _list[i].Y = (2.0f*_list[i].Y-_height)/_scale;
        _list[i].Z = (2.0f*_list[i].Z-_depth)*_thickness/_scale;
        _list[i].Radius = 2.0f*_list[i].Radius/_scale;
        _list[i].Value = _list[i].Value/255.0f;
    }
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
        glPointSize(8.0f);
        glBegin(GL_POINTS);
        for (size_t i=0; i<_list.size(); ++i) {
            glColor3f(_list[i].Value, 0.0f, 1.0f-_list[i].Value);
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
            glColor3f(_list[i].Value, 0.0f, 1.0f-_list[i].Value);
            gluSphere(quad, _list[i].Radius, 12, 12);
            glPopMatrix();
        } 
        gluDeleteQuadric(quad);
        glPopAttrib();
        return;
    }

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
            glColor4f(_list[i].Value, 0.0f, 1.0f-_list[i].Value, 0.75f);
            gluSphere(quad, _list[i].Radius, 12, 12);
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
    printf("#   radius average %.2f, minimum %.2f, maximum %.2f\n", mean, low, high);
    printf("#   surface %.2f, volume %.2f\n", surface, volume);
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

PCell Soma::GetPoint(size_t id) const
{
    Cell cell = GetCell(id);
    PCell point; // [-1,1] -> [0,S]
    point.X = (_scale*cell.X+_width)/2.0f;
    point.Y = (_scale*cell.Y+_height)/2.0f;
    point.Z = (_scale/_thickness*cell.Z+_depth)/2.0f;
    point.Radius = _scale*cell.Radius/2.0f;
    point.Value = cell.Value*255.0f;
    return point;
}

size_t Soma::AddPoint(const PCell &point)
{    
    Cell cell; // [0,S] -> [-1,1]
    cell.X = (2.0f*point.X-_width)/_scale;
    cell.Y = (2.0f*point.Y-_height)/_scale;
    cell.Z = (2.0f*point.Z-_depth)*_thickness/_scale;
    cell.Radius = 2.0f*point.Radius/_scale;
    cell.Value = point.Value/255.0f;
    return AddCell(cell);
}

size_t Soma::Reduce(size_t start)
{
    if (_list.empty() || start >= _list.size()) return _list.size();

    static const float rs = 0.61803399f;

    // copy optimal cells to temporary list
    std::vector<Cell> list;
    for (size_t i=0; i<start; ++i) list.push_back(_list[i]);

    // copy redundancy cells to temporary list & merge overlap
    bool repeat;
    float dr, dx, dy, dz;
    for (size_t i=start; i<_list.size(); ++i) {
        repeat = false;
        for (size_t j=0; j<list.size(); ++j) {
            dr = rs*(_list[i].Radius+list[j].Radius);
            dx = abs(_list[i].X-list[j].X);
            if (dx >= dr) continue;
            dy = abs(_list[i].Y-list[j].Y);
            if (dy >= dr) continue;
            dz = abs(_list[i].Z-list[j].Z);
            if (dz >= dr) continue;
            if (dx*dx + dy*dy + dz*dz <= dr*dr) {
                repeat = true;
                if (_merge) {
                    list[j].X = (_list[i].X+list[j].X)/2.0f;
                    list[j].Y = (_list[i].Y+list[j].Y)/2.0f;
                    list[j].Z = (_list[i].Z+list[j].Z)/2.0f;
                    list[j].Radius = rs*(_list[i].Radius+list[j].Radius);
                    list[j].Value = (_list[i].Value+list[j].Value)/2.0f;
                }
                break;
            }
        }
        if (!repeat) list.push_back(_list[i]);
    }

    // copy back optimal cells from temporary list
    _list.resize(list.size());
    std::copy(list.begin()+start, list.end(), _list.begin()+start);
    return _list.size();
}

size_t Soma::PruneSmall(float radius)
{
    if (_list.empty()) return 0;

    radius = 2.0f*radius/_scale;
    std::vector<Cell> list;
    for (size_t i=0; i<_list.size(); ++i) {
        if (_list[i].Radius >= radius)  list.push_back(_list[i]);
    }
    _list.resize(list.size());
    if (!list.empty()) std::copy(list.begin(), list.end(), _list.begin());
    return _list.size();
}
