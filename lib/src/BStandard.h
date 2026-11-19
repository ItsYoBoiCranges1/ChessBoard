#ifndef BSTANDARD_H
#define BSTANDARD_H

struct Point{
    int x;
    int y;

    Point();
    Point(int _x, int _y);
    bool operator==(const Point& other) const;
    bool operator!=(const Point& other) const;
    bool inBounds();
    void print();
};

#endif  