#include <string>
#include "class_data.h"
#include <jsoncpp/json/json.h>

class_data::class_data(std::string data)
{
    parseJSON(data);



}

void class_data::parseJSON(std::string data){

    Json::Value root;
    Json::Reader reader;
    bool parsesuccess = reader.parse(data.c_str(),root);
    if(!parsesuccess){
        std::cout  << "Failed to parse"
               << reader.getFormattedErrorMessages();
    }
    
    Json::Value& second = root[0];
    Json::Value& sections = second["sections"];
    Json::Value& section = sections[0];
    int openseats = section["openseats"].asInt();
    numOpenSeats = openseats;
    className = second["classTitle"].asString();
    numOfCredits = section["creditHigh"].asInt();
    
    


}

