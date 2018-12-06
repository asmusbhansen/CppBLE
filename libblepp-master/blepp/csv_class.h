#ifndef _CSVCLASS_H_
#define _CSVCLASS_H_

#include <string>
#include <fstream>
#include <vector>
#include <boost/algorithm/string/split.hpp>

class csv_class  
{
 
public:
	csv_class(std::string name = "csv_file", std::string delimiter = ",");
	std::string get_file_name();
	std::vector<std::vector<std::string>> read_csv();
	int write_csv(std::vector<std::string> * data);
	void print_data(std::vector<std::vector<std::string>> * data);
	
    
private:
 
	std::string file_name;
    std::string delimiter;
    
};

#endif

