#include <iostream>
#include <string>
#include <chrono>
#include <vector>
#include <cstdint>

#include "descriptor.h"
#include "code.h"
#include "code_table.h"

class Section3And4
{
private:
	uint32_t m_sizeSection3;
	uint32_t m_sizeSection4;
	uint16_t m_numberOfSubsets;
	bool m_observedData;
	bool m_compressedData;
	std::vector<Descriptor> m_descriptors;

	std::vector<uint8_t> m_section4Data;
	std::vector<SmallCode> m_codeTree;

	CodeTable m_codeTable;

	void displayDescriptors(std::ostream& os, const std::vector<Descriptor>& descs) const;
	std::vector<SmallCode> makeCodeTree(const std::vector<Descriptor>& descs);
	void displayCodeTree(std::ostream& os, const std::vector<SmallCode>& codes) const;

public:
	inline uint32_t getSection3Size() const { return m_sizeSection3; }
	inline uint32_t getSection4Size() const { return m_sizeSection4; }
	inline uint16_t getNumberOfSubsets() const { return m_numberOfSubsets; }
	inline bool containsObservedData() const { return m_observedData; }
	inline bool isCompressed() const { return m_compressedData; }

	explicit Section3And4(CodeTable table = CodeTable{});
	void setCodeTable(CodeTable table);

	uint32_t extractValue(unsigned long pos, int size) const;

	friend std::istream& operator>>(std::istream& is, Section3And4& s);
	friend std::ostream& operator<<(std::ostream& os, const Section3And4& s);
};
