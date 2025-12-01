#include "Chess.h"
#include <limits>
#include <cmath>
#include <cctype>
#include <iostream>

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
    
    ChessSquare* square = _grid->getSquare(x, y);
    if (!square) {
        return '0';  // Empty square if grid position is invalid
    }
    
    Bit *bit = square->bit();
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

std::string Chess::generateFENFromBoard() {
    std::string fen = "";
    
    for (int y = 7; y >= 0; y--) {
        int emptyCount = 0;
        
        for (int x = 0; x < 8; x++) {
            ChessSquare* square = _grid->getSquare(x, y);
            if (square && square->bit()) {
                // If we have empty squares before this piece, add the count
                if (emptyCount > 0) {
                    fen += std::to_string(emptyCount);
                    emptyCount = 0;
                }
                
                // Add piece notation
                char pieceChar = pieceNotation(x, y);
                if (pieceChar != ' ') {
                    fen += pieceChar;
                }
            } else {
                emptyCount++;
            }
        }
        
        // Add remaining empty squares at the end of the rank
        if (emptyCount > 0) {
            fen += std::to_string(emptyCount);
        }
        
        // Add rank separator except for the last rank
        if (y > 0) {
            fen += "/";
        }
    }
    
    return fen;
}

void Chess::rebuildBoardFromFEN() {
    // Use the same format as stateString() for consistency
    std::string currentState = stateString();
    
    // Clear existing pieces first
    _grid->forEachSquare([](ChessSquare* square, int x, int y) {
        square->destroyBit();
    });
    
    // Rebuild from the state string (64-character format)
    for (int y = 0; y < 8; y++) {
        for (int x = 0; x < 8; x++) {
            int index = y * 8 + x;
            if (index < currentState.length()) {
                char c = currentState[index];
                if (c != '0') {  // '0' represents empty square
                    // Determine piece type and color from character
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
                    
                    if (piece != NoPiece) {
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
                    }
                }
            }
        }
    }
}

void Chess::bitMovedFromTo(Bit &bit, BitHolder &src, BitHolder &dst) {
    // Get piece type and color
    int pieceType = bit.gameTag() & 0x7F;
    bool isWhite = (bit.gameTag() & 0x80) == 0;

    // Handle En Passant Capture
    if (pieceType == Pawn) {
        ChessSquare* dstSquare = dynamic_cast<ChessSquare*>(&dst);
        if (dstSquare && _enPassantColumn != -1 && 
            dstSquare->getColumn() == _enPassantColumn && 
            dstSquare->getRow() == _enPassantTargetRow) {
            
            ChessSquare* capturedSquare = _grid->getSquare(_enPassantColumn, _enPassantRow);
            if (capturedSquare) {
                capturedSquare->destroyBit();
            }
        }
    }

    // Update En Passant State for next turn
    int nextEnPassantCol = -1;
    int nextEnPassantRow = -1;
    int nextEnPassantTargetRow = -1;

    if (pieceType == Pawn) {
        ChessSquare* srcSquare = dynamic_cast<ChessSquare*>(&src);
        ChessSquare* dstSquare = dynamic_cast<ChessSquare*>(&dst);
        if (srcSquare && dstSquare && abs(srcSquare->getRow() - dstSquare->getRow()) == 2) {
            nextEnPassantCol = dstSquare->getColumn();
            nextEnPassantRow = dstSquare->getRow();
            nextEnPassantTargetRow = (srcSquare->getRow() + dstSquare->getRow()) / 2;
        }
    }

    _enPassantColumn = nextEnPassantCol;
    _enPassantRow = nextEnPassantRow;
    _enPassantTargetRow = nextEnPassantTargetRow;
    
    // Track king and rook movements
    if (pieceType == King) {
        if (isWhite) {
            _whiteKingMoved = true;
            _castlingRights[0] = false; // White kingside
            _castlingRights[1] = false; // White queenside
        } else {
            _blackKingMoved = true;
            _castlingRights[2] = false; // Black kingside
            _castlingRights[3] = false; // Black queenside
        }
    } 
    // Track rook movements
    else if (pieceType == Rook) {
        ChessSquare* srcSquare = dynamic_cast<ChessSquare*>(&src);
        if (srcSquare) {
            int x = srcSquare->getColumn();
            int y = srcSquare->getRow();
            
            if (isWhite) {
                if (x == 0 && y == 0) _whiteQueensideRookMoved = true;
                else if (x == 7 && y == 0) _whiteKingsideRookMoved = true;
            } else {
                if (x == 0 && y == 7) _blackQueensideRookMoved = true;
                else if (x == 7 && y == 7) _blackKingsideRookMoved = true;
            }
            
            // Update castling rights if a rook moves from its starting position
            if (y == (isWhite ? 0 : 7)) {
                if (x == 0) _castlingRights[isWhite ? 1 : 3] = false; // Queenside
                else if (x == 7) _castlingRights[isWhite ? 0 : 2] = false; // Kingside
            }
        }
    }
    
    // Handle Pawn Promotion
    if (pieceType == Pawn) {
        ChessSquare* dstSquare = dynamic_cast<ChessSquare*>(&dst);
        if (dstSquare) {
            int row = dstSquare->getRow();
            // White promotes at row 7, Black at row 0
            if ((isWhite && row == 7) || (!isWhite && row == 0)) {
                // Promote to Queen
                int newTag = (isWhite ? 0 : 128) + Queen;
                bit.setGameTag(newTag);
                
                std::string spritePath = std::string("") + (isWhite ? "w_" : "b_") + "queen.png";
                bit.LoadTextureFromFile(spritePath.c_str());
                bit.setSize(pieceSize, pieceSize);
            }
        }
    }

    // Handle castling - move the rook
    ChessSquare* srcSquare = dynamic_cast<ChessSquare*>(&src);
    ChessSquare* dstSquare = dynamic_cast<ChessSquare*>(&dst);
    
    if (!srcSquare || !dstSquare) {
        _moveCount++;
        endTurn();
        return;
    }
    
    if (pieceType == King && 
        abs(srcSquare->getColumn() - dstSquare->getColumn()) == 2) {
        // This is a castling move
        bool kingside = (dstSquare->getColumn() > srcSquare->getColumn());
        int row = isWhite ? 0 : 7;
        
        // Get rook squares
        int rookFromCol = kingside ? 7 : 0;
        int rookToCol = kingside ? 5 : 3;
        
        ChessSquare* rookSquare = _grid->getSquare(rookFromCol, row);
        ChessSquare* newRookSquare = _grid->getSquare(rookToCol, row);
        
        // Verify all squares exist and rook is present
        if (rookSquare && newRookSquare && rookSquare->bit()) {
            Bit* rook = rookSquare->bit();
            
            // Verify it's actually a rook
            int rookType = rook->gameTag() & 0x7F;
            if (rookType == Rook) {
                // Move the rook safely using releaseBit() to avoid deletion
                rookSquare->releaseBit(); 
                newRookSquare->setBit(rook);
                rook->setPosition(newRookSquare->getPosition());
            }
        }
    }
    
    // Increment move count for manual moves too
    _moveCount++;
    
    // End the turn
    endTurn();
}

bool Chess::actionForEmptyHolder(BitHolder &holder)
{
    return false;
}

bool Chess::canBitMoveFrom(Bit &bit, BitHolder &src)
{
    // need to implement friendly/unfriendly in bit so for now this hack
    int currentPlayer = getCurrentPlayer()->playerNumber();
    int pieceColor = bit.gameTag() & 128;  // 0 for white, 128 for black
    
    // Player 0 (white) moves pieces with gameTag < 128
    // Player 1 (black) moves pieces with gameTag >= 128
    if (currentPlayer == 0 && pieceColor == 0) return true;   // White player moving white piece
    if (currentPlayer == 1 && pieceColor == 128) return true;  // Black player moving black piece
    return false;
}

bool Chess::canBitMoveFromTo(Bit &bit, BitHolder &src, BitHolder &dst)
{
    if (!canBitMoveFromToPseudo(bit, src, dst)) {
        return false;
    }

    ChessSquare* srcSquare = dynamic_cast<ChessSquare*>(&src);
    ChessSquare* dstSquare = dynamic_cast<ChessSquare*>(&dst);
    if (!srcSquare || !dstSquare) return false;

    // Check if the move puts/leaves the king in check
    // Note: We need to pass the player number of the moving piece
    int playerNumber = (bit.gameTag() & 128) ? 1 : 0;
    if (wouldKingBeInCheckAfterMove(srcSquare->getColumn(), srcSquare->getRow(), 
                                    dstSquare->getColumn(), dstSquare->getRow(), playerNumber)) {
        return false;
    }

    return true;
}

bool Chess::canBitMoveFromToPseudo(Bit &bit, BitHolder &src, BitHolder &dst)
{
    // Get source and destination positions
    ChessSquare* srcSquare = dynamic_cast<ChessSquare*>(&src);
    ChessSquare* dstSquare = dynamic_cast<ChessSquare*>(&dst);
    
    if (!srcSquare || !dstSquare) {
        return false;
    }

    int srcX = srcSquare->getColumn();
    int srcY = srcSquare->getRow();
    int dstX = dstSquare->getColumn();
    int dstY = dstSquare->getRow();
    
    // Get the piece type and color from the game tag
    int pieceType = bit.gameTag() & 0x7F;  // Lower 7 bits for piece type
    bool isWhite = (bit.gameTag() & 0x80) == 0;  // Check if it's white's piece
    int direction = isWhite ? 1 : -1;  // White moves up (increasing y), black moves down (decreasing y)
    
    // Check if the destination has a friendly piece
    if (dst.bit() && dst.bit()->getOwner() == bit.getOwner()) {
        return false;
    }

    // Handle movement based on piece type
    switch (pieceType) {
        case Pawn: {
            // Pawns move forward one square, or two squares from starting position
            int startRank = isWhite ? 1 : 6;
            int forwardOne = srcY + direction;
            int forwardTwo = srcY + 2 * direction;
            
            // Check for forward move (no capture)
            if (srcX == dstX && !dst.bit()) {
                // Move forward one square
                if (dstY == forwardOne) {
                    return true;
                }
                // Move forward two squares from starting position
                if (srcY == startRank && dstY == forwardTwo && !_grid->getSquare(dstX, forwardOne)->bit()) {
                    return true;
                }
            }
            // Check for capture (diagonal)
            else if (abs(dstX - srcX) == 1 && dstY == forwardOne) {
                // Can capture if there's an opponent's piece at the destination
                if (dst.bit() && dst.bit()->getOwner() != bit.getOwner()) {
                    return true;
                }
                // En Passant
                if (_enPassantColumn != -1 && dstX == _enPassantColumn && dstY == _enPassantTargetRow) {
                    return true;
                }
            }
            return false;
        }
        
        case Knight: {
            // Knight moves in L-shape: 2 in one direction, then 1 perpendicular
            int dx = abs(dstX - srcX);
            int dy = abs(dstY - srcY);
            return (dx == 2 && dy == 1) || (dx == 1 && dy == 2);
        }
        
        case King: {
            // Check for castling
            int dx = dstX - srcX;
            int dy = dstY - srcY;
            
            // Normal king move (one square in any direction)
            if (abs(dx) <= 1 && abs(dy) <= 1) {
                return true;
            }
            
            // Check for castling (2 squares horizontally)
            if (abs(dx) == 2 && dy == 0) {
                bool kingside = (dx > 0);
                int row = isWhite ? 0 : 7;
                int playerIndex = isWhite ? 0 : 2;
                
                // Check if castling is allowed for this side
                if (!_castlingRights[playerIndex + (kingside ? 0 : 1)]) {
                    return false;
                }
                
                // Check if squares between king and rook are empty
                int checkStart = kingside ? srcX + 1 : 1;
                int checkEnd = kingside ? dstX : srcX;
                
                for (int x = checkStart; x < checkEnd; x++) {
                    ChessSquare* square = _grid->getSquare(x, row);
                    if (!square || square->bit()) {
                        return false;  // Path blocked or invalid
                    }
                }
                
                // NOTE: We don't check isSquareUnderAttack here to avoid infinite recursion
                // The attack checks are done in generateKingMoves instead
                return true;
            }
            
            return false;
        }
        
        case Rook: {
            // Rook moves horizontally or vertically any number of squares
            if (srcX != dstX && srcY != dstY) {
                return false; // Must be on the same rank or file
            }
            
            int stepX = (dstX > srcX) ? 1 : (dstX < srcX) ? -1 : 0;
            int stepY = (dstY > srcY) ? 1 : (dstY < srcY) ? -1 : 0;
            
            // Check if the path is clear
            int x = srcX + stepX;
            int y = srcY + stepY;
            while (x != dstX || y != dstY) {
                if (_grid->getSquare(x, y)->bit()) {
                    return false; // Path is blocked
                }
                x += stepX;
                y += stepY;
            }
            
            return true;
        }
        
        case Bishop: {
            // Bishop moves diagonally any number of squares
            int dx = abs(dstX - srcX);
            int dy = abs(dstY - srcY);
            
            // Must move diagonally (equal x and y distance)
            if (dx != dy) {
                return false;
            }
            
            int stepX = (dstX > srcX) ? 1 : -1;
            int stepY = (dstY > srcY) ? 1 : -1;
            
            // Check if the path is clear
            int x = srcX + stepX;
            int y = srcY + stepY;
            while (x != dstX && y != dstY) {  // Only need to check one coordinate since they change at the same rate
                if (_grid->getSquare(x, y)->bit()) {
                    return false; // Path is blocked
                }
                x += stepX;
                y += stepY;
            }
            
            return true;
        }
        
        case Queen: {
            // Queen combines rook and bishop movement
            int dx = abs(dstX - srcX);
            int dy = abs(dstY - srcY);
            
            // Must move in a straight line (same rank, file, or diagonal)
            if (dx != 0 && dy != 0 && dx != dy) {
                return false;
            }
            
            int stepX = (dstX > srcX) ? 1 : (dstX < srcX) ? -1 : 0;
            int stepY = (dstY > srcY) ? 1 : (dstY < srcY) ? -1 : 0;
            
            // Check if the path is clear
            int x = srcX + stepX;
            int y = srcY + stepY;
            while (x != dstX || y != dstY) {
                if (_grid->getSquare(x, y)->bit()) {
                    return false; // Path is blocked
                }
                x += stepX;
                y += stepY;
            }
            
            return true;
        }
        
        default:
            // Other pieces not implemented yet
            return false;
    }
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
    int currentPlayerNum = getCurrentPlayer()->playerNumber();
    
    // Check if current player has any legal moves
    auto moves = generateAllMoves(currentPlayerNum);
    if (moves.empty()) {
        if (isInCheck(currentPlayerNum)) {
            // Checkmate - Opponent wins
            return getPlayerAt(currentPlayerNum == 0 ? 1 : 0);
        } else {
            // Stalemate - Draw (handled in checkForDraw usually, but checkForWinner returns winner)
            return nullptr; 
        }
    }
    return nullptr;
}

bool Chess::checkForDraw()
{
    int currentPlayerNum = getCurrentPlayer()->playerNumber();
    auto moves = generateAllMoves(currentPlayerNum);
    if (moves.empty() && !isInCheck(currentPlayerNum)) {
        return true;
    }
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

// Move Generator Implementation
std::vector<Chess::Move> Chess::generateAllMoves(int playerNumber)
{
    std::vector<Move> allMoves;
    
    _grid->forEachSquare([&](ChessSquare* square, int x, int y) {
        Bit* piece = square->bit();
        if (!piece) return;
        
        // Check if this piece belongs to the current player
        int pieceColor = piece->gameTag() & 128;
        bool isWhitePiece = (pieceColor == 0);
        bool isPlayerWhite = (playerNumber == 0);
        
        if (isWhitePiece == isPlayerWhite) {
            int pieceType = piece->gameTag() & 0x7F;
            
            switch (pieceType) {
                case Pawn:
                    for (auto move : generatePawnMoves(x, y, piece)) {
                        allMoves.push_back(move);
                    }
                    break;
                case Knight:
                    for (auto move : generateKnightMoves(x, y, piece)) {
                        allMoves.push_back(move);
                    }
                    break;
                case King:
                    for (auto move : generateKingMoves(x, y, piece)) {
                        allMoves.push_back(move);
                    }
                    break;
                case Rook:
                    for (auto move : generateRookMoves(x, y, piece)) {
                        allMoves.push_back(move);
                    }
                    break;
                case Bishop:
                    for (auto move : generateBishopMoves(x, y, piece)) {
                        allMoves.push_back(move);
                    }
                    break;
                case Queen:
                    for (auto move : generateQueenMoves(x, y, piece)) {
                        allMoves.push_back(move);
                    }
                    break;
                default:
                    break;
            }
        }
    });
    
    // Filter out moves that leave the king in check
    std::vector<Move> validMoves;
    for (const auto& move : allMoves) {
        if (!wouldKingBeInCheckAfterMove(move.fromX, move.fromY, move.toX, move.toY, playerNumber)) {
            validMoves.push_back(move);
        }
    }
    
    return validMoves;
}

std::vector<Chess::Move> Chess::generatePawnMoves(int x, int y, Bit* piece)
{
    std::vector<Move> moves;
    bool isWhite = (piece->gameTag() & 128) == 0;
    int direction = isWhite ? 1 : -1;
    int startRank = isWhite ? 1 : 6;
    
    // Forward one square
    int newY = y + direction;
    if (newY >= 0 && newY < 8) {
        ChessSquare* targetSquare = _grid->getSquare(x, newY);
        if (targetSquare && !targetSquare->bit()) {
            moves.push_back(Move(x, y, x, newY, _grid->getSquare(x, y), targetSquare, piece));
        }
    }
    
    // Forward two squares from starting position
    if (y == startRank) {
        newY = y + 2 * direction;
        int middleY = y + direction;
        if (newY >= 0 && newY < 8) {
            ChessSquare* targetSquare = _grid->getSquare(x, newY);
            ChessSquare* middleSquare = _grid->getSquare(x, middleY);
            if (targetSquare && middleSquare && !targetSquare->bit() && !middleSquare->bit()) {
                moves.push_back(Move(x, y, x, newY, _grid->getSquare(x, y), targetSquare, piece));
            }
        }
    }
    
    // Diagonal captures
    for (int dx = -1; dx <= 1; dx += 2) {
        int newX = x + dx;
        newY = y + direction;
        if (newX >= 0 && newX < 8 && newY >= 0 && newY < 8) {
            ChessSquare* targetSquare = _grid->getSquare(newX, newY);
            if (targetSquare && targetSquare->bit() && targetSquare->bit()->getOwner() != piece->getOwner()) {
                moves.push_back(Move(x, y, newX, newY, _grid->getSquare(x, y), targetSquare, piece));
            }
        }
    }
    
    // En Passant
    if (_enPassantColumn != -1) {
        if (abs(x - _enPassantColumn) == 1) {
            int forwardOne = y + direction;
            if (forwardOne == _enPassantTargetRow) {
                ChessSquare* targetSquare = _grid->getSquare(_enPassantColumn, _enPassantTargetRow);
                if (targetSquare && !targetSquare->bit()) {
                    moves.push_back(Move(x, y, _enPassantColumn, _enPassantTargetRow, _grid->getSquare(x, y), targetSquare, piece));
                }
            }
        }
    }
    
    return moves;
}

std::vector<Chess::Move> Chess::generateKnightMoves(int x, int y, Bit* piece)
{
    std::vector<Move> moves;
    int knightMoves[8][2] = {{2,1}, {2,-1}, {-2,1}, {-2,-1}, {1,2}, {1,-2}, {-1,2}, {-1,-2}};
    
    for (auto& move : knightMoves) {
        int newX = x + move[0];
        int newY = y + move[1];
        
        if (newX >= 0 && newX < 8 && newY >= 0 && newY < 8) {
            ChessSquare* targetSquare = _grid->getSquare(newX, newY);
            if (targetSquare && (!targetSquare->bit() || targetSquare->bit()->getOwner() != piece->getOwner())) {
                moves.push_back(Move(x, y, newX, newY, _grid->getSquare(x, y), targetSquare, piece));
            }
        }
    }
    
    return moves;
}

std::vector<Chess::Move> Chess::generateKingMoves(int x, int y, Bit* piece)
{
    std::vector<Move> moves;
    
    // King moves one square in any direction
    for (int dx = -1; dx <= 1; dx++) {
        for (int dy = -1; dy <= 1; dy++) {
            if (dx == 0 && dy == 0) continue;
            
            int newX = x + dx;
            int newY = y + dy;
            
            if (newX >= 0 && newX < 8 && newY >= 0 && newY < 8) {
                ChessSquare* targetSquare = _grid->getSquare(newX, newY);
                if (targetSquare && (!targetSquare->bit() || targetSquare->bit()->getOwner() != piece->getOwner())) {
                    moves.push_back(Move(x, y, newX, newY, _grid->getSquare(x, y), targetSquare, piece));
                }
            }
        }
    }
    
    // Check for castling
    bool isWhite = (piece->gameTag() & 0x80) == 0;
    int row = isWhite ? 0 : 7;
    int playerIndex = isWhite ? 0 : 2; // 0=white, 2=black for _castlingRights
    
    // Kingside castling
    if (_castlingRights[playerIndex]) {
        bool canCastle = true;
        // Check if squares between king and rook are empty
        for (int i = x + 1; i < 7; i++) {
            if (_grid->getSquare(i, row)->bit()) {
                canCastle = false;
                break;
            }
        }
        // Check if squares king moves through are not under attack
        if (canCastle && !isSquareUnderAttack(x, y, !isWhite) && 
            !isSquareUnderAttack(x + 1, row, !isWhite) && 
            !isSquareUnderAttack(x + 2, row, !isWhite)) {
            ChessSquare* targetSquare = _grid->getSquare(x + 2, row);
            moves.push_back(Move(x, y, x + 2, row, _grid->getSquare(x, y), targetSquare, piece));
        }
    }
    
    // Queenside castling
    if (_castlingRights[playerIndex + 1]) {
        bool canCastle = true;
        // Check if squares between king and rook are empty
        for (int i = x - 1; i > 0; i--) {
            if (_grid->getSquare(i, row)->bit()) {
                canCastle = false;
                break;
            }
        }
        // Check if squares king moves through are not under attack
        if (canCastle && !isSquareUnderAttack(x, y, !isWhite) && 
            !isSquareUnderAttack(x - 1, row, !isWhite) && 
            !isSquareUnderAttack(x - 2, row, !isWhite)) {
            ChessSquare* targetSquare = _grid->getSquare(x - 2, row);
            moves.push_back(Move(x, y, x - 2, row, _grid->getSquare(x, y), targetSquare, piece));
        }
    }
    
    return moves;
}

bool Chess::isValidMove(int playerNumber, int fromX, int fromY, int toX, int toY)
{
    ChessSquare* fromSquare = _grid->getSquare(fromX, fromY);
    ChessSquare* toSquare = _grid->getSquare(toX, toY);
    
    if (!fromSquare || !toSquare || !fromSquare->bit()) {
        return false;
    }
    
    Bit* piece = fromSquare->bit();
    return canBitMoveFromTo(*piece, *fromSquare, *toSquare);
}

void Chess::makeRandomMove(int playerNumber)
{   
    _moveCount++;
    
    std::vector<Move> allMoves = generateAllMoves(playerNumber);
    
    if (allMoves.empty()) {
        return; // No moves available
    }
    
    // Select a random move
    int randomIndex = rand() % allMoves.size();
    Move selectedMove = allMoves[randomIndex];
    
    // Execute the move
    if (selectedMove.fromSquare && selectedMove.toSquare && selectedMove.piece) {
        // Remove piece from destination if there's a capture
        if (selectedMove.toSquare->bit()) {
            selectedMove.toSquare->destroyBit();
        }
        
        // Move the piece
        selectedMove.toSquare->setBit(selectedMove.piece);
        selectedMove.fromSquare->setBit(nullptr);
        
        // Update the visual position of the piece
        selectedMove.piece->setPosition(selectedMove.toSquare->getPosition());
        
        // End the turn
        endTurn();
    }
}

std::vector<std::pair<int,int>> Chess::getAllValidMovesForCurrentPlayer()
{
    std::vector<std::pair<int,int>> validMoves;
    int currentPlayer = getCurrentPlayer()->playerNumber();
    std::vector<Move> allMoves = generateAllMoves(currentPlayer);
    
    for (const auto& move : allMoves) {
        validMoves.push_back({move.toX, move.toY});
    }
    
    return validMoves;
}

bool Chess::isSquareUnderAttack(int x, int y, bool byWhite) {
    // Check if any of the opponent's pieces can move to (x,y)
    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 8; j++) {
            ChessSquare* square = _grid->getSquare(i, j);
            if (!square || !square->bit()) continue;
            
            // Only check pieces of the attacking color
            bool pieceIsWhite = (square->bit()->gameTag() & 0x80) == 0;
            if (pieceIsWhite != byWhite) continue;
            
            // Skip if it's a king (to avoid infinite recursion)
            int pieceType = square->bit()->gameTag() & 0x7F;
            if (pieceType == King) continue;
            
            // Check if this piece can attack the target square
            ChessSquare* targetSquare = _grid->getSquare(x, y);
            if (targetSquare && canBitMoveFromToPseudo(*square->bit(), *square, *targetSquare)) {
                return true;
            }
        }
    }
    return false;
}

bool Chess::wouldKingBeInCheckAfterMove(int fromX, int fromY, int toX, int toY, int playerNumber) {
    // Temporarily make the move
    ChessSquare* fromSquare = _grid->getSquare(fromX, fromY);
    ChessSquare* toSquare = _grid->getSquare(toX, toY);
    if (!fromSquare || !toSquare || !fromSquare->bit()) return true;
    
    // Save the original state
    // Use releaseBit() to prevent the pieces from being deleted by setBit()
    Bit* movingPiece = fromSquare->releaseBit();
    Bit* capturedPiece = toSquare->releaseBit();
    
    // Make the move
    toSquare->setBit(movingPiece);

    // Handle En Passant simulation
    Bit* enPassantCapturedPiece = nullptr;
    ChessSquare* enPassantCapturedSquare = nullptr;
    
    if (movingPiece && (movingPiece->gameTag() & 0x7F) == Pawn) {
        if (_enPassantColumn != -1 && toX == _enPassantColumn && toY == _enPassantTargetRow) {
             enPassantCapturedSquare = _grid->getSquare(_enPassantColumn, _enPassantRow);
             if (enPassantCapturedSquare) {
                 // Use releaseBit to avoid deleting the pawn
                 enPassantCapturedPiece = enPassantCapturedSquare->releaseBit();
             }
        }
    }
    
    // Find the king's position
    int kingX = -1, kingY = -1;
    bool kingFound = false;
    
    for (int y = 0; y < 8 && !kingFound; y++) {
        for (int x = 0; x < 8; x++) {
            ChessSquare* square = _grid->getSquare(x, y);
            if (square && square->bit()) {
                int pieceType = square->bit()->gameTag() & 0x7F;
                bool pieceIsWhite = (square->bit()->gameTag() & 0x80) == 0;
                if (pieceType == King && pieceIsWhite == (playerNumber == 0)) {
                    kingX = x;
                    kingY = y;
                    kingFound = true;
                    break;
                }
            }
        }
    }
    
    // Check if the king is in check
    bool inCheck = false;
    if (kingFound) {
        inCheck = isSquareUnderAttack(kingX, kingY, playerNumber != 0);
    }
    
    // Undo the move
    // Release movingPiece from toSquare before putting it back
    toSquare->releaseBit();
    fromSquare->setBit(movingPiece);
    toSquare->setBit(capturedPiece);

    if (enPassantCapturedSquare && enPassantCapturedPiece) {
        enPassantCapturedSquare->setBit(enPassantCapturedPiece);
    }
    
    return inCheck;
}

std::vector<Chess::Move> Chess::generateRookMoves(int x, int y, Bit* piece)
{
    std::vector<Move> moves;
    
    // Directions: right, left, up, down
    int directions[4][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
    
    for (auto& dir : directions) {
        int dx = dir[0];
        int dy = dir[1];
        int newX = x + dx;
        int newY = y + dy;
        
        // Move in the current direction until we hit the edge of the board or a piece
        while (newX >= 0 && newX < 8 && newY >= 0 && newY < 8) {
            ChessSquare* targetSquare = _grid->getSquare(newX, newY);
            if (!targetSquare) break;
            
            Bit* targetPiece = targetSquare->bit();
            
            // If the square is empty, add the move and continue
            if (!targetPiece) {
                moves.push_back(Move(x, y, newX, newY, _grid->getSquare(x, y), targetSquare, piece));
            } 
            // If the square has an opponent's piece, add the capture and stop in this direction
            else if (targetPiece->getOwner() != piece->getOwner()) {
                moves.push_back(Move(x, y, newX, newY, _grid->getSquare(x, y), targetSquare, piece));
                break;
            } 
            // If it's our own piece, stop in this direction
            else {
                break;
            }
            
            newX += dx;
            newY += dy;
        }
    }
    
    return moves;
}

std::vector<Chess::Move> Chess::generateBishopMoves(int x, int y, Bit* piece)
{
    std::vector<Move> moves;
    
    // Directions: top-right, top-left, bottom-right, bottom-left
    int directions[4][2] = {{1, 1}, {-1, 1}, {1, -1}, {-1, -1}};
    
    for (auto& dir : directions) {
        int dx = dir[0];
        int dy = dir[1];
        int newX = x + dx;
        int newY = y + dy;
        
        // Move in the current diagonal direction until we hit the edge of the board or a piece
        while (newX >= 0 && newX < 8 && newY >= 0 && newY < 8) {
            ChessSquare* targetSquare = _grid->getSquare(newX, newY);
            if (!targetSquare) break;
            
            Bit* targetPiece = targetSquare->bit();
            
            // If the square is empty, add the move and continue
            if (!targetPiece) {
                moves.push_back(Move(x, y, newX, newY, _grid->getSquare(x, y), targetSquare, piece));
            } 
            // If the square has an opponent's piece, add the capture and stop in this direction
            else if (targetPiece->getOwner() != piece->getOwner()) {
                moves.push_back(Move(x, y, newX, newY, _grid->getSquare(x, y), targetSquare, piece));
                break;
            } 
            // If it's our own piece, stop in this direction
            else {
                break;
            }
            
            newX += dx;
            newY += dy;
        }
    }
    
    return moves;
}

std::vector<Chess::Move> Chess::generateQueenMoves(int x, int y, Bit* piece)
{
    std::vector<Move> moves;
    
    // Queen moves are a combination of rook and bishop moves
    auto rookMoves = generateRookMoves(x, y, piece);
    auto bishopMoves = generateBishopMoves(x, y, piece);
    
    // Combine both move sets
    moves.insert(moves.end(), rookMoves.begin(), rookMoves.end());
    moves.insert(moves.end(), bishopMoves.begin(), bishopMoves.end());
    
    return moves;
}


bool Chess::isInCheck(int playerNumber) {
    // Find the king's position
    int kingX = -1, kingY = -1;
    bool kingFound = false;
    
    for (int y = 0; y < 8 && !kingFound; y++) {
        for (int x = 0; x < 8; x++) {
            ChessSquare* square = _grid->getSquare(x, y);
            if (square && square->bit()) {
                int pieceType = square->bit()->gameTag() & 0x7F;
                bool pieceIsWhite = (square->bit()->gameTag() & 0x80) == 0;
                if (pieceType == King && pieceIsWhite == (playerNumber == 0)) {
                    kingX = x;
                    kingY = y;
                    kingFound = true;
                    break;
                }
            }
        }
    }
    
    if (kingFound) {
        // Check if the king is under attack by the opponent
        // isSquareUnderAttack(x, y, byWhite) checks if 'byWhite' pieces are attacking
        // If we are White (player 0), we want to know if Black (byWhite=false) is attacking
        return isSquareUnderAttack(kingX, kingY, playerNumber != 0);
    }
    
    return false;
}
