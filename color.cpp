#include "vision.h"

#include <stdio.h>
#include <gl/glew.h>

Color::Color()
    : _width(512), _height(256),
    _scalex(_width/256.0f), _scaley(_height/256.0f),
    _style(LUT_LINE)
{
    _scalex = _width/256.0f;
    _scaley = _height/256.0f;
    _index = new unsigned char[256];
    memset(_index, 0, 256*sizeof(unsigned char));
    _value = new unsigned char[256];
    for (int i=0; i<256; ++i) _value[i] = i;
    _list[0] = 0;
    _list[255] = 255;
}

bool Color::Read(const char *path)
{
    FILE *file = fopen(path, "r");
    if (file == 0) {
        printf("[Color::Read] open LUT file %s failed\n", path);
        return false;
    }

    _list.clear();
    char line[256];
    int index, value;
    while (feof(file) == 0) {
        if (fgets(line, 255, file) == 0) continue;
        if (line[0]=='#' || line[0]==' ' || strlen(line)<3) continue;
        sscanf(line, "%d %d", &index, &value);
        if (index>=0 && index<=255 && value>=0 && value<=255) _list[index] = value;
    }
    fclose(file);
    printf("[Color::Read] read LUT file %s ok\n", path);

    map_t::const_iterator it, st;
    for (it = _list.begin(); it != _list.end(); ++it) {
        st = it;
        ++st;
        if (st != _list.end()) {
            for (int i=it->first; i<st->first; ++i)
                _value[i] = it->second + (unsigned char)((i-it->first)*(st->second-it->second)*1.0f/(st->first-it->first));
        }
    }
    return true;
}

bool Color::Write(const char *path) const
{
    if (_list.empty()) return false;

    FILE *file = fopen(path, "w");
    if (file == 0) {
        printf("[Color::Write] open LUT file %s failed\n", path);
        return false;
    }

    fprintf(file, "# volume color LUT data file\n");
    fprintf(file, "# created or edited by NeuronTracing\n");    
    fprintf(file, "# line: index value\n\n");
    for (map_t::const_iterator it = _list.begin(); it != _list.end(); ++it)
        fprintf(file, "%d %d\n", it->first, it->second);
    fclose(file);
    printf("[Color::Write] write LUT file %s ok\n", path);
    return true;
}

void Color::Draw() const
{
    if (_index == 0 || _value == 0 || _style == LUT_NONE) return;

    glPushMatrix();
    glScalef(1.0f/_width, 1.0f/_height, 0.0f);
    glTranslatef(-1.0f*_width, -1.0f*_height, 0.0f);
    glScalef(2.0f, 2.0f, 1.0f);
    glPushAttrib(GL_CURRENT_BIT | GL_POINT_BIT | GL_LINE_BIT);

    glColor3f(0.4f, 0.4f, 0.4f);
    glLineWidth(1.0f);
    glBegin(GL_LINE_LOOP);
    glVertex2i(0, 0);
    glVertex2i(_width, 0);
    glVertex2i(_width, _height);
    glVertex2i(0, _height);
    glEnd();

    glColor3f(0.5f, 0.5f, 0.5f);
    glLineWidth(2.0f);
    glBegin(GL_LINES);
    for (int i=0; i<256; ++i) {
        glVertex2f(i*_scalex, 0);
        glVertex2f(i*_scalex, _index[i]*_scaley);
    }
    glEnd();

    if (_style > LUT_BOUND) {
        glColor3f(0.8f, 0.8f, 0.8f);
        glPointSize(4.0f);
        glBegin(GL_POINTS);
        for (map_t::const_iterator it = _list.begin(); it != _list.end(); ++it)
            glVertex2f(it->first*_scalex, it->second*_scaley);
        glEnd();
        glColor3f(0.6f, 0.6f, 0.6f);
        glLineWidth(2.0f);
        glBegin(GL_LINE_STRIP);
        for (map_t::const_iterator it = _list.begin(); it != _list.end(); ++it)
            glVertex2f(it->first*_scalex, it->second*_scaley);
        glEnd();
    }
    glPopAttrib();
    glPopMatrix();
}

void Color::Show() const
{
    if (_list.empty()) return;

    printf("# statistics information of color map:\n");
    printf("#   width %d, height %d, scale x %.2f, scale y %.2f\n", _width, _height, _scalex, _scaley);
    printf("#   points count %d\n", _list.size());
}

void Color::SetIndex(const double *index)
{
    if (index == 0) return;

    if (_index == 0) _index = new unsigned char[256];
    double maxv = index[1];
    for (int i=2; i<255; ++i) if (index[i] > maxv) maxv = index[i];
    maxv = 255.0f/maxv;
    for (int i=1; i<255; ++i) _index[i] = (unsigned char)(index[i]*maxv+0.5f);
    _index[0] = _index[255] = 0;
}

void Color::AddPoint(unsigned char index, unsigned char value)
{
    _list[index] = value;
    map_t::const_iterator it = _list.find(index), pt = it, st = it;
    if (pt != _list.begin()) {
        --pt;
        for (int i=pt->first; i<it->first; ++i)
            _value[i] = pt->second + (unsigned char)((i-pt->first)*(it->second-pt->second)*1.0f/(it->first-pt->first));
    }
    ++st;
    if (st != _list.end()) {
        for (int i=it->first; i<st->first; ++i)
            _value[i] = it->second + (unsigned char)((i-it->first)*(st->second-it->second)*1.0f/(st->first-it->first));
    }
}

void Color::RemovePoint(unsigned char index, unsigned char value)
{    
    map_t::const_iterator it = _list.find(index);
    if (it == _list.begin() || it == _list.end() || abs(it->second-value) > 4) return;

    map_t::const_iterator pt = it, st = it;
    --pt;
    ++st;
    if (st != _list.end()) {
        for (int i=pt->first; i<st->first; ++i)
            _value[i] = pt->second + (unsigned char)((i-pt->first)*(st->second-pt->second)*1.0f/(st->first-pt->first));
        _list.erase(it);
    }
}

void Color::Clear()
{
    _list.clear();
    _list[0] = (unsigned char)0;
    _list[255] = (unsigned char)255;
    for (int i=0; i<256; ++i) _value[i] = (unsigned char)i;
}
