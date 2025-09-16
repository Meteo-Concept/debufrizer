#include <iostream>
#include <cstdint>

#include "section0.h"

std::istream& operator>>(std::istream& is, Section0& s)
{
	is.read(s.m_content.data(), 4);
	if (s.m_content != "BUFR") {
		is.setstate(std::ios_base::failbit);
		return is;
	}

	char buffer[3];
	is.read(buffer, 3);
	if (is) {
		s.m_size = (uint8_t(buffer[0]) << 16) + (uint8_t(buffer[1]) << 8) + uint8_t(buffer[2]);
	}

	is.read(buffer, 1);
	if (is) {
		s.m_version = static_cast<uint8_t>(buffer[0]);
	}

	return is;
}

std::ostream& operator<<(std::ostream& os, const Section0& s)
{
	os << s.m_content << "\n"
	   << "total size: " << s.m_size << "\n"
	   << "version: " << int(s.m_version) << "\n";
	return os;
}
