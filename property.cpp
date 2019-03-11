#include "property.h"
#include <iostream>
#include <sstream>
#include <fstream>
#include <list>
#include <string>
#include <vector>
#include <unordered_map>
#include <boost/lexical_cast.hpp>
#include <mutex>
#include <glog/logging.h>
#include <boost/thread/recursive_mutex.hpp>
using namespace std;
vector<string> UniverseTools::split(string str, string pattern) {
    str += pattern;
    vector<string> strvev;
    int lenstr = str.size();
    for (int i = 0; i<lenstr; i++) {
        int pos = str.find(pattern, i);
        if (pos	< lenstr) {

            string findstr = str.substr(i, pos - i);
            strvev.push_back(findstr);
            i = pos + pattern.size() - 1;
        }
    }
    return strvev;
}

