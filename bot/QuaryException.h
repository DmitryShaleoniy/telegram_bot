#ifndef QUARYEXEPTION_H
#define QUARYEXEPTION_H

#include <exception>
#include <iostream>

class QuaryException : public std::exception {
	std::string m_err;
public:
	QuaryException();

	const char* what() const noexcept override;
};

#endif // !QUARYEXEPTION_H

