#include <FastLED.h>
#include <bitset>
#include <array>
#include <vector>

#define DATA_PIN D6
CRGB leds[64];

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

class Chess{
  private:

  enum Behavior{
  rising,
  falling
};

  struct Point{
    int x;
    int y;

    Point(int _x, int _y){
      this->x = _x;
      this->y = _y;
    }

    Point(){}

    bool operator==(const Point& other) const{
      return (this->x == other.x) && (this->y == other.y);
    }

    bool inBounds(){
      bool inBound = false;

      if( (this->x >= 1) && (this->x <= 8)){
        inBound = true;
      }

      if( (this->y >= 1) && (this->y <= 8)){
        inBound = true;
      }

      return inBound;
    }
  };

  struct SquareState{
    Point point;
    Behavior behavior;

    SquareState(Point _point, Behavior _behavior){
      this->point = _point;
      this->behavior = _behavior;
    }

    bool operator==(const SquareState& other) const{
      return (this->point == other.point) && (this->behavior == other.behavior);
    }
  };

  std::vector<SquareState> getHallStateArrayDifferences(std::array<std::bitset<8>, 8> current, std::array<std::bitset<8>, 8> previous){

    std::vector<SquareState> differenceList;

    Behavior behavior;

    for(int x = 0; x < 8; x++){
      for(int y = 0; y < 8; y++){
        if(current[x][y] != previous[x][y]){
          if(current[x][y] > previous[x][y]){
            behavior = rising;
          }else{
            behavior = falling;
          }
          differenceList.push_back(SquareState(Point(x +1,y +1),behavior));
        }
      }
    }
    return differenceList;
  }

  enum PieceType{
    PAWN,
    KNIGHT,
    BISHOP,
    ROOK,
    QUEEN,
    KING
  };

  enum Team{
    RED,
    BLUE
  };

  enum MoveType{
    NONCAPTURE,
    CAPTURE,
    INVALID
  };

  struct Move{
    MoveType type;
    Point position;

    Move(Point _position, MoveType _type){      
      this->position = _position;
      this->type = _type;
    }
    Move(){}
  };

  struct Piece{
    PieceType type;    
    Team team;
    std::vector<Move> moves;
    bool isFirstMove = true;

    Piece(Team _team, PieceType _type){
      this->team = _team;
      this->type = _type;
    }

    Piece(){

    }
  };

  struct PieceRegistry{

    struct Entry{
      Point point;
      Piece piece;

      Entry(Point _point, Piece _piece){
        this->point = _point;
        this->piece = _piece;        
      }

      Entry(){}
    };

    std::vector<Entry> entry;

    PieceRegistry(){
      entry.push_back(Entry(Point(2,2), Piece(RED, PAWN)));
      entry.push_back(Entry(Point(3,3), Piece(BLUE, PAWN)));
    }
  };

  enum Mode{
    awaitingMove,
    awaitingPiecePlacement,
    error
  };

  int getLedAddress(Point point){//sets leds layed out in a zigzag fashion
  
    int ledArrayPos;

    int x = point.x;
    int y = point.y;

    if(y % 2 == 0){
      ledArrayPos = ((y - 1) * 8) + (8 - x);
    }else{
      ledArrayPos = ((y - 1) * 8) + (x - 1);
    }
    return ledArrayPos;
  }

  std::vector<SquareState> inputChange;
  PieceRegistry pieceRegistry;
  bool validPiece = false;
  Team currentTeam = RED;
  Mode mode = awaitingMove;
  std::vector<Point> currentPiecePositions;
  Piece pieceInPlay;
  std::vector<Move> possibleMoves;
  bool piecePossibleMovesUpdated = false;

  bool checkIfPointOccupied(Point point){
    bool pointIsOccupied = false;

    for(int i = 0; i < this->pieceRegistry.entry.size(); i++){
      if(point == this->pieceRegistry.entry[i].point){
        pointIsOccupied = true;
        break;
      }
    }

    return pointIsOccupied;
  }

  Team getPieceTeamAtPoint(Point point){
    Piece piece;

    for(int i = 0; i < this->pieceRegistry.entry.size(); i++){
      if(point == this->pieceRegistry.entry[i].point){
        piece = this->pieceRegistry.entry[i].piece;
        break;
      }
    }

    return piece.team;
  }

  struct SquareOcupationState{

    Team team;
    Point point;

    bool occupied = false;
    bool occupiedByFoe = false;
    bool occupiedByFriendly = false;

    SquareOcupationState(){}
  };

  SquareOcupationState getSquareState(Team team, Point point){
    SquareOcupationState squareOcupationState;

    squareOcupationState.occupied = checkIfPointOccupied(point);

    if(checkIfPointOccupied(point)){

      squareOcupationState.occupied = true;

      Team pieceTeamAtPoint = getPieceTeamAtPoint(point);

      if(pieceTeamAtPoint == team){
        squareOcupationState.occupiedByFriendly = true;
        squareOcupationState.occupiedByFoe = false;
      }else{
        squareOcupationState.occupiedByFriendly = false;
        squareOcupationState.occupiedByFoe = true;
      }
    }    

    return squareOcupationState;
  }

  void setPawnMoves(PieceRegistry::Entry& regEntry){

    Piece piece = regEntry.piece;
    Point position = regEntry.point;
    
    std::vector<Move> moves;
    std::vector<Point> testPoints;
    std::vector<SquareOcupationState> squareOcupationState;

    
    if(piece.team == RED){
      testPoints.push_back(Point(position.x, position.y + 1));
      testPoints.push_back(Point(position.x + 1, position.y + 1));
      testPoints.push_back(Point(position.x - 1, position.y + 1));
      testPoints.push_back(Point(position.x, position.y + 2));      
    }

    for(int i = 0; i < testPoints.size(); i++){
      squareOcupationState.push_back(getSquareState(RED, testPoints[i]));
    }

    if(!squareOcupationState[0].occupied && testPoints[0].inBounds()){
      moves.push_back(Move(testPoints[0], NONCAPTURE));
    }

    if(squareOcupationState[1].occupiedByFoe && testPoints[1].inBounds()){
      moves.push_back(Move(testPoints[1], CAPTURE));
    }

    if(squareOcupationState[2].occupiedByFoe && testPoints[2].inBounds()){
      moves.push_back(Move(testPoints[2], CAPTURE));
    }

    if(piece.isFirstMove && !squareOcupationState[3].occupied && testPoints[3].inBounds()){
      moves.push_back(Move(testPoints[3], NONCAPTURE));
    }

    regEntry.piece.moves = moves;
  }

  void setPieceMoves(){

    if(this->pieceRegistry.entry.size() > 0){
        for(int i = 0; i < this->pieceRegistry.entry.size(); i++){

        Team currentPeaceTeam = this->pieceRegistry.entry[i].piece.team;
        PieceType currentPieceType = this->pieceRegistry.entry[i].piece.type;

        if(this->currentTeam == currentPeaceTeam){

          switch(currentPieceType){

            case PAWN:

              setPawnMoves(this->pieceRegistry.entry[i]);

            break;

            default:

            break;
          }
        }
      }
    }    
  }

  void displayPath(CRGB leds[64]){
    if(this->possibleMoves.size() > 0){
        for(int i = 0; i < this->possibleMoves.size(); i++){

        int ledAddress = getLedAddress(this->possibleMoves[i].position);

        if(this->possibleMoves[i].type == NONCAPTURE){
          leds[ledAddress] = CRGB::Green;
        }else if(this->possibleMoves[i].type == CAPTURE){
          leds[ledAddress] = CRGB::Red;
        }
      }
      FastLED.show();
    }
  }

  public:

  void process(std::array<std::bitset<8>, 8> current, std::array<std::bitset<8>, 8> previous, CRGB leds[64]){

    if(!this->piecePossibleMovesUpdated){//updates possible moves

      setPieceMoves();

      this->piecePossibleMovesUpdated = true;
    }

    this->inputChange = getHallStateArrayDifferences(current, previous);    

    if(this->inputChange.size() > 0){

      Chess::SquareState changedPoint = this->inputChange[0];    

      switch(this->mode){

        case awaitingMove:

        this->validPiece = false;

        for(int i = 0; i < this->pieceRegistry.entry.size(); i++){//cycle through all piece entries

          PieceRegistry::Entry pieceRegistryEntry = this->pieceRegistry.entry[i];

          if( (changedPoint.point == pieceRegistryEntry.point)){

            if( (pieceRegistryEntry.piece.team == this->currentTeam) && (pieceRegistryEntry.piece.moves.size() > 0) ){

              this->pieceInPlay = pieceRegistryEntry.piece;

              this->possibleMoves = pieceRegistryEntry.piece.moves;

              this->validPiece = true;

              displayPath(leds);

              this->mode = awaitingPiecePlacement;
            }
          }
        }        

        break;

        case awaitingPiecePlacement:

        break;

        default:
        break;
      }
    }
  }

  void setLeds(CRGB leds[64]){

    for(int i = 0; i < 64; i++){
      leds[i] = CRGB::Black;
    }
  }
};

Chess chess;
HallArray hallArray;
std::array<std::bitset<8>, 8> currentHallArrayState;
std::array<std::bitset<8>, 8> previousHallArrayState;

void setup() {

  Serial.begin(115200);
  
  FastLED.addLeds<NEOPIXEL, DATA_PIN>(leds, 64);
  FastLED.setBrightness(15);

  int ledIterator = 0;

  for(int x = 0; x < 8; x++){//sets all leds to black
    for(int y = 0; y < 8; y++){

      leds[ledIterator] = CRGB::Black;
      ledIterator = ledIterator + 1;
    }
  }
  
  FastLED.show();

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
}

void loop(){

  currentHallArrayState = hallArray.read();

  if(previousHallArrayState != currentHallArrayState){

    chess.process(currentHallArrayState, previousHallArrayState, leds);    

    previousHallArrayState = currentHallArrayState;
  }
}