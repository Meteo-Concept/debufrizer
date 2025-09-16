#include <iostream>
#include <string>
#include <cstdint>

#include "descriptor.h"

#include <iomanip>

Descriptor::Descriptor(int f, int x, int y) :
	m_f{f},
	m_x{x},
	m_y{y}
{}

Descriptor::Descriptor(const std::string& s)
{
	int c = std::stoi(s);
	m_f = c / 100000;
	m_x = (c % 100000) / 1000;
	m_y = c % 1000;
}

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
	os << std::setw(1) << std::setfill('0') << d.m_f
	   << std::setw(2) << std::setfill('0') << d.m_x
	   << std::setw(3) << std::setfill('0') << d.m_y
	   << std::setw(0) << std::setfill(' ');
	return os;
}
