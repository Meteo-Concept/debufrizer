#include <iostream>
#include <chrono>
#include <cstdint>

#include "section1.h"

using namespace std::chrono;
using namespace std::chrono_literals;

std::istream& operator>>(std::istream& is, Section1& s)
{
	char buffer[7];

	// Size of section
	is.read(buffer, 3);
	if (is) {
		s.m_size = (buffer[0] << 16) + (buffer[1] << 8) + buffer[2];
		if (s.m_size < 22 && s.m_size > 23) {
			is.setstate(std::ios_base::failbit);
		}
	}

	// master table in use
	is.read(buffer, 1);
	if (is) {
		s.m_masterTable = static_cast<uint8_t>(buffer[0]);
	}

	// originating centre
	is.read(buffer, 2);
	if (is) {
		s.m_origCentre = (unsigned(buffer[0]) << 8) + unsigned(buffer[1]);
	}

	// originating subcentre
	is.read(buffer, 2);
	if (is) {
		s.m_origSubcentre = (unsigned(buffer[0]) << 8) + unsigned(buffer[1]);
	}

	// update
	is.read(buffer, 1);
	if (is) {
		s.m_update = static_cast<uint8_t>(buffer[0]);
	}

	// optional section 2 presence flag
	is.read(buffer, 1);
	if (is) {
		s.m_optionalSection2 = static_cast<uint8_t>(buffer[0]);
	}

	// data category
	is.read(buffer, 1);
	if (is) {
		s.m_dataCategory = static_cast<uint8_t>(buffer[0]);
	}

	// data subcategory
	is.read(buffer, 1);
	if (is) {
		s.m_dataSubcategory = static_cast<uint8_t>(buffer[0]);
	}

	// local data subcategory
	is.read(buffer, 1);
	if (is) {
		s.m_localDataSubcategory = static_cast<uint8_t>(buffer[0]);
	}

	// master table version
	is.read(buffer, 1);
	if (is) {
		s.m_masterTableVersion = static_cast<uint8_t>(buffer[0]);
	}

	// local table version
	is.read(buffer, 1);
	if (is) {
		s.m_localTableVersion = static_cast<uint8_t>(buffer[0]);
	}

	// date
	is.read(buffer, 7);
	if (is) {
		s.m_time = sys_days{
			std::chrono::year_month_day{
				year{(uint8_t(buffer[0]) << 8) + uint8_t(buffer[1])}/buffer[2]/buffer[3]
			}
		} +
		hours{buffer[4]} +
		minutes{buffer[5]} +
		seconds{buffer[6]};
	}

	// optional field
	if (is && s.m_size > 22) {
		is.read(buffer, 1);
		if (is) {
			s.m_optionalLocalField = static_cast<uint8_t>(buffer[0]);
		}
	}


	return is;
}

std::ostream& operator<<(std::ostream& os, const Section1& s)
{
	std::println(os, "size: {}", s.m_size);
	std::println(os, "master table: {}", s.m_masterTable);
	std::println(os, "originating centre: {}", s.m_origCentre);
	std::println(os, "originating subcentre: {}", s.m_origSubcentre);
	std::println(os, "update: {}", s.m_update);
	std::println(os, "section 2 present: {}", s.m_optionalSection2);
	std::println(os, "data category: {}", s.m_dataCategory);
	std::println(os, "data subcategory: {}", s.m_dataSubcategory);
	std::println(os, "master table version: {}", s.m_masterTableVersion);
	std::println(os, "local table version: {}", s.m_localTableVersion);
	std::println(os, "time: {}", s.m_time);
	if (s.m_size > 22) {
		std::println(os, "local field: {}", s.m_optionalLocalField);
	}
	return os;
}
