Fork or clone your this chess project into a new GitHub repository.

Add support for FEN stringsLinks to an external site. to your game setup so that instead of the current way you are setting up your game board you are setting it up with a call similar to the following call.

FENtoBoard("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR");

Your routine should be able to take just the board position portion of a FEN string, or the entire FEN string like so:

FENtoBoard("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");

(you can ignore the end for now)

This will allow you to quickly check that your castling, promotion and en passant code is working.

Added movement for the Knight, King, and Pawn. Used a switch statement to handle the different piece types and their movement patterns. Other pieces can't move at all right now as they don't have cases in the switch statement.

TODO:
Movement for rest of pieces DONE
Special cases for castling, en passant, and promotion
Negamax with ab pruning