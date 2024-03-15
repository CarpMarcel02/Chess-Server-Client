#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>

#define PORT 2908
#define MAX_CLIENTS 10
#define BOARD_SIZE 8

typedef struct
{
    char type;
    int color;
} ChessPiece;

typedef struct
{
    int startX;
    int startY;
    int endX;
    int endY;
    char promPiece;
} Move;

typedef struct
{
    ChessPiece board[BOARD_SIZE][BOARD_SIZE];
    int currentPlayer;
} ChessBoard;

typedef struct
{
    int player1;
    int player2;
    ChessBoard game;
    int currentPlayer;


} GameSession;

typedef struct thData
{
    int idThread;
    int cl;
    GameSession *gameSessionId;
} thData;

static void *treat(void *);
void initializeGame(ChessBoard *);
void printBoard(ChessBoard *);
int validateMove(ChessBoard *board, Move move, int playerColor);
int validatePawnMove(ChessBoard *board, Move move, int color);
int validateRookMove(ChessBoard *board, Move move, int color);
int validateKnightMove(ChessBoard *board, Move move, int color);
int validateBishopMove(ChessBoard *board, Move move, int color);
int validateQueenMove(ChessBoard *board, Move move, int color);
int validateKingMove(ChessBoard *board, Move move, int color);

int parseMove(char *input, Move *move);
void updateGameState(ChessBoard *game, Move move);
char *serializeGameSession(const GameSession *session);
int isKingInCheck(ChessBoard *board, int kingColor);
int attackedByPawn(ChessBoard *board, int xKing, int yKing);
int attackedByBishop(ChessBoard *board, int xKing, int yKing);
int attackedByKnight(ChessBoard *board, int xKing, int yKing);
int attackedByQueen(ChessBoard *board, int xKing, int yKing);
int attackedByRook(ChessBoard *board, int xKing, int yKing);
int canPawnBlockCheck(GameSession *gameSession, int xKing, int yKing, int opponentColor);
int canQueenBlockCheck(GameSession *gameSession, int xKing, int yKing, int opponentColor);
int canKnightBlockCheck(GameSession *gameSession, int xKing, int yKing, int opponentColor);
int canRookBlockCheck(GameSession *gameSession, int xKing, int yKing, int opponentColor);
int canBishopBlockCheck(GameSession *gameSession, int xKing, int yKing, int opponentColor);
int canKingEscapeCheck(GameSession *gameSession, int xKing, int yKing, int opponentColor);

void initializeGame(ChessBoard *board)
{
    
    for (int i = 0; i < BOARD_SIZE; i++)
    {
        for (int j = 0; j < BOARD_SIZE; j++)
        {
            board->board[i][j].type = 'L';
            board->board[i][j].color = 2; 
        }
    }

   
    for (int i = 0; i < BOARD_SIZE; i++)
    {
        board->board[1][i].type = 'P'; // Pioni albi
        board->board[1][i].color = 0;
        board->board[6][i].type = 'P'; // Pioni negri
        board->board[6][i].color = 1;
    }

    
    board->board[0][0].type = 'R';
    board->board[0][0].color = 0; // Turn alb
    board->board[0][7].type = 'R';
    board->board[0][7].color = 0; // Turn alb
    board->board[7][0].type = 'R';
    board->board[7][0].color = 1; // Turn negru
    board->board[7][7].type = 'R';
    board->board[7][7].color = 1; // Turn negru

    board->board[0][1].type = 'N'; // Cal alb
    board->board[0][1].color = 0;
    board->board[0][6].type = 'N'; // Cal alb
    board->board[0][6].color = 0;
    board->board[7][1].type = 'N'; // Cal negru
    board->board[7][1].color = 1;
    board->board[7][6].type = 'N'; // Cal negru
    board->board[7][6].color = 1;

    
    board->board[0][2].type = 'B'; // Nebun alb
    board->board[0][2].color = 0;
    board->board[0][5].type = 'B'; // Nebun alb
    board->board[0][5].color = 0;
    board->board[7][2].type = 'B'; // Nebun negru
    board->board[7][2].color = 1;
    board->board[7][5].type = 'B'; // Nebun negru
    board->board[7][5].color = 1;

    
    board->board[0][3].type = 'Q'; // Regina albă
    board->board[0][3].color = 0;
    board->board[7][3].type = 'Q'; // Regina neagră
    board->board[7][3].color = 1;

    
    board->board[0][4].type = 'K'; // Regele alb
    board->board[0][4].color = 0;
    board->board[7][4].type = 'K'; // Regele negru
    board->board[7][4].color = 1;

    // Setăm jucătorul care va începe jocul (0 pentru alb, 1 pentru negru)
    board->currentPlayer = 0;
}
void printGameSession(const GameSession *session)
{
    printf("GameSession details:\n");
    printf("Player 1: %d\n", session->player1);
    printf("Player 2: %d\n", session->player2);
    printf("Current Player: %d\n", session->game.currentPlayer);

    printf("ChessBoard:\n");
    for (int i = 0; i < BOARD_SIZE; i++)
    {
        for (int j = 0; j < BOARD_SIZE; j++)
        {
            ChessPiece piece = session->game.board[i][j];
            printf("Piece at [%d][%d] - Type: %c, Color: %d\n", i, j, piece.type, piece.color);
        }
    }
}

void sendUpdatedBoard(GameSession *session)
{
    char *serializedSession = serializeGameSession(session); 

    if (serializedSession != NULL)
    {
        
        printf(" %s ", serializedSession);
        fflush(stdout);
        send(session->player1, serializedSession, strlen(serializedSession), 0);
        

        send(session->player2, serializedSession, strlen(serializedSession), 0);

        free(serializedSession); 
    }
}
char *serializeGameSession(const GameSession *session)
{
    
    int bufferSize = 10000;                                     
    char *serializedSession = malloc(bufferSize * sizeof(char)); 
    if (serializedSession == NULL)
    {
        return NULL; 
    }

    
    int offset = 0; 
    offset += snprintf(serializedSession + offset, bufferSize - offset, "GameSession details:\n");
    offset += snprintf(serializedSession + offset, bufferSize - offset, "Player 1: %d\n", session->player1);
    offset += snprintf(serializedSession + offset, bufferSize - offset, "Player 2: %d\n", session->player2);
    offset += snprintf(serializedSession + offset, bufferSize - offset, "Current Player: %d\n", session->game.currentPlayer);

    offset += snprintf(serializedSession + offset, bufferSize - offset, "ChessBoard:\n");
    for (int i = 0; i < BOARD_SIZE; i++)
    {
        for (int j = 0; j < BOARD_SIZE; j++)
        {
            ChessPiece piece = session->game.board[i][j];
            offset += snprintf(serializedSession + offset, bufferSize - offset, "Piece at [%d][%d] - Type: %c, Color: %d\n", i, j, piece.type, piece.color);
        }
    }
    return serializedSession; // Return the serialized string
}
// Afișarea tablei de șah (util pentru debug)
void printBoard(ChessBoard *board)
{
    printf("  a b c d e f g h\n"); 
    for (int i = 0; i < BOARD_SIZE; i++)
    {
        printf("%d ", i + 1); 
        for (int j = 0; j < BOARD_SIZE; j++)
        {
            char piece = board->board[i][j].type;
            if (piece == 'L')
            {
                printf(". "); 
            }
            else
            {
                printf("%c ", piece); 
            }
        }
        printf("%d\n", i + 1); 
    }
    printf("  a b c d e f g h\n"); 
}
void makeMove(ChessBoard *game, Move move)
{
    
    ChessPiece movingPiece = game->board[move.startX][move.startY];
    game->board[move.endX][move.endY] = movingPiece;  
    game->board[move.startX][move.startY].type = 'L'; 

    
}
void updateGameState(ChessBoard *game, Move move)
{
    
    ChessPiece movingPiece = game->board[move.startX][move.startY];

    game->board[move.endX][move.endY] = movingPiece;  // Mutam piesa
    game->board[move.startX][move.startY].type = 'L'; 
    game->board[move.startX][move.startY].color = 2;

    int opponentColor = 1 - game->currentPlayer;

    game->currentPlayer = (game->currentPlayer == 0) ? 1 : 0;
}
static void *treat(void *arg)
{
    thData tdL = *((thData *)arg);

    pthread_detach(pthread_self());


    while (1)
    {
        // Ne asiguram ca muta player-ul care trebuie
        if (tdL.gameSessionId->currentPlayer == tdL.idThread % 2)
        {
            printf("[thread %d] Waiting for move...\n", tdL.idThread);
            fflush(stdout);

            int validMoveMade = 0;
            while (!validMoveMade)
            {
                char moveInput[5];
                memset(moveInput, 0, sizeof(moveInput));
                int bytes_read = read(tdL.cl, moveInput, sizeof(moveInput) - 1);

                usleep(5000);
                if (bytes_read <= 0)
                {
                    perror("Error reading from client");
                    break; // Exit if read error
                }
                moveInput[bytes_read] = '\0'; 

                Move move;

                if (parseMove(moveInput, &move))
                {
                    if (validateMove(&tdL.gameSessionId->game, move, tdL.gameSessionId->currentPlayer))//daca mutarea este ok
                    {
                       
                        updateGameState(&tdL.gameSessionId->game, move);

                        //acum verificam daca in urma mutarii s-a produs un sah sau sah mat
                        int opponentColor = 1 - tdL.gameSessionId->currentPlayer;

                        int xKing;
                        int yKing;

                        for (int x = 0; x < BOARD_SIZE; x++)
                        {
                            for (int y = 0; y < BOARD_SIZE; y++)
                            {

                                if (tdL.gameSessionId->game.board[x][y].color == opponentColor && tdL.gameSessionId->game.board[x][y].type == 'K')
                                {

                                    xKing = x;
                                    yKing = y;
                                }
                            }
                        }
                        int moveFound = 0;
                        //daca gasim o piesa sau mai multe care ataca regele
                        if ((attackedByPawn(&tdL.gameSessionId->game, xKing, yKing) == 1) ||
                            (attackedByBishop(&tdL.gameSessionId->game, xKing, yKing) == 1) ||
                            (attackedByKnight(&tdL.gameSessionId->game, xKing, yKing) == 1) ||
                            (attackedByQueen(&tdL.gameSessionId->game, xKing, yKing) == 1) ||
                            (attackedByRook(&tdL.gameSessionId->game, xKing, yKing) == 1))
                        {
                            
                            int count = 0;
                            for (int x = 0; x < BOARD_SIZE; x++)
                            {
                                for (int y = 0; y < BOARD_SIZE; y++)
                                {
                                    if (tdL.gameSessionId->game.board[x][y].color == opponentColor)
                                    {

                                        Move move1;

                                        if (tdL.gameSessionId->game.board[x][y].type == 'P')
                                        {
                                             //printGameSession(tdL.gameSessionId);
                                            if (canPawnBlockCheck(tdL.gameSessionId, xKing, yKing, opponentColor))
                                            {
                                                count++; // poate fi salvat

                                               
                                            }
                                        }
                                        if (tdL.gameSessionId->game.board[x][y].type == 'Q')
                                        {
                                            // printGameSession(tdL.gameSessionId);
                                            if (canQueenBlockCheck(tdL.gameSessionId, xKing, yKing, opponentColor))
                                            {
                                                count++;

                                                
                                            }
                                        }
                                        if (tdL.gameSessionId->game.board[x][y].type == 'N')
                                        {
                                            // printGameSession(tdL.gameSessionId);
                                            if (canKnightBlockCheck(tdL.gameSessionId, xKing, yKing, opponentColor))
                                            {
                                                count++;
                                                
                                            }
                                        }
                                        if (tdL.gameSessionId->game.board[x][y].type == 'R')
                                        {
                                            // printGameSession(tdL.gameSessionId);
                                            if (canRookBlockCheck(tdL.gameSessionId, xKing, yKing, opponentColor))
                                            {
                                                count++;
                                                
                                            }
                                        }
                                        if (tdL.gameSessionId->game.board[x][y].type == 'B')
                                        {
                                            if (canBishopBlockCheck(tdL.gameSessionId, xKing, yKing, opponentColor))
                                            {
                                                count++;
                                                
                                            }
                                        }
                                        if (tdL.gameSessionId->game.board[x][y].type == 'K')
                                        {
                                            if (canKingEscapeCheck(tdL.gameSessionId, xKing, yKing, opponentColor))
                                            {
                                                count++;
                                              
                                            }
                                        }
                                    }
                                }
                            }
                            if (count == 0) // daca regele nu se poate apara
                            {
                                printf("Check-Mate\n");
                                fflush(stdout);

                                char *msg = "Check-Mate";
                                tdL.gameSessionId->currentPlayer = 2;
                                send(tdL.gameSessionId->player1, msg, strlen(msg), 0);
                                send(tdL.gameSessionId->player2, msg, strlen(msg), 0);
                                sendUpdatedBoard(tdL.gameSessionId); 

                                usleep(5000);

                            }
                            else
                            {
                                if (opponentColor == 1)
                                    {
                                    printf("Black King is in check\n");
                                    fflush(stdout);
                                    }
                                    else if (opponentColor == 0)
                                    {
                                    printf("White King is in check\n");
                                    fflush(stdout);
                                    }

                            }

                        }
                        sendUpdatedBoard(tdL.gameSessionId); // Trimitem starea jocului inapoi catre client

                        tdL.gameSessionId->currentPlayer = opponentColor;
                        validMoveMade = 1; 
                    }
                    else
                    {
                        
                        printf("[thread %d] Invalid move. Try again.\n", tdL.idThread);
                        char *msg = "Invalid move. Try again: ";
                        send(tdL.cl, msg, strlen(msg), 0);
                        usleep(5000);
                    }
                }
                else
                {
                    
                    printf("[thread %d] Parsing error. Try again.\n", tdL.idThread);
                    char *msg = "Parsing error. Try again: ";
                    send(tdL.cl, msg, strlen(msg), 0);
                    usleep(5000);
                }
            }
        }
        
        usleep(10000); 
    }

    close(tdL.cl);
    free(arg);
    return NULL;
}
int canQueenBlockCheck(GameSession *gameSession, int xKing, int yKing, int opponentColor)
{

    for (int x = 0; x < BOARD_SIZE; x++)
    {
        for (int y = 0; y < BOARD_SIZE; y++)
        {
            ChessPiece piece = gameSession->game.board[x][y];
            if (piece.type == 'Q' && piece.color == opponentColor)
            { // cautam regina jucatorului care muta

                //Cautam toate mutarile posibile ale piesei
                for (int dx = -BOARD_SIZE; dx <= BOARD_SIZE; dx++)
                {
                    for (int dy = -BOARD_SIZE; dy <= BOARD_SIZE; dy++)
                    {
                        if (dx == 0 && dy == 0)
                            continue; 

                        int newX = x + dx;
                        int newY = y + dy;

                        //Daca suntem in interiorul tablei de joc
                        if (newX >= 0 && newX < BOARD_SIZE && newY >= 0 && newY < BOARD_SIZE)
                        {
                            Move testMove = {x, y, newX, newY, 'L'};
                            if (validateQueenMove(&gameSession->game, testMove, opponentColor))
                            {

                                ChessPiece originalPiece = gameSession->game.board[newX][newY];

                                gameSession->game.board[x][y].color = 2;
                                gameSession->game.board[x][y].type = 'L';

                                gameSession->game.board[newX][newY] = piece; 

                                
                                int gasit = 0;
                                
                                if (attackedByPawn(&gameSession->game, xKing, yKing) == 0)
                                {
                                    gasit += 1;
                                }
                                if (attackedByBishop(&gameSession->game, xKing, yKing) == 0)
                                {
                                    gasit += 1;
                                }
                                if (attackedByKnight(&gameSession->game, xKing, yKing) == 0)
                                {
                                    gasit += 1;
                                }
                                if (attackedByQueen(&gameSession->game, xKing, yKing) == 0)
                                {
                                    gasit += 1;
                                }
                                if (attackedByRook(&gameSession->game, xKing, yKing) == 0)
                                {
                                    gasit += 1;
                                }
                                gameSession->game.board[x][y] = piece;
                                gameSession->game.board[newX][newY] = originalPiece;

                                if (gasit == 5)
                                    return 1;
                            }
                        }
                    }
                }
            }
        }
    }
    return 0; 
}
int canKnightBlockCheck(GameSession *gameSession, int xKing, int yKing, int opponentColor)
{

    for (int x = 0; x < BOARD_SIZE; x++)
    {
        for (int y = 0; y < BOARD_SIZE; y++)
        {
            ChessPiece piece = gameSession->game.board[x][y];
            if (piece.type == 'N' && piece.color == opponentColor)
            { 

                
                int knightMoves[8][2] = {{1, 2}, {1, -2}, {-1, 2}, {-1, -2}, {2, 1}, {2, -1}, {-2, 1}, {-2, -1}};

                
                for (int i = 0; i < 8; i++)
                {
                    int newX = x + knightMoves[i][0];
                    int newY = y + knightMoves[i][1];

                    
                    if (newX >= 0 && newX < BOARD_SIZE && newY >= 0 && newY < BOARD_SIZE)
                    {
                        Move testMove = {x, y, newX, newY, 'L'};
                        if (validateKnightMove(&gameSession->game, testMove, opponentColor))
                        {

                            
                            ChessPiece originalPiece = gameSession->game.board[newX][newY];
                            gameSession->game.board[x][y].color = 2;
                            gameSession->game.board[x][y].type = 'L';

                            gameSession->game.board[newX][newY] = piece; 

                            
                            int stillInCheck = 0;
                            if (attackedByPawn(&gameSession->game, xKing, yKing) ||
                                attackedByBishop(&gameSession->game, xKing, yKing) ||
                                attackedByKnight(&gameSession->game, xKing, yKing) ||
                                attackedByQueen(&gameSession->game, xKing, yKing) ||
                                attackedByRook(&gameSession->game, xKing, yKing))
                            {
                                stillInCheck = 1; 
                            }

                            
                            gameSession->game.board[x][y] = piece;
                            gameSession->game.board[newX][newY] = originalPiece;

                            if (!stillInCheck)
                            {
                                return 1; 
                            }
                        }
                    }
                }
            }
        }
    }
    return 0; 
}
int canBishopBlockCheck(GameSession *gameSession, int xKing, int yKing, int opponentColor)
{
    for (int x = 0; x < BOARD_SIZE; x++)
    {
        for (int y = 0; y < BOARD_SIZE; y++)
        {
            ChessPiece piece = gameSession->game.board[x][y];
            if (piece.type == 'B' && piece.color == opponentColor)
            { 

                
                for (int dx = -1; dx <= 1; dx += 2)
                { 
                    for (int dy = -1; dy <= 1; dy += 2)
                    {
                        for (int dist = 1; dist < BOARD_SIZE; dist++)
                        { 
                            int newX = x + dist * dx;
                            int newY = y + dist * dy;

                            // Verificam daca sunte in tabla
                            if (newX >= 0 && newX < BOARD_SIZE && newY >= 0 && newY < BOARD_SIZE)
                            {
                                Move testMove = {x, y, newX, newY, 'L'}; // Assume no promotion for simplicity

                                if (validateBishopMove(&gameSession->game, testMove, opponentColor))
                                {

                                    //Testam mutarea daca e buna
                                    ChessPiece originalPiece = gameSession->game.board[newX][newY];
                                    gameSession->game.board[x][y].color = 2;
                                    gameSession->game.board[x][y].type = 'L';

                                    gameSession->game.board[newX][newY] = piece; 

                                    
                                    int stillInCheck = 0;
                                    if (attackedByPawn(&gameSession->game, xKing, yKing) ||
                                        attackedByBishop(&gameSession->game, xKing, yKing) ||
                                        attackedByKnight(&gameSession->game, xKing, yKing) ||
                                        attackedByQueen(&gameSession->game, xKing, yKing) ||
                                        attackedByRook(&gameSession->game, xKing, yKing))
                                    {
                                        stillInCheck = 1; 
                                    }

                                    //Undo la mutare
                                    gameSession->game.board[x][y] = piece;
                                    gameSession->game.board[newX][newY] = originalPiece;

                                    if (!stillInCheck)
                                    {
                                        return 1; 
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    return 0; 
}
int canPawnBlockCheck(GameSession *gameSession, int xKing, int yKing, int opponentColor)
{
    for (int x = 0; x < BOARD_SIZE; x++)
    {
        for (int y = 0; y < BOARD_SIZE; y++)
        {
            ChessPiece piece = gameSession->game.board[x][y];
            if (piece.type == 'P' && piece.color == opponentColor)
            { 

                
                int direction = (opponentColor == 0) ? 1 : -1; 
                int initialRow = (opponentColor == 0) ? 1 : 6;

                
                int moveCoords[4][2] = {{x + direction, y}, {x + 2 * direction, y}, {x + direction, y - 1}, {x + direction, y + 1}};
                for (int i = 0; i < 4; i++)
                {
                    int newX = moveCoords[i][0];
                    int newY = moveCoords[i][1];

                    
                    if (newX >= 0 && newX < BOARD_SIZE && newY >= 0 && newY < BOARD_SIZE)
                    {
                        Move testMove = {x, y, newX, newY, 'L'}; // Assume no promotion for simplicity

                        if (validatePawnMove(&gameSession->game, testMove, opponentColor))
                        {

                            
                            ChessPiece originalPiece = gameSession->game.board[newX][newY];
                            gameSession->game.board[x][y].color = 2;
                            gameSession->game.board[x][y].type = 'L';

                            gameSession->game.board[newX][newY] = piece; 

                            
                            int stillInCheck = 0;
                            if (attackedByPawn(&gameSession->game, xKing, yKing) ||
                                attackedByBishop(&gameSession->game, xKing, yKing) ||
                                attackedByKnight(&gameSession->game, xKing, yKing) ||
                                attackedByQueen(&gameSession->game, xKing, yKing) ||
                                attackedByRook(&gameSession->game, xKing, yKing))
                            {

                                stillInCheck = 1; 
                            }

                            
                            gameSession->game.board[x][y] = piece;
                            gameSession->game.board[newX][newY] = originalPiece;

                            if (!stillInCheck)
                            {
                                
                                return 1; 
                            }
                        }
                    }
                }
            }
        }
    }
    return 0; 
}
int canRookBlockCheck(GameSession *gameSession, int xKing, int yKing, int opponentColor)
{
    for (int x = 0; x < BOARD_SIZE; x++)
    {
        for (int y = 0; y < BOARD_SIZE; y++)
        {
            ChessPiece piece = gameSession->game.board[x][y];
            if (piece.type == 'R' && piece.color == opponentColor)
            { 

                
                int directions[4][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};

                
                for (int d = 0; d < 4; d++)
                {
                    for (int dist = 1; dist < BOARD_SIZE; dist++)
                    { 
                        int newX = x + directions[d][0] * dist;
                        int newY = y + directions[d][1] * dist;

                        
                        if (newX >= 0 && newX < BOARD_SIZE && newY >= 0 && newY < BOARD_SIZE)
                        {
                            Move testMove = {x, y, newX, newY, 'L'};
                            if (validateRookMove(&gameSession->game, testMove, opponentColor))
                            {

                                
                                ChessPiece originalPiece = gameSession->game.board[newX][newY];
                                gameSession->game.board[x][y].color = 2;
                                gameSession->game.board[x][y].type = 'L';

                                gameSession->game.board[newX][newY] = piece; 

                                
                                int stillInCheck = 0;
                                if (attackedByPawn(&gameSession->game, xKing, yKing) ||
                                    attackedByBishop(&gameSession->game, xKing, yKing) ||
                                    attackedByKnight(&gameSession->game, xKing, yKing) ||
                                    attackedByQueen(&gameSession->game, xKing, yKing) ||
                                    attackedByRook(&gameSession->game, xKing, yKing))
                                {
                                    stillInCheck = 1; 
                                }

                                
                                gameSession->game.board[x][y] = piece;
                                gameSession->game.board[newX][newY] = originalPiece;

                                if (!stillInCheck)
                                {
                                    return 1; 
                                }

                                
                                if (gameSession->game.board[newX][newY].type != 'L')
                                    break;
                            }
                        }
                    }
                }
            }
        }
    }
    return 0; 
}
int canKingEscapeCheck(GameSession *gameSession, int xKing, int yKing, int opponentColor)
{

    
    for (int dx = -1; dx <= 1; dx++)
    {
        for (int dy = -1; dy <= 1; dy++)
        {
            if (dx == 0 && dy == 0)
                continue; 

            int newX = xKing + dx;
            int newY = yKing + dy;

            
            if (newX >= 0 && newX < BOARD_SIZE && newY >= 0 && newY < BOARD_SIZE)
            {
                Move testMove = {xKing, yKing, newX, newY, 'L'}; 

                if (validateKingMove(&gameSession->game, testMove, opponentColor))
                {

                    ChessPiece originalPiece = gameSession->game.board[newX][newY];
                    gameSession->game.board[xKing][yKing].color = 2;
                    gameSession->game.board[xKing][yKing].type = 'L';

                    gameSession->game.board[newX][newY].color = opponentColor;
                    gameSession->game.board[newX][newY].type = 'K';

                    
                    int stillInCheck = 0;
                    if (attackedByPawn(&gameSession->game, newX, newY) ||
                        attackedByBishop(&gameSession->game, newX, newY) ||
                        attackedByKnight(&gameSession->game, newX, newY) ||
                        attackedByQueen(&gameSession->game, newX, newY) ||
                        attackedByRook(&gameSession->game, newX, newY))
                    {
                        stillInCheck = 1; 
                    }

                    
                    gameSession->game.board[xKing][yKing].color = opponentColor;
                    gameSession->game.board[xKing][yKing].type = 'K';

                    gameSession->game.board[newX][newY] = originalPiece;

                    if (!stillInCheck)
                    {
                        return 1; 
                    }
                }
            }
        }
    }
    return 0; 
}

int validatePawnMove(ChessBoard *board, Move move, int color)
{
    int direction = (color == 0) ? 1 : -1; // Direcția de mișcare, depinde de culoarea pionului
    int startRow = (color == 0) ? 1 : 6;   // Rândul de start pentru pion

    // Mișcare simplă înainte
    if (move.startX + direction == move.endX && move.startY == move.endY &&
        board->board[move.endX][move.endY].type == 'L')
    {
        return 1;
    }

    // Captură în diagonală
    if (move.startX + direction == move.endX &&
        (move.startY + 1 == move.endY || move.startY - 1 == move.endY) &&
        board->board[move.endX][move.endY].color == 1 - color)
    {
        return 1;
    }

    // Mișcare inițială de două poziții
    if (move.startX == startRow && move.startX + 2 * direction == move.endX &&
        move.startY == move.endY && board->board[move.endX][move.endY].type == 'L' &&
        board->board[move.startX + direction][move.startY].type == 'L')
    { // Drumul trebuie să fie liber
        return 1;
    }

    return 0;
}

int validateRookMove(ChessBoard *board, Move move, int color)
{
    
    if (move.startX == move.endX || move.startY == move.endY)
    {
        
        int directionX = (move.endX - move.startX) > 0 ? 1 : (move.endX - move.startX) < 0 ? -1
                                                                                           : 0;
        int directionY = (move.endY - move.startY) > 0 ? 1 : (move.endY - move.startY) < 0 ? -1
                                                                                           : 0;
        int distance = (move.startX == move.endX) ? abs(move.endY - move.startY) : abs(move.endX - move.startX);

        
        for (int i = 1; i < distance; i++)
        {
            if (board->board[move.startX + i * directionX][move.startY + i * directionY].type != 'L')
            {
                return 0; 
            }
        }

        
        ChessPiece destinationPiece = board->board[move.endX][move.endY];
        if (destinationPiece.color != 2 && destinationPiece.color != color)
        {
            
            return 1;
        }
        else if (destinationPiece.type == 'L')
        {
            
            return 1;
        }

        
        return 0;
    }

    return 0; 
}
int validateBishopMove(ChessBoard *board, Move move, int color)
{
    int deltaX = abs(move.endX - move.startX);
    int deltaY = abs(move.endY - move.startY);

    
    if (deltaX == deltaY && deltaX > 0)
    {
        // Determine the direction of the movement
        int directionX = (move.endX - move.startX) > 0 ? 1 : -1;
        int directionY = (move.endY - move.startY) > 0 ? 1 : -1;

        // Check for pieces in the path
        for (int i = 1; i < deltaX; i++) // Note: deltaX is same as deltaY here
        {
            if (board->board[move.startX + i * directionX][move.startY + i * directionY].type != 'L')
            {
                return 0; // Path is blocked
            }
        }

        // Destination square can be empty or occupied by an opponent's piece for capture
        ChessPiece destinationPiece = board->board[move.endX][move.endY];
        if (destinationPiece.color != 2 && destinationPiece.color != color)
        {
            // Capture
            return 1;
        }
        else if (destinationPiece.type == 'L')
        {
            // Empty square
            return 1;
        }

        // Destination is occupied by a friendly piece or not a valid move
        return 0;
    }

    return 0; // Not a diagonal move
}
int validateKnightMove(ChessBoard *board, Move move, int color)
{
    // Calculate differences in X and Y
    int deltaX = abs(move.endX - move.startX);
    int deltaY = abs(move.endY - move.startY);

    // Check for L shape move (2 squares in one direction and 1 square in the other)
    if ((deltaX == 2 && deltaY == 1) || (deltaX == 1 && deltaY == 2))
    {
        // Destination square can be empty or occupied by an opponent's piece for capture
        ChessPiece destinationPiece = board->board[move.endX][move.endY];
        if (destinationPiece.color != 2 && destinationPiece.color != color)
        {
            // Capture
            return 1;
        }
        else if (destinationPiece.type == 'L')
        {
            // Empty square
            return 1;
        }

        // Destination is occupied by a friendly piece or not a valid move
        return 0;
    }

    return 0; // Not a valid knight move
}
int max(int a, int b)
{
    return (a > b) ? a : b;
}
int validateQueenMove(ChessBoard *board, Move move, int color)
{
    int deltaX = abs(move.endX - move.startX);
    int deltaY = abs(move.endY - move.startY);
    // Check if the move is along a row or column (like a rook)
    if (move.startX == move.endX || move.startY == move.endY || deltaX == deltaY)
    {
        // Determine the direction of the movement
        int directionX = (move.endX - move.startX) > 0 ? 1 : (move.endX - move.startX) < 0 ? -1
                                                                                           : 0;
        int directionY = (move.endY - move.startY) > 0 ? 1 : (move.endY - move.startY) < 0 ? -1
                                                                                           : 0;
        int distance = max(deltaX, deltaY); // The greater of deltaX and deltaY

        // Check for pieces in the path
        for (int i = 1; i < distance; i++)
        {
            if (board->board[move.startX + i * directionX][move.startY + i * directionY].type != 'L')
            {
                return 0; // Path is blocked
            }
        }

        // Destination square can be empty or occupied by an opponent's piece for capture
        ChessPiece destinationPiece = board->board[move.endX][move.endY];
        if (destinationPiece.color != 2 && destinationPiece.color != color)
        {
            // Capture
            return 1;
        }
        else if (destinationPiece.type == 'L')
        {
            // Empty square
            return 1;
        }

        // Destination is occupied by a friendly piece or not a valid move
        return 0;
    }

    return 0; // Not a valid queen move
}
int validateKingMove(ChessBoard *board, Move move, int color)
{
    int deltaX = abs(move.endX - move.startX);
    int deltaY = abs(move.endY - move.startY);

    // Check if the move is only one square in any direction
    if (deltaX <= 1 && deltaY <= 1)
    {
        ChessPiece destinationPiece = board->board[move.endX][move.endY];

        // Check if the destination is adjacent to the enemy king
        for (int i = -1; i <= 1; i++)
        {
            for (int j = -1; j <= 1; j++)
            {
                int checkX = move.endX + i;
                int checkY = move.endY + j;

                // Ensure it's within the board and check for enemy king
                if (checkX >= 0 && checkX < BOARD_SIZE && checkY >= 0 && checkY < BOARD_SIZE)
                {
                    ChessPiece adjacentPiece = board->board[checkX][checkY];
                    if (adjacentPiece.type == 'K' && adjacentPiece.color != color)
                    {
                        return 0; // Move would place king adjacent to the other king
                    }
                }
            }
        }

        // Ensure destination is not occupied by a friendly piece
        if (destinationPiece.color == color)
        {
            return 0;
        }

        // Check if the destination square is not putting the king in check
        // (This part should be handled by your check detection mechanism, typically outside this function)

        return 1; // Valid move
    }

    return 0; // Not a valid king move
}

int attackedByPawn(ChessBoard *board, int xKing, int yKing)
{

    if (board->board[xKing][yKing].color == 1 && board->board[xKing][yKing].type == 'K') // Black rege
    {

        if ((board->board[xKing - 1][yKing - 1].type == 'P' && board->board[xKing - 1][yKing - 1].color == 0) ||
            (board->board[xKing - 1][yKing + 1].type == 'P' && board->board[xKing - 1][yKing + 1].color == 0))
        {
            printf(" Black King is attacked by Pawn");
            fflush(stdout);
            return 1;
        }
    }
    if (board->board[xKing][yKing].color == 0 && board->board[xKing][yKing].type == 'K') // White rege
    {



        if ((board->board[xKing + 1][yKing - 1].type == 'P' && board->board[xKing + 1][yKing - 1].color == 1) ||
            (board->board[xKing + 1][yKing + 1].type == 'P' && board->board[xKing + 1][yKing + 1].color == 1))
        {
            printf("White King is attacked by Pawn");
            fflush(stdout);
            return 1;
        }
    }

    return 0;
}
int attackedByBishop(ChessBoard *board, int xKing, int yKing)
{

    if (board->board[xKing][yKing].type == 'K')
    { // Check for King
        int kingColor = board->board[xKing][yKing].color;

        // Check all 4 diagonal directions
        int directions[4][2] = {{1, 1}, {1, -1}, {-1, 1}, {-1, -1}};
        for (int d = 0; d < 4; d++)
        {
            int dx = directions[d][0];
            int dy = directions[d][1];
            int i = xKing + dx, j = yKing + dy;

            // Traverse diagonally until a piece is hit or the end of the board is reached
            while (i >= 0 && i < BOARD_SIZE && j >= 0 && j < BOARD_SIZE)
            {
                if (board->board[i][j].type == 'B' && board->board[i][j].color != kingColor)
                {

                    return 1; // The King is being attacked by a Bishop
                }
                else if (board->board[i][j].type != 'L')
                {
                    break; // Stop if any other piece is blocking the path
                }
                i += dx;
                j += dy;
            }
        }
    }

    return 0; // No King is being attacked by a Bishop
}
int attackedByKnight(ChessBoard *board, int xKing, int yKing)
{
    // Directions a Knight can move: eight possible L shapes
    int knightMoves[8][2] = {
        {-2, -1}, {-2, 1}, // Up Left, Up Right
        {2, -1},
        {2, 1}, // Down Left, Down Right
        {-1, -2},
        {1, -2}, // Left Up, Left Down
        {-1, 2},
        {1, 2} // Right Up, Right Down
    };

    // Find the King
    if (board->board[xKing][yKing].type == 'K')
    {
        int kingColor = board->board[xKing][yKing].color;

        // Check all possible knight moves
        for (int m = 0; m < 8; m++) // DA
        {
            int i = xKing + knightMoves[m][0];
            int j = yKing + knightMoves[m][1];

            // Check bounds and then for Knight of opposite color
            if (i >= 0 && i < BOARD_SIZE && j >= 0 && j < BOARD_SIZE && board->board[i][j].type == 'N' && board->board[i][j].color != kingColor)
            {

                return 1; // The King is being attacked by a Knight
            }
        }
    }

    return 0; // No King is being attacked by a Knight
}
int attackedByQueen(ChessBoard *board, int xKing, int yKing)
{
    // Directions the Queen can move: combining Rook and Bishop moves
    int directions[8][2] = {
        {-1, 0}, {1, 0}, // Up, Down (like Rook)
        {0, -1},
        {0, 1}, // Left, Right (like Rook)
        {-1, -1},
        {1, 1}, // Diagonals (like Bishop)
        {-1, 1},
        {1, -1}};

    if (board->board[xKing][yKing].type == 'K')
    { // Check for King
        int kingColor = board->board[xKing][yKing].color;
        // Check all 8 directions around the king for a Queen
        for (int d = 0; d < 8; d++)
        {
            int dx = directions[d][0];
            int dy = directions[d][1];
            int i = xKing + dx, j = yKing + dy;

            // Traverse until a piece is hit or the end of the board is reached
            while (i >= 0 && i < BOARD_SIZE && j >= 0 && j < BOARD_SIZE)
            {
                if (board->board[i][j].type == 'Q' && board->board[i][j].color != kingColor)
                {

                    return 1; // The King is being attacked by a Queen
                }
                else if (board->board[i][j].type != 'L')
                {
                    break; // Stop if any other piece is blocking the path
                }
                i += dx;
                j += dy;
            }
        }
    }

    return 0; // No King is being attacked by a Queen
}
int attackedByRook(ChessBoard *board, int xKing, int yKing)
{
    // Directions the Rook can move: along rows and columns
    int directions[4][2] = {
        {-1, 0}, // Up
        {1, 0},  // Down
        {0, -1}, // Left
        {0, 1}   // Right
    };

    if (board->board[xKing][yKing].type == 'K')
    { // Check for King
        int kingColor = board->board[xKing][yKing].color;

        // Check all 4 straight directions around the king for a Rook
        for (int d = 0; d < 4; d++)
        {
            int dx = directions[d][0];
            int dy = directions[d][1];
            int i = xKing + dx, j = yKing + dy;

            // Traverse until a piece is hit or the end of the board is reached
            while (i >= 0 && i < BOARD_SIZE && j >= 0 && j < BOARD_SIZE)
            {
                if (board->board[i][j].type == 'R' && board->board[i][j].color != kingColor)
                {

                    return 1; // The King is being attacked by a Rook
                }
                else if (board->board[i][j].type != 'L')
                {
                    break; // Stop if any other piece is blocking the path
                }
                i += dx;
                j += dy;
            }
        }
    }

    return 0; // No King is being attacked by a Rook
}

int validateMove(ChessBoard *board, Move move, int playerColor)
{

    if (move.startX < 0 || move.startX >= BOARD_SIZE || move.startY < 0 || move.startY >= BOARD_SIZE ||
        move.endX < 0 || move.endX >= BOARD_SIZE || move.endY < 0 || move.endY >= BOARD_SIZE)
    {
        return 0;
    }

    // Obține piesa care se mișcă
    ChessPiece piece = board->board[move.startX][move.startY];

    if (piece.color != playerColor)
    {
        return 0; // Piesa nu este a jucătorului curent
    }

    // Dacă nu există o piesă în locația de start, returnează invalid
    if (piece.type == 'L' || piece.color == 2)
    {
        return 0;
    }

    // Dacă avem o piesa a aceluias jucator care vrea sa mute
    ChessPiece destinationPiece = board->board[move.endX][move.endY];
    if (destinationPiece.color == piece.color)
    {
        return 0;
    }
    ChessPiece tempStart = board->board[move.startX][move.startY];
    ChessPiece tempEnd = board->board[move.endX][move.endY];
    board->board[move.startX][move.startY] = (ChessPiece){.type = 'L', .color = 2}; 
    board->board[move.endX][move.endY] = tempStart; 

    // Aflam pozitia regelui
    int kingX = -1, kingY = -1;
    for (int x = 0; x < BOARD_SIZE; x++) {
        for (int y = 0; y < BOARD_SIZE; y++) {
            if (board->board[x][y].type == 'K' && board->board[x][y].color == playerColor) {
                kingX = x;
                kingY = y;
                break;
            }
        }
        if (kingX != -1) break; 
    }

    // Verificam daca regele e in check
    int kingInCheck = (attackedByPawn(board, kingX, kingY) ||
                       attackedByBishop(board, kingX, kingY) ||
                       attackedByKnight(board, kingX, kingY) ||
                       attackedByQueen(board, kingX, kingY) ||
                       attackedByRook(board, kingX, kingY));

    // Facem undo la mutare
    board->board[move.startX][move.startY] = tempStart;
    board->board[move.endX][move.endY] = tempEnd;

    // Daca regele a ramas in sah in urma mutarii este invalida mutarea
    if (kingInCheck) {
        return 0;
    }

    // Validare bazată pe tipul piesei
    switch (piece.type)
    {
    case 'P': // Pion
        return validatePawnMove(board, move, piece.color);
    case 'R': // Rook
        return validateRookMove(board, move, piece.color);
    case 'B': // Bishop
        return validateBishopMove(board, move, piece.color);
    case 'N': // Knight
        return validateKnightMove(board, move, piece.color);
    case 'Q': // Queen
        return validateQueenMove(board, move, piece.color);
    case 'K': // King
        return validateKingMove(board, move, piece.color);
    
    default:
        return 0;
    }
}

int algebraicToIndex(char c)
{

    return c - 'a';
}

int parseMove(char *input, Move *move)
{


    if (strlen(input) != 4)
    {

        return 0; 
    }

    move->startX = input[1] - '1';
    move->startY = algebraicToIndex(input[0]);

    move->endX = input[3] - '1';
    move->endY = algebraicToIndex(input[2]);

    return 1;
}

int main()
{
    struct sockaddr_in server;
    struct sockaddr_in from;
    int sd;
    int i = 0;
    pthread_t th[100];
     //Creem socket-ul 
    if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1) 
    {
        perror("[server]Eroare la socket().\n");
        return errno;
    }

    int on = 1;
    setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

    bzero(&server, sizeof(server));
    bzero(&from, sizeof(from));

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    server.sin_port = htons(PORT);

    if (bind(sd, (struct sockaddr *)&server, sizeof(struct sockaddr)) == -1)
    {
        perror("[server]Eroare la bind().\n");
        return errno;
    }

    if (listen(sd, 5) == -1)
    {
        perror("[server]Eroare la listen().\n");
        return errno;
    }

    GameSession *session = NULL;
    int waitingClient = -1;
    int numClientsConnected = 0;
    while (1)
    {
        printf("[server] Așteptăm la portul %d...\n", PORT);
        fflush(stdout);
        int length = sizeof(from);
        int client = accept(sd, (struct sockaddr *)&from, &length);
        if (client < 0)
        {
            perror("[server] Eroare la accept().\n");
            continue;
        }

        char *clientMsg;

        if (waitingClient == -1) // Primul client care se conecteaza
        {
            waitingClient = client;
            numClientsConnected = 1;                       
            clientMsg = "Player 1";                        
            send(client, clientMsg, strlen(clientMsg), 0); 
            usleep(5000);
        }
        else
        {

            numClientsConnected = 2;
            // Avem doi clienți, inițializăm o nouă sesiune de joc

            clientMsg = "Player 2"; 
            send(client, clientMsg, strlen(clientMsg), 0);
            usleep(5000);
            session = (GameSession *)malloc(sizeof(GameSession));
            initializeGame(&session->game);
            printBoard(&session->game);
            session->player1 = waitingClient;
            session->player2 = client;
            session->currentPlayer = 0;

            // Alocăm și inițializăm structurile thData pentru fiecare client
            thData *td1 = (thData *)malloc(sizeof(thData));
            td1->idThread = i++;
            td1->cl = session->player1;
            td1->gameSessionId = session;

            thData *td2 = (thData *)malloc(sizeof(thData));
            td2->idThread = i++;
            td2->cl = session->player2;
            td2->gameSessionId = session;

            char *serializedSession = serializeGameSession(session);
            if (serializedSession != NULL)
            {
                send(session->player1, serializedSession, strlen(serializedSession), 0);
                usleep(5000);
                send(session->player2, serializedSession, strlen(serializedSession), 0);
                usleep(5000);
                
            }

            char *clientMsg1 = "Both Players Have Connected"; // S-au conectat ambii jucatori intr-o sesiune
            send(session->player1, clientMsg1, strlen(clientMsg1), 0);
            usleep(5000);
            send(session->player2, clientMsg1, strlen(clientMsg1), 0);
            usleep(5000);

            if (serializedSession != NULL)
            {
                send(session->player1, serializedSession, strlen(serializedSession), 0);
                usleep(5000);
                send(session->player2, serializedSession, strlen(serializedSession), 0);
                usleep(5000);
                free(serializedSession); 
            }

            pthread_create(&th[i++], NULL, &treat, (void *)td1); // Creăm thread pentru primul client
            pthread_create(&th[i++], NULL, &treat, (void *)td2); // Creăm thread pentru al doilea client

            waitingClient = -1; // Resetăm clientul care așteaptă
        }
    }

    close(sd); // Inchidem serverul socket
    return 0;
}