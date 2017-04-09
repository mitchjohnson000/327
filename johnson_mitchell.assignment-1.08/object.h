/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   object.h
 * Author: mdj
 *
 * Created on April 4, 2017, 2:02 PM
 */

#ifndef OBJECT_H
#define OBJECT_H

#include <string>
# include <stdint.h>
# include <iostream>
# include "dice.h"


class object  {
public:
    std::string name, description;
    int type,x,y;
    uint32_t color;
    bool being_used;
    int hit,dodge,defence,weight,speed,attribute,value;
    dice damage;
    object(){};
    ~object(){};
    
};


#endif /* OBJECT_H */

