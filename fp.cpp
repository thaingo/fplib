//
//  fp.cpp
//  fplib
//
//  Created by Bao Thai Ngo on 7/08/2014.
//
//

#include "fp.h"

#include <string>
#include <sstream>
#include <iostream>

using namespace fp;

// our obj components

using name        = component<'name', std::string >;
using position    = component<'posf', float >;
using description = component<'desc', std::string >;

// our sample

#include <vector>

int main( int argc, char **argv )
{
    {
        // construct two objects dinamically
        
        int obj1 = 1, obj2 = 2;
        
        add<name>(obj1) = "obj1";
        add<position>(obj1) = 1.f;
        add<description>(obj1) = "this is our first object";
        
        copy( obj2, obj1 );
        get<name>(obj2) = "obj2";
        get<position>(obj2) ++;
        del<description>( obj1 );
        
        // check properties
        
        std::cout << "obj1 " << ( has<description>(obj1) ? "has" : "misses" ) << " description field" << std::endl;
        std::cout << "obj2 " << ( has<description>(obj2) ? "has" : "misses" ) << " description field" << std::endl;
        
        // print objects
        std::cout << dump(obj1) << std::endl;
        std::cout << dump(obj2) << std::endl;
    }
    
    return 0;
}