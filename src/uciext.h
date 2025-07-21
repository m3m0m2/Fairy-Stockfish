#ifndef UCIEXT_H_INCLUDED
#define UCIEXT_H_INCLUDED

#include "apiutil.h"

namespace Stockfish::UCIExt {

Move parseMove(Position& pos, std::string& str);
std::string candidateMoves(Position& pos, Stockfish::Notation notation=Stockfish::NOTATION_SAN);

} // namespace Stockfish::UCIExt

#endif // #ifndef UCIEXT_H_INCLUDED
