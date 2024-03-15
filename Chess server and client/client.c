#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <string.h>
#include <arpa/inet.h>
#include <SFML/Graphics.h>
#include <SFML/Window.h>

extern int errno;
int port;
char buffer[5000];
int firstClickX = -1, firstClickY = -1;
int secondClickX = -1, secondClickY = -1;
char lastMove[6];
int sd;

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

char *pixelToChessCoords(const sfVector2f boardOffset, int tileSize, sfVector2i firstClickPos, sfVector2i secondClickPos);
void deserializeGameSession(GameSession *session, char *buffer);

void deserializeGameSession(GameSession *session, char *buffer)
{
    


    int i, j;
    int color;
    char type;

    char *line = strtok(buffer, "\n");
    if (line == NULL)
    {
        printf("Malformed message: No initial line.\n");
        return;
    }

    for (int detail = 1; detail <= 3; detail++)
    {
        line = strtok(NULL, "\n");
        if (line == NULL)
        {
            printf("Malformed message: Missing details.\n");
            return;
        }
        if (detail == 1)
            sscanf(line, "Player 1: %d", &session->player1);
        else if (detail == 2)
            sscanf(line, "Player 2: %d", &session->player2);
        else if (detail == 3)
            sscanf(line, "Current Player: %d", &session->game.currentPlayer);
    }

    // Skip the "ChessBoard:" line
    strtok(NULL, "\n");

    for (i = 0; i < BOARD_SIZE; i++)
    {
        for (j = 0; j < BOARD_SIZE; j++)
        {
            line = strtok(NULL, "\n");
            if (line == NULL)
            {
                printf("Malformed message: Missing board details.\n");
                return;
            }
            if (sscanf(line, "Piece at [%d][%d] - Type: %c , Color: %d", &i, &j, &type, &color) == 4)
            {
                session->game.board[i][j].type = type;
                session->game.board[i][j].color = color;
            }
            else
            {
                printf("Failed to parse line: %s\n", line);
            }
        }
    }

  
}
void printDeserializedGameSession(const GameSession *session)
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
void drawChessBoard(const GameSession *session, sfRenderWindow *window, sfVideoMode mode, const int tileSize, const int boardSize, const sfVector2f boardOffset)
{
    sfRenderWindow_setFramerateLimit(window, 60);

    sfRectangleShape *tile = sfRectangleShape_create();
    sfRectangleShape_setSize(tile, (sfVector2f){tileSize, tileSize});

    sfTexture *whiteQueenTexture = sfTexture_createFromFile("whiteQueen.png", NULL);
    sfSprite *whiteQueenSprite = sfSprite_create();
    sfSprite_setTexture(whiteQueenSprite, whiteQueenTexture, sfTrue);

    sfTexture *whiteKingTexture = sfTexture_createFromFile("WhiteKing.png", NULL);
    sfSprite *whiteKingSprite = sfSprite_create();
    sfSprite_setTexture(whiteKingSprite, whiteKingTexture, sfTrue);

    sfTexture *whiteCalTexture = sfTexture_createFromFile("WhiteCal.png", NULL);
    sfSprite *whiteCalSprite = sfSprite_create();
    sfSprite_setTexture(whiteCalSprite, whiteCalTexture, sfTrue);

    sfTexture *whiteNebunTexture = sfTexture_createFromFile("WhiteNebun.png", NULL);
    sfSprite *whiteNebunSprite = sfSprite_create();
    sfSprite_setTexture(whiteNebunSprite, whiteNebunTexture, sfTrue);

    sfTexture *whitePawnTexture = sfTexture_createFromFile("WhitePawn.png", NULL); 
    sfSprite *whitePawnSprite = sfSprite_create();
    sfSprite_setTexture(whitePawnSprite, whitePawnTexture, sfTrue);

    sfTexture *whiteTuraTexture = sfTexture_createFromFile("WhiteTura.png", NULL);
    sfSprite *whiteTuraSprite = sfSprite_create();
    sfSprite_setTexture(whiteTuraSprite, whiteTuraTexture, sfTrue);

    sfTexture *blackQueenTexture = sfTexture_createFromFile("BlackQueen.png", NULL);
    sfSprite *blackQueenSprite = sfSprite_create();
    sfSprite_setTexture(blackQueenSprite, blackQueenTexture, sfTrue);

    sfTexture *blackKingTexture = sfTexture_createFromFile("BlackKing.png", NULL); 
    sfSprite *blackKingSprite = sfSprite_create();
    sfSprite_setTexture(blackKingSprite, blackKingTexture, sfTrue);

    sfTexture *blackCalTexture = sfTexture_createFromFile("BlackCal.png", NULL);
    sfSprite *blackCalSprite = sfSprite_create();
    sfSprite_setTexture(blackCalSprite, blackCalTexture, sfTrue);

    sfTexture *blackNebunTexture = sfTexture_createFromFile("BlackBishop.png", NULL);
    sfSprite *blackNebunSprite = sfSprite_create();
    sfSprite_setTexture(blackNebunSprite, blackNebunTexture, sfTrue);

    sfTexture *blackPawnTexture = sfTexture_createFromFile("BlackPawn.png", NULL); 
    sfSprite *blackPawnSprite = sfSprite_create();
    sfSprite_setTexture(blackPawnSprite, blackPawnTexture, sfTrue);

    sfTexture *blackTuraTexture = sfTexture_createFromFile("BlackTura.png", NULL); 
    sfSprite *blackTuraSprite = sfSprite_create();
    sfSprite_setTexture(blackTuraSprite, blackTuraTexture, sfTrue);

    sfSprite_setScale(whiteQueenSprite, (sfVector2f){0.85f, 0.85f});
    sfSprite_setScale(whiteKingSprite, (sfVector2f){0.90f, 0.90f});
    sfSprite_setScale(whiteCalSprite, (sfVector2f){0.90f, 0.90f});
    sfSprite_setScale(whiteNebunSprite, (sfVector2f){0.90f, 0.90f});
    sfSprite_setScale(whiteTuraSprite, (sfVector2f){0.90f, 0.90f});
    sfSprite_setScale(blackQueenSprite, (sfVector2f){0.85f, 0.85f});
    sfSprite_setScale(blackKingSprite, (sfVector2f){0.90f, 0.90f});
    sfSprite_setScale(blackCalSprite, (sfVector2f){0.90f, 0.90f});
    sfSprite_setScale(blackNebunSprite, (sfVector2f){0.90f, 0.90f});
    sfSprite_setScale(blackTuraSprite, (sfVector2f){0.90f, 0.90f});

    sfColor backgroundColor = sfColor_fromRGB(100, 100, 100); 

    sfRenderWindow_clear(window, backgroundColor);

    for (int i = 0; i < 8; ++i)
    {
        for (int j = 0; j < 8; ++j)
        {
            if ((i + j) % 2 == 0)
            {
                sfRectangleShape_setFillColor(tile, sfWhite);
            }
            else
            {
                sfRectangleShape_setFillColor(tile, sfBlue);
            }

            sfRectangleShape_setPosition(tile, (sfVector2f){
                                                   boardOffset.x + j * tileSize,
                                                   boardOffset.y + i * tileSize});
            sfRenderWindow_drawRectangleShape(window, tile, NULL);
        }
    }

    for (int i = 0; i < 8; ++i)
    {
        for (int j = 0; j < 8; ++j)
        {

            char pieceType = session->game.board[i][j].type;
            int pieceColor = session->game.board[i][j].color;

            sfSprite *pieceSprite = NULL;

            switch (pieceType)
            {
            case 'P':
                pieceSprite = (pieceColor == 0) ? whitePawnSprite : blackPawnSprite;
                if (pieceColor == 0)
                {
                    sfSprite_setPosition(pieceSprite, (sfVector2f){
                                                          boardOffset.x + j * tileSize,
                                                          boardOffset.y + i * tileSize});
                }
                else
                {
                    sfSprite_setPosition(pieceSprite, (sfVector2f){
                                                          boardOffset.x + j * tileSize,
                                                          boardOffset.y + i * tileSize - 5});
                }
                sfRenderWindow_drawSprite(window, pieceSprite, NULL);
                break;
            case 'K':
                pieceSprite = (pieceColor == 0) ? whiteKingSprite : blackKingSprite;
                if (pieceColor == 0)
                {
                    sfSprite_setPosition(pieceSprite, (sfVector2f){
                                                          boardOffset.x + j * tileSize,
                                                          boardOffset.y + i * tileSize - 13});
                }
                else
                {
                    sfSprite_setPosition(pieceSprite, (sfVector2f){
                                                          boardOffset.x + j * tileSize - 9,
                                                          boardOffset.y + i * tileSize - 9});
                }
                sfRenderWindow_drawSprite(window, pieceSprite, NULL);
                break;
            case 'Q': 
                pieceSprite = (pieceColor == 0) ? whiteQueenSprite : blackQueenSprite;
                if (pieceColor == 0)
                {
                    sfSprite_setPosition(pieceSprite, (sfVector2f){
                                                          boardOffset.x + j * tileSize - 6,
                                                          boardOffset.y + i * tileSize - 10});
                }
                else
                {
                    sfSprite_setPosition(pieceSprite, (sfVector2f){
                                                          boardOffset.x + j * tileSize - 8,
                                                          boardOffset.y + i * tileSize - 3});
                }

                sfRenderWindow_drawSprite(window, pieceSprite, NULL);
                break;
            case 'N':
                pieceSprite = (pieceColor == 0) ? whiteCalSprite : blackCalSprite;
                if (pieceColor == 0)
                {
                    sfSprite_setPosition(pieceSprite, (sfVector2f){
                                                          boardOffset.x + j * tileSize - 7,
                                                          boardOffset.y + i * tileSize});
                }
                else
                {
                    sfSprite_setPosition(pieceSprite, (sfVector2f){
                                                          boardOffset.x + j * tileSize - 7,
                                                          boardOffset.y + i * tileSize - 5});
                }
                sfRenderWindow_drawSprite(window, pieceSprite, NULL);
                break;
            case 'B': 
                pieceSprite = (pieceColor == 0) ? whiteNebunSprite : blackNebunSprite;
                if (pieceColor == 0)
                {
                    sfSprite_setPosition(pieceSprite, (sfVector2f){
                                                          boardOffset.x + j * tileSize,
                                                          boardOffset.y + i * tileSize});
                }
                else
                {
                    sfSprite_setPosition(pieceSprite, (sfVector2f){
                                                          boardOffset.x + j * tileSize,
                                                          boardOffset.y + i * tileSize - 5});
                }
                sfRenderWindow_drawSprite(window, pieceSprite, NULL);
                break;
            case 'R':
                pieceSprite = (pieceColor == 0) ? whiteTuraSprite : blackTuraSprite;
                if (pieceColor == 0)
                {
                    sfSprite_setPosition(pieceSprite, (sfVector2f){
                                                          boardOffset.x + j * tileSize - 4,
                                                          boardOffset.y + i * tileSize});
                }
                else
                {
                    sfSprite_setPosition(pieceSprite, (sfVector2f){
                                                          boardOffset.x + j * tileSize,
                                                          boardOffset.y + i * tileSize - 5});
                }
                sfRenderWindow_drawSprite(window, pieceSprite, NULL);
                break;
            }
        }
    }

    sfRenderWindow_display(window);
}
void updateChessBoard(const GameSession *session, sfRenderWindow *window)
{
    sfVideoMode mode = {1600, 1000, 32};
    const int tileSize = 100;
    const sfVector2f boardOffset = {
        (mode.width - tileSize * 8) / 2.0f, 
        (mode.height - tileSize * 8) / 2.0f 
    };

    sfRectangleShape *tile = sfRectangleShape_create();
    sfRectangleShape_setSize(tile, (sfVector2f){tileSize, tileSize});
    for (int i = 0; i < 8; i++)
        for (int j = 0; j < 8; j++)
        {
            int pieceColor = session->game.board[i][j].color;
            if (pieceColor == 2)

            {

                if ((i + j) % 2 == 0)
                {
                    sfRectangleShape_setFillColor(tile, sfWhite);
                }
                else
                {
                    sfRectangleShape_setFillColor(tile, sfBlue);
                }

                sfRectangleShape_setPosition(tile, (sfVector2f){
                                                       boardOffset.x + j * tileSize,
                                                       boardOffset.y + i * tileSize});
                sfRenderWindow_drawRectangleShape(window, tile, NULL);
            }
        }
}

char *pixelToChessCoords(const sfVector2f boardOffset, int tileSize, sfVector2i firstClickPos, sfVector2i secondClickPos)
{

    static char lastMove[6];

    int firstClickX = (firstClickPos.x - (int)boardOffset.x) / tileSize;
    int firstClickY = ((firstClickPos.y - (int)boardOffset.y) / tileSize);

    int secondClickX = (secondClickPos.x - (int)boardOffset.x) / tileSize;
    int secondClickY = ((secondClickPos.y - (int)boardOffset.y) / tileSize);

    if (firstClickX < 0 || firstClickX >= 8 || firstClickY < 0 || firstClickY >= 8 ||
        secondClickX < 0 || secondClickX >= 8 || secondClickY < 0 || secondClickY >= 8)
    {
        return NULL;
    }

    snprintf(lastMove, sizeof(lastMove), "%c%d%c%d",
             'a' + firstClickX, 1 + firstClickY,
             'a' + secondClickX, 1 + secondClickY);

    return lastMove;
}
void writeTurnInfo(sfRenderWindow *window, int clientID, const GameSession *session, sfVideoMode mode)
{

    char turnInfo[50];
    sfFont *font = sfFont_createFromFile("arial_narrow_7.ttf");

    if (clientID == session->game.currentPlayer)
    {
        snprintf(turnInfo, sizeof(turnInfo), "Your Turn");
    }
    else
    {
        snprintf(turnInfo, sizeof(turnInfo), "Enemy's Turn");
    }

    sfText *turnText = sfText_create();
    sfText_setString(turnText, turnInfo);
    sfText_setFont(turnText, font);
    sfText_setCharacterSize(turnText, 24);
    sfText_setColor(turnText, sfWhite);

    sfFloatRect textRect = sfText_getLocalBounds(turnText);
    sfText_setOrigin(turnText, (sfVector2f){textRect.width / 2, textRect.height / 2});
    sfText_setPosition(turnText, (sfVector2f){mode.width / 2.0f, 10});

    sfRenderWindow_drawText(window, turnText, NULL);
    sfRenderWindow_display(window);

}
void clearTurnInfo(sfRenderWindow *window, sfVideoMode mode, const sfVector2f boardOffset)
{
    sfRectangleShape *coverRect = sfRectangleShape_create();
    sfRectangleShape_setSize(coverRect, (sfVector2f){mode.width, 50});           
    sfRectangleShape_setFillColor(coverRect, sfBlack);                            
    sfRectangleShape_setPosition(coverRect, (sfVector2f){0, boardOffset.y - 50});

    sfRenderWindow_drawRectangleShape(window, coverRect, NULL);

    sfRenderWindow_display(window);

    sfRectangleShape_destroy(coverRect);
}

int main(int argc, char *argv[])
{

    struct sockaddr_in server;
    char move[5] = {0};
    ChessBoard board;
    int isMyTurn = 0;
    int CLIENT_ID;
    int validMove = 0;
    GameSession session;
    sfRenderWindow *window;

    sfVideoMode mode = {1600, 1000, 32};
    const int tileSize = 100;
    const int boardSize = 8 * tileSize;
    const sfVector2f boardOffset = {
        (mode.width - tileSize * 8) / 2.0f,
        (mode.height - tileSize * 8) / 2.0f 
    };

    if (argc != 3)
    {
        printf("Sintaxa: %s <adresa_server> <port>\n", argv[0]);
        return -1;
    }

    port = atoi(argv[2]);

    if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("Eroare la socket().\n");
        return errno;
    }

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = inet_addr(argv[1]);
    server.sin_port = htons(port);

    if (connect(sd, (struct sockaddr *)&server, sizeof(struct sockaddr)) == -1)
    {
        perror("[client]Eroare la connect().\n");
        return errno;
    }

    char idMsg[30];
    memset(buffer, 0, sizeof(idMsg));
    int bytes_read = read(sd, idMsg, sizeof(idMsg) - 1);

    if (bytes_read > 0)
    {
        idMsg[bytes_read] = '\0';
        if (strcmp(idMsg, "Player 1") == 0)
        {
            CLIENT_ID = 0; 
        }
        else if (strcmp(idMsg, "Player 2") == 0)
        {
            CLIENT_ID = 1; 
        }
    }

    memset(buffer, 0, sizeof(buffer));
    bytes_read = read(sd, buffer, sizeof(buffer) - 1);
    if (bytes_read <= 0)
    {
        perror("[client] Error reading from server.\n");
    }
    buffer[bytes_read] = '\0';

   
    deserializeGameSession(&session, buffer);

    char readyToPlay[30];
    memset(buffer, 0, sizeof(readyToPlay));
    bytes_read = read(sd, readyToPlay, sizeof(readyToPlay) - 1);


    if (bytes_read > 0)
    {
        readyToPlay[bytes_read] = '\0';
        if (strstr(readyToPlay, "Both Players Have Connected") != NULL)
        {

            sfRenderWindow *window = sfRenderWindow_create(mode, "Chess Board with White Queen", sfResize | sfClose, NULL);
            sfRenderWindow_setFramerateLimit(window, 60);
            drawChessBoard(&session, window, mode, tileSize, boardSize, boardOffset);
            writeTurnInfo(window, CLIENT_ID, &session, mode);

            int gameState = 1;
            while (1)
            {

                memset(buffer, 0, sizeof(buffer));
                int bytes_read = read(sd, buffer, sizeof(buffer) - 1);
                if (bytes_read <= 0)
                {
                    perror("[client] Error reading from server.\n");
                    break;
                }
                buffer[bytes_read] = '\0';
                if (strncmp(buffer, "Check-Mate", strlen("Check-Mate")) == 0)
                {
                    printf(" Check-Mate, Player %d has won\n", 1 - CLIENT_ID);
                    fflush(stdout);

                    memset(buffer, 0, sizeof(buffer));
                    bytes_read = read(sd, buffer, sizeof(buffer) - 1);
                    usleep(5000);
                    if (bytes_read <= 0)
                    {
                        perror("[client]Error reading from server.\n");
                        exit(1);
                    }
                    deserializeGameSession(&session, buffer);
                    sfFont *font = sfFont_createFromFile("arial_narrow_7.ttf");
                    sfText *text = sfText_create();

                    int numberOfWinner = 1 - CLIENT_ID;
                    char winText[50]; 
                    snprintf(winText, sizeof(winText), "Player %d has Won", numberOfWinner);
                    sfText_setString(text, winText);

                    sfText_setFont(text, font);
                    sfText_setCharacterSize(text, 24);
                    sfText_setColor(text, sfRed);
                    sfFloatRect textRect = sfText_getLocalBounds(text);

                    sfText_setOrigin(text, (sfVector2f){textRect.width / 2, textRect.height / 2});
                    sfText_setPosition(text, (sfVector2f){mode.width / 2.0f, 30});

                    sfRenderWindow_clear(window, sfBlack);
                    int clientFake = -1;

                    drawChessBoard(&session, window, mode, tileSize, boardSize, boardOffset);

                    sfRenderWindow_drawText(window, text, NULL);
                    sfRenderWindow_display(window);

                    for (int i = 0; i < 8; ++i)
                    {
                        for (int j = 0; j < 8; ++j)
                            if (session.game.board[i][j].type == 'K' && session.game.board[i][j].color == CLIENT_ID)
                            {

                                printf("INTRU PE COLORARE \n");
                                fflush(stdout);
                                sfRectangleShape *tile = sfRectangleShape_create();
                                sfRectangleShape_setSize(tile, (sfVector2f){tileSize, tileSize});
                                sfRectangleShape_setFillColor(tile, sfRed);
                                sfText_setColor(text, sfRed);

                                sfRectangleShape_setPosition(tile, (sfVector2f){
                                                                       boardOffset.x + j * tileSize,
                                                                       boardOffset.y + i * tileSize});
                                sfRenderWindow_drawRectangleShape(window, tile, NULL);
                                sfRectangleShape_destroy(tile); 

                                sfTexture *whiteKingTexture = sfTexture_createFromFile("WhiteKing.png", NULL);
                                sfSprite *whiteKingSprite = sfSprite_create();
                                sfSprite_setTexture(whiteKingSprite, whiteKingTexture, sfTrue);
                                sfTexture *blackKingTexture = sfTexture_createFromFile("BlackKing.png", NULL);
                                sfSprite *blackKingSprite = sfSprite_create();
                                sfSprite_setTexture(blackKingSprite, blackKingTexture, sfTrue);
                                sfSprite *pieceSprite = NULL;
                                sfSprite_setScale(whiteKingSprite, (sfVector2f){0.90f, 0.90f});
                                sfSprite_setScale(blackKingSprite, (sfVector2f){0.90f, 0.90f});
                                pieceSprite = (CLIENT_ID == 0) ? whiteKingSprite : blackKingSprite;
                                if (CLIENT_ID == 0)
                                {
                                    sfSprite_setPosition(pieceSprite, (sfVector2f){
                                                                          boardOffset.x + j * tileSize,
                                                                          boardOffset.y + i * tileSize - 13});
                                }
                                else
                                {
                                    sfSprite_setPosition(pieceSprite, (sfVector2f){
                                                                          boardOffset.x + j * tileSize - 9,
                                                                          boardOffset.y + i * tileSize - 9});
                                }
                                sfRenderWindow_drawSprite(window, pieceSprite, NULL);
                            }
                    }
                    sfRenderWindow_display(window);

                    while (sfRenderWindow_isOpen(window))
                    {
                        sfEvent event;
                        while (sfRenderWindow_pollEvent(window, &event))
                        {
                            if (event.type == sfEvtClosed)
                            {
                                sfRenderWindow_close(window);
                            }
                        }
                    }
                }

                deserializeGameSession(&session, buffer);
                drawChessBoard(&session, window, mode, tileSize, boardSize, boardOffset);
                writeTurnInfo(window, CLIENT_ID, &session, mode);

                printf("%d sau %d", session.game.currentPlayer, CLIENT_ID);
                fflush(stdout);

                if (session.game.currentPlayer == CLIENT_ID)
                {

                    sfVector2i firstClickPos = {-1, -1};
                    sfVector2i secondClickPos = {-1, -1};
                    int clickCount = 0;

                    char *move = NULL;

                    int moveIsValid = 0;

                    while (!moveIsValid && sfRenderWindow_isOpen(window))
                    {
                        sfEvent event;

                        int x = 0;
                        while (x == 0)
                        {
                            sfRenderWindow_pollEvent(window, &event);

                            if (event.type == sfEvtClosed)
                            {

                                sfRenderWindow_close(window);
                            }
                            else if (event.type == sfEvtMouseButtonPressed && event.mouseButton.button == sfMouseLeft)
                            {
                              
                                if (clickCount == 0)
                                {

                                    firstClickPos.x = event.mouseButton.x;
                                    firstClickPos.y = event.mouseButton.y;
                                    clickCount++;

                                    sfTime delayTime = sfMilliseconds(1000); 
                                    sfSleep(delayTime);
                                }
                                else if (clickCount == 1)
                                {
                                    secondClickPos.x = event.mouseButton.x;
                                    secondClickPos.y = event.mouseButton.y;
                                    clickCount++;

                                    move = pixelToChessCoords(boardOffset, tileSize, firstClickPos, secondClickPos);

                                    if (move != NULL)
                                    {                                       
                                        
                                        if (write(sd, move, strlen(move)) <= 0)
                                        {

                                            perror("[client]Error writing to server.\n");
                                            exit(1); 
                                            break;
                                        }
                                        usleep(5000);

                                        x = 1;
                                    }
                                    else
                                    {
                                        clickCount = 0;
                                        firstClickPos = (sfVector2i){-1, -1};
                                        secondClickPos = (sfVector2i){-1, -1};
                                    }
                                }
                            }
                        }

                        memset(buffer, 0, sizeof(buffer));
                        bytes_read = read(sd, buffer, sizeof(buffer) - 1);
                        usleep(5000);
                        if (bytes_read <= 0)
                        {
                            perror("[client]Error reading from server.\n");
                            exit(1);
                        }
                        buffer[bytes_read] = '\0';
                        if (strncmp(buffer, "Check-Mate", strlen("Check-Mate")) == 0)
                        {
                            printf(" Check-Mate, Player %d has won\n", CLIENT_ID);
                            fflush(stdout);

                            memset(buffer, 0, sizeof(buffer));
                            bytes_read = read(sd, buffer, sizeof(buffer) - 1);
                            usleep(5000);
                            if (bytes_read <= 0)
                            {
                                perror("[client]Error reading from server.\n");
                                exit(1); 
                            }
                            deserializeGameSession(&session, buffer);
                            drawChessBoard(&session, window, mode, tileSize, boardSize, boardOffset);

                            sfFont *font = sfFont_createFromFile("arial_narrow_7.ttf");
                            sfText *text = sfText_create();

                            int numberOfWinner = CLIENT_ID;
                            char winText[50]; 
                            snprintf(winText, sizeof(winText), "Player %d has Won", numberOfWinner);
                            sfText_setString(text, winText);

                            sfText_setFont(text, font);
                            sfText_setCharacterSize(text, 24);
                            sfText_setColor(text, sfRed);
                            sfFloatRect textRect = sfText_getLocalBounds(text);
                            sfText_setOrigin(text, (sfVector2f){textRect.width / 2, textRect.height / 2});
                            sfText_setPosition(text, (sfVector2f){mode.width / 2.0f, 30});

                            sfRenderWindow_clear(window, sfBlack);
                            drawChessBoard(&session, window, mode, tileSize, boardSize, boardOffset);
                            sfRenderWindow_drawText(window, text, NULL);
                            sfRenderWindow_display(window);

                            for (int i = 0; i < 8; ++i)
                            {
                                for (int j = 0; j < 8; ++j)
                                    if (session.game.board[i][j].type == 'K' && session.game.board[i][j].color == 1 - CLIENT_ID)
                                    {

   
                                        sfRectangleShape *tile = sfRectangleShape_create();
                                        sfRectangleShape_setSize(tile, (sfVector2f){tileSize, tileSize});
                                        sfRectangleShape_setFillColor(tile, sfRed);
                                        sfRectangleShape_setPosition(tile, (sfVector2f){
                                                                               boardOffset.x + j * tileSize,
                                                                               boardOffset.y + i * tileSize});
                                        sfRenderWindow_drawRectangleShape(window, tile, NULL);
                                        sfRectangleShape_destroy(tile); 
                                        sfTexture *whiteKingTexture = sfTexture_createFromFile("WhiteKing.png", NULL); 
                                        sfSprite *whiteKingSprite = sfSprite_create();
                                        sfSprite_setTexture(whiteKingSprite, whiteKingTexture, sfTrue);
                                        sfTexture *blackKingTexture = sfTexture_createFromFile("BlackKing.png", NULL);
                                        sfSprite *blackKingSprite = sfSprite_create();
                                        sfSprite_setTexture(blackKingSprite, blackKingTexture, sfTrue);
                                        sfSprite *pieceSprite = NULL;

                                        sfSprite_setScale(whiteKingSprite, (sfVector2f){0.90f, 0.90f});
                                        sfSprite_setScale(blackKingSprite, (sfVector2f){0.90f, 0.90f});

                                        pieceSprite = (1 - CLIENT_ID == 0) ? whiteKingSprite : blackKingSprite;
                                        if (1 - CLIENT_ID == 0)
                                        {
                                            sfSprite_setPosition(pieceSprite, (sfVector2f){
                                                                                  boardOffset.x + j * tileSize,
                                                                                  boardOffset.y + i * tileSize - 13});
                                        }
                                        else
                                        {
                                            sfSprite_setPosition(pieceSprite, (sfVector2f){
                                                                                  boardOffset.x + j * tileSize - 9,
                                                                                  boardOffset.y + i * tileSize - 9});
                                        }
                                        sfRenderWindow_drawSprite(window, pieceSprite, NULL);
                                    }
                            }
                            sfRenderWindow_display(window);

                            while (sfRenderWindow_isOpen(window))
                            {
                                sfEvent event;
                                while (sfRenderWindow_pollEvent(window, &event))
                                {
                                    if (event.type == sfEvtClosed)
                                    {
                                        sfRenderWindow_close(window);
                                    }
                                }
                            }
                        }
                        if (strncmp(buffer, "Invalid move", strlen("Invalid move")) == 0)
                        {
                            clickCount = 0;
                            firstClickPos = (sfVector2i){-1, -1};
                            secondClickPos = (sfVector2i){-1, -1};
                        }
                        else
                        {
                            moveIsValid = 1;

                            deserializeGameSession(&session, buffer); // Update the board with the latest state
                            drawChessBoard(&session, window, mode, tileSize, boardSize, boardOffset);
                            writeTurnInfo(window, CLIENT_ID, &session, mode);

                            break; 
                        }
                    }
                }
                else
                {
                    printf("Waiting for the other player...\n");
                }
            }

            close(sd);
        }
    }
}