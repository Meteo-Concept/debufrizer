#ifndef DEBUFRIZER_DESCRIPTOR_H
#define DEBUFRIZER_DESCRIPTOR_H


class Descriptor
{
private:
	int m_f = -1;
	int m_x = -1;
	int m_y = -1;

public:
	Descriptor() = default;
	Descriptor(int f, int x, int y);
	// not explicit on purpose
	Descriptor(const std::string& s);
	inline int getF() const { return m_f; }
	inline int getX() const { return m_x; }
	inline int getY() const { return m_y; }
	inline int getCode() const { return m_f * 100000 + m_x * 1000 + m_y; }
	friend std::istream& operator>>(std::istream& is, Descriptor& d);
	friend std::ostream& operator<<(std::ostream& os, const Descriptor& d);
};


#endif //DEBUFRIZER_DESCRIPTOR_H
