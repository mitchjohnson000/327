/* 
 * File:   class_data.h
 * Author: mdj
 *
 * Created on April 18, 2017, 10:42 AM
 */

#ifndef CLASS_DATA_H
#define CLASS_DATA_H
#include <string>


class class_data
{
public:
    class_data(std::string data);
    std::string getClassName(){return className;}
    int getNumOpenSeats(){return numOpenSeats;}
    int getNumOfCredits(){return numOfCredits;}

private:
    std::string className;
    int numOpenSeats;
    int numOfCredits;
    void parseJSON(std::string data);

};



#endif /* CLASS_DATA_H */

