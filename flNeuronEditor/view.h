#pragma once

#include <string.h>
#include <FL/Fl.h>
#include <FL/Fl_Table_Row.h>
#include <FL/Fl_Gl_Window.h>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include "vision.h"

class Table : public Fl_Table_Row {
public:
    Table(int x, int y, int w, int h);
    ~Table() {}

    void SetTree(std::vector<Tree*> &trees);

protected:
    virtual void draw_cell(TableContext context, int r=0, int c=0, int x=0, int y=0, int w=0, int h=0);

private:
    struct Item {
        char _text[8][12];
        Item() { memset(_text, 0, 8*12*sizeof(char)); }
        const char *operator[](int i) const { return _text[i]; }
        char *operator[](int i) { return _text[i]; }
    };

    std::vector<Item> _body;
    Item _head;
};

class Canvas : public Fl_Gl_Window {
public:
    Canvas(int x, int y, int w, int h);
    ~Canvas() {}

    void SetPersp(bool b);
    void ResetView();
    void SaveScene(const char *path);

protected:
    virtual void draw();
    virtual int handle(int event);
    virtual void InitContext() {}
    virtual void SetProjection();
    virtual void DrawScene() {}
    virtual void ParsePaste(const char *text) {}
    virtual void SelectObject(int winx, int winy) {}
    virtual void ProcessKey(int key) {}

private:
    void MouseMove();
    float TrackProj(float x, float y, float radius=0.95f);

private:
    bool _persp;
    int _winx, _winy;
    glm::vec3 _trans;
    glm::quat _quat;
};

enum OP_MODE { OP_NONE, OP_APO, OP_SWC };

class View : public Canvas {
public:
    View(int x, int y, int w, int h);
    ~View();

    void FileLoad();
    void FileSave();
    void TreeSave();
    void SceneSave();

    int SetMode(int mode) { _opmode = mode; if (_opmode > OP_SWC) _opmode = OP_NONE; return _opmode; }
    bool SetMult(bool b) { _mult = b; return _mult; }
    bool SetSync(bool b) { _sync = b; return _sync; }
    void SelectAll(bool b);
    void SelectReverse();
    void RemoveSelect();
    void RemoveAll();
    void TreeRotate();
    void TreeSplit();
    void TreeLink();

    void VolumeFlip(bool b) { _volume->SetFlip(b); redraw(); }
    void VolumeBound(bool b) { _volume->SetBound(b); redraw(); }
    void VolumeStyle(VOL_STYLE style) { _volume->SetStyle(style); redraw(); }
    void SomaStyle(APO_STYLE style) { _soma->SetStyle(style); redraw(); }
    void TreeStyle(SWC_STYLE style);

    void VolumeThickness();
    void VolumeSample();
    void VolumeTranslate();
    void SomaPrune();
    void TreePrune();
    void TreeFixup();
    void ShowVolume() { if (_volume->IsValid()) _volume->Show(); }
    void ShowSoma() { if (_soma->IsValid()) _soma->Show(); }
    void ShowTree();
    void ShowTable(Table *table);

protected:
    virtual void InitContext();
    virtual void DrawScene();
    virtual void ParsePaste(const char *text);
    virtual void SelectObject(int winx, int winy);
    virtual void ProcessKey(int key);

private:
    Volume *_volume;
    Soma *_soma;
    std::vector<Tree*> _trees;
    int _tid, _ptid;
    int _opmode; // OP_MODE
    bool _mult, _sync;
};
