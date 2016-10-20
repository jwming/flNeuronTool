#pragma once

#include <vector>

struct IVision {
    virtual ~IVision() {}
    virtual bool Read(const char *path) = 0;
    virtual bool Write(const char *path) const = 0;
    virtual void Draw() const = 0;
    virtual void Show() const = 0;
};

enum VOL_STYLE { VOL_NONE, VOL_MIP, VOL_ACCUM, VOL_BLEND };

class Volume : public IVision { // TIFF
public:
    Volume() : _buffer(0), _width(0), _height(0), _depth(0), _thickness(1.0f), _scale(1.0f), _offx(0), _offy(0), _offz(0), _texture(0), _flip(false), _bound(true), _style(VOL_MIP) {}
    ~Volume();

    bool Read(const char *path);
    bool Write(const char *path) const;
    void Draw() const;
    void Show() const;
    
    bool IsValid() const { return _buffer != 0; }
    size_t GetWidth() const { return _width; }
    size_t GetHeight() const { return _height; }
    size_t GetDepth() const { return _depth; }
    float GetThickness() const { return _thickness; }
    float GetScale() const { return _scale; }
    void SetThickness(float thickness=1.0f) { _thickness = thickness; _scale = std::max(std::max(_width, _height)*1.0f, _depth*_thickness); }
    void SetExtent(size_t width, size_t height, size_t depth, float thickness=1.0f) { _width = width; _height = height; _depth = depth; SetThickness(thickness); }
    void SetTranslate(int x, int y, int z) { _offx = x; _offy = y; _offz = z; }
    bool GetFlip() const { return _flip; }
    bool SetFlip(bool b) { _flip = b; return _flip; }
    bool GetBound() const { return _bound; }
    bool SetBound(bool b) { _bound = b; return _bound; }
    int GetStyle() const { return _style; }
    int SetStyle(int style) { _style = style; if (_style > VOL_BLEND) _style = VOL_NONE; return _style; }
    unsigned char GetVoxel(size_t x, size_t y, size_t z) const { return (_buffer==0 || x>=_width || y>=_height || z>=_depth) ? 0 : _buffer[z*_height*_width+y*_width+x]; }
    void SetSample(int level) const;

private:
    unsigned char *_buffer;
    size_t _width, _height, _depth;
    float _thickness, _scale;
    int _offx, _offy, _offz;
    unsigned int _texture;
    bool _flip, _bound;
    int _style; // VOL_STYLE
};

struct Cell {
    Cell() : X(0.0f), Y(0.0f), Z(0.0f), Radius(0.0f), Value(0.0f), Cur(false) {}
    float X, Y, Z, Radius, Value;
    bool Cur;
};

enum APO_STYLE { APO_NONE, APO_POINT, APO_WIRE, APO_SOLID };

class Soma : public IVision { // APO
public:
    Soma() : _list(0), _width(0), _height(0), _depth(0), _thickness(1.0f), _scale(1.0f), _style(APO_POINT) {}
    ~Soma() {}

    bool Read(const char *path);
    bool Write(const char *path) const;
    void Draw() const;
    void Show() const;

    bool GetId(size_t id) { return (id < _list.size()) ? _list[id].Cur : false; }
    void SetId(size_t id, bool b) { if (id < _list.size())  _list[id].Cur = b; }
    void SelectAll(bool b) { for (size_t i=0; i<_list.size(); ++i) _list[i].Cur = b; }
    void SelectReverse() { for (size_t i=0; i<_list.size(); ++i) _list[i].Cur = !_list[i].Cur; }
    void DrawId() const;

    bool IsValid() const { return !_list.empty(); }
    void SetExtent(size_t width, size_t height, size_t depth, float thickness=1.0f);
    int GetStyle() const  { return _style; }
    int SetStyle(int style) { _style = style; if (_style > APO_SOLID) _style = APO_NONE; return _style; }
    size_t GetSize() const { return _list.size(); }
    size_t Remove();
    void Clear() { _list.clear(); }
    size_t PruneSmall(float radius);

private:
    std::vector<Cell> _list;
    size_t _width, _height, _depth;
    float _thickness, _scale;
    int _style; // APO_STYLE
};

struct Node {
    Node() : Id(0), Tag(0), X(0.0f), Y(0.0f), Z(0.0f), Radius(0.0f), Pid(0) {}
    size_t Id;
    int Tag;  // FAKE=-1, NONE=0, BODY=1, AXON=2, DENDRITE=3, SPINE=4, JUNC=5, END=6, DEFINE=7
    float X, Y, Z, Radius;
    long Pid;
};

enum SWC_STYLE { SWC_NONE, SWC_LINE, SWC_STRIP, SWC_DISK, SWC_BALL, SWC_WIRE, SWC_SOLID };

class Tree : public IVision { // SWC
public:
    typedef std::vector<Tree*> Trees;
    Tree() : _list(0), _width(0), _height(0), _depth(0), _thickness(1.0f), _scale(1.0f), _cur(false), _nid(0), _style(SWC_LINE) {}
    ~Tree() {}

    bool Read(const char *path);
    bool Write(const char *path) const;
    void Draw() const;
    void Show() const;
    void Show(char *string) const;

    bool GetCur() const { return _cur; }
    void SetCur(bool b) { _cur = b; }
    int GetId() const { return _nid; }
    void SetId(int id) { _nid = id; }
    void PrevId() { _nid = (--_nid + _list.size()) % _list.size(); }
    void NextId() { _nid = (++_nid) % _list.size(); }
    void UpId() { do PrevId(); while (_list[_nid].Tag==2 || _list[_nid].Tag==3); }
    void DownId() { do NextId(); while (_list[_nid].Tag==2 || _list[_nid].Tag==3); }
    void DrawId() const;

    bool IsValid() const { return !_list.empty(); }
    void GetExtend(float &width, float &height, float &depth, float &thickness) const;
    void SetExtent(size_t width, size_t height, size_t depth, float thickness=1.0f);
    int GetStyle() const { return _style; }
    int SetStyle(int style) { _style = style; if (_style > SWC_SOLID) _style = SWC_NONE; return _style; }
    size_t GetSize() const { return _list.size(); }
    void Clear() { _list.clear(); }
    size_t FixupRadius(float lower, float upper);
    void Taging();
    void Rotate();

    void Resolve(Trees &trees);
    void Split(Tree &tree);
    void Merge(const Trees &trees);
    void Merge(const Tree &tree);
    void Link(Tree &trace);

private:
    std::vector<Node> _list;
    size_t _width, _height, _depth;
    float _thickness, _scale;
    bool _cur;
    int _nid;
    int _style; // SWC_STYLE
};
