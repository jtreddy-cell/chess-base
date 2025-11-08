#include "Chess.h"
#include <limits>
#include <cmath>
#include <cctype>

Chess::Chess()
{
    _grid = new Grid(8, 8);
}

Chess::~Chess()
{
    delete _grid;
}

char Chess::pieceNotation(int x, int y) const
{
    const char *wpieces = { "0PNBRQK" };
    const char *bpieces = { "0pnbrqk" };
    Bit *bit = _grid->getSquare(x, y)->bit();
    char notation = '0';
    if (bit) {
        notation = bit->gameTag() < 128 ? wpieces[bit->gameTag()] : bpieces[bit->gameTag()-128];
    }
    return notation;
}

Bit* Chess::PieceForPlayer(const int playerNumber, ChessPiece piece)
{
    const char* pieces[] = { "pawn.png", "knight.png", "bishop.png", "rook.png", "queen.png", "king.png" };

    Bit* bit = new Bit();
    // should possibly be cached from player class?
    const char* pieceName = pieces[piece - 1];
    std::string spritePath = std::string("") + (playerNumber == 0 ? "w_" : "b_") + pieceName;
    bit->LoadTextureFromFile(spritePath.c_str());
    bit->setOwner(getPlayerAt(playerNumber));
    bit->setSize(pieceSize, pieceSize);

    return bit;
}

void Chess::setUpBoard()
{
    setNumberOfPlayers(2);
    _gameOptions.rowX = 8;
    _gameOptions.rowY = 8;

    _grid->initializeChessSquares(pieceSize, "boardsquare.png");
    FENtoBoard("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR");

    startGame();
}

void Chess::FENtoBoard(const std::string& fen) {
    // Parse board portion of FEN (supports board-only or full FEN with spaces)
    // Clear existing pieces first
    _grid->forEachSquare([](ChessSquare* square, int x, int y) {
        square->destroyBit();
    });

    // Extract the first field (piece placement)
    std::string placement = fen;
    size_t spacePos = fen.find(' ');
    if (spacePos != std::string::npos) {
        placement = fen.substr(0, spacePos);
    }

    int y = 7; // FEN starts at rank 8 (top) -> internal y = 7
    int x = 0;
    for (size_t i = 0; i < placement.size() && y >= 0; ++i) {
        char c = placement[i];
        if (c == '/') {
            y -= 1;
            x = 0;
            continue;
        }
        if (std::isdigit(static_cast<unsigned char>(c))) {
            x += (c - '0');
            continue;
        }

        // Piece letter
        bool isWhite = std::isupper(static_cast<unsigned char>(c)) != 0;
        char lower = (char)std::tolower(static_cast<unsigned char>(c));
        ChessPiece piece = NoPiece;
        switch (lower) {
            case 'p': piece = Pawn; break;
            case 'n': piece = Knight; break;
            case 'b': piece = Bishop; break;
            case 'r': piece = Rook; break;
            case 'q': piece = Queen; break;
            case 'k': piece = King; break;
            default: piece = NoPiece; break;
        }
        if (piece != NoPiece && x >= 0 && x < 8 && y >= 0 && y < 8) {
            int playerNumber = isWhite ? 0 : 1;
            ChessSquare* sq = _grid->getSquare(x, y);
            if (sq) {
                Bit* bit = PieceForPlayer(playerNumber, piece);
                if (bit) {
                    // Set gameTag: white 0..6, black 128+0..6 (index by ChessPiece enum)
                    int tag = (isWhite ? 0 : 128) + static_cast<int>(piece);
                    bit->setGameTag(tag);
                    bit->setPosition(sq->getPosition());
                    sq->setBit(bit);
                }
            }
            x += 1;
        }
    }
}

bool Chess::actionForEmptyHolder(BitHolder &holder)
{
    return false;
}

bool Chess::canBitMoveFrom(Bit &bit, BitHolder &src)
{
    // need to implement friendly/unfriendly in bit so for now this hack
    int currentPlayer = getCurrentPlayer()->playerNumber() * 128;
    int pieceColor = bit.gameTag() & 128;
    if (pieceColor == currentPlayer) return true;
    return false;
}

bool Chess::canBitMoveFromTo(Bit &bit, BitHolder &src, BitHolder &dst)
{
    return true;
}

void Chess::stopGame()
{
    _grid->forEachSquare([](ChessSquare* square, int x, int y) {
        square->destroyBit();
    });
}

Player* Chess::ownerAt(int x, int y) const
{
    if (x < 0 || x >= 8 || y < 0 || y >= 8) {
        return nullptr;
    }

    auto square = _grid->getSquare(x, y);
    if (!square || !square->bit()) {
        return nullptr;
    }
    return square->bit()->getOwner();
}

Player* Chess::checkForWinner()
{
    return nullptr;
}

bool Chess::checkForDraw()
{
    return false;
}

std::string Chess::initialStateString()
{
    return stateString();
}

std::string Chess::stateString()
{
    std::string s;
    s.reserve(64);
    _grid->forEachSquare([&](ChessSquare* square, int x, int y) {
            s += pieceNotation( x, y );
        }
    );
    return s;}

void Chess::setStateString(const std::string &s)
{
    _grid->forEachSquare([&](ChessSquare* square, int x, int y) {
        int index = y * 8 + x;
        char playerNumber = s[index] - '0';
        if (playerNumber) {
            square->setBit(PieceForPlayer(playerNumber - 1, Pawn));
        } else {
            square->setBit(nullptr);
        }
    });
}
