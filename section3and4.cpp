#include <iostream>
#include <chrono>
#include <algorithm>
#include <iterator>
#include <optional>
#include <cstdint>

#include <date/date.h>

#include "section3and4.h"
#include "descriptor.h"
#include "code_table.h"

Section3And4::Section3And4(CodeTable table) :
	m_codeTable{table}
{}

void Section3And4::setCodeTable(CodeTable table)
{
	m_codeTable = std::move(table);
}

std::istream& operator>>(std::istream& is, Section3And4& s)
{
	auto beginningOfSection3 = is.tellg();

	char buffer[3];
	// Size of section 3
	is.read(buffer, 3);
	if (is) {
		s.m_sizeSection3 = (uint8_t(buffer[0]) << 16) + (uint8_t(buffer[1]) << 8) + uint8_t(buffer[2]);
	}
	if (s.m_sizeSection3 < 5) {
		is.setstate(std::ios::failbit);
		return is;
	}

	// reserved octet
	is.read(buffer, 1);

	// number of data subsets
	is.read(buffer, 2);
	if (is) {
		s.m_numberOfSubsets = (uint8_t(buffer[0]) << 8) + uint8_t(buffer[1]);
	}

	// flags
	is.read(buffer, 1);
	if (is) {
		s.m_observedData = static_cast<uint8_t>(buffer[0]) & 0b1000'0000;
		s.m_compressedData = static_cast<uint8_t>(buffer[0]) & 0b0100'0000;
	}

	// descriptors
	s.m_descriptors.resize((s.m_sizeSection3 - 7) / 2);
	std::copy_n(std::istream_iterator<Descriptor>(is),
		    s.m_descriptors.size(),
		    std::begin(s.m_descriptors));


	is.seekg(beginningOfSection3);
	// There can be padding so jump to the real beginning of section 4 using the
	// size of section 3 as offset
	is.seekg(s.m_sizeSection3, std::ios_base::cur);

	// Size of section 4
	is.read(buffer, 3);
	if (is) {
		s.m_sizeSection4 = (uint8_t(buffer[0]) << 16) + (uint8_t(buffer[1]) << 8) + uint8_t(buffer[2]);
	}
	if (s.m_sizeSection4 < 4) {
		is.setstate(std::ios::failbit);
		return is;
	}

	// reserved octet
	is.read(buffer, 1);

	// slurp all descriptors
	s.m_section4Data.resize(s.m_sizeSection4 - 4);
	is.read(reinterpret_cast<char*>(s.m_section4Data.data()), s.m_section4Data.size());

	return is;
}

std::ostream& operator<<(std::ostream& os, const Section3And4& s)
{
	std::println(os, "size section 3: {}", s.m_sizeSection3);
	std::println(os, "size section 4: {}", s.m_sizeSection4);
	std::println(os, "observed data? : {}", s.m_observedData);
	std::println(os, "compressed data? : {}", s.m_compressedData);
	std::println(os, "number of subsets: {}", s.m_numberOfSubsets);
	std::println(os, "number of descriptors: {}", s.m_descriptors.size());
	for (auto&& d : s.m_descriptors) {
		os << "\t" << d;
		auto c = s.m_codeTable.getCode(d.getCode());
		if (c) {
			os << " " << c->description;
		}
		os << "\n";
	}

	return os;
}
