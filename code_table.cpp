#include <iostream>
#include <fstream>
#include <sstream>
#include <map>
#include <string>
#include <optional>
#include <regex>
#include <vector>

#include "code.h"
#include "code_table.h"

CodeTable::CodeTable(int globalTable, int globalTableVersion, int origCentre, int localTableVersion)
{
	parseElementTable(std::format("bufr/tables/{}/wmo/{}/element.table", globalTable, globalTableVersion));
	parseElementTable(std::format("bufr/tables/{}/local/{}/{}/0/element.table", globalTable, localTableVersion, origCentre));
	parseSequenceTable(std::format("bufr/tables/{}/wmo/{}/sequence.def", globalTable, globalTableVersion));
	parseSequenceTable(std::format("bufr/tables/{}/local/{}/{}/0/sequence.def", globalTable, localTableVersion, origCentre));
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
		if (line.empty() || line[0] == '#')
			continue;
		std::istringstream in{line};
		Code c;
		std::string fragment;
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
	}
}

void CodeTable::parseSequenceTable(const std::string& filename)
{
	std::ifstream is{filename};
	if (!is) {
		std::println(std::cerr, "Table {} unavailable", filename);
	}

	const std::regex re(R"-(\s*"(\d+)"\s*=\s*)-");
	int parsedSequences = 0;

	while (is) {
		std::string entry;
		std::getline(is, entry, '[');

		std::smatch match;
		if (std::regex_match(entry, match, re)) {
			int sequenceCode = std::stoi(match[1].str());
			std::vector<Descriptor> componentCodes;

			std::string listOfCodes;
			std::getline(is, listOfCodes, ']');
			std::istringstream in{listOfCodes};
			for (std::string c ; std::getline(in, c, ',') ;) {
				componentCodes.emplace_back(c);
			}
			m_sequences.emplace(sequenceCode, std::move(componentCodes));
			parsedSequences++;
		}
	}
}


std::optional<Code> CodeTable::getElementCode(int code) const
{
	auto it = m_codes.find(code);
	if (it != m_codes.end())
		return it->second;
	else
		return std::nullopt;
}

std::optional<std::vector<Descriptor>> CodeTable::getSequence(int code) const
{
	auto it = m_sequences.find(code);
	if (it != m_sequences.end())
		return it->second;
	else
		return std::nullopt;
}

std::vector<Descriptor> CodeTable::expand(const std::vector<Descriptor>& baseDescriptors) const
{
	std::vector<Descriptor> output;

	using SeqIt = std::vector<Descriptor>::iterator;

	std::vector<std::vector<Descriptor>> sequences;
	sequences.emplace_back(std::move(baseDescriptors));

	std::vector<std::pair<SeqIt, SeqIt>> pos;
	pos.emplace_back(std::make_pair(sequences.back().begin(), sequences.back().end()));

	while (!sequences.empty()) {
		while (pos.back().first != pos.back().second) {
			Descriptor d = *(pos.back().first);
			pos.back().first++;
			if (d.getF() < 3) {
				output.emplace_back(std::move(d));
			} else if (d.getF() == 3) {
				auto seq = getSequence(d.getCode());
				if (seq) {
					sequences.emplace_back(*seq);
				}
				//TODO else { deal with unknown sequence }
				pos.emplace_back(std::make_pair(sequences.back().begin(), sequences.back().end()));
			}
		}
		sequences.pop_back();
		pos.pop_back();
	}

	return output;
}
