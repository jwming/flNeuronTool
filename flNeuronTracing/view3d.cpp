#include "view.h"

#include <stdio.h>
#include <GL/glew.h>
#include <tiffio.h>
#include <FL/filename.h>
#include <FL/Fl_Ask.h>
#include <FL/Fl_Native_File_Chooser.h>

View3D::View3D(int x, int y, int w, int h)
    : Canvas(x, y, w, h),
    _volume(0), _color(0), _soma(0), _tree(0),
    _mapping(0), _probing(0), _tracing(0),
    _opmode(OP_NONE)
{
    end(); // no children widgets
}

void View3D::InitContext()
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

void View3D::DrawScene() const
{
    if (_volume->IsValid()) _volume->Draw();
    if (_soma->IsValid())   _soma->Draw();
    if (_tree->IsValid())   _tree->Draw();
}

void View3D::ParsePaste(const char *text)
{
    char path[256], ext[5];
    strcpy(path, text);
    strtok(path, "\r\n;,?");
    strcpy(ext, fl_filename_ext(path));
    _strlwr(ext);

    if (strcmp(ext, ".tif") == 0) {
        char str[256];
        sprintf(str, "flNeuronTool Tracing - loading %s ...", fl_filename_name(path));
        window()->label(str);
        _volume->Read(path);        
        _volume->Show();
        make_current();
        _mapping->SetParam();
        _probing->SetParam();
        _tracing->SetParam();
        redraw();
        sprintf(str, "flNeuronTool Tracing - %s", fl_filename_name(path));
        window()->label(str);
        return;
    }

    if (strcmp(ext, ".apo") == 0) {
        if (!_volume->IsValid()) {
            fl_alert("Please load a volume (TIFF) file first.\n");
            return;
        }
        _soma->Read(path);
        _soma->Show();
        redraw();
        return;
    }

    if (strcmp(ext, ".swc") == 0) {
        if (!_volume->IsValid()) {
            fl_alert("Please load a volume (TIFF) file first.\n");
            return;
        }
        _tree->Read(path);
        _tree->Show();
        redraw();
        return;
    }

    fl_alert("Unknown file type. \nSupport volume (TIFF), soma (APO), tree (SWC) file.\n");
}

void View3D::SelectObject(int winx, int winy)
{
    if (!_volume->IsValid()) return;

    int view[4];
    double proj[16], model[16], obj[3];
    make_current();
    glGetIntegerv(GL_VIEWPORT, view);
    glGetDoublev(GL_PROJECTION_MATRIX, proj);
    glGetDoublev(GL_MODELVIEW_MATRIX, model);
    winy = view[3]-winy-1;
    gluUnProject(winx, winy, 0.0, model, proj, view, &obj[0], &obj[1], &obj[2]);
    Point point0 = _volume->GetPoint((float)obj[0], (float)obj[1], (float)obj[2]);
    gluUnProject(winx, winy, 1.0, model, proj, view, &obj[0], &obj[1], &obj[2]);
    Point point1 = _volume->GetPoint((float)obj[0], (float)obj[1], (float)obj[2]);
    Point point = _volume->GetPoint(point0, point1);
    printf("[View3D::SelectObject] volume position (%.1f, %.1f, %.1f), value %.1f\n", point.X, point.Y, point.Z, point.Value);
}

void View3D::EditObject(int winx, int winy)
{
    if (!_volume->IsValid() || _opmode == OP_NONE) return;

    make_current();
    int view[4];
    double proj[16], model[16], obj[3];
    glGetIntegerv(GL_VIEWPORT, view);
    glGetDoublev(GL_PROJECTION_MATRIX, proj);
    glGetDoublev(GL_MODELVIEW_MATRIX, model);
    winy = view[3]-winy-1;
    gluUnProject(winx, winy, 0.0, model, proj, view, &obj[0], &obj[1], &obj[2]);
    Point point0 = _volume->GetPoint((float)obj[0], (float)obj[1], (float)obj[2]);
    gluUnProject(winx, winy, 1.0, model, proj, view, &obj[0], &obj[1], &obj[2]);
    Point point1 = _volume->GetPoint((float)obj[0], (float)obj[1], (float)obj[2]);
    Point point = _volume->GetPoint(point0, point1);
    switch (_opmode) {
        case OP_PROBING:
            _probing->AddPoint(point);
            //_probing->Update();
            break;
        case OP_TRACING:
            _tracing->AddSeed(point);
            //_tracing->Update();
            _tracing->BeginUpdate();
            break;
    }
    redraw();
}

void View3D::ProcessKey(int key)
{
    switch (key) {
    case 'v': case 'V':
        if (_volume->IsValid()) {
            _volume->SetStyle(_volume->GetStyle()+1);
            redraw();
        }
        break;

    case 'a': case 'A':
        if (_soma->IsValid()) {
            _soma->SetStyle(_soma->GetStyle()+1);
            redraw();
        }
        break;

    case 't': case 'T':
        if (_tree->IsValid()) {
            _tree->SetStyle(_tree->GetStyle()+1);
            redraw();
        }
        break;

    case 'b': case 'B':
        if (_volume->IsValid()) {
            _volume->SetBound(!_volume->GetBound());
            redraw();
        }
        break;

    case FL_Home:
        ResetView();
        break;
    }
}

void View3D::ProcessFresh()
{
    if (_probing->IsDoing() || _tracing->IsDoing()) {
        redraw();
        Fl::repeat_timeout(0.1, Canvas::FreshThread, this);
    }
    else {
        redraw();
        Fl::repeat_timeout(0.5, Canvas::FreshThread, this);
    }
}
