#include "BStandard.h"
#include <Arduino.h>

int x = -1;
int y = -1;

  Point::Point(int _x, int _y){
    x = _x;
    y = _y;
  }

  Point::Point(){}

  bool Point::operator==(const Point& other) const{
    return (x == other.x) && (y == other.y);
  }

  bool Point::operator!=(const Point& other) const{
    return (x != other.x) || (y != other.y);
  }

  bool Point::inBounds(){
    bool inBound = false;

    if(((x >= 1) && (x <= 8)) && ( (y >= 1) && (y <= 8))){
      inBound = true;
    }

    return inBound;
  }

  void Point::print(){
    Serial.print("x: ");
    Serial.print(x);

    Serial.print(" y: ");
    Serial.print(y);
  }