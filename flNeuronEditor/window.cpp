#include "window.h"

#include <stdio.h>
#include <FL/Fl_Ask.h>

Window::Window(int w, int h, const char *label)
    : Fl_Window(w, h, label),
    _menu(new Fl_Menu_Bar(0, 0, w, 25)),
    _view(new View(0, 25, w, h-25)),
    _dialog(new Fl_Window(580, 290, label)),
    _table(new Table(0, 0, 580, 290))
{
    _view->SetMult(false);
    _view->SetPersp(false);
    _view->VolumeStyle(VOL_MIP);
    _view->SomaStyle(APO_POINT);
    _view->TreeStyle(SWC_LINE);
    _view->VolumeFlip(false);
    _view->VolumeBound(true);
    _view->SetSync(true);

    _menu->align(FL_ALIGN_TOP | FL_ALIGN_INSIDE);
    _menu->add("&File/Load File\t", FL_COMMAND+'o', FileLoad, (void*)_view);
    _menu->add("&File/Save File\t", FL_COMMAND+'s', FileSave, (void*)_view);
    _menu->add("&File/Save Selected Trees\t", 0, TreeSave, (void*)_view, FL_MENU_DIVIDER);
    _menu->add("&File/Save Scene\t", 0, SceneSave, (void*)_view, FL_MENU_DIVIDER);
    _menu->add("&File/Quit", 0, Quit);

    _menu->add("&Edit/Select Mode/Select None\t", FL_COMMAND+(FL_F+1), EditNone, (void*)_view, FL_MENU_RADIO | FL_MENU_VALUE);
    _menu->add("&Edit/Select Mode/Select Soma\t", FL_COMMAND+(FL_F+2), EditSoma, (void*)_view, FL_MENU_RADIO);
    _menu->add("&Edit/Select Mode/Select Tree\t", FL_COMMAND+(FL_F+3), EditTree, (void*)_view, FL_MENU_RADIO | FL_MENU_DIVIDER);
    _menu->add("&Edit/Maintain Multiple Selected\t", 'M', EditMult, (void*)_view, FL_MENU_TOGGLE | FL_MENU_DIVIDER);
    _menu->add("&Edit/Select All Objects", FL_COMMAND+'a', SelectAll, (void*)_view);
    _menu->add("&Edit/Cancel Selection", FL_COMMAND+'d', SelectNone, (void*)_view);
    _menu->add("&Edit/Reverse Selection", FL_COMMAND+'e', SelectReverse, (void*)_view, FL_MENU_DIVIDER);
    _menu->add("&Edit/Delete Selected Objects", FL_Delete, RemoveSelect, (void*)_view);
    _menu->add("&Edit/Delete All Objects", FL_SHIFT+FL_Delete, RemoveAll, (void*)_view, FL_MENU_DIVIDER);
    _menu->add("&Edit/Select Previous Tree", FL_Page_Up, 0, 0, 0);
    _menu->add("&Edit/Select Next Tree", FL_Page_Down, 0, 0, 0);
    _menu->add("&Edit/Select Previous Node", FL_Left, 0, 0, 0);
    _menu->add("&Edit/Select Next Node", FL_Right, 0, 0, 0);
    _menu->add("&Edit/Select Previous Key Node", FL_Up, 0, 0, 0);
    _menu->add("&Edit/Select Next Key Node", FL_Down, 0, 0, 0);
    _menu->add("&Edit/Select First Node", FL_Home, 0, 0, 0);
    _menu->add("&Edit/Select Last Node", FL_End, 0, 0, FL_MENU_DIVIDER);
    _menu->add("&Edit/Rotate Selected Tree", FL_COMMAND+'r', TreeRotate, (void*)_view);
    _menu->add("&Edit/Split Selected Tree", FL_COMMAND+'x', TreeSplit, (void*)_view);
    _menu->add("&Edit/Link Selected Trees", FL_COMMAND+'v', TreeLink, (void*)_view);

    _menu->add("&View/Perspective View\t", 0, ViewPersp, (void*)_view, FL_MENU_TOGGLE | FL_MENU_DIVIDER);
    _menu->add("&View/Volume Style/Volume None\t", 0, VolumeNone, (void*)_view, FL_MENU_RADIO);
    _menu->add("&View/Volume Style/Volume MIP\t", 0, VolumeMIP, (void*)_view, FL_MENU_RADIO | FL_MENU_VALUE);
    _menu->add("&View/Volume Style/Volume Accum\t", 0, VolumeAccum, (void*)_view, FL_MENU_RADIO);
    _menu->add("&View/Volume Style/Volume Blend\t", 0, VolumeBlend, (void*)_view, FL_MENU_RADIO | FL_MENU_DIVIDER);  
    _menu->add("&View/Soma Style/Soma None\t", 0, SomaNone, (void*)_view, FL_MENU_RADIO);
    _menu->add("&View/Soma Style/Soma Point\t", 0, SomaPoint, (void*)_view, FL_MENU_RADIO | FL_MENU_VALUE);
    _menu->add("&View/Soma Style/Soma Wire\t", 0, SomaWire, (void*)_view, FL_MENU_RADIO);
    _menu->add("&View/Soma Style/Soma Solid\t", 0, SomaSolid, (void*)_view, FL_MENU_RADIO | FL_MENU_DIVIDER);
    _menu->add("&View/Tree Style/Tree None\t", 0, TreeNone, (void*)_view, FL_MENU_RADIO);
    _menu->add("&View/Tree Style/Tree Line\t", 0, TreeLine, (void*)_view, FL_MENU_RADIO | FL_MENU_VALUE);
    _menu->add("&View/Tree Style/Tree Strip\t", 0, TreeStrip, (void*)_view, FL_MENU_RADIO);
    _menu->add("&View/Tree Style/Tree Disk\t", 0, TreeDisk, (void*)_view, FL_MENU_RADIO);
    _menu->add("&View/Tree Style/Tree Ball\t", 0, TreeBall, (void*)_view, FL_MENU_RADIO);
    _menu->add("&View/Tree Style/Tree Wire\t", 0, TreeWire, (void*)_view, FL_MENU_RADIO);
    _menu->add("&View/Tree Style/Tree Solid\t", 0, TreeSolid, (void*)_view, FL_MENU_RADIO | FL_MENU_DIVIDER);
    _menu->add("&View/Volume Flip\t", 0, VolumeFlip, (void*)_view, FL_MENU_TOGGLE);
    _menu->add("&View/Show Volume Bounding\t", 0, VolumeBound, (void*)_view, FL_MENU_TOGGLE | FL_MENU_VALUE);
    _menu->add("&View/Tree Synchronous Style\t", 0, TreeSync, (void*)_view, FL_MENU_TOGGLE | FL_MENU_VALUE | FL_MENU_DIVIDER);
    _menu->add("&View/Reset Viewport\t", FL_COMMAND+FL_Home, ViewReset, (void*)_view);

    _menu->add("&Tool/Set Slice Thickness\t", 0, VolumeThickness, (void*)_view);
    _menu->add("&Tool/Down Sampling Volume\t", 0, VolumeSample, (void*)_view);
    _menu->add("&Tool/Translate Volume\t", 0, VolumeTranslate, (void*)_view, FL_MENU_DIVIDER);
    _menu->add("&Tool/Prune Small Soma\t", 0, SomaPrune, (void*)_view);
    _menu->add("&Tool/Prune Short Tree\t", 0, TreePrune, (void*)_view);
    _menu->add("&Tool/Fixup Thin Tree\t", 0, TreeFixup, (void*)_view, FL_MENU_DIVIDER);
    _menu->add("&Tool/Show Volume Information\t", 0, ShowVolume, (void*)_view);
    _menu->add("&Tool/Show Soma Information\t", 0, ShowSoma, (void*)_view);
    _menu->add("&Tool/Show Tree Information\t", 0, ShowTree, (void*)_view);
    _menu->add("&Tool/Show Tree Table\t", 0, ShowTable, (void*)this);

    _menu->add("&Help/About\t", 0, About, (void*)this);

    _view->align(FL_ALIGN_CENTER | FL_ALIGN_INSIDE);
    resizable(_view);
    end();

    _dialog->resizable(_table);
    _dialog->set_non_modal();
    _dialog->end();    
}

Window::~Window()
{
    if (_menu != 0) delete _menu;
    if (_view != 0) delete _view;
    if (_table != 0) delete _table;
}

void Window::About(Fl_Widget *obj, void *data)
{
    fl_message("flNeuronTool Editor\
               \nA Fast Light Neuron Editor Tool\
               \nversion 1.0\
               \nmingxing@@hust.edu.cn");
}
