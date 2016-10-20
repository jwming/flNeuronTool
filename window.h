#pragma once

#include <FL/Fl.h>
#include <FL/Fl_Window.h>
#include <FL/Fl_Menu_Bar.h>
#include <FL/Fl_Menu_Button.h>

#include "vision.h"
#include "filter.h"
#include "view.h"

class Window : public Fl_Window {
public:
    Window(int w, int h, const char *label=0);
    ~Window();   

private:
    static void FileLoad(Fl_Widget *obj, void *data) { ((Window*)data)->FileLoad_i(); }
    static void FileSave(Fl_Widget *obj, void *data) { ((Window*)data)->FileSave_i(); }
    static void SceneSave(Fl_Widget *obj, void *data) { ((Window*)data)->SceneSave_i(); }
    static void Quit(Fl_Widget *obj, void *data) { exit(0); }  

    static void EditNone(Fl_Widget *obj, void *data) { ((Window*)data)->EditMode_i(OP_NONE); }
    static void EditSoma(Fl_Widget *obj, void *data) { ((Window*)data)->EditMode_i(OP_PROBING); }
    static void EditTree(Fl_Widget *obj, void *data) { ((Window*)data)->EditMode_i(OP_TRACING); }
    static void EditSelect(Fl_Widget *obj, void *data) { ((Window*)data)->EditSelect_i(((Fl_Menu_*)obj)->mvalue()->value()==FL_MENU_VALUE); }
    static void EditFresh(Fl_Widget *obj, void *data) { ((Window*)data)->EditFresh_i(((Fl_Menu_*)obj)->mvalue()->value()==FL_MENU_VALUE); }
    static void SomaParam(Fl_Widget *obj, void *data) { ((Window*)data)->SomaParam_i(); }
    static void SomaLocal(Fl_Widget *obj, void *data) { ((Window*)data)->SomaLocal_i(((Fl_Menu_*)obj)->mvalue()->value()==FL_MENU_VALUE); }
    static void SomaMerge(Fl_Widget *obj, void *data) { ((Window*)data)->SomaMerge_i(((Fl_Menu_*)obj)->mvalue()->value()==FL_MENU_VALUE); }
    static void SomaUpdate(Fl_Widget *obj, void *data) { ((Window*)data)->SomaUpdate_i(); }
    static void SomaCancel(Fl_Widget *obj, void *data) { ((Window*)data)->SomaCancel_i(); } 
    static void SomaRemove(Fl_Widget *obj, void *data) { ((Window*)data)->SomaRemove_i(); }
    static void SomaClear(Fl_Widget *obj, void *data) { ((Window*)data)->SomaClear_i(); }
    static void SomaReduce(Fl_Widget *obj, void *data) { ((Window*)data)->SomaReduce_i(); }
    static void SomaPrune(Fl_Widget *obj, void *data) { ((Window*)data)->SomaPrune_i(); }
    static void TreeSampling(Fl_Widget *obj, void *data) { ((Window*)data)->TreeSampling_i(); }
    static void TreeParam(Fl_Widget *obj, void *data) { ((Window*)data)->TreeParam_i(); }
    static void TreeLocal(Fl_Widget *obj, void *data) { ((Window*)data)->TreeLocal_i(((Fl_Menu_*)obj)->mvalue()->value()==FL_MENU_VALUE); }
    static void TreeLink(Fl_Widget *obj, void *data) { ((Window*)data)->TreeLink_i(((Fl_Menu_*)obj)->mvalue()->value()==FL_MENU_VALUE); }
    static void TreeUpdate(Fl_Widget *obj, void *data) { ((Window*)data)->TreeUpdate_i(); }
    static void TreeCancel(Fl_Widget *obj, void *data) { ((Window*)data)->TreeCancel_i(); }
    static void TreeRemove(Fl_Widget *obj, void *data) { ((Window*)data)->TreeRemove_i(); }
    static void TreeClear(Fl_Widget *obj, void *data) { ((Window*)data)->TreeClear_i(); }
    static void TreeReduce(Fl_Widget *obj, void *data) { ((Window*)data)->TreeReduce_i(); }        
    static void TreePrune(Fl_Widget *obj, void *data) { ((Window*)data)->TreePrune_i(); }
    static void TreeStretch(Fl_Widget *obj, void *data) { ((Window*)data)->TreeStretch_i(); }
    static void TreeFixup(Fl_Widget *obj, void *data) { ((Window*)data)->TreeFixup_i(); }

    static void ViewPersp(Fl_Widget *obj, void *data) { ((Window*)data)->ViewPersp_i(((Fl_Menu_*)obj)->mvalue()->value()==FL_MENU_VALUE); }
    static void VolumeNone(Fl_Widget *obj, void *data) { ((Window*)data)->VolumeStyle_i(VOL_NONE); }
    static void VolumeMIP(Fl_Widget *obj, void *data) { ((Window*)data)->VolumeStyle_i(VOL_MIP); }
    static void VolumeAccum(Fl_Widget *obj, void *data) { ((Window*)data)->VolumeStyle_i(VOL_ACCUM); }
    static void VolumeBlend(Fl_Widget *obj, void *data) { ((Window*)data)->VolumeStyle_i(VOL_BLEND); }
    static void SomaNone(Fl_Widget *obj, void *data) { ((Window*)data)->SomaStyle_i(APO_NONE); }
    static void SomaPoint(Fl_Widget *obj, void *data) { ((Window*)data)->SomaStyle_i(APO_POINT); }
    static void SomaWire(Fl_Widget *obj, void *data) { ((Window*)data)->SomaStyle_i(APO_WIRE); }
    static void SomaSolid(Fl_Widget *obj, void *data) { ((Window*)data)->SomaStyle_i(APO_SOLID); }
    static void TreeNone(Fl_Widget *obj, void *data) { ((Window*)data)->TreeStyle_i(SWC_NONE); }
    static void TreeLine(Fl_Widget *obj, void *data) { ((Window*)data)->TreeStyle_i(SWC_LINE); }
    static void TreeStrip(Fl_Widget *obj, void *data) { ((Window*)data)->TreeStyle_i(SWC_STRIP); }
    static void TreeDisk(Fl_Widget *obj, void *data) { ((Window*)data)->TreeStyle_i(SWC_DISK); }
    static void TreeBall(Fl_Widget *obj, void *data) { ((Window*)data)->TreeStyle_i(SWC_BALL); }
    static void TreeWire(Fl_Widget *obj, void *data) { ((Window*)data)->TreeStyle_i(SWC_WIRE); }
    static void TreeSolid(Fl_Widget *obj, void *data) { ((Window*)data)->TreeStyle_i(SWC_SOLID); }
    static void VolumeBound(Fl_Widget *obj, void *data) { ((Window*)data)->VolumeBound_i(((Fl_Menu_*)obj)->mvalue()->value()==FL_MENU_VALUE); }
    static void ViewReset(Fl_Widget *obj, void *data) { ((Window*)data)->ViewReset_i(); }

    static void VolumeThickness(Fl_Widget *obj, void *data) { ((Window*)data)->VolumeThickness_i(); }
    static void VolumeSample(Fl_Widget *obj, void *data) { ((Window*)data)->VolumeSample_i(); }
    static void VolumeColormap(Fl_Widget *obj, void *data) { ((Window*)data)->VolumeColormap_i(); }
    static void ShowVolume(Fl_Widget *obj, void *data) { ((Window*)data)->ShowVolume_i(); }
    static void ShowSoma(Fl_Widget *obj, void *data) { ((Window*)data)->ShowSoma_i(); }
    static void ShowTree(Fl_Widget *obj, void *data) { ((Window*)data)->ShowTree_i(); }

    static void ColorLoad(Fl_Widget *obj, void *data) { ((Window*)data)->ColorLoad_i(); }
    static void ColorSave(Fl_Widget *obj, void *data) { ((Window*)data)->ColorSave_i(); }
    static void ColorInsert(Fl_Widget *obj, void *data) { ((Window*)data)->ColorInsert_i(); }
    static void ColorRemove(Fl_Widget *obj, void *data) { ((Window*)data)->ColorRemove_i(); }
    static void ColorUpdate(Fl_Widget *obj, void *data) { ((Window*)data)->ColorUpdate_i(); }
    static void ColorReset(Fl_Widget *obj, void *data) { ((Window*)data)->ColorReset_i(); }

    static void About(Fl_Widget *obj, void *data) { ((Window*)data)->About_i(); }

private:
    void FileLoad_i();
    void FileSave_i();
    void SceneSave_i();

    void EditMode_i(OP_MODE mode);
    void EditSelect_i(bool b) { _view3d->SetSelect(b); }
    void EditFresh_i(bool b) { _view3d->SetFresh(b); }
    void SomaParam_i();
    void SomaLocal_i(bool b) { _probing->SetLocal(b); }
    void SomaMerge_i(bool b) { _soma->SetMerge(b); }
    void SomaUpdate_i() { _probing->BeginUpdate(); }
    void SomaCancel_i() { _probing->CancelUpdate(); }
    void SomaRemove_i() { _soma->Remove(); _view3d->redraw(); }
    void SomaClear_i() { _soma->Clear(); _view3d->redraw(); }
    void SomaReduce_i();
    void SomaPrune_i();
    void TreeSampling_i();
    void TreeParam_i();
    void TreeLocal_i(bool b) { _tracing->SetLocal(b); }
    void TreeLink_i(bool b) { _tree->SetLink(b); } 
    void TreeUpdate_i();
    void TreeCancel_i() { _tracing->CancelUpdate(); _view3d->redraw(); }
    void TreeRemove_i() { _tree->Remove(); _view3d->redraw(); }
    void TreeClear_i() { _tree->Clear(); _view3d->redraw(); }
    void TreeReduce_i();    
    void TreePrune_i();
    void TreeStretch_i();
    void TreeFixup_i();

    void VolumeBound_i(bool b) { _volume->SetBound(b); _view3d->redraw(); }
    void VolumeStyle_i(VOL_STYLE style) { _volume->SetStyle(style); _view3d->redraw(); }
    void SomaStyle_i(APO_STYLE style) { _soma->SetStyle(style); _view3d->redraw(); }
    void TreeStyle_i(SWC_STYLE style) { _tree->SetStyle(style); _view3d->redraw(); }
    void ViewPersp_i(bool b) { _view3d->SetPersp(b); }
    void ViewReset_i() { _view3d->ResetView(); }

    void VolumeThickness_i();
    void VolumeSample_i();    
    void VolumeColormap_i() { if (_volume->IsValid()) _dialog->show(); }
    void ShowVolume_i() { if (_volume->IsValid()) _volume->Show(); }
    void ShowSoma_i() { if (_soma->IsValid()) _soma->Show(); }
    void ShowTree_i() { if (_tree->IsValid()) _tree->Show(); }

    void ColorLoad_i();
    void ColorSave_i();
    void ColorInsert_i() { _view2d->InsertPoint(); }
    void ColorRemove_i() { _view2d->RemovePoint(); }
    void ColorUpdate_i();
    void ColorReset_i();

    void About_i();

private:
    Volume *_volume;
    Color *_color;
    Soma *_soma;
    Tree *_tree;
    Mapping *_mapping;
    Probing *_probing;
    Tracing *_tracing;
    int _opmode; // OP_MODE

    Fl_Menu_Bar *_menu3d;
    std::vector<int> _ids;
    View3D *_view3d;

    Fl_Window *_dialog;
    Fl_Menu_Button *_menu2d;
    View2D *_view2d;
};
