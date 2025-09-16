#include <iostream>
#include <fstream>
#include <string>

#include "section0.h"
#include "section1.h"
#include "section3and4.h"

int main(int argc, char** argv)
{
	if (argc <= 1 || argc > 3) {
		std::println(std::cerr, "Usage: {} <BUFR Météo France file> [<output file>]", argv[0]);
		return 1;
	}
	std::string inputFile = std::string{argv[1]};
	std::ifstream is{inputFile, std::ios::binary};

	std::cerr << std::boolalpha << "Parser is ready: " << bool(is) << std::endl;

	Section0 section0;
	is >> section0;

	std::cout << section0 << std::endl;
	std::cerr << std::boolalpha << "Parsing is going alright after section 0: " << bool(is) << std::endl;

	Section1 section1;
	is >> section1;

	std::cout << section1 << std::endl;
	std::cerr << std::boolalpha << "Parsing is going alright after section 1: " << bool(is) << std::endl;

	if (!section1.hasSection2()) {
		std::cerr << "No section 2" << std::endl;
	}

	Section3And4 section3And4(
		CodeTable(section1.getMasterTable(), section1.getMasterTableVersion(),
			section1.getOrigCentre(), section1.getLocalTableVersion())
	);
	is >> section3And4;

	std::cout << section3And4 << std::endl;
	std::cerr << std::boolalpha << "Parsing is going alright after section 3: " << bool(is) << std::endl;

	std::string outputFile = inputFile + ".tif";
	if (argc == 3) {
		outputFile = argv[2];
	}
	section3And4.buildTiff(outputFile);
}
