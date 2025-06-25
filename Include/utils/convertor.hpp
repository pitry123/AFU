#pragma once

#include <iostream>
#include <vector>


// Schwartz Liran 
// Convertors : util hpp file to convert between any obj
namespace afu
{
	
	namespace covertor
	{
		// Vectors , Arrayes 
		template <typename T, std::size_t ROWS, std::size_t COLS>
		static std::vector<std::vector<T>> arrayToVector(T(&array)[ROWS][COLS]) 
		{
			std::vector<std::vector<T>> vec;
			for (std::size_t i = 0; i < ROWS; ++i) {
				std::vector<T> row(array[i], array[i] + COLS);
				vec.push_back(std::move(row));
			}
			return vec;
		}
		
		
		namespace string
		{
			static std::vector<std::string> split(const std::string& str, char delimiter)
			{
				std::vector<std::string> tokens;
				std::istringstream stream(str);
				std::string token;

				while (std::getline(stream, token, delimiter)) {
					tokens.push_back(token);
				}
				return tokens;
			}
		}
	}
	
	
}



