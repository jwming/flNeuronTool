#include "view.h"

#include <stdio.h>
#include <string.h>
#include <FL/Fl_Draw.h>

Table::Table(int x, int y, int w, int h)
    : Fl_Table_Row(x, y, w, h),
    _body(0)
{
    strcpy(_head[0], "#id");
    strcpy(_head[1], "node");
    strcpy(_head[2], "junction");
    strcpy(_head[3], "endpoint");
    strcpy(_head[4], "radius");
    strcpy(_head[5], "minradius");
    strcpy(_head[6], "maxradius");
    strcpy(_head[7], "length");
    
    cols(8);
    col_header(1);
    col_header_height(25);
    col_width_all(80);
    col_resize_min(40);
    col_resize(1);

    table_box(FL_NO_BOX);
    type(SELECT_SINGLE);
    selection_color(FL_SELECTION_COLOR);
    end();
}

void Table::SetTree(std::vector<Tree*> &trees)
{
    if (!_body.empty()) _body.clear();

    if (!trees.empty()) {
        char string[7*12];
        for (size_t i=0; i<trees.size(); ++i) {
            trees[i]->Show(string);
            Item item;
            sprintf(item[0], "%d", i+1);
            sscanf(string, "%s %s %s %s %s %s %s", item[1], item[2], item[3], item[4], item[5], item[6], item[7]);
            _body.push_back(item);
        }
    }
    rows((int)_body.size());
    redraw();
}

void Table::draw_cell(TableContext context, int r, int c, int x, int y, int w, int h)
{
    if (_body.empty()) return;

    switch (context) {
    case CONTEXT_STARTPAGE:
        fl_font(FL_HELVETICA, 14);
        return;

    case CONTEXT_COL_HEADER:
        fl_push_clip(x, y, w, h);
        fl_draw_box(FL_THIN_UP_BOX, x, y, w, h, col_header_color());
        fl_color(FL_BLACK);
        fl_draw(_head[c], x, y, w, h, FL_ALIGN_CENTER);
        fl_pop_clip();
        return;

    case CONTEXT_CELL:
        fl_push_clip(x, y, w, h);
        fl_color(row_selected(r) ? selection_color() : FL_WHITE);
        fl_rectf(x, y, w, h);
        fl_color(FL_BLACK);
        fl_draw(_body[r][c], x, y, w, h, FL_ALIGN_CENTER);
        fl_color(color()); 
        fl_rect(x, y, w, h);
        fl_pop_clip();
        return;
    }
}
