#include "QuaryException.h"

QuaryException::QuaryException() :m_err("\nquary failed!\n") {}

const char* QuaryException::what() const throw() {
	return m_err.c_str();
}