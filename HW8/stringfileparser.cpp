#include "stringfileparser.h"
#include "npcdescription.h"
#include "dice.h"
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <iostream>
#include <cstdlib>

dice parse_dice(string line) {
    string di;
    string sides;
    string base;
    dice * dice_t = new dice();
    int i;
    for (i = 0; i < line.size(); i++) {
        if (line.at(i) == '+') {
            dice_t->base = atoi(base.c_str());
            break;
        }
        base += line.at(i);
    }
    int j;
    for (j = i + 1; j < line.size(); j++) {
        if (line.at(j) == 'd') {
            dice_t->di = atoi(di.c_str());
            break;
        }
        di += line.at(j);
    }
    int z;
    for (z = j + 1; z < line.size(); z++) {
        sides += line.at(z);
    }
    dice_t->sides = atoi(sides.c_str());

    return *dice_t;
}

void parse_monsters(const char * path) {

    vector<npcdescription *>monsters;
    npcdescription * monster;

    ifstream infile(path);
    string line;
    int still_parsing = 1;
    getline(infile, line);
    if (line.compare("RLG327 MONSTER DESCRIPTION 1")) {
        std::exit(1);
    }
    getline(infile, line);
    while (still_parsing) {
        while(line.compare("BEGIN MONSTER")){
            getline(infile,line);
        }
    
        monster = new npcdescription();
        int parse = 1;
        while (still_parsing = (bool)getline(infile, line)) {
            if(!parse) break;
            istringstream ss(line);
            while (ss >> line) {
                if (!(line.compare("SYMB"))) {
                    if(!monster->symbol.empty()){
                        parse = 0;
                        break;
                    }
                    ss >> line;
                    monster->symbol = line;
                    break;

                } else if (!(line.compare("NAME"))) {
                    if(!monster->name.empty()){
                        parse = 0;
                        break;
                    }
                    string name;
                    while (ss >> line) {
                        name += " " + line;
                    }
                    monster->name = name;
                    break;

                } else if (!(line.compare("COLOR"))&& monster->colors.empty()) {
                    if(!monster->colors.empty()){
                        parse = 0;
                        break;
                    }
                    vector<string>colors;
                    while (ss >> line) {
                        colors.push_back(line);
                    }
                    if (colors.size() == 0) {
                        break;
                    }
                    monster->colors = colors;
                    break;
                } else if (!(line.compare("DESC"))&& monster->description.empty()) {
                    if(!monster->description.empty()){
                        parse = 0;
                        break;
                    }
                    string desc;
                    while (getline(infile, line)) {
                        if (line.size() > 78) {
                            parse = 0;
                            break;
                        }
                        if (!(line.compare("."))) {
                            break;
                        }
                        desc += line + '\n';
                    }
                    monster->description = desc;
                    break;
                } else if (!(line.compare("SPEED"))&& monster->speed.empty()) {
                    if(!monster->speed.empty()){
                        parse = 0;
                        break;
                    }
                    ss >> line;
                    monster->speed = parse_dice(line);
                    break;
                } else if (!(line.compare("DAM"))&& monster->attack_damage.empty()) {
                    if(!monster->attack_damage.empty()){
                        parse = 0;
                        break;
                    }
                    ss >> line;
                    monster->attack_damage = parse_dice(line);
                    break;
                } else if (!(line.compare("HP"))&& monster->hitpoints.empty()) {
                    if(!monster->hitpoints.empty()){
                        parse = 0;
                        break;
                    }
                    ss >> line;
                    monster->hitpoints = parse_dice(line);
                    break;
                } else if (!(line.compare("ABIL"))&& monster->abilities.empty()) {
                    if(!monster->abilities.empty()){
                        parse = 0;
                        break;
                    }
                    vector<string>abilities;
                    while (ss >> line) {
                        abilities.push_back(line);
                    }
                    if (abilities.size() == 0) {
                        break;
                    }
                    monster->abilities = abilities;
                    break;
                } else if(!(line.compare("END"))){
                    if(monster->abilities.empty()||monster->attack_damage.empty()||monster->colors.empty()||monster->description.empty()||monster->hitpoints.empty()||monster->name.empty()||monster->speed.empty()||monster->symbol.empty()){
                        //one of the categories didn't get parsed 
                        parse = 0;
                        break;
                    }
                    monsters.push_back(monster);
                    parse = 0;
                    break;
                }else{
                    break;
                }
            }
        }
    }
    for(vector<npcdescription *>::iterator it = monsters.begin(); it != monsters.end(); ++it){
        cout << "Name: " + (*it)->name << endl;
        cout << "Desc: " << endl;
        cout << (*it)->description;
        cout << "Color:";
        for(vector<string>::iterator it2 = (*it)->colors.begin(); it2 != (*it)->colors.end(); it2++){
            cout << " " << (*it2);
        }
        cout << endl;
        cout << "Speed: " << "Base: " << (*it)->speed.base << " Dice: " << (*it)->speed.di <<  " Sides: " << (*it)->speed.sides << endl;
        
        cout << "Abilities: ";
        for(vector<string>::iterator it2 = (*it)->abilities.begin(); it2 != (*it)->abilities.end(); it2++){
            cout << " " << (*it2);
        }
        cout << endl;
        
        cout << "Hitpoints: ";
        cout << "Base: " << (*it)->hitpoints.base;
        cout << " Dice: " << (*it)->hitpoints.di;
        cout << " Sides: " << (*it)->hitpoints.sides << endl;
                
        cout << "Attack Damage: ";
        cout << "Base: " << (*it)->attack_damage.base;
        cout << " Dice: " << (*it)->attack_damage.di;
        cout << " Sides: " << (*it)->attack_damage.sides << endl;
        
        cout << endl;
        
        
    }


}


