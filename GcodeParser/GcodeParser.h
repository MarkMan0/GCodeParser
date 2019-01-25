#pragma once

#include <string>
#include <vector>
#include <list>

class GcodeParser
{
private:
	std::list<std::string> *lines;
	long layerCount;
	std::string filename;

public:
	GcodeParser(std::string filename);

	void layersToLcd();
	void heatBedAndExt();
	void optimizeAcceleration();
	

	void writeToFile(std::string filename) const;



	~GcodeParser();
};

