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

    size_t len = _list.size();
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
    for (size_t i=len; i<_list.size(); ++i) { // [0,S] -> [-1,1]
        _list[i].X = (2.0f*_list[i].X-_width)/_scale;
        _list[i].Y = (2.0f*_list[i].Y-_height)/_scale;
        _list[i].Z = (2.0f*_list[i].Z-_depth)*_thickness/_scale;
        _list[i].Radius = 2.0f*_list[i].Radius/_scale;
    }
    printf("[Tree::Read] resolve multiple nodes (#%d) ok\n", _list.size()-len);
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
        glPushAttrib(GL_POINT_BIT | GL_LINE_BIT);
        if (_cur) glColor3f(1.0f, 0.5f, 0.25f);
        glLineWidth(2.0f);
        glBegin(GL_LINES);
        for (size_t i=0; i<_list.size(); ++i) {
            if (_list[i].Pid <= 0) continue;
            long p = _list[i].Pid - 1;
            glVertex3f(_list[p].X, _list[p].Y, _list[p].Z);
            glVertex3f(_list[i].X, _list[i].Y, _list[i].Z);
        }
        glEnd();
        if (_nid >= 0 && _nid < (int)_list.size()) {
            glColor3f(1.0f, 0.0f, 0.0f);
            glPointSize(8.0f);
            glBegin(GL_POINTS);
            glVertex3f(_list[_nid].X, _list[_nid].Y, _list[_nid].Z);
            glEnd();
        }
        glPopAttrib();
        return;
    }

    if (_style == SWC_STRIP) {
        glPushAttrib(GL_POINT_BIT | GL_LINE_BIT);
        if (_cur) glColor3f(1.0f, 0.5f, 0.25f);
        glLineWidth(2.0f);
        glBegin(GL_LINES);
        for (size_t i=0; i<_list.size(); ++i) {
            if (_list[i].Pid <= 0) continue;
            long p = _list[i].Pid - 1;
            glVertex3f(_list[p].X, _list[p].Y, _list[p].Z);
            glVertex3f(_list[i].X, _list[i].Y, _list[i].Z);
        }
        glEnd();
        if (_cur) glColor3f(1.0f, 0.25f, 0.25f);
        glPointSize(4.0f);
        glBegin(GL_POINTS);
        for (size_t i=0; i<_list.size(); ++i)
            glVertex3f(_list[i].X, _list[i].Y, _list[i].Z);
        glEnd();
        if (_nid >= 0 && _nid < (int)_list.size()) {
            glColor3f(1.0f, 0.0f, 0.0f);
            glPointSize(8.0f);
            glBegin(GL_POINTS);
            glVertex3f(_list[_nid].X, _list[_nid].Y, _list[_nid].Z);
            glEnd();
        }
        glPopAttrib();
        return;
    }

    if (_style == SWC_DISK) {
        glPushAttrib(GL_LINE_BIT | GL_POLYGON_BIT | GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_LIGHTING_BIT);
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
        glDisable(GL_LIGHTING);
        glDisable(GL_NORMALIZE);
        if (_cur) glColor3f(1.0f, 0.25f, 0.25f);
        glLineWidth(2.0f);
        glBegin(GL_LINES);
        for (size_t i=0; i<_list.size(); ++i) {
            if (_list[i].Pid <= 0) continue;
            long p = _list[i].Pid - 1;
            glVertex3f(_list[p].X, _list[p].Y, _list[p].Z);
            glVertex3f(_list[i].X, _list[i].Y, _list[i].Z);
        }
        glEnd();
        if (_cur) glColor3f(1.0f, 0.5f, 0.25f);
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
        if (_nid >= 0 && _nid < (int)_list.size()) {
            glColor3f(1.0f, 0.0f, 0.0f);
            glPointSize(8.0f);
            glBegin(GL_POINTS);
            glVertex3f(_list[_nid].X, _list[_nid].Y, _list[_nid].Z);
            glEnd();
        }        
        glPopAttrib();
        return;
    }

    if (_style == SWC_BALL) {
        glPushAttrib(GL_LINE_BIT | GL_POLYGON_BIT | GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_LIGHTING_BIT);
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
        glDisable(GL_LIGHTING);
        glDisable(GL_NORMALIZE);
        if (_cur) glColor3f(1.0f, 0.25f, 0.25f);
        glLineWidth(2.0f);
        glBegin(GL_LINES);
        for (size_t i=0; i<_list.size(); ++i) {
            if (_list[i].Pid <= 0) continue;
            long p = _list[i].Pid - 1;
            glVertex3f(_list[p].X, _list[p].Y, _list[p].Z);
            glVertex3f(_list[i].X, _list[i].Y, _list[i].Z);
        }
        glEnd();
        glEnable(GL_LIGHTING);
        glEnable(GL_NORMALIZE);
        if (_cur) glColor3f(1.0f, 0.5f, 0.25f);
        GLUquadricObj *quad = gluNewQuadric();
        gluQuadricDrawStyle(quad, GLU_FILL);
        for (size_t i=0; i<_list.size(); ++i) {
            if (i == _nid) continue;
            glPushMatrix();
            glTranslatef(_list[i].X, _list[i].Y, _list[i].Z);
            gluSphere(quad, _list[i].Radius, 12, 12);
            glPopMatrix();
        }
        if (_nid >= 0 && _nid < (int)_list.size()) {
            glColor3f(1.0f, 0.0f, 0.0f);
            gluQuadricDrawStyle(quad, GLU_FILL);
            glPushMatrix();
            glTranslatef(_list[_nid].X, _list[_nid].Y, _list[_nid].Z);
            gluSphere(quad, _list[_nid].Radius, 12, 12);
            glPopMatrix();
        }
        gluDeleteQuadric(quad);
        glPopAttrib();
        return;
    }

    if (_style == SWC_WIRE) {
        glPushAttrib(GL_LINE_BIT | GL_POLYGON_BIT | GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_LIGHTING_BIT);
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
        glDisable(GL_LIGHTING);
        glDisable(GL_NORMALIZE);
        if (_cur) glColor3f(1.0f, 0.5f, 0.25f);
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
        if (_nid >= 0 && _nid < (int)_list.size()) {
            glColor3f(1.0f, 0.0f, 0.0f);
            gluQuadricDrawStyle(quad, GLU_FILL);
            glPushMatrix();
            glTranslatef(_list[_nid].X, _list[_nid].Y, _list[_nid].Z);
            gluSphere(quad, _list[_nid].Radius, 12, 12);
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
        if (_cur) glColor3f(1.0f, 0.5f, 0.25f);
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
        if (_nid >= 0 && _nid < (int)_list.size()) {
            glColor3f(1.0f, 0.0f, 0.0f);
            gluQuadricDrawStyle(quad, GLU_FILL);
            glPushMatrix();
            glTranslatef(_list[_nid].X, _list[_nid].Y, _list[_nid].Z);
            gluSphere(quad, _list[_nid].Radius, 12, 12);
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
    printf("#   average radius %.2f, minimum radius %.2f, maximum radius %.2f\n", mean, low, high);
    printf("#   length %.2f\n", length);
    //printf("#, %d, %d, %d, %.2f, %.2f\n", count, junction, endpoint, mean, length);
}

void Tree::Show(char *string) const
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

    sprintf(string, "%d %d %d %.2f %.2f %.2f %.2f", count, junction, endpoint, mean, low, high, length);
}

void Tree::DrawId() const
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
    return;
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

void Tree::Taging()
{
    if (_list.empty()) return;

    for (size_t i=0; i<_list.size(); ++i) {
        _list[i].Tag = 0;
        if (_list[i].Pid <= 0) continue;
        ++_list[_list[i].Pid-1].Tag;
        if (_list[i].Pid != _list[i].Id-1) ++_list[_list[i].Pid-1].Tag;
    }
    for (size_t i=0; i<_list.size(); ++i) {
        if (_list[i].Tag < 1) _list[i].Tag = 6; // END
        else if (_list[i].Tag > 1) _list[i].Tag = 5; // JUNC
        else _list[i].Tag = 2; // AXON
    }
    _list[0].Tag = 1; // BODY
}

void Tree::Rotate()
{
    if (_list.empty() || _nid <= 0 || _nid >= (int)_list.size()) return;

    std::vector<Node> list(_list.begin(), _list.end());
    for (size_t i=0; i<list.size(); ++i) list[i].Tag = 0;
    _list.clear();

    int i = _nid;
    int p = list[i].Pid - 1;
    while (i >= 0) {
        p = list[i].Pid - 1;
        list[i].Pid = _list.size();
        list[i].Id = _list.size() + 1;
        list[i].Tag = 1;
        _list.push_back(list[i]);
        i = p;
    }

    for (size_t i=0; i<list.size(); ++i) {
        if (list[i].Tag == 1 || list[i].Pid <= 0) continue;
        list[i].Pid = list[list[i].Pid-1].Id;
        list[i].Id = _list.size() + 1;
        list[i].Tag = 1;
        _list.push_back(list[i]);
    }

    _list[0].Pid = -1;
    _nid = 0;
}

void Tree::Resolve(Trees &trees)
{
    size_t len = trees.size();
    int tid = (int)len;
    for (size_t i=0; i<_list.size(); ++i) {
        if (_list[i].Pid <= 0) {
            _list[i].Tag = tid++;
            Tree *tree = new Tree();
            tree->SetExtent(_width, _height, _depth, _thickness);
            tree->SetStyle(_style);
            trees.push_back(tree);
            continue;
        }
        _list[i].Tag = _list[_list[i].Pid-1].Tag;
    }

    Tree *ptr = 0;
    for (size_t i=0; i<_list.size(); ++i) {
        ptr = trees[_list[i].Tag];
        _list[i].Id = ptr->_list.size() + 1;
        if (_list[i].Pid > 0) _list[i].Pid = _list[_list[i].Pid-1].Id;
        ptr->_list.push_back(_list[i]);
    }
    printf("[Tree::Resolve] resolve multiple trees (#%d) ok\n", trees.size()-len);
}

void Tree::Split(Tree &tree)
{
    if (_nid <= 0 || _nid >= (int)_list.size()) return;

    std::vector<Node> list(_list.begin(), _list.end());
    _list.clear();
    list[_nid].Pid = -1;
    list[_nid].Tag = list[0].Tag + 1;
    for (size_t i=0; i<list.size(); ++i)
        if (list[i].Pid > 0) list[i].Tag = list[list[i].Pid-1].Tag;

    tree.SetExtent(_width, _height, _depth, _thickness);
    tree.SetStyle(_style);
    Tree *ptr = 0;
    for (size_t i=0; i<list.size(); ++i) {
        ptr = list[i].Tag == list[0].Tag ? this : &tree;
        list[i].Id = ptr->_list.size() + 1;
        if (list[i].Pid > 0) list[i].Pid = list[list[i].Pid-1].Id;
        ptr->_list.push_back(list[i]);
    }
}

void Tree::Merge(const Trees &trees)
{
    size_t nid = 0;    
    for (size_t i=0; i<trees.size(); ++i) {
        nid = _list.size();
        std::copy(trees[i]->_list.begin(), trees[i]->_list.end(), std::back_inserter(_list));
        for (size_t i=nid; i<_list.size(); ++i) {
            _list[i].Id += nid;
            if (_list[i].Pid > 0) _list[i].Pid += nid;
        }
    }
}

void Tree::Merge(const Tree &tree)
{
    int nid = _list.size();
    std::copy(tree._list.begin(), tree._list.end(), std::back_inserter(_list));
    for (size_t i=nid; i<_list.size(); ++i) {
        _list[i].Id += nid;
        if (_list[i].Pid > 0)  _list[i].Pid += nid;
    }
}

void Tree::Link(Tree &tree)
{
    if (_nid <= 0 || _nid >= (int)_list.size()) _nid = 0;

    tree.Rotate();
    size_t nid = _list.size();
    std::copy(tree._list.begin(), tree._list.end(), std::back_inserter(_list));
    for (size_t i=nid; i<_list.size(); ++i) {
        _list[i].Id += nid;
        if (_list[i].Pid > 0)  _list[i].Pid += nid;
    }
    _list[nid].Pid = _list[_nid].Id;
}
