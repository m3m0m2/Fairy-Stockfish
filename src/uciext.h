#ifndef UCIEXT_H_INCLUDED
#define UCIEXT_H_INCLUDED

#include <list>

#include "apiutil.h"

namespace Stockfish::UCIExt {

Move parseMove(Position& pos, std::string& str);
std::string candidateMoves(Position& pos, Stockfish::Notation notation=NOTATION_SAN);
std::string variationLine(Position& pos, const std::vector<Move>& moves);
std::vector<std::string> first2Moves(Position& pos, const std::vector<Move>& moves);
std::string normalizeLine(Position& pos, const std::list<std::string>& strMoves);

} // namespace Stockfish::UCIExt

#endif // #ifndef UCIEXT_H_INCLUDED
