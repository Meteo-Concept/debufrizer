#include <iostream>
#include <fstream>
#include <sstream>
#include <map>
#include <string>
#include <optional>

#include "code_table.h"

CodeTable::CodeTable(int globalTable, int globalTableVersion, int origCentre, int localTableVersion)
{
	parseElementTable(std::format("bufr/tables/{}/wmo/{}/element.table", globalTable, globalTableVersion));
	parseElementTable(std::format("bufr/tables/{}/local/{}/{}/0/element.table", globalTable, localTableVersion, origCentre));
}

void CodeTable::parseElementTable(const std::string& filename)
{
	std::ifstream is{filename};
	if (!is) {
		std::println(std::cerr, "Table {} unavailable", filename);
	}

	while (is) {
		std::string line;
		std::getline(is, line);
		std::cerr << line << "\n";
		if (line.empty() || line[0] == '#')
			continue;
		std::istringstream in{line};
		CodeTable::Code c;
		std::string fragment;
		std::cerr << ";" << fragment << ";";
		std::getline(in, fragment, '|');
		c.code = std::stoi(fragment);
		std::getline(in, c.name, '|');
		std::getline(in, c.type, '|');
		std::getline(in, c.description, '|');
		std::getline(in, c.unit, '|');
		std::getline(in, fragment, '|');
		c.factor = std::stoi(fragment);
		std::getline(in, fragment, '|');
		c.offset = std::stoi(fragment);
		std::getline(in, fragment, '|');
		c.size = std::stoi(fragment);
		m_codes.emplace(c.code, std::move(c));
		std::cerr << "\n";
	}
}

std::optional<CodeTable::Code> CodeTable::getCode(int code) const
{
	auto it = m_codes.find(code);
	if (it != m_codes.end())
		return it->second;
	else
		return std::nullopt;
}
