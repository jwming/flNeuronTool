#pragma once

#include <vector>
#include <map>

struct IVision {
    virtual ~IVision() {}
    virtual bool Read(const char *path) = 0;
    virtual bool Write(const char *path) const = 0;
    virtual void Draw() const = 0;
    virtual void Show() const = 0;
};

struct Point { // [0,S]
    Point() : X(0.0f), Y(0.0f), Z(0.0f), Value(0.0f) {}
    float X, Y, Z, Value;
};

enum VOL_STYLE { VOL_NONE, VOL_MIP, VOL_ACCUM, VOL_BLEND };

class Volume : public IVision { // TIFF
public:
    Volume() : _buffer(0), _width(0), _height(0), _depth(0), _thickness(1.0f), _scale(1.0f), _mean(0.0f), _low(0.0f), _high(0.0), _texture(0), _color(0), _program(0), _bound(true), _style(VOL_MIP) {}
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
    bool GetBound() const { return _bound; }
    bool SetBound(bool b) { _bound = b; return _bound; }
    int GetStyle() const { return _style; }
    int SetStyle(int style) { _style = style; if (_style > VOL_BLEND) _style = VOL_NONE; return _style; }
    void SetSample(int level) const;
    void SetColor(const unsigned char *color) const;
    unsigned char GetVoxel(size_t x, size_t y, size_t z) const { return (_buffer==0 || x>=_width || y>=_height || z>=_depth) ? 0 : _buffer[z*_height*_width+y*_width+x]; }
    float GetVoxel(float x, float y, float z) const;
    float GetVoxel(Point &point) const { return GetVoxel(point.X, point.Y, point.Z); }
    Point GetPoint(float x, float y, float z) const; // [-1,1] -> [0,S]
    Point GetPoint(const Point &point0, const Point &point1) const;
    void GetValue(float &mean, float &low, float &high, size_t x, size_t y, size_t z, size_t radius) const;
    void GetValue(float &mean, float &low, float &high) const { mean = _mean; low = _low; high = _high; }
    void SetValue(int low, int high, size_t x, size_t y, size_t z, size_t radius);
    void SetValue(int low, int high);
    void GetIndex(double *index, size_t x, size_t y, size_t, size_t radius) const;
    void GetIndex(double *index) const { GetIndex(index, 0, 0, 0, (size_t)(_scale+0.5f)); }
    void SetValue(const unsigned char *value, size_t x, size_t y, size_t z, size_t radius);
    void SetValue(const unsigned char *value);

private:
    unsigned char *_buffer;
    size_t _width, _height, _depth;
    float _thickness, _scale;
    float _mean, _low, _high;
    unsigned _texture, _color, _program;
    bool _bound;
    int _style; // VOL_STYLE
};

enum LUT_STYLE { LUT_NONE, LUT_BOUND, LUT_LINE };

class Color : public IVision { // LUT
public:
    Color();
    ~Color() { if (_index != 0) delete[] _index; if (_value != 0) delete[] _value; }

    bool Read(const char *path);
    bool Write(const char *path) const;
    void Draw() const;
    void Show() const;

    bool IsValid() const { return !_list.empty(); }
    void SetExtend(size_t width, size_t height) { _width = width; _height = height; _scalex = _width/256.0f; _scaley = _height/256.0f; }
    int GetIndex(float x) const { return (int)((_width*x+_width)/(_scalex*2.0f)); }
    int GetValue(float y) const { return (int)((_height*y+_height)/(_scaley*2.0f)); }
    int GetStyle() const { return _style; }
    int SetStyle(int style) { _style = style; if (_style > LUT_LINE) _style = LUT_NONE; return _style; }
    void SetIndex(const double *index);
    const unsigned char *GetValue() const { return _value; }
    void AddPoint(unsigned char index, unsigned char value);
    void RemovePoint(unsigned char index, unsigned char value);
    void Clear();

private:
    typedef std::map<int, int> map_t;
    map_t _list;
    unsigned char *_index, *_value;
    size_t _width, _height;
    float _scalex, _scaley;
    int _style; // LUT_STYLE
};

struct Cell { // [-1,1]
    Cell() : X(0.0f), Y(0.0f), Z(0.0f), Radius(0.0f), Value(0.0f) {}
    float X, Y, Z, Radius, Value;
};

struct PCell : public Point { // [0,S]
    PCell() : Point(), Radius(0.0f), Minor(0.0f) {}
    PCell(const Point& point) : Point(point), Radius(0.0f), Minor(0.0f) {}
    float Radius, Minor;
};

enum APO_STYLE { APO_NONE, APO_POINT, APO_WIRE, APO_SOLID };

class Soma : public IVision { // APO
public:
    Soma() : _list(0), _width(0), _height(0), _depth(0), _thickness(1.0f), _scale(1.0f), _style(APO_POINT), _merge(false) {}
    ~Soma() {}

    bool Read(const char *path);
    bool Write(const char *path) const;
    void Draw() const;
    void Show() const;

    bool IsValid() const { return !_list.empty(); }
    void SetExtent(size_t width, size_t height, size_t depth, float thickness=1.0f);
    int GetStyle() const  { return _style; }
    int SetStyle(int style) { _style = style; if (_style > APO_SOLID) _style = APO_NONE; return _style; }
    bool GetMerge() const { return _merge; }
    bool SetMerge(bool b) { _merge = b; return _merge; }
    size_t GetSize() const { return _list.size(); }
    Cell GetCell(size_t id) const { return (id < _list.size()) ? _list[id] : Cell(); }
    PCell GetPoint(size_t id) const; // [-1,1] -> [0,S]
    size_t AddCell(const Cell &cell) { _list.push_back(cell); return _list.size(); }
    size_t AddPoint(const PCell &point); // [0,S] -> [-1,1]
    size_t Remove() { if (!_list.empty()) _list.pop_back(); return _list.size(); }
    void Clear() { _list.clear(); }
    size_t Reduce(size_t start=0);
    size_t PruneSmall(float radius);

private:
    std::vector<Cell> _list;
    size_t _width, _height, _depth;
    float _thickness, _scale;
    int _style; // APO_STYLE
    bool _merge;
};

struct Node { // [-1,1]
    Node() : Id(0), Child(0), Path(0), Tag(0), X(0.0f), Y(0.0f), Z(0.0f), Radius(0.0f), Pid(0) {}
    size_t Id;
    int Child, Path, Tag;  // FAKE=-1, NONE=0, BODY=1, AXON=2, DENDRITE=3, SPINE=4, JUNC=5, END=6, DEFINE=7
    float X, Y, Z, Radius;
    long Pid;
};

struct PNode : public Point { // [0,S]
    PNode() : Point(), Id(0), Radius(0.0f), I(0.0f), J(0.0f), K(0.0f), Pid(0) {}
    PNode(const Point& point) : Point(point), Id(0), Radius(0.0f), I(0.0f), J(0.0f), K(0.0f), Pid(0) {}
    size_t Id;
    float Radius, I, J, K;
    long Pid;
};

enum SWC_STYLE { SWC_NONE, SWC_LINE, SWC_STRIP, SWC_DISK, SWC_BALL, SWC_WIRE, SWC_SOLID };

class Tree : public IVision { // SWC
public:
    Tree() : _list(0), _width(0), _height(0), _depth(0), _thickness(1.0f), _scale(1.0f), _style(SWC_LINE), _link(false) {}
    ~Tree() {}

    bool Read(const char *path);
    bool Write(const char *path) const;
    void Draw() const;
    void Show() const;

    bool IsValid() const { return !_list.empty(); }
    void SetExtent(size_t width, size_t height, size_t depth, float thickness=1.0f);
    int GetStyle() const { return _style; }
    int SetStyle(int style) { _style = style; if (_style > SWC_SOLID) _style = SWC_NONE; return _style; }
    bool GetLink() const { return _link; }
    bool SetLink(bool b) { _link = b; return _link; }
    size_t GetSize() const { return _list.size(); }
    Node GetNode(size_t id) const { return (id < _list.size()) ? _list[id] : Node(); }
    PNode GetPoint(size_t id) const; // [-1,1] -> [0,S]
    size_t AddNode(const Node &node) { _list.push_back(node); return node.Id; }
    size_t AddPoint(const PNode &point); // [0,S] -> [-1,1]
    size_t Remove() { if (!_list.empty()) _list.pop_back(); return _list.size(); }
    void Clear() { _list.clear(); }
    size_t Reduce(size_t start=0, int lower=1);
    size_t Stretch(size_t start=0);
    size_t FixupRadius(float lower, float upper);

private:
    std::vector<Node> _list;
    size_t _width, _height, _depth;
    float _thickness, _scale;
    int _style; // SWC_STYLE
    bool _link;
};
