#include <iostream>
#include <fstream>
#include <string>

#include "section0.h"
#include "section1.h"
#include "section3and4.h"

int main(int argc, char** argv)
{
	if (argc <= 1 || argc > 3) {
		std::println(std::cerr, "Usage: {} <BUFR Météo France file> [<output file prefix>]", argv[0]);
		return 1;
	}
	std::string inputFile = std::string{argv[1]};
	std::ifstream is{inputFile, std::ios::binary};

	Section0 section0;
	is >> section0;

	std::cerr << section0 << std::endl;

	Section1 section1;
	is >> section1;

	std::cerr << section1 << std::endl;

	if (!section1.hasSection2()) {
		std::cerr << "No section 2" << std::endl;
	}

	Section3And4 section3And4(
		CodeTable(section1.getMasterTable(), section1.getMasterTableVersion(),
			section1.getOrigCentre(), section1.getLocalTableVersion())
	);
	is >> section3And4;

	std::cerr << section3And4 << std::endl;

	std::string outputFile = std::format("{0:%Y}-{0:%m}-{0:%d}_{0:%H}-{0:%M}-{0:%S}.tif", section1.getTime());
	if (argc == 3) {
		outputFile = argv[2] + outputFile;
	}
	section3And4.buildTiff(outputFile);

	std::cout << outputFile << std::endl;
}
