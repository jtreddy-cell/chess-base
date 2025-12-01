Fork or clone your this chess project into a new GitHub repository.

Add support for FEN stringsLinks to an external site. to your game setup so that instead of the current way you are setting up your game board you are setting it up with a call similar to the following call.

FENtoBoard("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR");

Your routine should be able to take just the board position portion of a FEN string, or the entire FEN string like so:

FENtoBoard("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");

(you can ignore the end for now)

This will allow you to quickly check that your castling, promotion and en passant code is working.

Added movement for the Knight, King, and Pawn. Used a switch statement to handle the different piece types and their movement patterns. Other pieces can't move at all right now as they don't have cases in the switch statement.

TODO:
Bishop movement DONE
Rook movement DONE
Queen movement DONE
Castling DONE
En passant DONE
Promotion DONE
Check/Checkmate
Negamax with ab pruning

Bishop, Rook, and Queen movement were implemented simalirly to the previous piece types. 
Castling was implemented by checking if the king and rook have moved, and if the squares between them are empty. 
En passant was implemented by checking if the pawn has moved two spaces, and if the square next to it is empty. 
Promotion was implemented by checking if the pawn has reached the opposite end of the board. Promotion just changes the piece to a queen, the player does not get to choose the piece.
 