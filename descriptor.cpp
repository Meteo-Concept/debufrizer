#include <iostream>
#include <string>

#include "descriptor.h"

std::istream& operator>>(std::istream& is, Descriptor& d)
{
	char buffer[2];
	is.read(buffer, 2);

	if (is) {
		d.m_f = (uint8_t(buffer[0]) & 0b1100'0000) >> 6;
		d.m_x = uint8_t(buffer[0]) & 0b0011'1111;
		d.m_y = uint8_t(buffer[1]);
	}
	return is;
}

std::ostream& operator<<(std::ostream& os, const Descriptor& d)
{
	std::print(os, "{}{:02d}{:03d}", d.m_f, d.m_x, d.m_y);
	return os;
}
