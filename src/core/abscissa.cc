#include "abscissa.hh"

namespace Tempus
{

Abscissa::Abscissa() : d_(0) {}

Abscissa::Abscissa( float v )
{
    BOOST_ASSERT( (v>=0.0)&&(v<=1.0) );
    d_ = uint16_t(v * 65535);
}

Abscissa::operator float() const
{
    return float(d_) / 65535.0;
}


std::istream& operator>>( std::istream& istr, Abscissa& a )
{
    double d;
    istr >> d;
    a = d;
    return istr;
}
}
