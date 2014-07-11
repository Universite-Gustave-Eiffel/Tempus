#include "abscissa.hh"

namespace Tempus
{
std::istream& operator>>( std::istream& istr, Abscissa& a )
{
    double d;
    istr >> d;
    a = d;
    return istr;
}
}
