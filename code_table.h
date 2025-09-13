#ifndef CODE_TABLE_H
#define CODE_TABLE_H

#include <iostream>
#include <map>
#include <string>
#include <vector>
#include <optional>

#include "descriptor.h"
#include "code.h"

class CodeTable
{
private:
	std::map<int, Code> m_codes;
	std::map<int, std::vector<Descriptor>> m_sequences;

	void parseElementTable(const std::string& filename);
	void parseSequenceTable(const std::string& filename);

public:
	CodeTable() = default;
	CodeTable(int globalTable, int globalTableVersion, int origCentre, int localTableVersion);
	std::optional<Code> getElementCode(int code) const;
	std::optional<std::vector<Descriptor>> getSequence(int code) const;
	std::vector<Descriptor> expand(const std::vector<Descriptor>& baseDescriptors) const;
};

#endif
