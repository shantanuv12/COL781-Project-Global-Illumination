#ifndef OBJECT_H

#define OBJECT_H

class Object
{
public:
    Object();
    virtual ~Object();
    virtual bool intersect();
    double radius;
};

#endif