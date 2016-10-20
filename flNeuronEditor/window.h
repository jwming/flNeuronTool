#pragma once

#include <FL/Fl.h>
#include <FL/Fl_Window.h>
#include <FL/Fl_Menu_Bar.h>

#include "vision.h"
#include "view.h"

class Window : public Fl_Window {
public:
    Window(int w, int h, const char *label=0);
    ~Window();

private:
    static void FileLoad(Fl_Widget *obj, void *data) { ((View*)data)->FileLoad(); }
    static void FileSave(Fl_Widget *obj, void *data) { ((View*)data)->FileSave(); }
    static void TreeSave(Fl_Widget *obj, void *data) { ((View*)data)->TreeSave(); }
    static void SceneSave(Fl_Widget *obj, void *data) { ((View*)data)->SceneSave(); }
    static void Quit(Fl_Widget *obj, void *data) { exit(0); }  

    static void EditNone(Fl_Widget *obj, void *data) { ((View*)data)->SetMode(OP_NONE); }
    static void EditSoma(Fl_Widget *obj, void *data) { ((View*)data)->SetMode(OP_APO); }
    static void EditTree(Fl_Widget *obj, void *data) { ((View*)data)->SetMode(OP_SWC); }
    static void EditMult(Fl_Widget *obj, void *data) { ((View*)data)->SetMult(((Fl_Menu_*)obj)->mvalue()->value()==FL_MENU_VALUE); }
    static void SelectAll(Fl_Widget *obj, void *data) { ((View*)data)->SelectAll(true); }
    static void SelectNone(Fl_Widget *obj, void *data) { ((View*)data)->SelectAll(false); }
    static void SelectReverse(Fl_Widget *obj, void *data) { ((View*)data)->SelectReverse(); }
    static void RemoveSelect(Fl_Widget *obj, void *data) { ((View*)data)->RemoveSelect(); }
    static void RemoveAll(Fl_Widget *obj, void *data) { ((View*)data)->RemoveAll(); }
    static void TreeRotate(Fl_Widget *obj, void *data) { ((View*)data)->TreeRotate(); }
    static void TreeSplit(Fl_Widget *obj, void *data) { ((View*)data)->TreeSplit(); }
    static void TreeLink(Fl_Widget *obj, void *data) { ((View*)data)->TreeLink(); }

    static void ViewPersp(Fl_Widget *obj, void *data) { ((View*)data)->SetPersp(((Fl_Menu_*)obj)->mvalue()->value()==FL_MENU_VALUE); }
    static void VolumeNone(Fl_Widget *obj, void *data) { ((View*)data)->VolumeStyle(VOL_NONE); }
    static void VolumeMIP(Fl_Widget *obj, void *data) { ((View*)data)->VolumeStyle(VOL_MIP); }
    static void VolumeAccum(Fl_Widget *obj, void *data) { ((View*)data)->VolumeStyle(VOL_ACCUM); }
    static void VolumeBlend(Fl_Widget *obj, void *data) { ((View*)data)->VolumeStyle(VOL_BLEND); }
    static void VolumeFlip(Fl_Widget *obj, void *data) { ((View*)data)->VolumeFlip(((Fl_Menu_*)obj)->mvalue()->value()==FL_MENU_VALUE); }
    static void VolumeBound(Fl_Widget *obj, void *data) { ((View*)data)->VolumeBound(((Fl_Menu_*)obj)->mvalue()->value()==FL_MENU_VALUE); }
    static void SomaNone(Fl_Widget *obj, void *data) { ((View*)data)->SomaStyle(APO_NONE); }
    static void SomaPoint(Fl_Widget *obj, void *data) { ((View*)data)->SomaStyle(APO_POINT); }
    static void SomaWire(Fl_Widget *obj, void *data) { ((View*)data)->SomaStyle(APO_WIRE); }
    static void SomaSolid(Fl_Widget *obj, void *data) { ((View*)data)->SomaStyle(APO_SOLID); }
    static void TreeNone(Fl_Widget *obj, void *data) { ((View*)data)->TreeStyle(SWC_NONE); }
    static void TreeLine(Fl_Widget *obj, void *data) { ((View*)data)->TreeStyle(SWC_LINE); }
    static void TreeStrip(Fl_Widget *obj, void *data) { ((View*)data)->TreeStyle(SWC_STRIP); }
    static void TreeDisk(Fl_Widget *obj, void *data) { ((View*)data)->TreeStyle(SWC_DISK); }
    static void TreeBall(Fl_Widget *obj, void *data) { ((View*)data)->TreeStyle(SWC_BALL); }
    static void TreeWire(Fl_Widget *obj, void *data) { ((View*)data)->TreeStyle(SWC_WIRE); }    
    static void TreeSolid(Fl_Widget *obj, void *data) { ((View*)data)->TreeStyle(SWC_SOLID); }
    static void TreeSync(Fl_Widget *obj, void *data) { ((View*)data)->SetSync(((Fl_Menu_*)obj)->mvalue()->value()==FL_MENU_VALUE); }
    static void ViewReset(Fl_Widget *obj, void *data) { ((View*)data)->ResetView(); }

    static void VolumeThickness(Fl_Widget *obj, void *data) { ((View*)data)->VolumeThickness(); }
    static void VolumeSample(Fl_Widget *obj, void *data) { ((View*)data)->VolumeSample(); }
    static void VolumeTranslate(Fl_Widget *obj, void *data) { ((View*)data)->VolumeTranslate(); }
    static void SomaPrune(Fl_Widget *obj, void *data) { ((View*)data)->SomaPrune(); }
    static void TreePrune(Fl_Widget *obj, void *data) { ((View*)data)->TreePrune(); }
    static void TreeFixup(Fl_Widget *obj, void *data) { ((View*)data)->TreeFixup(); }
    static void ShowVolume(Fl_Widget *obj, void *data) { ((View*)data)->ShowVolume(); }
    static void ShowSoma(Fl_Widget *obj, void *data) { ((View*)data)->ShowSoma(); }
    static void ShowTree(Fl_Widget *obj, void *data) { ((View*)data)->ShowTree(); }
    static void ShowTable(Fl_Widget *obj, void *data) { ((Window*)data)->_view->ShowTable(((Window*)data)->_table); ((Window*)data)->_dialog->show(); }

    static void About(Fl_Widget *obj, void *data);

private:
    Fl_Menu_Bar *_menu;
    View *_view;
    Fl_Window *_dialog;
    Table *_table;
};
