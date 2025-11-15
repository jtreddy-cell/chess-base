#pragma once

#include "Game.h"
#include "Grid.h"
#include <vector>

constexpr int pieceSize = 80;

enum ChessPiece
{
    NoPiece,
    Pawn,
    Knight,
    Bishop,
    Rook,
    Queen,
    King
};

class Chess : public Game
{
public:
    Chess();
    ~Chess();

    void setUpBoard() override;

    bool canBitMoveFrom(Bit &bit, BitHolder &src) override;
    bool canBitMoveFromTo(Bit &bit, BitHolder &src, BitHolder &dst) override;
    bool actionForEmptyHolder(BitHolder &holder) override;

    void stopGame() override;

    Player *checkForWinner() override;
    bool checkForDraw() override;

    std::string initialStateString() override;
    std::string stateString() override;
    void setStateString(const std::string &s) override;

    Grid* getGrid() override { return _grid; }
    
    // Public move generator interface
    void makeRandomMoveForCurrentPlayer() { makeRandomMove(getCurrentPlayer()->playerNumber()); }
    std::vector<std::pair<int,int>> getAllValidMovesForCurrentPlayer();

    // Move counter for debugging
    int getMoveCount() const { return _moveCount; }
    
    // Board rebuild methods
    void rebuildBoardFromFEN();
    void bitMovedFromTo(Bit &bit, BitHolder &src, BitHolder &dst) override;

private:
    Bit* PieceForPlayer(const int playerNumber, ChessPiece piece);
    Player* ownerAt(int x, int y) const;
    void FENtoBoard(const std::string& fen);
    char pieceNotation(int x, int y) const;
    std::string generateFENFromBoard();
    
    // Move generator methods
    struct Move {
        int fromX, fromY, toX, toY;
        ChessSquare* fromSquare;
        ChessSquare* toSquare;
        Bit* piece;
        
        Move(int fx, int fy, int tx, int ty, ChessSquare* fs, ChessSquare* ts, Bit* p)
            : fromX(fx), fromY(fy), toX(tx), toY(ty), fromSquare(fs), toSquare(ts), piece(p) {}
    };
    
    std::vector<Move> generateAllMoves(int playerNumber);
    std::vector<Move> generatePawnMoves(int x, int y, Bit* piece);
    std::vector<Move> generateKnightMoves(int x, int y, Bit* piece);
    std::vector<Move> generateKingMoves(int x, int y, Bit* piece);
    bool isValidMove(int playerNumber, int fromX, int fromY, int toX, int toY);
    void makeRandomMove(int playerNumber);

    Grid* _grid;
    
    // Move counter for debugging
    int _moveCount = 0;
};