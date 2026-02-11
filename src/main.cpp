#include <FastLED.h>
#include <bitset>
#include <array>
#include <vector>

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

class Chess{
  private:

  enum SensorBehavior{
    rising,
    falling
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

      if( (x >= 1) && (x <= 8)){
        inBound = true;
      }

      if( (y >= 1) && (y <= 8)){
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

  struct SquareState{
    Point point;
    SensorBehavior behavior;

    SquareState(Point _point, SensorBehavior _behavior){
      point = _point;
      behavior = _behavior;
    }

    bool operator==(const SquareState& other) const{
      return (point == other.point) && (behavior == other.behavior);
    }
  };

  enum Team{
    RED,
    BLUE
  };

  struct Move{

    enum Type{
      UNCONTESTED,
      CONTESTED,
      INCHECK
  };

    Type type;
    Point position;

    Move(Point _position, Type _type):      
      position(_position),
      type(_type){}
    
    Move() = default;

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
    int id = 0;
    std::vector<Move> moves;
    Team team;
    bool isFirstMove = true;

    Piece(Type _type, Team _team, int _id){
      type = _type;
      team = _team;
      id = _id;
    }

    Piece(){

    }

    bool operator==(const Piece& other) const{
      return (type == other.type) && (team == other.team);
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
    };

    std::vector<Entry> reg;

    PieceRegistry(){
      reg.push_back(PieceRegistry::Entry(Point(2,2), Piece(Piece::PAWN, RED, 0)));
      reg.push_back(PieceRegistry::Entry(Point(1,3), Piece(Piece::PAWN, BLUE, 1)));
      reg.push_back(PieceRegistry::Entry(Point(1,5), Piece(Piece::PAWN, BLUE, 2)));
    }

    bool isSquareOccupied(Point point){
      bool isOccupied = false;

      for(int i = 0; i < reg.size(); i++){
        if(point == reg[i].point){
          isOccupied = true;
        }
      }

      return isOccupied;
    }

    Entry getPieceAtPoint(Point point){

      Entry piece;

      if(reg.size() > 0){

        for(int i = 0; i < reg.size(); i++){

          if(point == reg[i].point){
            piece = reg[i];
          }
        }
      }
      return piece;
    }

    void removeTakenPiece(int id){
      reg.erase(reg.begin() + id);
    }
  };

  enum BoardState{
    awaitingPiecePickup,
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
  
  PieceRegistry pieceRegistry;
  PieceRegistry::Entry currentPiece;
  Team currentTeam = RED;
  BoardState boardState = awaitingPiecePickup;
  std::vector<Move> validMoves;
  bool inCheck = false;
  bool movesUpToDate = false;

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

  void setPawnMoves(PieceRegistry::Entry& pieceRegEntry){

    std::vector<Move> moves;
    int x = pieceRegEntry.point.x;
    int y = pieceRegEntry.point.y;

    std::vector<Point> testPoints;

    if(pieceRegEntry.piece.team == RED){
      testPoints.push_back(Point(x + 1, y + 1));
      testPoints.push_back(Point(x -1, y + 1));
      testPoints.push_back(Point(x, y + 1));
      testPoints.push_back(Point(x, y + 2));
    }else if(pieceRegEntry.piece.team == BLUE){
      testPoints.push_back(Point(x + 1, y - 1));
      testPoints.push_back(Point(x -1, y - 1));
      testPoints.push_back(Point(x, y - 1));
      testPoints.push_back(Point(x, y - 2));
    }

    PieceRegistry::Entry testPiece;
    bool isSquareOccupied;

    for(int i = 0; i < 2; i++){

      testPiece = pieceRegistry.getPieceAtPoint(testPoints[i]);

      isSquareOccupied = pieceRegistry.isSquareOccupied(testPoints[i]);

      if(!isSquareOccupied){

        if((testPiece.piece.team != currentTeam) && (testPoints[i].inBounds()) ){

          if(testPiece.piece.type == Piece::KING){

            moves.push_back(Move(testPoints[i], Move::INCHECK));
          }else{

            moves.push_back(Move(testPoints[i], Move::CONTESTED));
          }      
        }
      }
    }

    testPiece = pieceRegistry.getPieceAtPoint(testPoints[2]);

    if((isSquareOccupied) && (testPoints[2].inBounds())){

      moves.push_back(Move(testPoints[2], Move::UNCONTESTED));

      testPiece = pieceRegistry.getPieceAtPoint(testPoints[3]);

      if((isSquareOccupied) && (pieceRegEntry.piece.isFirstMove)){

        moves.push_back(Move(testPoints[3], Move::UNCONTESTED));  
      }
    }

    pieceRegEntry.piece.moves = moves;
  }

  void setPieceMoves(){

    for(int i = 0; i < pieceRegistry.reg.size(); i++){

      if(pieceRegistry.reg[i].piece.team == currentTeam){

        switch(pieceRegistry.reg[i].piece.type){

          case Piece::PAWN:

            setPawnMoves(pieceRegistry.reg[i]);

          break;
        }
      }
    }
  }

  void changeTeam(){
    if(currentTeam == RED){
      currentTeam = BLUE;
    }else{
      currentTeam = RED;
    }
  }

  public:

  void process(std::array<std::bitset<8>, 8> current, std::array<std::bitset<8>, 8> previous, CRGB leds[64]){

    if(!movesUpToDate){

      setPieceMoves();

      movesUpToDate = true;
    }

    std::vector<SquareState> inputChange = getInputChanges(current,previous);

    if(inputChange.size() > 0){

      bool isPointOccupied = pieceRegistry.isSquareOccupied(inputChange[0].point);

      switch(boardState){

        case awaitingPiecePickup:          

          if(!isPointOccupied){

            PieceRegistry::Entry pieceAtChangedPoint = pieceRegistry.getPieceAtPoint(inputChange[0].point);

            if((pieceAtChangedPoint.piece.team == currentTeam) && (pieceAtChangedPoint.piece.moves.size() > 0) && (inputChange[0].behavior == falling)){

              currentPiece = pieceAtChangedPoint;
              boardState = awaitingPiecePlacement;
              //pieceAtChangedPoint = nullptr;

              for(int i = 0; i < currentPiece.piece.moves.size(); i++){

                CRGB color;

                switch(currentPiece.piece.moves[i].type){

                  case Move::UNCONTESTED:
                    color = CRGB::Green;
                  break;

                  case Move::CONTESTED:
                    color = CRGB::Red;
                  break;
                }

                int ledAddress = getLedAddress(currentPiece.piece.moves[i].position);
                leds[ledAddress] = color;
              }

              FastLED.show();
            }
          }
        break;

        case awaitingPiecePlacement:

        for(int i = 0; i < currentPiece.piece.moves.size(); i++){

          if( (currentPiece.piece.moves[i].position == inputChange[0].point) && (inputChange[0].behavior == rising) && (currentPiece.piece.moves[i].type == Move::UNCONTESTED)){

            currentPiece.point = inputChange[0].point;
            boardState = awaitingPiecePickup;
            movesUpToDate = false;

            if(currentPiece.piece.isFirstMove){
              currentPiece.piece.isFirstMove = false;
            }
            int ledIterator = 0;

            for(int x = 0; x < 8; x++){//sets all leds to black
              for(int y = 0; y < 8; y++){
                leds[ledIterator] = CRGB::Black;
                ledIterator = ledIterator + 1;
              }
            }            
            FastLED.show();

            changeTeam();

            break;
          }

          if( (currentPiece.piece.moves[i].position == inputChange[0].point) && (inputChange[0].behavior == falling) && (currentPiece.piece.moves[i].type == Move::CONTESTED)){
            PieceRegistry::Entry removedPiece = pieceRegistry.getPieceAtPoint(inputChange[0].point);
            pieceRegistry.removeTakenPiece(removedPiece.piece.id);
            currentPiece.piece.moves[i].type = Move::UNCONTESTED;
          }
        }
        break;
      }
    }
  }

  void testPiecePathOutput(){

    setPieceMoves();

    for(int i = 0; i < pieceRegistry.reg[0].piece.moves.size(); i++){

      pieceRegistry.reg[0].piece.moves[i].print();
    }
  }
};

HallArray hallArray;
Chess chess;
CRGB leds[64];

std::array<std::bitset<8>, 8> currentHallArrayState;
std::array<std::bitset<8>, 8> previousHallArrayState;

void setup() {

  Serial.begin(115200);  

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
}

void loop(){

  currentHallArrayState = hallArray.read();

  if(previousHallArrayState != currentHallArrayState){

    chess.process(currentHallArrayState, previousHallArrayState, leds);

    previousHallArrayState = currentHallArrayState;
  }
}