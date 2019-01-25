#include "pch.h"
#include "GcodeParser.h"
#include <iostream>
#include <fstream>
#include <ios>
#include <string>


GcodeParser::GcodeParser(std::string filename) {

	//reads the file into a queue

	using namespace std;

	this->filename = filename;

	ifstream inFile;
	
	inFile.open(filename, ios::in);

	lines = new list<string>;

	string line;

	while (getline(inFile, line)) {
		lines->push_back(line);
	}

	inFile.close();
}

void GcodeParser::layersToLcd()
{
	using namespace std;
	//find the number of the layers

	bool searching = true;

	auto currLine = lines->rbegin();

	while (searching && currLine != lines->rend())
	{
		string line = *currLine;

		if (line.find(";LAYER:") != string::npos) {
			//found it
			//first now points to the first number after the colon

			this->layerCount = stol(line.substr(strlen(";LAYER:")));
			searching = false;
		}
		++currLine;
	}



	for (auto it = lines->begin(); it != lines->end(); ++it ) {

		string line = *it;
		
		if (line.find(";LAYER:") != string::npos) {

			string layerNo = line.substr(strlen(";LAYER:"));		//extract the number
			long currLayer = stol(layerNo);

			string plusLine = "M117 Layer " + to_string(currLayer + 1);		//zero index to 1 index
			plusLine += "/" + to_string(this->layerCount);				//M117  Updates the LCD

			//Message will be something like:
			//Layer 50/100
			
			//insert the new line
			lines->insert(it, plusLine);
		}

	}


}

void GcodeParser::heatBedAndExt() {
	
	/*
	the default looks like this:
		M140 Sxx		set bed without wait
		M105			update LCD
		M190 Sxx		set bed with wait
		M104 Syyy		set hotend w/o wait
		M105			update LCD
		M109 Syyy		set hotend with wait

	and we want this:
		M140 Sxx		set bed
		M104 Syyy		set hotend
		M105			update LCD
		M190 Sxx		wait bed
		M109 Syyy		wait hotend
	*/


	//search for M140

	using namespace std;

	auto currLine = lines->begin();

	bool searchingBed = true, searchingExt = true;

	list<string>::iterator startPos, eraseStart, eraseEnd;

	int bedTemp, extTemp;

	while (currLine != lines->end() && (searchingBed || searchingExt)) {
		if (searchingBed && (*currLine).find("M140 S") != string::npos ) {
			//found it
			string strTemp = (*currLine).substr(strlen("M140 S"));
			bedTemp = stoi(strTemp);

			searchingBed = false;
			eraseStart = currLine;
			startPos = --currLine;		//startpos points one element before M140
			
			++currLine;
			cout << *startPos << endl;
		}

		if (searchingExt && (*currLine).find("M104 S") != string::npos) {
			string strTemp = (*currLine).substr(strlen("M104 S"));
			extTemp = stoi(strTemp);

			searchingExt = false;
		}
		++currLine;
	}

	eraseEnd = ++(++currLine);


	//insert the new code after startpos/before the code already there
	++startPos;
	startPos = lines->insert(startPos, { "M140 S" + to_string(bedTemp),
		"M104 S" + to_string(extTemp),
		"M105",
		"M190 S" + to_string(bedTemp),
		"M109 S" + to_string(extTemp) });

	currLine = startPos;
	advance(currLine, 7);		//points after the old wait ext temp

	lines->erase(eraseStart, eraseEnd);

}

bool beginsWith(const std::string &src, const std::string &phrase) {
	return (src.find(phrase) != std::string::npos);
}


void splitString(const std::string& src, const std::string& delim, std::vector<std::string>& cont) {
	std::string::size_type curr, prev = 0;

	curr = src.find_first_of(delim);
	while (curr != std::string::npos) {
		cont.push_back(src.substr(prev, curr - prev));
		prev = curr + 1;
		curr = src.find_first_of(delim, prev);
	}
	cont.push_back(src.substr(prev, curr - prev));
}





void GcodeParser::optimizeAcceleration()
{

	/*Cura uses the old method of setting acceleration
		Like this:
		M204 Sxxxx	- big number for travel
		M204 Syyy	- smaller number for printing moves 

		This can be optimized like this:
		
		M204 Pyyy	Txxxx
	*/

	using namespace std;

	auto line = lines->begin();
	
	bool searchFirst = true, searchSecond = false;


	int pAccel, tAccel;


	//scan for the first setting
	while (line != lines->end() && (searchFirst || searchSecond)) {
		if ((searchFirst || searchSecond) && (*line).find("M204 S") != string::npos) {
			//found the first
			string str = (*line).substr(strlen("M204 S"));
			int d = stoi(str);
			
			if (searchFirst) {
				searchFirst = false;
				searchSecond = true;
				tAccel = d;
			}
			else if (searchSecond) {
				searchSecond = false;
				pAccel = d;
			}
				 
		}
		++line;
	}

	if (pAccel > tAccel) {
		//wrong order, swap
		int helper = tAccel;
		tAccel = pAccel;
		pAccel = helper;
	}


	//insert at start
	line = lines->insert(lines->begin(), { "M204 T" + to_string(tAccel), "M204 P" + to_string(pAccel) });
	++line;
	++line;


	string opt1 = "M204 S" + to_string(pAccel);
	string opt2 = "M204 S" + to_string(tAccel);

	//start scanning after this
	while (line != lines->end()) {
		if ((*line).compare(opt1) == 0 || (*line).compare(opt2) == 0) {
			//delete line
			line = lines->erase(line);
		}
		else if (beginsWith(*line, "M204")) {
			//different acceleration, do not delete
			//find the next point of acceleration change and continue from there
			while (line != lines->end()) {
				if ((*line).compare(opt1) == 0 || (*line).compare(opt2) == 0) {
					//found the next change
					line = lines->erase(line);
					line = lines->insert(line, { "M204 T" + to_string(tAccel), "M204 P" + to_string(pAccel) });
					++line;
					++line;
					break;
				}
				else
					++line;
			}
		}
		else
			++line;
	}
	
}





void GcodeParser::writeToFile(std::string filename) const
{

	using namespace std;

	ofstream outFile;

	outFile.open(filename, ios::out);

	auto curr = lines->cbegin();
	while (curr != lines->cend()) {
		outFile << *curr << endl;
		++curr;
	}

	outFile.close();

}




GcodeParser::~GcodeParser()
{
	delete lines;
}
