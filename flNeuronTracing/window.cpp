#include "window.h"

#include <stdio.h>
#include <process.h>
#include <FL/Fl_Ask.h>
#include <FL/filename.h>
#include <FL/Fl_Native_File_Chooser.h>

Window::Window(int w, int h, const char *label)
    : Fl_Window(w, h, label),
    _volume(new Volume()), _color(new Color()), _soma(new Soma()), _tree(new Tree()),
    _mapping(new Mapping()), _probing(new Probing()), _tracing(new Tracing()),
    _opmode(OP_NONE),
    _ids(32, 0),
    _menu3d(new Fl_Menu_Bar(0, 0, w, 25)),
    _view3d(new View3D(0, 25, w, h-25)),
    _dialog(new Fl_Window(512, 256, label)),
    _menu2d(new Fl_Menu_Button(0, 0, 512, 256)),
    _view2d(new View2D(0, 0, 512, 256))
{
    _mapping->SetVision(_volume, _color);
    _probing->SetVision(_volume, _soma);
    _tracing->SetVision(_volume, _tree);

    _volume->SetBound(true);
    _volume->SetStyle(VOL_MIP);
    _color->SetStyle(LUT_LINE);
    _soma->SetStyle(APO_POINT);
    _soma->SetMerge(false);
    _tree->SetStyle(SWC_LINE);
    _tree->SetLink(false);
    _mapping->SetRemove(false);
    _probing->SetLocal(false);
    _tracing->SetLocal(false);
    _view3d->SetPersp(false);
    _view3d->SetSelect(true);
    _view3d->SetFresh(true);
    _view3d->SetMode(OP_NONE);

    _menu3d->align(FL_ALIGN_TOP | FL_ALIGN_INSIDE);
    _menu3d->add("&File/Load File\t", FL_COMMAND+'o', FileLoad, (void*)this);
    _menu3d->add("&File/Save File\t", FL_COMMAND+'s', FileSave, (void*)this, FL_MENU_DIVIDER);
    _menu3d->add("&File/Save Scene\t", 0, SceneSave, (void*)this, FL_MENU_DIVIDER);
    _menu3d->add("&File/Quit", 0, Quit);

    _menu3d->add("&Edit/Edit Mode/Edit None\t", FL_COMMAND+(FL_F+1), EditNone, (void*)this, FL_MENU_RADIO | FL_MENU_VALUE);
    _menu3d->add("&Edit/Edit Mode/Soma Probing\t", FL_COMMAND+(FL_F+2), EditSoma, (void*)this, FL_MENU_RADIO);
    _menu3d->add("&Edit/Edit Mode/Tree Tracing\t", FL_COMMAND+(FL_F+3), EditTree, (void*)this, FL_MENU_RADIO | FL_MENU_DIVIDER);
    _menu3d->add("&Edit/Enable Volume Select\t", 0, EditSelect, (void*)this, FL_MENU_TOGGLE | FL_MENU_VALUE);
    _menu3d->add("&Edit/Auto Fresh Progress\t", 0, EditFresh, (void*)this, FL_MENU_TOGGLE | FL_MENU_VALUE | FL_MENU_DIVIDER);
    _ids[0] = _menu3d->add("&Edit/Probing Options/Set Global Parameters\t", 0, SomaParam, (void*)this);
    _ids[1] = _menu3d->add("&Edit/Probing Options/Using Local Parameters\t", 0, SomaLocal, (void*)this, FL_MENU_TOGGLE);
    _ids[2] = _menu3d->add("&Edit/Probing Options/Merge Overlap Soma\t", 0, SomaMerge, (void*)this, FL_MENU_TOGGLE);
    _ids[3] = _menu3d->add("&Edit/Update Probing\t", FL_COMMAND+'u', SomaUpdate, (void*)this);
    _ids[4] = _menu3d->add("&Edit/Cancel Probing\t", FL_COMMAND+'e', SomaCancel, (void*)this);
    _ids[5] = _menu3d->add("&Edit/Remove Last Soma\t", FL_Delete, SomaRemove, (void*)this);
    _ids[6] = _menu3d->add("&Edit/Clear Soma\t", FL_SHIFT+FL_Delete, SomaClear, (void*)this);
    _ids[7] = _menu3d->add("&Edit/Reduce Soma\t", FL_COMMAND+'r', SomaReduce, (void*)this);
    _ids[8] = _menu3d->add("&Edit/Prune Small Soma\t", 0, SomaPrune, (void*)this, FL_MENU_DIVIDER);
    _ids[9] = _menu3d->add("&Edit/Tracing Options/Set Sampling Parameters\t", 0, TreeSampling, (void*)this);    
    _ids[10] = _menu3d->add("&Edit/Tracing Options/Set Global Parameters\t", 0, TreeParam, (void*)this);
    _ids[11] = _menu3d->add("&Edit/Tracing Options/Using Local Parameters\t", 0, TreeLocal, (void*)this, FL_MENU_TOGGLE);
    _ids[12] = _menu3d->add("&Edit/Tracing Options/Link Gap Tree\t", 0, TreeLink, (void*)this, FL_MENU_TOGGLE);
    _ids[13] = _menu3d->add("&Edit/Update Tracing\t", 0, TreeUpdate, (void*)this);
    _ids[14] = _menu3d->add("&Edit/Cancel Tracing\t", FL_COMMAND+'e', TreeCancel, (void*)this);
    _ids[15] = _menu3d->add("&Edit/Remove Last Tree\t", FL_Delete, TreeRemove, (void*)this);
    _ids[16] = _menu3d->add("&Edit/Clear Tree\t", FL_SHIFT+FL_Delete, TreeClear, (void*)this);
    _ids[17] = _menu3d->add("&Edit/Reduce Tree\t", FL_COMMAND+'r', TreeReduce, (void*)this);    
    _ids[18] = _menu3d->add("&Edit/Prune Short Tree\t", 0, TreePrune, (void*)this);
    _ids[19] = _menu3d->add("&Edit/Stretch Tree\t", 0, TreeStretch, (void*)this);
    _ids[20] = _menu3d->add("&Edit/Fixup Thin Tree\t", 0, TreeFixup, (void*)this);

    for (int i=0; i<21; ++i) _menu3d->mode(_ids[i], _menu3d->mode(_ids[i]) | FL_MENU_INACTIVE);

    _menu3d->add("&View/Perspective View\t", 0, ViewPersp, (void*)this, FL_MENU_TOGGLE | FL_MENU_DIVIDER);
    _menu3d->add("&View/Volume Style/Volume None\t", 0, VolumeNone, (void*)this, FL_MENU_RADIO);
    _menu3d->add("&View/Volume Style/Volume MIP\t", 0, VolumeMIP, (void*)this, FL_MENU_RADIO | FL_MENU_VALUE);
    _menu3d->add("&View/Volume Style/Volume Accum\t", 0, VolumeAccum, (void*)this, FL_MENU_RADIO);
    _menu3d->add("&View/Volume Style/Volume Blend\t", 0, VolumeBlend, (void*)this, FL_MENU_RADIO | FL_MENU_DIVIDER);
    _menu3d->add("&View/Soma Style/Soma None\t", 0, SomaNone, (void*)this, FL_MENU_RADIO);
    _menu3d->add("&View/Soma Style/Soma Point\t", 0, SomaPoint, (void*)this, FL_MENU_RADIO | FL_MENU_VALUE);
    _menu3d->add("&View/Soma Style/Soma Wire\t", 0, SomaWire, (void*)this, FL_MENU_RADIO);
    _menu3d->add("&View/Soma Style/Soma Solid\t", 0, SomaSolid, (void*)this, FL_MENU_RADIO | FL_MENU_DIVIDER);
    _menu3d->add("&View/Tree Style/Tree None\t", 0, TreeNone, (void*)this, FL_MENU_RADIO);
    _menu3d->add("&View/Tree Style/Tree Line\t", 0, TreeLine, (void*)this, FL_MENU_RADIO | FL_MENU_VALUE);
    _menu3d->add("&View/Tree Style/Tree Strip\t", 0, TreeStrip, (void*)this, FL_MENU_RADIO);
    _menu3d->add("&View/Tree Style/Tree Disk\t", 0, TreeDisk, (void*)this, FL_MENU_RADIO);
    _menu3d->add("&View/Tree Style/Tree Ball\t", 0, TreeBall, (void*)this, FL_MENU_RADIO);
    _menu3d->add("&View/Tree Style/Tree Wire\t", 0, TreeWire, (void*)this, FL_MENU_RADIO);
    _menu3d->add("&View/Tree Style/Tree Solid\t", 0, TreeSolid, (void*)this, FL_MENU_RADIO | FL_MENU_DIVIDER);
    _menu3d->add("&View/Show Volume Bounding\t", 0, VolumeBound, (void*)this, FL_MENU_TOGGLE | FL_MENU_VALUE | FL_MENU_DIVIDER);
    _menu3d->add("&View/Reset Viewport\t", 0, ViewReset, (void*)this);

    _menu3d->add("&Tool/Set Slice Thickness\t", 0, VolumeThickness, (void*)this);
    _menu3d->add("&Tool/Volume Down Sampling\t", 0, VolumeSample, (void*)this);
    _menu3d->add("&Tool/Volume Color Mapping\t", 0, VolumeColormap, (void*)this, FL_MENU_DIVIDER);
    _menu3d->add("&Tool/Show Volume Information\t", 0, ShowVolume, (void*)this);
    _menu3d->add("&Tool/Show Soma Information\t", 0, ShowSoma, (void*)this);
    _menu3d->add("&Tool/Show Tree Information\t", 0, ShowTree, (void*)this);
    
    _menu3d->add("&Help/About\t", 0, About, (void*)this);

    _view3d->align(FL_ALIGN_CENTER | FL_ALIGN_INSIDE);
    _view3d->SetVolume(_volume);
    _view3d->SetColor(_color);
    _view3d->SetSoma(_soma);
    _view3d->SetTree(_tree);
    _view3d->SetMappping(_mapping);
    _view3d->SetProbing(_probing);
    _view3d->SetTracing(_tracing);
    resizable(_view3d);
    end();

    _menu2d->align(FL_ALIGN_TOP | FL_ALIGN_INSIDE);
    _menu2d->type(Fl_Menu_Button::POPUP3);
    _menu2d->add("Load Colormap\t", 0, ColorLoad, (void*)this);
    _menu2d->add("Save Colormap\t", 0, ColorSave, (void*)this, FL_MENU_DIVIDER);
    _menu2d->add("Insert Point\t", 0, ColorInsert, (void*)this);
    _menu2d->add("Remove Point\t", 0, ColorRemove, (void*)this, FL_MENU_DIVIDER);
    _menu2d->add("Update Volume\t", 0, ColorUpdate, (void*)this);
    _menu2d->add("Reset Colormap\t", 0, ColorReset, (void*)this);

    _view2d->align(FL_ALIGN_BOTTOM_LEFT | FL_ALIGN_INSIDE);
    _view2d->SetView(_view3d);
    _view2d->SetVolume(_volume);
    _view2d->SetColor(_color);
    _view2d->SetMapping(_mapping);
    
    _dialog->resizable(_view2d);
    _dialog->set_non_modal();
    _dialog->end();
}

Window::~Window()
{    
    if (_menu2d != 0)   delete _menu2d;
    if (_view2d != 0)   delete _view2d;
    if (_dialog != 0)   delete _dialog;

    if (_menu3d != 0)   delete _menu3d;
    if (_view3d != 0)   delete _view3d;

    if (_volume != 0)   delete _volume;
    if (_color != 0)    delete _color;
    if (_soma != 0)     delete _soma;
    if (_tree != 0)     delete _tree;
    if (_mapping != 0)  delete _mapping;
    if (_probing != 0)  delete _probing;
    if (_tracing != 0)  delete _tracing;
}

void Window::FileLoad_i()
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
            sprintf(str, "flNeuronTool Tracing - loading %s ...", fl_filename_name(path));
            label(str);
            _volume->Read(path);
            _volume->Show();            
            _mapping->SetParam();
            _probing->SetParam();
            _tracing->SetParam();
            _view3d->redraw();            
            sprintf(str, "flNeuronTool Tracing - %s", fl_filename_name(path));
            label(str);
            return;
        }
        if (strcmp(ext, ".apo") == 0) {
            if (!_volume->IsValid()) {
                fl_alert("Please load a volume (TIFF) file first.\n");
                return;
            }
            _soma->Read(path);
            _soma->Show();
            _view3d->redraw();
            return;
        }
        if (strcmp(ext, ".swc") == 0) {
            if (!_volume->IsValid()) {
                fl_alert("Please load a volume (TIFF) file first.\n");
                return;
            }
            _tree->Read(path);
            _tree->Show();
            _view3d->redraw();
            return;
        }
        fl_alert("Unknown file type. Support volume (TIFF), soma (APO), tree (SWC) file.\n");
    }
}

void Window::FileSave_i()
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
            if (!_tree->IsValid()) {
                fl_alert("No data to save.\n");
                return;
            }
            strcat(path, ".swc");
            _tree->Write(path);
            return;
        }
        fl_alert("Unknown file type. Support volume (TIFF), soma (APO), tree (SWC) file.\n");
    }
}

void Window::SceneSave_i()
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
        _view3d->SaveScene(path);
    }
}

void Window::EditMode_i(OP_MODE mode)
{
    _view3d->SetMode(mode);
    if (mode == OP_NONE) {
        for (int i=0; i<21; ++i) _menu3d->mode(_ids[i], _menu3d->mode(_ids[i]) | FL_MENU_INACTIVE);
        printf("[Window::EditMode] switch to none filter mode\n");
    }
    else if (mode == OP_PROBING) {
        for (int i=0; i<9; ++i) _menu3d->mode(_ids[i], _menu3d->mode(_ids[i]) ^ FL_MENU_INACTIVE);
        for (int i=9; i<21; ++i) _menu3d->mode(_ids[i], _menu3d->mode(_ids[i]) | FL_MENU_INACTIVE);
        printf("[Window::EditMode] switch to soma probing mode\n");
    }
    else if (mode == OP_TRACING) {
        for (int i=0; i<9; ++i) _menu3d->mode(_ids[i], _menu3d->mode(_ids[i]) | FL_MENU_INACTIVE);
        for (int i=9; i<21; ++i) _menu3d->mode(_ids[i], _menu3d->mode(_ids[i]) ^ FL_MENU_INACTIVE);
        printf("[Window::EditMode] switch to tree tracing mode\n");
    }
}

void Window::SomaParam_i()
{
    const char *s = fl_input("Set soma probing golbal parameters include minimum radius, high value, low value and value grads:\n", "4.0 127.0 127.0 127.0");
    if (s != 0) {
        float radius, high, low, grads;
        sscanf(s, "%f %f %f %f", &radius, &high, &low, &grads);
        _probing->SetParam(radius, high, low, grads);
        printf("[Window::SomaParam] set soma probing golbal parameters radius %.1f, high %.1f, low %.1f, grads %.1f\n", radius, high, low, grads);
    }
}

void Window::SomaReduce_i()
{
    if (!_soma->IsValid()) return;

    while (_soma->GetSize() != _soma->Reduce()) continue;
    printf("[Window::SomaReduce] simpify soma model to %d cells\n", _soma->GetSize());
    _view3d->redraw();
}

void Window::SomaPrune_i()
{
    if (!_soma->IsValid()) return;

    const char *s = fl_input("set soma model pruning parameters minimum radius:\n", "4.0");
    if (s != 0) {
        float radius = (float)atof(s);
        printf("[Window::SomaPrune] prune soma model to %d cells\n", _soma->PruneSmall(radius));
        _view3d->redraw();
    }
}

void Window::TreeSampling_i()
{
    const char *s = fl_input("Set tree tracing sampling parameters include maximum distance and location step:\n", "3.0 2.0");
    if (s != 0) {
        float dist, step;
        sscanf(s, "%f %f", &dist, &step);
        _tracing->SetParam(dist, step);
        printf("[Window::TreeSampling] set tree tracing sampling parameters maximum distance %.1f, location step %.1f\n", dist, step);
    }
}

void Window::TreeParam_i()
{
    const char *s = fl_input("Set tree tracing golbal parameters include maximum radius, high value, low value and value grads:\n", "16.0 127.0 127.0 127.0");
    if (s != 0) {
        float radius, high, low, grads;
        sscanf(s, "%f %f %f %f", &radius, &high, &low, &grads);
        _tracing->SetParam(radius, high, low, grads);
        printf("[Window::TreeParam] set tree tracing golbal parameters radius %.1f, high %.1f, low %.1f, grads %.1f\n", radius, high, low, grads);
    }
}

void Window::TreeUpdate_i()
{
    fl_message("Please select press mouse RIGHT button to select a seed point at desired position.\n");
}

void Window::TreeReduce_i()
{
    if (!_tree->IsValid()) return;

    while (_tree->GetSize() != _tree->Reduce()) continue;
    printf("[Window::TreeReduce] simpify tree model to %d nodes\n", _tree->GetSize());
    _view3d->redraw();
}

void Window::TreePrune_i()
{
    if (!_tree->IsValid()) return;

    const char *s = fl_input("Set short branch length lower limit (length value in node)\n", "1");
    if (s != 0) {
        int lower;
        sscanf(s, "%d", &lower);
        while (_tree->GetSize() != _tree->Reduce(0, lower)) continue;
        printf("[Window::TreePrune] prune tree model to %d nodes\n", _tree->GetSize());
        _view3d->redraw();
    }
}

void Window::TreeStretch_i()
{
    if (!_tree->IsValid()) return;

    _tree->Stretch();
    printf("[Window::TreeStretch] simpify tree model to %d nodes\n", _tree->GetSize());
    _view3d->redraw();
}

void Window::TreeFixup_i()
{
    if (!_tree->IsValid()) return;

    const char *s = fl_input("set tree model radius fixup parameters include lower value, upper value:" "0.5 16.0");
    if (s != 0) {
        float lower, upper;
        sscanf(s, "%f %f", &lower, &upper);
        printf("[Window::TreeFixup] fixup %d nodes for tree model\n", _tree->FixupRadius(lower, upper));
        _view3d->redraw();
    }
}

void Window::VolumeThickness_i()
{
    if (!_volume->IsValid()) return;

    char value[8];
    sprintf(value, "%.2f", _volume->GetThickness());
    const char *s = fl_input("Set volume slice thickness (float). Current value is %s.\n", value, value);
    if (s != 0) {
        float t0 = _volume->GetThickness();
        float t1 = (float)atof(s);
        if (t1 != t0) {
            _volume->SetThickness(t1);
            _soma->SetExtent(_volume->GetWidth(), _volume->GetHeight(), _volume->GetDepth(), _volume->GetThickness());
            _tree->SetExtent(_volume->GetWidth(), _volume->GetHeight(), _volume->GetDepth(), _volume->GetThickness());
            _view3d->redraw();
        }
    }
}

void Window::VolumeSample_i()
{
    if (!_volume->IsValid()) return;

    const char *s = fl_input("Set volume down sampling level (1/2/4) for rendering. Default is 1.\n", "1");
    if (s != 0) {
        int level = atoi(s);
        _view3d->make_current();
        _volume->SetSample(level);
        _view3d->redraw();
    }
}

void Window::ColorLoad_i()
{
    Fl_Native_File_Chooser fc;
    fc.title("Load File");
    fc.type(Fl_Native_File_Chooser::BROWSE_FILE);
    fc.filter("Color Mapping File (*.lut)\t*.{lut}\n");
    if (fc.show() == 0) {
        char path[256], ext[5];
        strcpy(path, fc.filename());
        strcpy(ext, fl_filename_ext(path));
        _strlwr(ext);
        if (strcmp(ext, ".lut") == 0) {
            char str[256];
            sprintf(str, "flNeuronTool Tracing - loading %s ...", fl_filename_name(path));
            _dialog->label(str);
            _color->Read(path);
            _color->Show();
            _view2d->redraw();
            _view3d->make_current();
            _volume->SetColor(_color->GetValue());
            _view3d->redraw();
            sprintf(str, "flNeuronTool Tracing - %s", fl_filename_name(path));
            _dialog->label(str);
            return;
        }

        fl_alert("Unknown file type. \nSupport color mapping (LUT) file.\n");
    }
}

void Window::ColorSave_i()
{
    if (!_color->IsValid()) {
        fl_alert("No data to save.\n");
        return;
    }

    Fl_Native_File_Chooser fc;
    fc.title("Save File");
    fc.type(Fl_Native_File_Chooser::BROWSE_SAVE_FILE);
    fc.options(fc.options() | Fl_Native_File_Chooser::SAVEAS_CONFIRM);
    fc.filter("Color Mapping File (*.lut)\t*.{lut}\n");
    if (fc.show() == 0) {
        char path[256];
        strcpy(path, fc.filename());
        if (fc.filter_value() == 0) {
            strcat(path, ".lut");
            _color->Write(path);
            return;
        }

        fl_alert("Unknown file type. \nSupport color mapping (LUT) file.\n");
    }
}

void Window::ColorUpdate_i()
{
    _view3d->make_current();
    _mapping->Update();
    _view3d->redraw();
    _view2d->redraw();
    _mapping->SetParam();
    _probing->SetParam();
    _tracing->SetParam();
}

void Window::ColorReset_i()
{
    _view3d->make_current();
    _mapping->Reset();
    _view3d->redraw();
    _view2d->redraw();
}

void Window::About_i()
{
    fl_message("flNeuronTool Tracing\
               \nA Fast Light Neuron Tracing Tool\
               \nversion 1.0\
               \nmingxing@@hust.edu.cn");
}
