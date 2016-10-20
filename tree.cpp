#include "vision.h"

#include <stdio.h>
#include <math.h>
#include <GL/glew.h>
#include <glm/glm.hpp>

bool Tree::Read(const char *path)
{
    FILE *file = fopen(path, "r");
    if (file == 0) {
        printf("[Tree::Read] open SWC file %s failed\n", path);
        return false;
    }

    _list.clear();
    char line[256];
    Node node;
    while (feof(file) == 0) {
        if (fgets(line, 256, file) == 0) continue;
        if (line[0]=='#' || line[0]==' ' || strlen(line)<13) continue;
        sscanf(line, "%d %d %f %f %f %f %d", &node.Id, &node.Tag, &node.X, &node.Y, &node.Z, &node.Radius, &node.Pid);
        //if (node.Radius < 0.5f) node.Radius = 0.5f;
        _list.push_back(node);
    }
    fclose(file);
    printf("[Tree::Read] read SWC file %s ok\n", path);

    if (_scale < 1.0f) _scale = 1.0f;
    for (size_t i=0; i<_list.size(); ++i) { // [0,S] -> [-1,1]
        _list[i].X = (2.0f*_list[i].X-_width)/_scale;
        _list[i].Y = (2.0f*_list[i].Y-_height)/_scale;
        _list[i].Z = (2.0f*_list[i].Z-_depth)*_thickness/_scale;
        _list[i].Radius = 2.0f*_list[i].Radius/_scale;
    }
    return true;
}

bool Tree::Write(const char *path) const
{
    FILE *file = fopen(path, "w");
    if (file == 0) {
        printf("[Tree::Write] open SWC file %s failed\n", path);
        return false;
    }

    fprintf(file, "# neuron tree SWC model file\n");
    fprintf(file, "# created or edited by flNeuronTool\n");
    fprintf(file, "# width %d, height %d, depth %d\n", _width, _height, _depth);
    fprintf(file, "# line: id tag x y z radius pid\n\n");
    Node node;
    for (size_t i=0; i<_list.size(); ++i) {
        node = _list[i];
        node.X = (_scale*node.X+_width)/2.0f;
        node.Y = (_scale*node.Y+_height)/2.0f;
        node.Z = (_scale/_thickness*node.Z+_depth)/2.0f;
        node.Radius = _scale*node.Radius/2.0f;
        if (node.Pid <= 0)  node.Pid = -1;
        fprintf(file, "%d %d %.2f %.2f %.2f %.2f %d\n", node.Id, node.Tag, node.X, node.Y, node.Z, node.Radius, node.Pid);
    }
    fclose(file);
    printf("[Tree::Write] write SWC file %s ok\n", path);
    return true;
}

void Tree::Draw() const
{
    if (_list.empty() || _style == SWC_NONE) return;

    if (_style == SWC_LINE) {
        glPushAttrib(GL_LINE_BIT);
        glColor3f(1.0f, 0.5f, 0.25f);
        glLineWidth(4.0f);
        glBegin(GL_LINES);
        for (size_t i=0; i<_list.size(); ++i) {
            if (_list[i].Pid <= 0) continue;
            long p = _list[i].Pid - 1;
            glVertex3f(_list[p].X, _list[p].Y, _list[p].Z);
            glVertex3f(_list[i].X, _list[i].Y, _list[i].Z);
        }
        glEnd();
        glPopAttrib();
        return;
    }

    if (_style == SWC_STRIP) {
        glPushAttrib(GL_POINT_BIT | GL_LINE_BIT);
        glColor3f(1.0f, 0.5f, 0.25f);
        glLineWidth(2.0f);
        glBegin(GL_LINES);
        for (size_t i=0; i<_list.size(); ++i) {
            if (_list[i].Pid <= 0) continue;
            long p = _list[i].Pid - 1;
            glVertex3f(_list[p].X, _list[p].Y, _list[p].Z);
            glVertex3f(_list[i].X, _list[i].Y, _list[i].Z);
        }
        glEnd();
        glColor3f(1.0f, 0.25f, 0.25f);
        glPointSize(4.0f);
        glBegin(GL_POINTS);
        for (size_t i=0; i<_list.size(); ++i)
            glVertex3f(_list[i].X, _list[i].Y, _list[i].Z);
        glEnd();
        glPopAttrib();
        return;
    }

    if (_style == SWC_DISK) {
        glPushAttrib(GL_LINE_BIT | GL_POLYGON_BIT | GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_LIGHTING_BIT);
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
        glDisable(GL_LIGHTING);
        glDisable(GL_NORMALIZE);
        glColor3f(1.0f, 0.25f, 0.25f);
        glLineWidth(2.0f);
        glBegin(GL_LINES);
        for (size_t i=0; i<_list.size(); ++i) {
            if (_list[i].Pid <= 0) continue;
            long p = _list[i].Pid - 1;
            glVertex3f(_list[p].X, _list[p].Y, _list[p].Z);
            glVertex3f(_list[i].X, _list[i].Y, _list[i].Z);
        }
        glEnd();
        glColor3f(1.0f, 0.5f, 0.25f);
        GLUquadricObj *quad = gluNewQuadric();
        gluQuadricDrawStyle(quad, GLU_SILHOUETTE);
        for (size_t i=0; i<_list.size(); ++i) {
            if (_list[i].Pid <= 0) continue;
            glPushMatrix();
            glTranslatef(_list[i].X, _list[i].Y, _list[i].Z);
            size_t p = _list[i].Pid-1;
            glm::vec3 line(_list[p].X-_list[i].X, _list[p].Y-_list[i].Y, _list[p].Z-_list[i].Z);
            float len =  glm::length(line);
            line = glm::normalize(line);
            glm::vec3 zaxis(0.0f, 0.0f, 1.0f);
            glm::vec3 axis = glm::cross(zaxis, line);
            float angle = glm::acos(glm::dot(zaxis, line));
            glRotatef(glm::degrees(angle), axis.x, axis.y, axis.z);
            gluDisk(quad, 0.0f, _list[i].Radius, 16, 1);
            glPopMatrix();
        }
        gluDeleteQuadric(quad); 
        glPopAttrib();
        return;
    }

    if (_style == SWC_BALL) {
        glPushAttrib(GL_LINE_BIT | GL_POLYGON_BIT | GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_LIGHTING_BIT);
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
        glEnable(GL_LIGHTING);
        glEnable(GL_NORMALIZE);
        glColor3f(1.0f, 0.5f, 0.25f);
        GLUquadricObj *quad = gluNewQuadric();
        gluQuadricDrawStyle(quad, GLU_FILL);
        for (size_t i=0; i<_list.size(); ++i) {
            glPushMatrix();
            glTranslatef(_list[i].X, _list[i].Y, _list[i].Z);
            gluSphere(quad, _list[i].Radius, 12, 12);
            glPopMatrix();
        }
        gluDeleteQuadric(quad);
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
        glDisable(GL_LIGHTING);
        glDisable(GL_NORMALIZE);
        glColor3f(1.0f, 0.25f, 0.25f);
        glLineWidth(2.0f);
        glBegin(GL_LINES);
        for (size_t i=0; i<_list.size(); ++i) {
            if (_list[i].Pid <= 0) continue;
            long p = _list[i].Pid - 1;
            glVertex3f(_list[p].X, _list[p].Y, _list[p].Z);
            glVertex3f(_list[i].X, _list[i].Y, _list[i].Z);
        }
        glEnd();
        glPopAttrib();
        return;
    }

    if (_style == SWC_WIRE) {
        glPushAttrib(GL_LINE_BIT | GL_POLYGON_BIT | GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_LIGHTING_BIT);
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
        glDisable(GL_LIGHTING);
        glDisable(GL_NORMALIZE);
        glColor3f(1.0f, 0.5f, 0.25f);
        GLUquadricObj *quad = gluNewQuadric();
        gluQuadricDrawStyle(quad, GLU_LINE);
        for (size_t i=0; i<_list.size(); ++i) {
            if (_list[i].Pid <= 0) continue;
            glPushMatrix();
            glTranslatef(_list[i].X, _list[i].Y, _list[i].Z);
            size_t p = _list[i].Pid-1;
            glm::vec3 line(_list[p].X-_list[i].X, _list[p].Y-_list[i].Y, _list[p].Z-_list[i].Z);
            float len =  glm::length(line);
            line = glm::normalize(line);
            glm::vec3 zaxis(0.0f, 0.0f, 1.0f);
            glm::vec3 axis = glm::cross(zaxis, line);
            float angle = glm::acos(glm::dot(zaxis, line));
            glRotatef(glm::degrees(angle), axis.x, axis.y, axis.z);
            gluCylinder(quad, _list[i].Radius, _list[p].Radius, len, 8, 1);
            glPopMatrix();
        }
        gluDeleteQuadric(quad);
        glPopAttrib();
        return;
    }

    if (_style == SWC_SOLID) {
        glPushAttrib(GL_LINE_BIT | GL_POLYGON_BIT | GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_LIGHTING_BIT);
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
        glEnable(GL_LIGHTING);
        glEnable(GL_NORMALIZE);
        glColor3f(1.0f, 0.5f, 0.25f);
        GLUquadricObj *quad = gluNewQuadric();
        gluQuadricDrawStyle(quad, GLU_FILL);
        for (size_t i=0; i<_list.size(); ++i) {
            glPushMatrix();
            glTranslatef(_list[i].X, _list[i].Y, _list[i].Z);
            gluSphere(quad, _list[i].Radius, 8, 8);
            if (_list[i].Pid <= 0) {
                glPopMatrix();
                continue;
            }
            size_t p = _list[i].Pid-1;
            glm::vec3 line(_list[p].X-_list[i].X, _list[p].Y-_list[i].Y, _list[p].Z-_list[i].Z);
            float len =  glm::length(line);
            line = glm::normalize(line);
            glm::vec3 zaxis(0.0f, 0.0f, 1.0f);
            glm::vec3 axis = glm::cross(zaxis, line);
            float angle = glm::acos(glm::dot(zaxis, line));
            glRotatef(glm::degrees(angle), axis.x, axis.y, axis.z);
            gluCylinder(quad, _list[i].Radius, _list[p].Radius, len, 8, 1);
            glPopMatrix();
        }
        gluDeleteQuadric(quad);
        glPopAttrib();
        return;
    }
}

void Tree::Show() const
{
    size_t count, junction, endpoint;
    float height, mean, low, high, length;
    count = _list.size();
    junction = endpoint = 0;
    height = mean = high = length = 0.0f;
    low = _scale;
    for (size_t i=0; i<count; ++i) {
        if (_list[i].Tag == 5)  ++junction;
        else if (_list[i].Tag == 6) ++endpoint;
        mean += _list[i].Radius;
        if (_list[i].Radius < low) low = _list[i].Radius;
        if (_list[i].Radius > high) high = _list[i].Radius;
        if (_list[i].Pid <= 0) continue;
        size_t p = _list[i].Pid - 1;
        height = sqrt((_list[p].X-_list[i].X)*(_list[p].X-_list[i].X) + (_list[p].Y-_list[i].Y)*(_list[p].Y-_list[i].Y) + (_list[p].Z-_list[i].Z)*(_list[p].Z-_list[i].Z));
        length += height;
    }
    mean /= count;
    mean = _scale*mean/2.0f;
    low = _scale*low/2.0f;
    high = _scale*high/2.0f;
    length = _scale*length/2.0f;

    printf("# statistics information of SWC model:\n");
    printf("#   width %d, height %d, depth %d, thickness %.2f, scale %.2f\n", _width, _height, _depth, _thickness, _scale);
    printf("#   nodes %d, junctions %d, endpoints %d\n", count, junction, endpoint);
    printf("#   radius average %.2f, minimum %.2f, maximum %.2f\n", mean, low, high);
    printf("#   length %.2f\n", length);
}

void Tree::SetExtent(size_t width, size_t height, size_t depth, float thickness)
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
        _list[i].X = (2.0f*_list[i].X-_width)/_scale;
        _list[i].Y = (2.0f*_list[i].Y-_height)/_scale;
        _list[i].Z = (2.0f*_list[i].Z-_depth)*_thickness/_scale;
        _list[i].Radius = 2.0f*_list[i].Radius/_scale;
    }
}

PNode Tree::GetPoint(size_t id) const
{
    Node node = GetNode(id);
    PNode point; // [-1,1] -> [0,S]
    point.Id = node.Id;
    point.X = (_scale*node.X+_width)/2.0f;
    point.Y = (_scale*node.Y+_height)/2.0f;
    point.Z = (_scale/_thickness*node.Z+_depth)/2.0f;
    point.Radius = _scale*node.Radius/2.0f;
    point.Pid = node.Pid;
    return point;
}

size_t Tree::AddPoint(const PNode &point)
{
    Node node; // [0,S] -> [-1,1]
    node.Id = _list.size() + 1;
    node.X = (2.0f*point.X-_width)/_scale;
    node.Y = (2.0f*point.Y-_height)/_scale;
    node.Z = (2.0f*point.Z-_depth)*_thickness/_scale;
    node.Radius = 2.0f*point.Radius/_scale;
    node.Pid = point.Pid;
    return AddNode(node);
}

size_t Tree::Reduce(size_t start, int lower)
{
    if (_list.empty() || start >= _list.size()) return _list.size();

    static const float rs = 0.61803399f;

    // connect all nodes into one tree
    if (_link) {
        float r, minr;
        for (size_t i=start; i<_list.size(); ++i) {
            if (_list[i].Pid > 0) continue;
            minr = _scale;
            for (size_t j=0; j<i; ++j) {
                r = abs(_list[i].X-_list[j].X) + abs(_list[i].Y-_list[j].Y) + abs(_list[i].Z-_list[j].Z);
                if (r < minr) { minr = r; _list[i].Pid = _list[j].Id; }
            }
        }
    }

    // calculate node path length for pruning short path later
    bool doing = true;
    do {
        doing = false;
        for (size_t i=start; i<_list.size(); ++i) {
            if (_list[i].Tag == -1) continue; // FAKE
            _list[i].Child = _list[i].Path = _list[i].Tag = 0; // NONE
            if (_list[i].Pid <= 0) _list[i].Tag = 1; // BODY
            else ++_list[_list[i].Pid-1].Child;
        }
        for (size_t i=start; i<_list.size(); ++i) {
            if (_list[i].Tag == -1) continue; // FAKE
            if (_list[i].Tag == 1) { // BODY
                if (_list[i].Child == 0) _list[i].Tag = -1; // FAKE
                continue;
            }
            if (_list[i].Child == 1) _list[i].Tag = 2; //AXON
            else if (_list[i].Child >= 2) _list[i].Tag = 5; //JUNC
            else if (_list[i].Child == 0) _list[i].Tag = 6; //END
            size_t p = _list[i].Pid - 1;
            if (_list[p].Tag == 1 || _list[p].Tag == 2) _list[i].Path = _list[p].Path + 1;
            else if (_list[p].Tag == 5) _list[i].Path = 1;
            if (_list[i].Tag == 6 && _list[i].Path <= lower) {
                _list[i].Tag = -1; // FAKE
                doing = true;
            }
        }
    } while (doing);

    // copy optimal nodes to temporary list
    std::vector<Node> list;
    for (size_t i=0; i<start; ++i) list.push_back(_list[i]);

    // copy redundancy nodes to temporary list & merge overlap
    bool repeat;
    float dr, dx, dy, dz;
    for (size_t i=start; i<_list.size(); ++i) {
        if (_list[i].Tag == -1) continue;
        if (_list[i].Tag == 1) {
            list.push_back(_list[i]);
            continue;
        }
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
                list[j].X = (_list[i].X+list[j].X)/2.0f;
                list[j].Y = (_list[i].Y+list[j].Y)/2.0f;
                list[j].Z = (_list[i].Z+list[j].Z)/2.0f;
                list[j].Radius = (_list[i].Radius+list[j].Radius)/2.0f;
                for (size_t k=i+1; k<_list.size(); ++k) {
                    if (_list[i].Child == 0) break;
                    if (_list[k].Pid == _list[i].Id) {
                        _list[k].Pid = list[j].Id;
                        --_list[i].Child;
                        ++list[j].Child;
                    }
                }
                break;
            }
        }
        if (!repeat) list.push_back(_list[i]);
    }

    // rearange nodes in temporary list
    std::vector<size_t> ids(_list.size());
    for (size_t i=0; i<list.size(); ++i) ids[list[i].Id-1] = i+1;
    for (size_t i=start; i<list.size(); ++i) {
        list[i].Id = i+1;
        if (list[i].Pid <= 0) continue;
        list[i].Pid = ids[list[i].Pid-1];
    }

    // copy back optimal nodes from temporary list
    _list.resize(list.size());
    std::copy(list.begin()+start, list.end(), _list.begin()+start);
    return _list.size();
}

size_t Tree::Stretch(size_t start)
{
    if (_list.empty() || start >= _list.size()) return _list.size();

    // calculate child nodes number for tagging
    for (size_t i=start; i<_list.size(); ++i) {
        _list[i].Child = _list[i].Path = _list[i].Tag = 0; // NONE
        if (_list[i].Pid > 0) ++_list[_list[i].Pid-1].Child;
    }
    for (size_t i=start; i<_list.size(); ++i) {
        if (_list[i].Pid <= 0) _list[i].Tag = 1; // BODY
        else if (_list[i].Child == 1) _list[i].Tag = 2; //AXON
        else if (_list[i].Child >= 2) _list[i].Tag = 5; //JUNC
        else if (_list[i].Child == 0) _list[i].Tag = 6; //END
    }

    // copy critical nodes to temporary list
    std::vector<Node> list;
    for (size_t i=0; i<start; ++i) list.push_back(_list[i]);

    // copy critical nodes to temporary list
    Node parent;
    for (size_t i=start; i<_list.size(); ++i) {
        if (_list[i].Tag == 1) {
            list.push_back(_list[i]);
            continue;
        }
        if (_list[i].Tag == 5 || _list[i].Tag == 6) {
            parent = _list[_list[i].Pid-1]; 
            while (parent.Tag != 1 && parent.Tag != 5) parent = _list[parent.Pid-1];
            _list[i].Pid = parent.Id;
            list.push_back(_list[i]);
        }
    }

    // rearange nodes in temporary list
    std::vector<size_t> ids(_list.size());
    for (size_t i=0; i<list.size(); ++i) ids[list[i].Id-1] = i+1;
    for (size_t i=start; i<list.size(); ++i) {
        list[i].Id = i+1;
        if (list[i].Pid <= 0) continue;
        list[i].Pid = ids[list[i].Pid-1];
    }

    // copy back critical nodes from temporary list
    _list.resize(list.size());
    std::copy(list.begin()+start, list.end(), _list.begin()+start);
    return _list.size();
}

size_t Tree::FixupRadius(float lower, float upper)
{
    if (_list.empty()) return 0;

    lower = 2.0f*lower/_scale;
    upper = 2.0f*upper/_scale;
    size_t cnt = 0;
    for (size_t i=0; i<_list.size(); ++i) {
        if (_list[i].Radius < lower) { _list[i].Radius = lower; ++cnt; }
        else if (_list[i].Radius > upper) { _list[i].Radius = upper; ++cnt; }
    }
    return cnt;
}
