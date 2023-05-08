#pragma once
class Test
{ 
public:
    static int id;
    static int getID(){
        id++;
        return id;
    }
};
int Test::id = 0;


