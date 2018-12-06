#include <iostream>
#include <string>
#include <fstream>
#include <vector>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include "blepp/csv_class.h"

 
//Source - https://thispointer.com/how-to-read-data-from-a-csv-file-in-c/

//Constructor for class
csv_class::csv_class(std::string name, std::string d)
{
	file_name = name;
	delimiter = d;
}

std::string csv_class::get_file_name(void)
{
	return file_name;
}

std::vector<std::vector<std::string>> csv_class::read_csv()
{
	
	//Open filestream
	
	std::cout << "Opening csv file: " << file_name << std::endl;

	std::ifstream csv_file(file_name);

	if( !csv_file.is_open() )
	{
		std::cout << "Error opening csv file" << std::endl;
		exit (EXIT_FAILURE);
	}
	
	//Create vector of string vectors
	std::vector<std::vector<std::string>> data;
	
	std::string line = "";
	
	//Iterate through file, line by line
	while( getline(csv_file, line) )
	{
		
		//Create string vector
		std::vector<std::string> vect;
		
		//Split line for every "delimiter" into string vector
		boost::algorithm::split(vect, line, boost::is_any_of(delimiter));
		
		//Add string vector to data
		data.push_back(vect);
		
	}
	
	//Close file
	csv_file.close();
	
	return data;
	
}

void csv_class::print_data(std::vector<std::vector<std::string>> * data)
{
	
	//Declare iterator - This iterator iterates through a vector of vectors
	std::vector<std::vector<std::string>>::iterator iV;
	
	//Declare iterator - This iterator iterates through a vector of strings
	std::vector<std::string>::iterator iS;
	
	for(iV = data->begin(); iV != data->end(); iV++)
	{
	
		//Iterate through strings vector
		//std::cout << vect << std::endl;
		for(iS = iV->begin(); iS != iV->end(); iS++)
		{
			std::cout << *iS ;
			std::cout << " ";
		}
		std::cout << "\n";
	
	}
	
}

int csv_class::write_csv(std::vector<std::string> * data)
{
	
	//Open filestream
	std::ofstream csv_file(file_name, std::ios::app);
	
	
	if( !csv_file.is_open() )
	{
		std::cout << "Error opening csv file" << std::endl;
		exit (EXIT_FAILURE);
	}
	
	//Declare iterator - This iterator iterates through a vector of strings
	std::vector<std::string>::iterator iS;
	
	for(iS = data->begin(); iS != data->end(); iS++)
	{
		//If this is not the first entry, write delimiter
		if(iS != data->begin())
		{
			csv_file << delimiter;
		}
		
		//Write data
		csv_file << *iS;
		
	}
	
	//Write newline
	csv_file << "\n";
	
	csv_file.close();
	
	return 0;
	
}
