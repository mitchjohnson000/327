/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   dice.h
 * Author: mdj
 *
 * Created on March 27, 2017, 9:18 PM
 */

#ifndef DICE_H
#define DICE_H

class dice {
public:
    int base;
    int sides;
    int di;
    bool empty(){
        if(base == -1 ||sides == -1||di == -1) return true;
        return false;
    }

    dice(){
        base = -1;
        sides = -1;
        di = -1;
        };
    ~dice(){};
};
    



#endif /* DICE_H */

