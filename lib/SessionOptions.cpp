#include "SessionOptions.hpp"
#include <iomanip>
#include <iostream>

using namespace Clara;

std::ostream &Clara::operator<<(std::ostream &os,
                                const SessionOptions &options) {
  os << "{SessionOptions at " << std::hex << &options << '}';
  return os;
}
