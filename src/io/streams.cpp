#include "streams.hpp"

template class srb2::io::ZlibInputStream<srb2::io::SpanStream>;
template class srb2::io::ZlibInputStream<srb2::io::VecStream>;
