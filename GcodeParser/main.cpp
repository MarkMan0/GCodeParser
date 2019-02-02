// GcodeParser.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"
#include <iostream>
#include <string>
#include <ios>
#include <fstream>
#include <algorithm>

#include "GcodeParser.h"



int main(int argc, char *argv[])
{
	using namespace std;

	if (argc < 2) {
		//no cmd argument
		cout << "enter the filename as the first argument" << endl;
		return 1;
	}

	string filename = argv[1];		//get the cmd argument


	string outName = filename.substr(0, filename.find('.')) + "_PARSED" + filename.substr(filename.find('.'));

	cout << "Reading file: " << filename << endl;
	cout << "Output file: " << outName << endl;


	GcodeParser parser(filename);

	parser.heatBedAndExt();
	parser.layersAndTimeToLcd();
	parser.optimizeAcceleration();
	parser.writeToFile(outName);

	
}










