#include "view.h"

#include <stdio.h>
#include <GL/glew.h>
#include <jpeglib.h>

Canvas::Canvas(int x, int y, int w, int h)
    : Fl_Gl_Window(x, y, w, h),
    _persp(false),
    _winx(0), _winy(0),
    _trans(0.0f, 0.0f, 1.0f), _quat(1.0f, 0.0f, 0.0f, 0.0f)
{
    mode(FL_RGB | GL_DOUBLE | FL_DEPTH);
    when(FL_WHEN_RELEASE);
    end(); // no children widgets
}

void Canvas::SetPersp(bool b)
{
    if (b != _persp) {
        _persp = b;
        valid(0); // forced to reset projection
        redraw();
    }
}

void Canvas::ResetView()
{
    _winx = _winy = 0;
    _trans = glm::vec3(0.0f, 0.0f, 1.0f);
    _quat = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    redraw();
}

void Canvas::SaveScene(const char *path)
{
    FILE *file = fopen(path, "wb");
    if (file == 0) {
        printf("[Canvas::SaveScene] open JPEG file %s failed\n", path);
        return;
    }

    int w = this->w();
    int h = this->h();
    unsigned char *buf = new unsigned char[3*w*h];
    make_current();
    glReadBuffer(GL_BACK);
    glReadPixels(0, 0, w, h, GL_RGB, GL_UNSIGNED_BYTE, buf);

    jpeg_compress_struct jcs;
    jpeg_error_mgr err;
    jcs.err = jpeg_std_error(&err);
    jpeg_create_compress(&jcs);
    jpeg_stdio_dest(&jcs, file); 
    jcs.image_width = w;
    jcs.image_height = h;
    jcs.input_components = 3;
    jcs.in_color_space = JCS_RGB;
    jpeg_set_defaults(&jcs);
    jpeg_set_quality(&jcs, 100, TRUE);

    jpeg_start_compress(&jcs, TRUE);
    unsigned char *ptr = buf + 3*w*h;
    while (jcs.next_scanline < jcs.image_height) {
        ptr -= 3*w;
        jpeg_write_scanlines(&jcs, &ptr, 1);
    }
    jpeg_finish_compress(&jcs);

    jpeg_destroy_compress(&jcs);
    delete[] buf;
    fclose(file);
    printf("[Canvas::SaveScene] save scene to JPEG file %s ok\n", path);
}

void Canvas::draw()
{
    if (!context_valid()) {
        glewInit();
        printf("------------------------------------------------------------\n");
        printf("OpenGL Vendor: %s\n", glGetString(GL_VENDOR));
        printf("OpenGL Renderer: %s\n", glGetString(GL_RENDERER));
        printf("OpenGL Version: %s\n", glGetString(GL_VERSION));
        printf("------------------------------------------------------------\n");
        InitContext();
    }

    if (!valid()) {
        int s = w() < h() ? w() : h();
        glViewport((w()-s)/2, (h()-s)/2, s, s);
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        SetProjection();
        glMatrixMode(GL_MODELVIEW);
    }

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();
    //glScalef(1.0f, -1.0f, 1.0f);
    glTranslatef(_trans.x, _trans.y, 0.0f);
    glm::mat4 mt = glm::mat4_cast(_quat);
    glMultMatrixf(&mt[0][0]);    
    glScalef(_trans.z, _trans.z, _trans.z);
    DrawScene();
}

void Canvas::SetProjection()
{
    if (_persp) gluPerspective(27.5, 1.0, 0.1, 10.0);
    else glOrtho(-1.0, 1.0, -1.0, 1.0, 0.1, 10.0);
    glTranslatef(0.0f, 0.0f, -5.0f);
}

int Canvas::handle(int event)
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
        if (Fl::event_button() == FL_RIGHT_MOUSE) {
            SelectObject(Fl::event_x(), Fl::event_y());
            return 0;
        }
        _winx = Fl::event_x();
        _winy = Fl::event_y();
        return 1;  // accept subsequent mouse event

    case FL_DRAG:
        if (Fl::event_inside(this) == 0)    return 0;
        MouseMove();
        _winx = Fl::event_x();
        _winy = Fl::event_y();
        redraw();
        return 0;

    case FL_MOUSEWHEEL:
        _trans.z -= Fl::event_dy()/20.0f;
        _trans.z = glm::clamp(_trans.z, 0.25f, 10.0f);
        redraw();
        return 0;

    case FL_FOCUS:
    case FL_UNFOCUS:
        return 1;  // accept subsequent keyboard event

    case FL_KEYDOWN:
        ProcessKey(Fl::event_key());
        return 0;

    default:
        return Fl_Gl_Window::handle(event);
    }
}

void Canvas::MouseMove()
{
    if (Fl::event_button() == FL_LEFT_MOUSE) {
        glm::vec2 p1(2.0f*_winx/w()-1.0f, 1.0f-2.0f*_winy/h());
        glm::vec2 p2(2.0f*Fl::event_x()/w()-1.0f, 1.0f-2.0f*Fl::event_y()/h());
        glm::vec3 v1 = glm::normalize(glm::vec3(p1.x, p1.y, TrackProj(p1.x, p1.y)));
        glm::vec3 v2 = glm::normalize(glm::vec3(p2.x, p2.y, TrackProj(p2.x, p2.y)));
        glm::vec3 axis = glm::normalize(glm::cross(v1, v2));
        float angle = glm::acos(glm::dot(v1, v2))/2.0f;       
        _quat = glm::quat(glm::cos(angle), glm::sin(angle)*axis) * _quat;  // left-multiple
        return;
    }

    if (Fl::event_button() == FL_MIDDLE_MOUSE) {
        _trans.x += 2.0f*(Fl::event_x()-_winx)/w();
        _trans.y += 2.0f*(_winy-Fl::event_y())/h();
        return;
    }
}

float Canvas::TrackProj(float x, float y, float radius)
{
    float d = x*x + y*y;
    float r = radius*radius;
    float t = r/2.0f;
    if (d > t) return t/glm::sqrt(d);
    return glm::sqrt(r-d);
}
