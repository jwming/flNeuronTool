#include "filter.h"

#include <stdio.h>

void Mapping::SetParam()
{
    if (_volume == 0 || _color == 0) return;

    double index[256];
    memset(index, 0, 256*sizeof(double));
    _volume->GetIndex(index);
    _color->SetIndex(index);
    _volume->SetColor(_color->GetValue());
    printf("[Mapping::SetParam] set volume color mapping ok\n");
}

void Mapping::AddPoint(int index, int value)
{
    if (_volume == 0 || _color == 0 || index < 0 || index > 255 || value < 0 || value > 255) return;

    _color->AddPoint((unsigned char)index, (unsigned char)value);
    _volume->SetColor(_color->GetValue());
}

void Mapping::RemovePoint(int index, int value)
{
    if (_volume == 0 || _color == 0 || index < 0 || index > 255 || value < 0 || value > 255) return;

    if (index > 0) _color->RemovePoint((unsigned char)index-1, (unsigned char)value);
    _color->RemovePoint((unsigned char)index, (unsigned char)value);
    if (index < 255) _color->RemovePoint((unsigned char)index+1, (unsigned char)value);
    _volume->SetColor(_color->GetValue());
}

void Mapping::Update()
{
    if (_volume == 0 || _color == 0) return;

    _volume->SetValue(_color->GetValue());
    double index[256];
    memset(index, 0, 256*sizeof(double));
    _volume->GetIndex(index);
    _color->SetIndex(index);
    _color->Clear();
    _volume->SetColor(_color->GetValue());
    printf("[Mapping::Upadte] update volume value and reset color mapping ok\n");
}

void Mapping::Reset()
{
    if (_volume == 0 || _color == 0) return;

    _color->Clear();
    _volume->SetColor(_color->GetValue());
}
