#pragma once

#include <FL/Fl.h>
#include <FL/Fl_Gl_Window.h>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include "vision.h"
#include "filter.h"

class Canvas : public Fl_Gl_Window {
public:
    Canvas(int x, int y, int w, int h);
    ~Canvas() {}

    void SetPersp(bool b);
    void ResetView();
    void SaveScene(const char *path);
    void SetSelect(bool b) { _select = b; }
    void SetFresh(bool b);
    static void FreshThread(void *data) { ((Canvas*)data)->ProcessFresh(); }

protected:
    virtual void draw();
    virtual int handle(int event);
    virtual void InitContext() {}
    virtual void SetProjection();
    virtual void DrawScene() const {}
    virtual void ParsePaste(const char *text) {}
    virtual void SelectObject(int winx, int winy) {}
    virtual void EditObject(int winx, int winy) {}
    virtual void ProcessKey(int key) {}
    virtual void ProcessFresh() {}

private:
    void MouseMove();
    float TrackProj(float x, float y, float radius=0.95f);

private:
    bool _persp;
    int _winx, _winy;
    glm::vec3 _trans;
    glm::quat _quat;
    bool _select;
};

class View3D : public Canvas {
public:
    View3D(int x, int y, int w, int h);
    ~View3D() {}

    void SetVolume(Volume *volume) { _volume = volume; }
    void SetColor(Color *color) { _color = color; }
    void SetSoma(Soma *soma) { _soma = soma; }
    void SetTree(Tree *tree) { _tree = tree; }
    void SetMappping(Mapping *mapping) { _mapping = mapping; }
    void SetProbing(Probing *probing) { _probing = probing; }
    void SetTracing(Tracing *tracing) { _tracing = tracing; }
    void SetMode(int mode) { _opmode = mode; if (_opmode > OP_TRACING) _opmode = OP_NONE; }

protected:
    virtual void InitContext();
    virtual void DrawScene() const;
    virtual void ParsePaste(const char *text);
    virtual void SelectObject(int winx, int winy);
    virtual void EditObject(int winx, int winy);
    virtual void ProcessKey(int key);
    virtual void ProcessFresh();

private:
    Volume *_volume;
    Color *_color;
    Soma *_soma;
    Tree *_tree;
    Mapping *_mapping;
    Probing *_probing;
    Tracing *_tracing;
    int _opmode; // OP_MODE
};

class View2D : public Fl_Gl_Window {
public:
    View2D(int x, int y, int w, int h);
    ~View2D() {}

    void SetView(View3D *view) { _view3d = view; }
    void SetVolume(Volume *volume) { _volume = volume; }
    void SetColor(Color *color) { _color = color; _color->SetExtend(w(), h()); }
    void SetMapping(Mapping *mapping) { _mapping = mapping; }
    void InsertPoint();
    void RemovePoint();

protected:
    virtual void draw();
    virtual int handle(int event);
    virtual void ParsePaste(const char *text);
    virtual void InsertPoint(int winx, int winy);
    virtual void RemovePoint(int winx, int winy);

private:
    View3D *_view3d;
    Volume *_volume;
    Color *_color;
    Mapping *_mapping;
};
