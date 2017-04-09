/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   npcdescription.h
 * Author: mdj
 *
 * Created on March 27, 2017, 8:47 PM
 */
#include <string>
#include <vector>
#include "dice.h"

#ifndef NPCDESCRIPTION_H
#define NPCDESCRIPTION_H

#define RED "RED";
#define BLUE "BLUE";
#define CYAN "CYAN";
#define YELLOW "YELLOW";
#define MAGENTA "MAGENTA";
#define WHITE "WHITE";
#define BLACK "BLACK";

#define SMART "SMART";
#define TELE "TELE";
#define TUNNEL "TUNNEL";
#define ERRATIC "ERRATIC";
#define PASS "PASS";
#define PICKUP "PICKUP";
#define DESTROY "DESTROY";

using namespace std;

class npcdescription {
public:

    string name;
    string description;
    string symbol;
    vector<string>colors;
    vector<string>abilities;
    dice speed;
    dice hitpoints;
    dice attack_damage;
    npcdescription(){};
    ~npcdescription(){
        delete &speed;
        delete &hitpoints;
        delete &attack_damage;
    };
};




#endif /* NPCDESCRIPTION_H */

