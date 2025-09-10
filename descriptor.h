#ifndef DEBUFRIZER_DESCRIPTOR_H
#define DEBUFRIZER_DESCRIPTOR_H


class Descriptor
{
private:
	int m_f;
	int m_x;
	int m_y;

public:
	inline int getCode() const { return m_f * 100000 + m_x * 1000 + m_y; }
	friend std::istream& operator>>(std::istream& is, Descriptor& d);
	friend std::ostream& operator<<(std::ostream& os, const Descriptor& d);
};


#endif //DEBUFRIZER_DESCRIPTOR_H
