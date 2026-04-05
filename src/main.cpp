#include <FastLED.h>
#include <bitset>
#include <array>
#include <vector>
#include <deque>

#define DATA_PIN D6

//74HC595 connections
const int latch_clock = D4; //pin12 on 74HC595
const int shift_clock = D5; //pin11 on 74HC595
const int serial_data_Input_A = D3; //pin14 on 74HC595
const int reset = D8;

//74HC165 connections
int sh_ld = D0;//pin1 on 74HC165
int sh_inh = D10;//pin15 on 74HC165
int nQH = D7;//pin7 on 74HC165
int clk = D2;//pin2 on 74HC165

class HallArray{
    private:

    void setColumn(int columNum){

      uint8_t columnBit[8] = {1,2,4,8,16,32,64,128};

      uint8_t selectedColumnBit;

      selectedColumnBit = columnBit[columNum];

      delayMicroseconds(5000);
     
      digitalWrite(latch_clock, LOW);
     
      shiftOut(serial_data_Input_A, shift_clock, MSBFIRST, selectedColumnBit);

      digitalWrite(latch_clock, HIGH);

      delayMicroseconds(5000);
    }
   
    std::bitset<8> getRowState(){

      digitalWrite(sh_ld, LOW);
      delayMicroseconds(50);
      digitalWrite(sh_ld, HIGH);
      delayMicroseconds(50);
     
      digitalWrite(sh_inh, LOW);

      std::bitset<8> row;

      for(int y = 0; y < 8; y++){//reading and shifting out next bit from 74HC165
        row[y] = digitalRead(nQH);
        digitalWrite(clk, HIGH);
        digitalWrite(clk, LOW);
      }

      digitalWrite(sh_inh, HIGH);
   
      return row;
    }
   
    public:
   
    std::array<std::bitset<8>, 8> read(){

      std::array<std::bitset<8>, 8> state;

      for(int x = 0; x < 8; x++){

        setColumn(x);

        delayMicroseconds(125);

        state[x] = getRowState();
      }
      return state;
    }
  };

struct Point{
  int x = -1;
  int y = -1;

  Point(int _x, int _y){
    x = _x;
    y = _y;
  }

  Point(){}

  bool operator==(const Point& other) const{
    return (x == other.x) && (y == other.y);
  }

  bool inBounds(){
    bool inBound = false;

    if(((x >= 1) && (x <= 8)) && ( (y >= 1) && (y <= 8))){
      inBound = true;
    }

    return inBound;
  }

  void print(){

    Serial.print("x: ");
    Serial.print(x);

    Serial.print(" y: ");
    Serial.print(y);

  }
};

class Chess{
  public:

  enum Team{
    RED,
    BLUE,
    NONE
  };

  struct Move{

    enum Type{
      UNCONTESTED,
      CONTESTED,
      INCHECK
  };

    Type type;
    Point position;

    Move(){}

    Move(Point _position, Type _type){
      position = _position;
      type = _type;
    }

    bool operator==(const Move& other) const{
      return (type == other.type) && (position == other.position);
    }

    void print(){

      position.print();

      Serial.print(" ");

      switch(type){
        case UNCONTESTED:

          Serial.println("UNCONTESTED");

        break;

        case CONTESTED:

          Serial.println("CONTESTED");

        break;

        case INCHECK:

          Serial.println("INCHECK");

        break;
      }
    }
  };

  struct Piece{

    enum Type{
      PAWN,
      KNIGHT,
      BISHOP,
      ROOK,
      QUEEN,
      KING
  };

    Type type;
    std::vector<Move> moves;
    Team team;
    bool isFirstMove = true;

    Piece(Type _type, Team _team){
      type = _type;
      team = _team;
    }

    Piece(){

    }

    bool operator==(const Piece& other) const{
      return (type == other.type) && (team == other.team);
    }

    void printPath(){
      for(int i = 0 ; i < moves.size(); i++){
        moves[i].print();      
      }

      if (isFirstMove){
        Serial.println("is first move");
      }else{
        Serial.println(" not first move");
      }
      Serial.println();
    }
  };

  struct PieceRegistry{

    struct Entry{
      Point point;
      Piece piece;

      Entry(Point _point, Piece _piece){
        point = _point;
        piece = _piece;        
      }

      Entry(){}

      bool operator==(const Entry& other) const{
        return (point == other.point) && (piece == other.piece);
      }
    };

    std::deque<Entry> reg;

    PieceRegistry(){

      //reg.push_back(PieceRegistry::Entry(Point(3,6), Piece(Piece::PAWN, RED)));
      //reg.push_back(PieceRegistry::Entry(Point(1,2), Piece(Piece::PAWN, RED)));

      reg.push_back(PieceRegistry::Entry(Point(4,4), Piece(Piece::ROOK, BLUE)));
      //reg.push_back(PieceRegistry::Entry(Point(2,4), Piece(Piece::PAWN, BLUE)));
    }

  };
 
  PieceRegistry pieceRegistry;  
  Team currentTeam = BLUE;
  bool inCheck = false; 

  int getPiecIndexAtPoint(Point point){

    int pieceIndex = -1;

    if(pieceRegistry.reg.size() > 0){

      for(int i = 0; i < pieceRegistry.reg.size(); i++){

        if(point == pieceRegistry.reg[i].point){
          pieceIndex = i;
          break;
        }
      }
    }
    return pieceIndex;
  }

  bool isSquareOccupied(Point point){

    bool isOccupied = false;    
    int pieceIndex = getPiecIndexAtPoint(point);

    if(pieceIndex >= 0){
      isOccupied = true;
    }

  return isOccupied;
}

  bool ifPieceHasMoves(Point point){

    bool ifPieceHasMoves = false;

    int pieceIndex = getPiecIndexAtPoint(point);

    if(pieceIndex >= 0){
      if(pieceRegistry.reg[pieceIndex].piece.moves.size() > 0){
        ifPieceHasMoves = true;
      }
    }   

    return ifPieceHasMoves;
  }

  void removeTakenPiece(Point point){

    int pieceIndex = getPiecIndexAtPoint(point);

    if(pieceIndex >= 0){
      pieceRegistry.reg.erase(pieceRegistry.reg.begin() + pieceIndex);
    }    
  }

  void changeTeam(){
    if(currentTeam == RED){
      currentTeam = BLUE;
    }else if(currentTeam == BLUE){
      currentTeam = RED;
    }
  }

  Team getPieceTeam(Point point){

    Team pieceTeam = Team::NONE;

    int pieceIndex = getPiecIndexAtPoint(point);

    if(pieceIndex >= 0){
      pieceTeam = pieceRegistry.reg[pieceIndex].piece.team;
    }

    return pieceTeam;
  }

  std::vector<Move> theFunctionToBeTested(Point start, Point stop, Team team){

    Serial.println("straight line to be generated");

    int squareCount = 0;
    int xMod = 0;
    int yMod = 0;
    int xDifference = start.x - stop.x;
    int yDifference = start.y - stop.y;
    std::vector<Move> moves;

    if(xDifference > 0){
      xMod = 1;
    }else if(xDifference < 0){
      xMod = -1;
    }

    if(yDifference > 0){
      yMod = 1;
    }else if(yDifference < 0){
      yMod = -1;
    }

    xDifference = abs(xDifference);
    yDifference = abs(yDifference);

    if(xDifference > yDifference){
      squareCount = xDifference;
    }else{
      squareCount = yDifference;
    }

    bool pathBlocked = false;

    Serial.println("point difference calculated");

    Serial.print("xDifference: ");
    Serial.println(xDifference);

    Serial.print("yDifference: ");
    Serial.println(yDifference);

    Serial.print("squareCount: ");
    Serial.println(squareCount);



    for(int i = 1; i < squareCount; i++){

      Point point(start.x + xMod * i, start.y + yMod * i);
      bool pointInbounds = point.inBounds();

      if(!pathBlocked && pointInbounds){

        Serial.println("path not blocked and in bounds");

        bool isOcuppied = isSquareOccupied(point);

        if(isOcuppied){

          Serial.println("point is occupied");

          pathBlocked = true;

          Team teamOfOcupiedPiece = getPieceTeam(point);

          if(team != teamOfOcupiedPiece){
            Serial.println("point is contested");
            moves.push_back(Move(point, Move::CONTESTED));
          }
        }else{

          Serial.println("point is uncontested");
          moves.push_back(Move(point, Move::UNCONTESTED));
        }
      }
    }

    Serial.println("straight Moves Generated");


    return moves;
  }

  std::vector<Move> generateStraightMoves(Team team, Point startingPoint){
    std::vector<Move> moves;

    moves = theFunctionToBeTested(startingPoint, Point(8, startingPoint.y), team);

    return moves;
  }

  std::vector<Move> getPawnMoves(Team team, Point point, bool isFirstMove){

    std::vector<Move> moves;
    int x = point.x;
    int y = point.y;

    std::vector<Point> testPoints;

    if(team == RED){
      testPoints.push_back(Point(x + 1, y + 1));
      testPoints.push_back(Point(x -1, y + 1));
      testPoints.push_back(Point(x, y + 1));
      testPoints.push_back(Point(x, y + 2));
    }else if(team == BLUE){
      testPoints.push_back(Point(x + 1, y - 1));
      testPoints.push_back(Point(x -1, y - 1));
      testPoints.push_back(Point(x, y - 1));
      testPoints.push_back(Point(x, y - 2));
    }

    int testPieceIndex = -1;

    for(int i = 0; i < 2; i++){

      if(isSquareOccupied(testPoints[i])){

        testPieceIndex = getPiecIndexAtPoint(testPoints[i]);

        if((pieceRegistry.reg[testPieceIndex].piece.team != currentTeam) && (testPoints[i].inBounds()) ){

          if(pieceRegistry.reg[testPieceIndex].piece.type == Piece::KING){

            moves.push_back(Move(testPoints[i], Move::INCHECK));
          }else{

            moves.push_back(Move(testPoints[i], Move::CONTESTED));
          }      
        }
      }
    }

    if((!isSquareOccupied(testPoints[2])) && (testPoints[2].inBounds())){

      moves.push_back(Move(testPoints[2], Move::UNCONTESTED));

      if((!isSquareOccupied(testPoints[3])) && (isFirstMove)){

        moves.push_back(Move(testPoints[3], Move::UNCONTESTED));  
      }
    }

    return moves;
  }

  std::vector<Move> getRookMoves(Team team, Point point){
    std::vector<Move> moves = generateStraightMoves(team, point);

    return moves;
  }

  void updatePieceMoves(){

    Serial.println("Move generation start");

    for(int i = 0; i < pieceRegistry.reg.size(); i++){
      Point position = pieceRegistry.reg[i].point;

      if(pieceRegistry.reg[i].piece.team == currentTeam){

        switch(pieceRegistry.reg[i].piece.type){

          case Piece::PAWN:

            pieceRegistry.reg[i].piece.moves = getPawnMoves(pieceRegistry.reg[i].piece.team, position, pieceRegistry.reg[i].piece.isFirstMove);

          break;

          case Piece::ROOK:

            pieceRegistry.reg[i].piece.moves = getRookMoves(pieceRegistry.reg[i].piece.team, position);

          break;
        }
      }else{
        pieceRegistry.reg[i].piece.moves.clear();
      }
    }
    Serial.println("Move generation complete");
  }

  int getPieceCount(){
    return pieceRegistry.reg.size();
  }

  Point getPiecePosition(int pieceIndex){
    return pieceRegistry.reg[pieceIndex].point;
  }

  std::vector<Move> getPieceMoves(Point point){

    int pieceIndex = getPiecIndexAtPoint(point);

    std::vector<Move> pieceMoves;

    if(pieceIndex >= 0){
      pieceMoves = pieceRegistry.reg[pieceIndex].piece.moves;
    }

    return pieceMoves;
  }

  void movePiece(Point startingPosition, Point newPosition){

    int pieceIndex = getPiecIndexAtPoint(startingPosition);

    if(pieceIndex >= 0){
      pieceRegistry.reg[pieceIndex].point = newPosition;

      pieceRegistry.reg[pieceIndex].piece.isFirstMove = false;
    }    
  }

  Chess(){    
  }
};

class Board{
  public:

  enum SensorBehavior{
      rising,
      falling
  };

  struct SquareState{
    Point point;
    SensorBehavior behavior;

    SquareState(Point _point, SensorBehavior _behavior){
      point = _point;
      behavior = _behavior;

      Serial.print("point changed: ");
      point.print();

      if(behavior == SensorBehavior::rising){
        Serial.println(" rising");
      }else{
        Serial.println(" falling");
      }
    }

    bool operator==(const SquareState& other) const{
      return (point == other.point) && (behavior == other.behavior);
    }
  };

  std::vector<SquareState> getInputChanges(std::array<std::bitset<8>, 8> current, std::array<std::bitset<8>, 8> previous){

    std::vector<SquareState> state;
    SensorBehavior behavior;

    for(int x = 0; x < 8; x++){
      for(int y = 0; y < 8; y++){
        if(current[x][y] != previous[x][y]){
          if(current[x][y] > previous[x][y]){
            behavior = rising;
          }else{
            behavior = falling;
          }
          state.push_back(SquareState(Point(x +1,y +1),behavior));
        }
      }
    }
    return state;
  }

  enum BoardState{
    awaitingInitialPieceSetup,
    awaitingPiecePickup,
    awaitingPiecePlacement,
    incorrectPiecePickup,
    incorrectPiecePlacement
  };

  int getLedAddress(Point point){//sets leds layed out in a zigzag fashion

    int x = point.x;
    int y = point.y;

    int ledArrayPos = ((y - 1) * 8);

    if(y % 2 == 0){
      ledArrayPos = ledArrayPos + (8 - x);
    }else{
      ledArrayPos = ledArrayPos + (x - 1);
    }
    return ledArrayPos;
  }

  void setLedColor(Point point, CRGB color, CRGB leds[64]){

    int ledAddress = getLedAddress(point);

    leds[ledAddress] = color;
  }

  void displayMoves(std::vector<Chess::Move> moves, CRGB leds[64]){

    for(int i = 0; i < moves.size(); i++){

      switch(moves[i].type){
        case Chess::Move::UNCONTESTED:
          setLedColor(moves[i].position, CRGB::Green, leds);
        break;

        case Chess::Move::CONTESTED:
          setLedColor(moves[i].position, CRGB::Red, leds);
        break;
      }
    }
  }

  void clearLeds(CRGB leds[64]){

    for(int i = 0; i < 64; i++){
      leds[i] = CRGB::Black;
    }
  }

  BoardState boardState = awaitingInitialPieceSetup;
  std::vector<Chess::Move> currentPieceMoves;
  Point currentPiecePosition = Point(-1,-1);
  int incorrectPieceIndex = -1;
  Chess chess;

  bool isPointOccupied(Point point, std::array<std::bitset<8>, 8> current){
    bool isOccupied = false;

    if(current[point.x - 1][point.y - 1] == 1){
      isOccupied = true;
    }

    return isOccupied;
  }

  void awaitingInitialPieceSetupRoutine(CRGB leds[64], std::array<std::bitset<8>, 8> sensorState){

    clearLeds(leds);

    int pieceCount = chess.getPieceCount();

    int piecesNotSetup = pieceCount;

    if(pieceCount > 0){

      for(int i = 0; i < pieceCount; i++){

        Point point = chess.getPiecePosition(i);
        bool isOccupied = isPointOccupied(point, sensorState);
        Chess::Team team = chess.getPieceTeam(point);

        if(isOccupied){
          piecesNotSetup = piecesNotSetup - 1;
          setLedColor(point, CRGB::Black, leds);
        }else{
          if(team == Chess::Team::RED){
            setLedColor(point, CRGB::Red, leds);
          }
          if(team == Chess::Team::BLUE){
            setLedColor(point, CRGB::Blue, leds);
          }
        }
      }

      FastLED.show();

      if(piecesNotSetup == 0){
        boardState = awaitingPiecePickup;
        chess.updatePieceMoves();
      }
    }
  }

  void awaitingPiecePickupRoutine(SquareState change, CRGB leds[64]){

    currentPiecePosition = Point(-1,-1);
    incorrectPieceIndex = -1;

    bool isChangedPointOccupied = chess.isSquareOccupied(change.point);

    Serial.println("piece pickup routine");

    if(isChangedPointOccupied){
      bool ifPieceHasMoves = chess.ifPieceHasMoves(change.point);

      if(ifPieceHasMoves){
        currentPiecePosition = change.point;
        boardState = awaitingPiecePlacement;

        currentPieceMoves = chess.getPieceMoves(change.point);
        displayMoves(currentPieceMoves, leds);

        FastLED.show();

      }else{
        incorrectPieceIndex = chess.getPiecIndexAtPoint(change.point);
        boardState = incorrectPiecePickup;

        setLedColor(change.point, CRGB::Red, leds);

        FastLED.show();

        //change square to red
      }
    }
  }

  void incorrectPiecePickupRoutine(SquareState change, CRGB leds[64]){

    Point position = chess.getPiecePosition(incorrectPieceIndex);

    if(change.point == position){
      incorrectPieceIndex = -1;

      boardState = awaitingPiecePickup;
      setLedColor(change.point, CRGB::Black, leds);

      FastLED.show();
    }
  }

  void awaitingPiecePlacementRoutine(SquareState change, CRGB leds[64]){

    for(int i = 0; i < currentPieceMoves.size(); i++){

      if( (change.point == currentPieceMoves[i].position)){

        int moveIndex = i;

        if((currentPieceMoves[moveIndex].type == Chess::Move::UNCONTESTED)){
          chess.movePiece(currentPiecePosition, change.point);
          chess.changeTeam();
          chess.updatePieceMoves();
          boardState = awaitingPiecePickup;
          clearLeds(leds);
          FastLED.show();
          break;
        }else if((currentPieceMoves[moveIndex].type == Chess::Move::CONTESTED)){
          chess.removeTakenPiece(change.point);
          currentPieceMoves[moveIndex].type = Chess::Move::UNCONTESTED;
          break;
        }     
      }
    }
  }

  void processInput(std::array<std::bitset<8>, 8> current, std::array<std::bitset<8>, 8> previous, CRGB leds[64]){

    std::vector<SquareState> inputChanges = getInputChanges(current,previous);

    if(inputChanges.size() > 0){

      SquareState change = inputChanges[0];

      switch(boardState){

        case awaitingInitialPieceSetup:

          awaitingInitialPieceSetupRoutine(leds, current);

        break;

        case awaitingPiecePickup:

          awaitingPiecePickupRoutine(change, leds);

        break;

        case incorrectPiecePickup:

          incorrectPiecePickupRoutine(change, leds);

        break;

        case awaitingPiecePlacement:

          awaitingPiecePlacementRoutine(change, leds);

        break;

      }
    }
  }
};

std::array<std::bitset<8>, 8> currentHallArrayState;
std::array<std::bitset<8>, 8> previousHallArrayState;

HallArray hallArray;
Board board;
CRGB leds[64];

void setup() {

  Serial.begin(115200);

  Serial.println("setup begin");
  Serial.println();

  //74HC595 pin setup
  pinMode(latch_clock, OUTPUT);
  pinMode(shift_clock, OUTPUT);
  pinMode(serial_data_Input_A, OUTPUT);
  pinMode(reset, OUTPUT);
  digitalWrite(reset, HIGH);

  //74HC165 pin setup
  pinMode(sh_ld, OUTPUT);
  pinMode(sh_inh, OUTPUT);
  digitalWrite(sh_inh, LOW);
  pinMode(clk, OUTPUT);
  pinMode(nQH, INPUT_PULLUP);

  previousHallArrayState = hallArray.read();

  FastLED.addLeds<NEOPIXEL, DATA_PIN>(leds, 64);
  FastLED.setBrightness(25);

  delay(150);

  int ledIterator = 0;

  for(int x = 0; x < 8; x++){//sets all leds to black
    for(int y = 0; y < 8; y++){

      leds[ledIterator] = CRGB::Black;
      ledIterator = ledIterator + 1;
    }
  }
 
  FastLED.show();

  board.awaitingInitialPieceSetupRoutine(leds, currentHallArrayState);
 
  Serial.println("Setup complete");
}

void loop(){

  currentHallArrayState = hallArray.read();

  if(previousHallArrayState != currentHallArrayState){

      board.processInput(currentHallArrayState, previousHallArrayState, leds);

      previousHallArrayState = currentHallArrayState;
    }
}
