#pragma once

#include <stack>

#include "vision.h"

struct IFilter {
    virtual ~IFilter() {}
    virtual void SetParam() = 0;
    virtual void BeginUpdate() = 0;
    virtual void Update() = 0;
};

enum OP_MODE { OP_NONE, OP_MAPPING, OP_PROBING, OP_TRACING };

class Mapping : public IFilter { // LUT
public:
    Mapping() : _volume(0), _color(0), _remove(false) {}
    ~Mapping() {}

public:
    void SetVision(Volume *volume, Color *color) { _volume = volume; _color = color; }
    void SetParam();
    bool GetRemove() const { return _remove; }
    bool SetRemove(bool b) { _remove = b; return _remove; }
    void AddPoint(int index, int value);
    void RemovePoint(int index, int value);
    void BeginUpdate() { Update(); }
    void Update();
    void Reset();
    
private:
    Volume *_volume;
    Color *_color;
    bool _remove;
};

class Probing : public IFilter { // APO
public:
    Probing() : _volume(0), _soma(0), _radius(4.0f), _high(127.0f), _low(127.0f), _grads(127.0f), _local(true), _doing(false), _cancel(false) {}
    ~Probing() {}

public:
    void SetVision(Volume *volume, Soma *soma) { _volume = volume; _soma = soma; }
    void SetParam();
    void SetParam(float radius, float high, float low, float grads) { _radius = radius; _high = high; _low = low; _grads = grads; }
    bool GetLocal() const { return _local; }
    bool SetLocal(bool b) { _local = b; return _local; }
    void AddPoint(const Point &point);
    void BeginUpdate();
    static void UpdateThread(void *data) { ((Probing*)data)->Update(); }
    void Update();
    void RefinePoint(PCell &point) const;
    void CancelUpdate() { _cancel = true; }
    bool IsDoing() { return _doing; }

private:
    Volume *_volume;
    Soma *_soma;
    float _radius, _high, _low, _grads;
    bool _local;
    volatile bool _doing, _cancel;
};

class Tracing : public IFilter { // SWC
public:
    Tracing() : _volume(0), _tree(0), _dist(3.0f), _step(2.0f), _radius(16.0f), _high(127.0f), _low(127.0f), _grads(127.0), _local(true), _doing(false), _cancel(false) {}
    ~Tracing() {}

public:
    void SetVision(Volume *volume, Tree *tree) { _volume = volume; _tree = tree; }
    void SetParam(float dist, float step) { _dist = dist; _step = step; }
    void SetParam();
    void SetParam(float radius, float high, float low, float grads) { _radius = radius; _high = high; _low = low; _grads = grads; }
    void SetParam(size_t x, size_t y, size_t z, size_t radius);
    void SetParam(PNode &point, float radius) { SetParam((size_t)(point.X+0.5f), (size_t)(point.Y+0.5f), (size_t)(point.Z+0.5f), (size_t)(point.Radius*radius+0.5f)); }
    bool GetLocal() const { return _local; }
    bool SetLocal(bool b) { _local = b; return _local; }
    void AddSeed(const Point &point);
    void BeginUpdate();
    static void UpdateThread(void *data) { ((Tracing*)data)->Update(); }
    void Update();
    void Advance(PNode &point, std::vector<PNode> &children) const;
    void GetCenter(unsigned char *image, size_t dimension) const;
    void RefinePoint(PNode &parent, PNode &point) const;
    void CancelUpdate() { _cancel = true; }
    bool IsDoing() { return _doing; }

private:
    Volume *_volume;
    Tree *_tree;
    float _dist, _step, _radius, _high, _low, _grads;
    bool _local;
    std::stack<PNode> _seeds;
    volatile bool _doing, _cancel;
};
