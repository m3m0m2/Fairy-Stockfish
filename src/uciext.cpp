#include "uciext.h"

using namespace std;

namespace Stockfish::UCIExt {

enum class MoveTokenType { PIECE, FILE, RANK, PROMOTION, DROP };

struct MoveToken
{
    MoveTokenType type;
    unsigned value;

    MoveToken(MoveTokenType _type, unsigned _value): type(_type), value(_value)
    {}
};

enum class HasSquare { NO, SQUARE, ONLY_RANK, ONLY_FILE };

// Parsing tokens:
// file | rank | piece | = (promote) | x (capture) | @ (drop)
// ignore remarks at end: + (check) | # mate | ?,!,?!,!!,?!
struct MoveInfo
{
    vector<MoveToken> tokens;
    bool error = false;
    bool capture = false;
    bool promote = false;
    bool drop = false;

    HasSquare hasFrom = HasSquare::NO;
    Square from_sq = SQ_NONE;
    int from_int = 0;

    Square to = SQ_NONE;
    PieceType piece = NO_PIECE_TYPE;
    PieceType promotion = NO_PIECE_TYPE;
};

MoveInfo parseMoveInfo(const Position& pos, const string& origStr)
{
    // Strip number and dots before the move if given as 1...Nc6 -> Nc6
    auto lastDot = origStr.find_last_of('.');
    auto str = (lastDot == string::npos) ? origStr : origStr.substr(lastDot + 1);

    MoveInfo info;
    auto& tokens = info.tokens;
    for (unsigned char c : str)
    {
        if (c >= 'a' && c < 'x')
            tokens.emplace_back(MoveTokenType::FILE, c - 'a');
        else if (c >= '1' && c <= '9')
        {
            int value = c - '1';
            if (tokens.size() > 0 && tokens.back().type == MoveTokenType::RANK)
            {
                // For large boards over 10 ranks
                tokens.back().value *= 10;
                tokens.back().value += value;
            } else
                tokens.emplace_back(MoveTokenType::RANK, value);
        }
        else if (c >= 'A' && c <= 'Z')
        {
            string pieces = pos.piece_to_char();
            auto idx = pieces.find(c);
            if (idx == string::npos)
            {
                info.error = true;
                break;
            }
            if (info.promote)
            {
                tokens.emplace_back(MoveTokenType::PROMOTION, idx);
                info.promotion = PieceType(idx);
            }
            else
            {
                tokens.emplace_back(MoveTokenType::PIECE, idx);
                info.piece = PieceType(idx);
            }
        }
        else if (c == 'x')
            info.capture = true;
        else if (c == '=')
            info.promote = true;
        else if (c == '@')
            info.drop = true;
        // Ignore annotations: + (check), # (mate), ?!
    }
    int i = tokens.size() - 1;
    // To square
    for (; i > 0; i--)
    {
        if (tokens[i].type == MoveTokenType::RANK)
        {
            if (tokens[i-1].type == MoveTokenType::FILE)
            {
                info.to = make_square(File(tokens[i-1].value), Rank(tokens[i].value));
                i -= 2;
            } else
            {
                // Missing to file
                info.error = true;
                i--;
            }
            break;
        }
    }
    // From Square or partial file/rank
    for (; i >= 0; i--)
    {
        if (tokens[i].type == MoveTokenType::RANK)
        {
            if (i > 0 && tokens[i-1].type == MoveTokenType::FILE)
            {
                info.from_sq = make_square(File(tokens[i-1].value), Rank(tokens[i].value));
                info.hasFrom = HasSquare::SQUARE;
                i -= 2;
            } else
            {
                info.from_int = tokens[i].value;
                info.hasFrom = HasSquare::ONLY_RANK;
                i--;
            }
            break;
        }
        else if (tokens[i].type == MoveTokenType::FILE)
        {
            info.from_int = tokens[i].value;
            info.hasFrom = HasSquare::ONLY_FILE;
            i--;
            break;
        }
    }
    if (info.to == SQ_NONE) info.error = true;
    if (info.drop && info.piece == NO_PIECE_TYPE) info.error = true;
    return info;
}

// Parsing notation:
// piece [from-file] [from-rank] [x] to [=piece] [+|#]  // piece
// from-file [from-rank] [x] to [=piece] [+|#]          // pawn
// piece @ to                                           // drop
// O-O | O-O-O                                          // castling
Move parseMoveRelaxed(const Position& pos, string& str, const MoveList<LEGAL>& allMoves)
{
    if (str.length() < 2) return MOVE_NONE;

    Color turn = pos.side_to_move();

    // Castling
    if (str.find("O-O") == 0)
    {
        bool longCastle = (str.find("O-O-O") == 0);
        Square king = pos.castling_king_square(turn);

        CastlingRights rights = NO_CASTLING;
        if (turn == WHITE)
            rights = longCastle ? WHITE_OOO : WHITE_OO;
        else
            rights = longCastle ? BLACK_OOO : BLACK_OO;
        Square rook = pos.castling_rook_square(rights);
        //rook = make_square(to > from ? pos.castling_kingside_file() : pos.castling_queenside_file(), rank_of(from));

        return make<CASTLING>(king, rook);
    }

    MoveInfo info = parseMoveInfo(pos, str);
    if (info.error) return MOVE_NONE;

    if (info.drop)
    {
        return make_drop(info.to, info.piece /* pt_in_hand*/, info.piece /*pt_dropped*/);
    }

    // Detect piece type
    auto pieceType = info.piece;
    Piece pieceFrom = NO_PIECE;
    if (pieceType == NO_PIECE_TYPE)
    {
        // If from is set look up piece on board
        if (info.hasFrom == HasSquare::SQUARE)
        {
            pieceFrom = pos.piece_on(info.from_sq);
            pieceType = type_of(pieceFrom);
        }
        else
        {
            // Assume a pawn
            pieceType = PAWN;
        }
    }
    if (pieceType == NO_PIECE_TYPE) return MOVE_NONE;

    Square from = info.from_sq;

    // Find match on all legal moves
    for (const auto& m : allMoves)
    {
        if (to_sq(m) != info.to) continue;
        Square moveFrom = from_sq(m);

        if (from != SQ_NONE)
        {
            if (moveFrom != from) continue;
        }
        else if (info.hasFrom == HasSquare::ONLY_FILE)
        {
            if (file_of(moveFrom) != info.from_int) continue;
        }
        else if (info.hasFrom == HasSquare::ONLY_RANK)
        {
            if (rank_of(moveFrom) != info.from_int) continue;
        }

        Piece movingPiece = pos.piece_on(moveFrom);
        if (movingPiece == NO_PIECE) continue;
        if (color_of(movingPiece) != turn) continue;
        if (type_of(movingPiece) != pieceType) continue;

        if (promotion_type(m) == info.promotion)
            return m;
    }

  return MOVE_NONE;
}

Move parseMoveStrict(Position& pos, string sanMove, const MoveList<LEGAL>& allMoves, Stockfish::Notation notation) {
    for (const ExtMove& move : allMoves) {
        if (sanMove == Stockfish::SAN::move_to_san(pos, move, notation)) {
            return move;
        }
    }
    return MOVE_NONE;
}

Move parseMove(Position& pos, string& str)
{
    auto allMoves = MoveList<LEGAL>(pos);

    Move move = parseMoveStrict(pos, str, allMoves, Stockfish::NOTATION_SAN);
    if (move != MOVE_NONE) return move;

    return parseMoveRelaxed(pos, str, allMoves);
}

string candidateMoves(Position& pos, Stockfish::Notation notation)
{
    string r;
    auto moveList = MoveList<LEGAL>(pos);
    for (auto pmove = moveList.begin(); pmove != moveList.end(); pmove++)
    {
        auto m = *pmove;

        if (m == MOVE_NONE || m == MOVE_NULL || is_pass(m))
            continue;
        // if (notation == Stockfish::NOTATION_DEFAULT) r += " " + UCI::move(pos, m);
        auto moveStr = Stockfish::SAN::move_to_san(pos, m, notation);
        r += " " + moveStr;
    }
    return r;
}

string variationLine(Position& pos, const std::vector<Move>& moves)
{
    string r;
    StateListPtr states(new std::deque<StateInfo>(1));
    int moveIdx = 0;

    for (auto move : moves)
    {
        if (r.length() > 0) r += " ";

        auto ply = pos.game_ply();
        if (ply % 2 == 0 || moveIdx == 0)
        {
            r += std::to_string(1 + (ply / 2));
            r += (ply % 2 == 0) ? "." : "...";
        }

        r += Stockfish::SAN::move_to_san(pos, move, NOTATION_SAN);
        states->emplace_back();
        pos.do_move(move, states->back());
        moveIdx++;
    }

    for (auto it = moves.rbegin(); it != moves.rend(); it++)
    {
        pos.undo_move(*it);
    }

    return r;
}

vector<string> first2Moves(Position& pos, const vector<Move>& moves)
{
    vector<string> r;
    if (moves.size() == 0) return r;

    Move move = moves[0];
    r.push_back(Stockfish::SAN::move_to_san(pos, move, NOTATION_SAN));

    if (moves.size() == 1) return r;

    StateInfo si;
    pos.do_move(move, si);

    r.push_back(Stockfish::SAN::move_to_san(pos, moves[1], NOTATION_SAN));

    pos.undo_move(move);

    return r;
}

} // namespace Stockfish::UCIExt
