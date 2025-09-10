#ifndef CODE_TABLE_H
#define CODE_TABLE_H

#include <iostream>
#include <map>
#include <string>
#include <vector>
#include <optional>

class CodeTable
{
public:
	//example: 006196|maxObliqueRange|long|Distance oblique maximal d'utilisation des donnees|m|-3|0|16
	struct Code {
		int code = -1;
		std::string name;
		std::string type;
		std::string description;
		std::string unit;
		int factor;
		int offset;
		int size;
	};
private:
	std::map<int, Code> m_codes;
	std::map<int, std::vector<int>> m_sequences;

	void parseElementTable(const std::string& filename);

public:
	CodeTable() = default;
	CodeTable(int globalTable, int globalTableVersion, int origCentre, int localTableVersion);
	std::optional<Code> getCode(int code) const;
};

#endif
