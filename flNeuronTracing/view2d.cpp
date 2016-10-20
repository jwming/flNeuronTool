#include "view.h"

#include <stdio.h>
#include <GL/glew.h>
#include <FL/filename.h>
#include <FL/Fl_Ask.h>

View2D::View2D(int x, int y, int w, int h)
    : Fl_Gl_Window(x, y, w, h),
    _view3d(0),
    _color(0), _mapping(0)
{
    mode(FL_RGB | GL_DOUBLE);
    when(FL_WHEN_RELEASE);
    end();  // no children widgets
}

void View2D::InsertPoint()
{
    fl_message("Please press mouse LEFT button at desired position.\n");
}

void View2D::RemovePoint()
{
    fl_message("Please press mouse MIDDLE button at desired position.\n");
}

void View2D::draw()
{
    if (!context_valid()) {
        glewInit();
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_BLEND);
    }

    if (!valid()) {
        glViewport(0, 0, w(), h());
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        gluOrtho2D(-1.0, 1.0, -1.0, 1.0);
        glMatrixMode(GL_MODELVIEW);
    }

    glClear(GL_COLOR_BUFFER_BIT);
    glLoadIdentity();
    _color->Draw();
}

int View2D::handle(int event)
{
    switch (event) {
    case FL_DND_ENTER:
    case FL_DND_DRAG:
    case FL_DND_RELEASE:
        return 1;  // accept subsequent paste event

    case FL_PASTE:
        ParsePaste(Fl::event_text());
        return 0;

    case FL_PUSH:
        if (Fl::event_button() == FL_LEFT_MOUSE) InsertPoint(Fl::event_x(), Fl::event_y());
        if (Fl::event_button() == FL_MIDDLE_MOUSE) RemovePoint(Fl::event_x(), Fl::event_y());
        return 0;

    default:
        return Fl_Gl_Window::handle(event);
    } 
}

void View2D::ParsePaste(const char *text)
{
    char path[256], ext[5];
    strcpy(path, text);
    strtok(path, "\r\n;,?");
    strcpy(ext, fl_filename_ext(path));
    _strlwr(ext);

    if (strcmp(ext, ".lut") == 0) {
        char str[256];
        sprintf(str, "flNeuronTool Tracing - loading %s ...", fl_filename_name(path));
        window()->label(str);
        _color->Read(path);
        _color->Show();
        redraw();
        _view3d->make_current();
        _volume->SetColor(_color->GetValue());
        _view3d->redraw();
        sprintf(str, "flNeuronTool Tracing - %s", fl_filename_name(path));
        window()->label(str);
        return;
    }

    fl_alert("Unknown file type. \nSupport color mapping (LUT) file.\n");
}

void View2D::InsertPoint(int winx, int winy)
{
    make_current();
    int view[4];
    double proj[16], model[16], obj[3];
    glGetIntegerv(GL_VIEWPORT, view);
    glGetDoublev(GL_PROJECTION_MATRIX, proj);
    glGetDoublev(GL_MODELVIEW_MATRIX, model);
    winy = view[3]-winy-1;
    gluUnProject(winx, winy, 0.0, model, proj, view, &obj[0], &obj[1], &obj[2]);
    int index = _color->GetIndex((float)obj[0]);
    int value = _color->GetValue((float)obj[1]);
    _view3d->make_current();
    _mapping->AddPoint(index, value);
    redraw();
    _view3d->redraw();
}

void View2D::RemovePoint(int winx, int winy)
{
    make_current();
    int view[4];
    double proj[16], model[16], obj[3];
    glGetIntegerv(GL_VIEWPORT, view);
    glGetDoublev(GL_PROJECTION_MATRIX, proj);
    glGetDoublev(GL_MODELVIEW_MATRIX, model);
    winy = view[3]-winy-1;
    gluUnProject(winx, winy, 0.0, model, proj, view, &obj[0], &obj[1], &obj[2]);
    int index = _color->GetIndex((float)obj[0]);
    int value = _color->GetValue((float)obj[1]);
    _view3d->make_current();
    _mapping->RemovePoint(index, value);
    redraw();
    _view3d->redraw();
}
