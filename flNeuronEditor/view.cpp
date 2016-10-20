#include "view.h"

#include <stdio.h>
#include <GL/glew.h>
#include <FL/filename.h>
#include <FL/Fl_Ask.h>
#include <FL/Fl_Native_File_Chooser.h>

View::View(int x, int y, int w, int h)
    : Canvas(x, y, w, h),
    _volume(new Volume()), _soma(new Soma()), _trees(0),
    _tid(-1), _ptid(-1),
    _opmode(OP_NONE),
    _mult(false), _sync(true)
{
    _volume->SetExtent(512, 512, 512, 1.0f);
    _soma->SetExtent(_volume->GetWidth(), _volume->GetHeight(), _volume->GetDepth(), _volume->GetThickness());
    end(); // no children widgets
}

View::~View()
{
    if (_volume != 0) delete _volume;
    if (_soma != 0)   delete _soma;
    for (size_t i=0; i<_trees.size(); ++i) delete _trees[i];
}

void View::InitContext()
{
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClearDepth(1.0f);
    glDisable(GL_TEXTURE_1D);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_TEXTURE_3D);
    glDisable(GL_TEXTURE_GEN_S);
    glDisable(GL_TEXTURE_GEN_T);
    glDisable(GL_TEXTURE_GEN_R);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glDisable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
    glCullFace(GL_BACK);
    glDisable(GL_CULL_FACE);
    glDisable(GL_NORMALIZE);
    glDisable(GL_LIGHTING);
    GLfloat posi0[4] = { 1.0f, 1.0f, 1.0f, 0.0f };
    GLfloat diff0[4] = { 0.9f, 0.9f, 0.9f, 0.0f };
    glLightfv(GL_LIGHT0, GL_POSITION, posi0);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, diff0);
    glEnable(GL_LIGHT0);
    GLfloat posi1[4] = { -1.0f, 1.0f, 1.0f, 0.0f };
    GLfloat diff1[4] = { 0.7f, 0.7f, 0.7f, 0.0f };
    glLightfv(GL_LIGHT1, GL_POSITION, posi1);
    glLightfv(GL_LIGHT1, GL_DIFFUSE, diff1);
    glEnable(GL_LIGHT1);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    glEnable(GL_COLOR_MATERIAL);
}

void View::DrawScene()
{
    //glScalef(1.0f, -1.0f, 1.0f);

    if (_volume->IsValid()) _volume->Draw();
    if (_soma->IsValid())   _soma->Draw();

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

    for (size_t i=0; i<_trees.size(); ++i) {
        if (!_trees[i]->GetCur()) glColor3ubv(color[i%8]);
        _trees[i]->Draw();
    }

    //glScalef(1.0f, -1.0f, 1.0f);
}

void View::ParsePaste(const char *text)
{
    char path[256], ext[5];
    strcpy(path, text);
    strtok(path, "\r\n;,?");
    strcpy(ext, fl_filename_ext(path));
    _strlwr(ext);

    if (strcmp(ext, ".tif") == 0) {
        char str[256];
        sprintf(str, "flNeuronTool Editor - loading %s ...", fl_filename_name(path));
        window()->label(str);
        _volume->SetThickness(1.0f);
        _volume->SetTranslate(0, 0, 0);
        _volume->Read(path);
        _soma->SetExtent(_volume->GetWidth(), _volume->GetHeight(), _volume->GetDepth(), _volume->GetThickness());
        for (size_t i=0; i<_trees.size(); ++i)
            _trees[i]->SetExtent(_volume->GetWidth(), _volume->GetHeight(), _volume->GetDepth(), _volume->GetThickness());
        _volume->Show();
        redraw();
        sprintf(str, "flNeuronTool Editor - %s", fl_filename_name(path));
        window()->label(str);
        return;
    }

    if (strcmp(ext, ".apo") == 0) {
        if (!_volume->IsValid()) {
            fl_message("Please load a volume (TIFF) file first.\n");
            return;
        }
        _soma->Read(path);
        redraw();
        return;
    }

    if (strcmp(ext, ".swc") == 0) {
        if (!_volume->IsValid()) {
            fl_message("Please load a volume (TIFF) file first.\n");
            return;
        }
        Tree tree;
        tree.SetExtent(_volume->GetWidth(), _volume->GetHeight(), _volume->GetDepth(), _volume->GetThickness());
        tree.Read(path);
        size_t len = _trees.size();
        tree.Resolve(_trees);
        for (size_t i=len; i<_trees.size(); ++i)
            _trees[i]->Taging();
        redraw();
        return;
    }

    fl_alert("Unknown file type. \nSupport volume (TIFF), soma (APO), tree (SWC) file.\n");
}

void View::SelectObject(int winx, int winy)
{
    if (_opmode == OP_NONE) return;
    if (_opmode == OP_APO && !_soma->IsValid()) return;
    if (_opmode == OP_SWC && _trees.empty()) return;

    int view[4];
    glGetIntegerv(GL_VIEWPORT, view);
    unsigned int buffer[256];
    glSelectBuffer(256, buffer);
    glRenderMode(GL_SELECT);
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluPickMatrix(winx, view[3]-winy-1, 16.0, 16.0, view);
    SetProjection();
    glMatrixMode(GL_MODELVIEW);
    if (_opmode == OP_APO && _soma->IsValid()) {
        _soma->DrawId();
        glFlush();

        int h = glRenderMode(GL_RENDER);
        if (h > 0  && *buffer > 0) {
            size_t t = buffer[3];
            if (_mult) _soma->SetId(t, true);
            else _soma->SetId(t, !_soma->GetId(t));
        }
    }

    if (_opmode == OP_SWC && !_trees.empty()) {
        for (size_t i=0; i<_trees.size(); ++i) {
            glPushName(i);
            _trees[i]->DrawId();
            glPopName();
        }
        glFlush();

        int h = glRenderMode(GL_RENDER);
        if (h > 0  && *buffer > 0) {
            size_t t = buffer[3];
            if (_mult) _trees[t]->SetCur(true);
            else _trees[t]->SetCur(!_trees[t]->GetCur());
            if (_trees[t]->GetCur()) {
                _ptid = _tid;
                _tid = (int)t;
                if (h > 1 && buffer[4] < _trees[_tid]->GetSize()) _trees[_tid]->SetId(buffer[4]);
            }
            else _tid = -1;
        } 
    }

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    redraw();
}

void View::ProcessKey(int key)
{
    if (_opmode != OP_SWC || _trees.empty()) return;
    if (_tid < 0 || _tid >= (int)_trees.size()) _tid = -1;

    switch (key) {
    case FL_Page_Down:
        if (_tid != -1) _trees[_tid]->SetCur(false);
        _ptid = _tid;
        _tid = (++_tid) % _trees.size();
        _trees[_tid]->SetCur(true);
        redraw();
        break;

    case FL_Page_Up:
        if (_tid != -1) _trees[_tid]->SetCur(false);
        _ptid = _tid;
        _tid = (--_tid + _trees.size()) % _trees.size();
        _trees[_tid]->SetCur(true);
        redraw();
        break;

    case FL_Left:
        if (_tid == -1) return;
        _trees[_tid]->SetCur(true);
        _trees[_tid]->PrevId();
        redraw();
        break;

    case FL_Right:
        if (_tid == -1) return;
        _trees[_tid]->SetCur(true);
        _trees[_tid]->NextId();
        redraw();
        break;

    case FL_Up:
        if (_tid == -1) return;
        _trees[_tid]->SetCur(true);
        _trees[_tid]->UpId();
        redraw();
        break;

    case FL_Down:
        if (_tid == -1) return;
        _trees[_tid]->SetCur(true);
        _trees[_tid]->DownId();
        redraw();
        break;

    case FL_Home:
        if (_tid == -1) return;
        _trees[_tid]->SetCur(true);
        _trees[_tid]->SetId(0);
        redraw();
        break;

    case FL_End:
        if (_tid == -1) return;
        _trees[_tid]->SetCur(true);
        _trees[_tid]->SetId(_trees[_tid]->GetSize()-1);
        redraw();
        break;
    }
}

void View::FileLoad()
{
    Fl_Native_File_Chooser fc;
    fc.title("Load File");
    fc.type(Fl_Native_File_Chooser::BROWSE_FILE);
    fc.filter("Volume File (*.tif)\t*.{tif}\nSoma File (*.apo)\t*.{apo}\nTree File (*.swc)\t*.{swc}\n");
    if (fc.show() == 0) {
        char path[256], ext[5];
        strcpy(path, fc.filename());
        strcpy(ext, fl_filename_ext(path));
        _strlwr(ext);
        if (strcmp(ext, ".tif") == 0) {
            char str[256];
            sprintf(str, "flNeuronTool Editor - loading %s ...", fl_filename_name(path));
            window()->label(str);
            _volume->SetThickness(1.0f);
            _volume->SetTranslate(0, 0, 0);
            _volume->Read(path);        
            _soma->SetExtent(_volume->GetWidth(), _volume->GetHeight(), _volume->GetDepth(), _volume->GetThickness());
            for (size_t i=0; i<_trees.size(); ++i)
                _trees[i]->SetExtent(_volume->GetWidth(), _volume->GetHeight(), _volume->GetDepth(), _volume->GetThickness());
            _volume->Show();
            redraw();
            sprintf(str, "flNeuronTool Editor - %s", fl_filename_name(path));
            window()->label(str);
            return;
        }

        if (strcmp(ext, ".apo") == 0) {
            if (!_volume->IsValid()) {
                fl_message("Please load a volume (TIFF) file first.\n");
                return;
            }
            _soma->Read(path);
            redraw();
            return;
        }

        if (strcmp(ext, ".swc") == 0) {
            if (!_volume->IsValid()) {
                fl_message("Please load a volume (TIFF) file first.\n");
                return;
            }
            Tree tree;
            tree.SetExtent(_volume->GetWidth(), _volume->GetHeight(), _volume->GetDepth(), _volume->GetThickness());
            tree.Read(path);
            size_t len = _trees.size();
            tree.Resolve(_trees);
            for (size_t i=len; i<_trees.size(); ++i)
                _trees[i]->Taging();
            redraw();
            return;
        }

        fl_alert("Unknown file type. \nSupport volume (TIFF), soma (APO), tree (SWC) file.\n");
    }
}

void View::FileSave()
{
    if (!_volume->IsValid()) {
        fl_alert("No data to save.\n");
        return;
    }

    Fl_Native_File_Chooser fc;
    fc.title("Save File");
    fc.type(Fl_Native_File_Chooser::BROWSE_SAVE_FILE);
    fc.options(fc.options() | Fl_Native_File_Chooser::SAVEAS_CONFIRM);
    fc.filter("Volume File (*.tif)\t*.{tif}\nSoma File (*.apo)\t*.{apo}\nTree File (*.swc)\t*.{swc}\n");
    if (fc.show() == 0) {
        char path[256];
        strcpy(path, fc.filename());
        int fit = fc.filter_value();
        if (fit == 0) {
            strcat(path, ".tif");
            _volume->Write(path);
            return;
        }
        if (fit == 1) {
            if (!_soma->IsValid()) {
                fl_alert("No data to save.\n");
                return;
            }
            strcat(path, ".apo");
            _soma->Write(path);
            return;
        }
        if (fit == 2) {
            if (_trees.empty()) {
                fl_alert("No data to save.\n");
                return;
            }
            Tree tree;
            tree.SetExtent(_volume->GetWidth(), _volume->GetHeight(), _volume->GetDepth(), _volume->GetThickness());
            tree.Merge(_trees);
            strcat(path, ".swc");
            tree.Write(path);
            return;
        }
        fl_alert("Unknown file type. Support volume (TIFF), soma (APO), tree (SWC) file.\n");
    }
}

void View::TreeSave()
{
    if (!_volume->IsValid() || _trees.empty()) {
        fl_alert("No data to save.\n");
        return;
    }

    Tree tree;
    tree.SetExtent(_volume->GetWidth(), _volume->GetHeight(), _volume->GetDepth(), _volume->GetThickness());
    for (size_t i=0; i<_trees.size(); ++i) {
        if (_trees[i]->GetCur()) tree.Merge(*_trees[i]);
    }
    if (tree.GetSize() == 0) {
        fl_alert("No selected data to save.\nPlease select one or more trees first.\n");
        return;
    }

    Fl_Native_File_Chooser fc;
    fc.filter("SWC File\t*.swc\n");
    fc.title("Save File");
    fc.type(Fl_Native_File_Chooser::BROWSE_SAVE_FILE);
    if (fc.show() == 0) {
        char path[256];
        strcpy(path, fc.filename());
        strcat(path, ".swc");
        tree.Write(path);
    }
}

void View::SceneSave()
{
    if (!_volume->IsValid()) {
        fl_alert("No data to save.\n");
        return;
    }

    Fl_Native_File_Chooser fc;
    fc.title("Save File");
    fc.type(Fl_Native_File_Chooser::BROWSE_SAVE_FILE);
    fc.options(fc.options() | Fl_Native_File_Chooser::SAVEAS_CONFIRM);
    fc.filter("JPEG File\t*.jpg\n");
    if (fc.show() == 0) {
        char path[256];
        strcpy(path, fc.filename());
        strcat(path, ".jpg");
        SaveScene(path);
    }
}

void View::SelectAll(bool b)
{
    if (_opmode == OP_NONE) return;

    if (_opmode == OP_APO && _soma->IsValid()) {
        _soma->SelectAll(b);
        redraw();
        return;
    }

    if (_opmode == OP_SWC && !_trees.empty()) {
        for (size_t i=0; i<_trees.size(); ++i)
            _trees[i]->SetCur(b);
        if (!b) _ptid = _tid = -1;
        redraw();
        return;
    }
}

void View::SelectReverse()
{
    if (_opmode == OP_NONE) return;

    if (_opmode == OP_APO && _soma->IsValid()) {
        _soma->SelectReverse();
        redraw();
        return;
    }

    if (_opmode == OP_SWC && !_trees.empty()) {
        for (size_t i=0; i<_trees.size(); ++i)
            _trees[i]->SetCur(!_trees[i]->GetCur());
        _ptid = _tid = -1;
        redraw();
        return;
    }
}

void View::RemoveSelect()
{
    if (_opmode == OP_NONE) return;

    if (_opmode == OP_APO && _soma->IsValid()) {
        _soma->Remove();
        redraw();
        return;
    }

    if (_opmode == OP_SWC && !_trees.empty()) {
        Tree::Trees trees(_trees.begin(), _trees.end());
        _trees.clear();
        for (size_t i=0; i<trees.size(); ++i) {
            if (trees[i]->GetCur()) delete trees[i];
            else _trees.push_back(trees[i]);
        }
        redraw();
        return;
    }
}

void View::RemoveAll()
{
    if (_opmode == OP_NONE) return;

    if (_opmode == OP_APO && _soma->GetSize() > 0) {
        _soma->Clear();
        redraw();
        return;
    }

    if (_opmode == OP_SWC && !_trees.empty()) {
        for (size_t i=0; i<_trees.size(); ++i)
            if (_trees[i]->GetCur()) delete _trees[i];
        _trees.clear();
        redraw();
        return;
    }
}

void View::TreeRotate()
{
    if (_tid < 0 || _tid >= (int)_trees.size()) {
        _tid = -1;
        fl_alert("No available tree data.\n");
        return;
    }

    if (_trees[_tid]->GetId() != 0) {
        _trees[_tid]->Rotate();
        _trees[_tid]->Taging();
        redraw();
    }
}

void View::TreeSplit()
{
    if (_tid < 0 || _tid >= (int)_trees.size()) {
        _tid = -1;
        fl_alert("No available tree data.\n");
        return;
    }

    Tree *tree = new Tree();
    _trees[_tid]->Split(*tree);
    if (tree->GetSize() == 0) {
        delete tree;
        fl_alert("No tree data was split.\n");
        return;
    }

    _trees[_tid]->Taging();
    tree->Taging();
    _trees.push_back(tree);
    tree->SetCur(true);
    redraw();
}

void View::TreeLink()
{
    if (_tid < 0 || _tid >= (int)_trees.size() || _ptid < 0 || _ptid >= (int)_trees.size() || _ptid == _tid) {
        fl_alert("Please select TWO traces to link. The FIRST tree is main trunk.\n");
        return;
    }

    _trees[_ptid]->Link(*_trees[_tid]);
    _trees[_ptid]->Taging();
    delete _trees[_tid];
    _trees.erase(_trees.begin()+_tid);
    _tid = _ptid;
    _ptid = -1;
    redraw();
}

void View::TreeStyle(SWC_STYLE style)
{
    if (_trees.empty()) return;

    for (size_t i=0; i<_trees.size(); ++i)
        if (_sync || _trees[i]->GetCur()) _trees[i]->SetStyle(style);
    redraw();
}

void View::VolumeThickness()
{
    char value[8];
    float t = _volume->GetThickness();
    sprintf(value, "%.2f", t);
    const char *s = fl_input("Set volume slice thickness (z value in voxel). Current value is %.2f.\n", value, t);
    if (s != 0) {
        float t1 = (float)atof(s);
        if (t1 != t) {
            _volume->SetThickness(t1);
            _soma->SetExtent(_volume->GetWidth(), _volume->GetHeight(), _volume->GetDepth(), _volume->GetThickness());
            for (size_t i=0; i<_trees.size(); ++i)
                _trees[i]->SetExtent(_volume->GetWidth(), _volume->GetHeight(), _volume->GetDepth(), _volume->GetThickness());
            redraw();
        }
    }
}

void View::VolumeSample()
{
    if (!_volume->IsValid()) {
        fl_alert("No available volume data.\n");
        return;
    }

    const char *s = fl_input("Set volume rendering sampling rate (value in voxel). Default is 1.\n", "1");
    if (s != 0) {    
        int level = atoi(s);
        _volume->SetSample(level);
        redraw();
    }
}

void View::VolumeTranslate()
{
    if (!_volume->IsValid()) {
        fl_alert("No available volume data.\n");
        return;
    }

    const char *s = fl_input("Set volume translate position (x y z value in voxel).\n", "0 0 0");
    if (s != 0) {
        int x, y, z;
        sscanf(s, "%d %d %d", &x, &y, &z);
        _volume->SetTranslate(x, y, z);
        redraw();
    }
}

void View::SomaPrune()
{
    if (!_soma->IsValid()) return;

    const char *s = fl_input("Set small soma radius lower limit.\n", "0.5");
    if (s != 0) {
        float radius;
        sscanf(s, "%f", &radius);
        printf("[View::SomaPrune] prune small soma to %d cells\n", _soma->PruneSmall(radius));
        redraw();
    }
}

void View::TreePrune()
{
    if (_trees.empty()) return;

    const char *s = fl_input("Set short tree length lower limit (length value in node)\n", "5");
    if (s != 0) {
        size_t length;
        sscanf(s, "%d", &length);
        Tree::Trees trees(_trees.begin(), _trees.end());
        _trees.clear();
        for (size_t i=0; i<trees.size(); ++i) {
            if (trees[i]->GetSize() < length) delete trees[i];
            else _trees.push_back(trees[i]);
        }
        printf("[View::TreePrune] fixup short trees to %d trees\n", _trees.size());
        redraw();
    }
}

void View::TreeFixup()
{
    if (_trees.empty()) return;

    const char *s = fl_input("Set tree radius lower and upper limit (length value in voxel).\n", "0.25 25.0");
    if (s != 0) {
        float lower, upper;
        sscanf(s, "%f %f", &lower, &upper);
        for (size_t i=0; i<_trees.size(); ++i)
            _trees[i]->FixupRadius(lower, upper);
        redraw();
    }
}

void View::ShowTree()
{
    if (_trees.empty()) return;

    for (size_t i=0; i<_trees.size(); ++i) _trees[i]->Show();
}

void View::ShowTable(Table *table)
{
    if (!_trees.empty()) {
        table->SetTree(_trees);
        table->show();
    }
}
