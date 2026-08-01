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

  bool operator!=(const Point& other) const{
    return (x != other.x) || (y != other.y);
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

std::vector<Point> generateDiagnalLine(Point start, int xMod, int yMod){
  std::vector<Point> line;

  int pointX = start.x;
  int pointY = start.y;
  bool pathIsValid = true;

  while(pathIsValid){

    pointX = pointX + xMod;
    pointY = pointY + yMod;

    Point testPoint(pointX, pointY);

    if(testPoint.inBounds()){

      line.push_back(testPoint);

    }else{
      pathIsValid = false;
    }
  }

  return line;
}

std::vector<Point> generateStraightLine(Point start, Point stop, bool isEndInclusive){

  int xMod = 0;
  int yMod = 0;
  int xDifference = 0;
  int yDifference = 0;
  int pointAmount = 0;

  if(start.x == stop.x){

    if(start.y > stop.y){
      yMod = -1;
    }else{
      yMod = 1;
    }
    pointAmount = abs(start.y - stop.y);

  }else if(start.y == stop.y){

    if(start.x > stop.x){
      xMod = -1;
    }else{
      xMod = 1;
    }

    pointAmount = abs(start.x - stop.x);
  }

  std::vector<Point> points;
  int pointX = start.x;
  int pointY = start.y;

  if((pointAmount >= 0) ){
    for(int i = 1; i <= pointAmount; i++){

      pointX = pointX + xMod;
      pointY = pointY + yMod;

      Point testPoint(pointX, pointY);
      bool isValidPoint = false;

      if(testPoint.inBounds()){
        isValidPoint = true;

        if(!isEndInclusive){
          if((testPoint == start) || (testPoint == stop)){
            isValidPoint = false;
          }
        }        
      }

      if(isValidPoint){
        points.push_back(testPoint);
      }
    }
  }

  return points;
}

std::vector<Point> generateConcentricRing(Point center, int step){
  std::vector<Point> ring;
  std::vector<Point> points;

  points.push_back(Point(center.x + step, center.y + step));
  points.push_back(Point(center.x - step, center.y + step));
  points.push_back(Point(center.x - step, center.y - step));
  points.push_back(Point(center.x + step, center.y - step));

  /*

  if(corners[0].inBounds()){
    ring.push_back(corners[0]);
  }

  if(corners[1].inBounds()){
    ring.push_back(corners[1]);
  }

  if(corners[2].inBounds()){
    ring.push_back(corners[2]);
  }

  if(corners[3].inBounds()){
    ring.push_back(corners[3]);
  }

  */

  std::vector<Point> sideA = generateStraightLine(points[0], points[1], false);
  std::vector<Point> sideB = generateStraightLine(points[1], points[2], false);
  std::vector<Point> sideC = generateStraightLine(points[2], points[3], false);
  std::vector<Point> sideD = generateStraightLine(points[3], points[0], false);

  if(sideA.size() > 0){
    points.insert(points.end(), sideA.begin(), sideA.end());
  }

  if(sideB.size() > 0){
    points.insert(points.end(), sideB.begin(), sideB.end());
  }

  if(sideC.size() > 0){
    points.insert(points.end(), sideC.begin(), sideC.end());
  }

  if(sideD.size() > 0){
    points.insert(points.end(), sideD.begin(), sideD.end());
  }

  for(int i = 0; i < points.size(); i++){

    if(points[i].inBounds()){
      ring.push_back(points[i]);
    }
  }

  return ring;
}

std::vector<std::vector<Point>> generateConcentricRings(Point center){
  std::vector<std::vector<Point>> rings;

  for(int i = 0; i < 8; i++){

    std::vector<Point> ring = generateConcentricRing(center, i);

    if(ring.size() > 0){

      rings.push_back(ring);
    }else{
      break;
    }
  }

  return rings;
}

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
        INCHECK,
        PIN
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

      bool operator!=(const Move& other) const{
        return (type != other.type) || (position != other.position);
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
        PAWN,//complete
        KNIGHT,//complete
        BISHOP,//complete
        ROOK,//complete
        QUEEN,//complete
        KING,
        NONE
    };

      Type type;
      std::vector<Move> moves;
      Team team;
      bool isFirstMove = true;

      Piece(Type _type, Team _team) : type(_type), team(_team) {}
      Piece() = default;
      Piece(const Piece& other) = default;
      Piece(Piece&& other) noexcept = default;
      Piece& operator=(const Piece& other) = default;
      Piece& operator=(Piece&& other) noexcept = default;
      ~Piece() = default;

      bool operator==(const Piece& other) const{
        return (type == other.type) && (team == other.team);
      }

      void removePath(){
          moves.clear();
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
        reg.push_back(PieceRegistry::Entry(Point(4,2), Piece(Piece::KING, RED)));
        reg.push_back(PieceRegistry::Entry(Point(7,2), Piece(Piece::KING, BLUE)));

        reg.push_back(PieceRegistry::Entry(Point(8,3), Piece(Piece::BISHOP, BLUE)));
        reg.push_back(PieceRegistry::Entry(Point(6,8), Piece(Piece::BISHOP, RED)));
      }

      /*

      PieceRegistry(){
        reg.push_back(PieceRegistry::Entry(Point(4,1), Piece(Piece::KING, RED)));
        reg.push_back(PieceRegistry::Entry(Point(5,8), Piece(Piece::KING, BLUE)));

        reg.push_back(PieceRegistry::Entry(Point(1,2), Piece(Piece::PAWN, RED)));
        reg.push_back(PieceRegistry::Entry(Point(2,2), Piece(Piece::PAWN, RED)));
        reg.push_back(PieceRegistry::Entry(Point(3,2), Piece(Piece::PAWN, RED)));
        reg.push_back(PieceRegistry::Entry(Point(4,2), Piece(Piece::PAWN, RED)));
        reg.push_back(PieceRegistry::Entry(Point(5,2), Piece(Piece::PAWN, RED)));
        reg.push_back(PieceRegistry::Entry(Point(6,2), Piece(Piece::PAWN, RED)));
        reg.push_back(PieceRegistry::Entry(Point(7,2), Piece(Piece::PAWN, RED)));
        reg.push_back(PieceRegistry::Entry(Point(8,2), Piece(Piece::PAWN, RED)));
        reg.push_back(PieceRegistry::Entry(Point(1,1), Piece(Piece::ROOK, RED)));
        reg.push_back(PieceRegistry::Entry(Point(8,1), Piece(Piece::ROOK, RED)));
        reg.push_back(PieceRegistry::Entry(Point(2,1), Piece(Piece::KNIGHT, RED)));
        reg.push_back(PieceRegistry::Entry(Point(7,1), Piece(Piece::KNIGHT, RED)));
        reg.push_back(PieceRegistry::Entry(Point(3,1), Piece(Piece::BISHOP, RED)));
        reg.push_back(PieceRegistry::Entry(Point(6,1), Piece(Piece::BISHOP, RED)));
        reg.push_back(PieceRegistry::Entry(Point(5,1), Piece(Piece::QUEEN, RED)));
        reg.push_back(PieceRegistry::Entry(Point(1,7), Piece(Piece::PAWN, BLUE)));
        reg.push_back(PieceRegistry::Entry(Point(2,7), Piece(Piece::PAWN, BLUE)));
        reg.push_back(PieceRegistry::Entry(Point(3,7), Piece(Piece::PAWN, BLUE)));
        reg.push_back(PieceRegistry::Entry(Point(4,7), Piece(Piece::PAWN, BLUE)));
        reg.push_back(PieceRegistry::Entry(Point(5,7), Piece(Piece::PAWN, BLUE)));
        reg.push_back(PieceRegistry::Entry(Point(6,7), Piece(Piece::PAWN, BLUE)));
        reg.push_back(PieceRegistry::Entry(Point(7,7), Piece(Piece::PAWN, BLUE)));
        reg.push_back(PieceRegistry::Entry(Point(8,7), Piece(Piece::PAWN, BLUE)));      
        reg.push_back(PieceRegistry::Entry(Point(1,8), Piece(Piece::ROOK, BLUE)));
        reg.push_back(PieceRegistry::Entry(Point(8,8), Piece(Piece::ROOK, BLUE)));
        reg.push_back(PieceRegistry::Entry(Point(2,8), Piece(Piece::KNIGHT, BLUE)));
        reg.push_back(PieceRegistry::Entry(Point(7,8), Piece(Piece::KNIGHT, BLUE)));
        reg.push_back(PieceRegistry::Entry(Point(3,8), Piece(Piece::BISHOP, BLUE)));
        reg.push_back(PieceRegistry::Entry(Point(6,8), Piece(Piece::BISHOP, BLUE)));
        reg.push_back(PieceRegistry::Entry(Point(4,8), Piece(Piece::QUEEN, BLUE)));
      }
      */

    };
 
    PieceRegistry pieceRegistry;
    Team currentTeam = RED;
    PieceRegistry::Entry& redKing = pieceRegistry.reg[0];
    PieceRegistry::Entry&  blueKing = pieceRegistry.reg[1];

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

    bool isOccupied(Point point){
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

    Piece::Type getPieceType(Point point){

      Piece::Type pieceType = Piece::Type::NONE;

      int pieceIndex = getPiecIndexAtPoint(point);

      if(pieceIndex >= 0){
        pieceType = pieceRegistry.reg[pieceIndex].piece.type;
      }

      return pieceType;
    }

    bool isOccupiedByOpponent(Point point, Team team){

      Team pieceTeam = getPieceTeam(point);

      if(pieceTeam != team){
        return true;
      }else{
        return false;
      }
    }

    bool isOccupiedByOpponentKing(Point point, Team team){

      bool isOccupiedByOpponentKing = false;

      if(isOccupied(point)){      

        if(isOccupiedByOpponent(point, team)){

          int occupyingPieceIndex = getPiecIndexAtPoint(point);
       
          if(pieceRegistry.reg[occupyingPieceIndex].piece.type == Piece::KING){
            isOccupiedByOpponentKing = true;
          }
        }
      }
      return isOccupiedByOpponentKing;
    }

    PieceRegistry::Entry* getPieceAtPoint(Point point){
      for(int i = 0; i < pieceRegistry.reg.size(); i++){
        if(point == pieceRegistry.reg[i].point){
          return &pieceRegistry.reg[i];
        }
      }
    }
   
    std::vector<Move> getPathFromStraightLine(Point start, Point stop, Team team){
      std::vector<Point> testPoints = generateStraightLine(start, stop, true);
      std::vector<Move> moves;
      bool pathIsBlocked = false;

      for(int i = 0; i < testPoints.size(); i++){

        if(!pathIsBlocked){

          if(isOccupied(testPoints[i])){

            pathIsBlocked = true;

            if(isOccupiedByOpponent(testPoints[i], team)){

              if(!isOccupiedByOpponentKing(testPoints[i], team)){
                  moves.push_back(Move(testPoints[i], Move::CONTESTED));
              }
            }
          }else{
              moves.push_back(Move(testPoints[i], Move::UNCONTESTED));
          }
        }
      }
   
      return moves;
    }

    std::vector<Move> getPathFromDiagnalLine(Point start, int xMod, int yMod, Team team){
      std::vector<Point> testPoints = generateDiagnalLine(start, xMod, yMod);
      std::vector<Move> moves;
      int pointX = start.x;
      int pointY = start.y;
      bool isPathBlocked = false;
      bool pathIsValid = true;


      for(int i = 0; i < testPoints.size(); i++){

        if(!isPathBlocked){
          if(isOccupied(testPoints[i])){

          isPathBlocked = true;

          if(isOccupiedByOpponent(testPoints[i], team)){
            moves.push_back(Move(testPoints[i], Move::CONTESTED));
          }
          }else{
            moves.push_back(Move(testPoints[i], Move::UNCONTESTED));
          }
        }
      }
   
      return moves;
    }

    std::vector<Move> generateStraightMoves(Team team, Point startingPoint){
      std::vector<Move> moves;

      std::vector<Move> posXMoves = getPathFromStraightLine(startingPoint, Point(8, startingPoint.y), team);

      if(posXMoves.size() > 0){
        moves.insert(moves.end(), posXMoves.begin(), posXMoves.end());
      }

      std::vector<Move> negXMoves = getPathFromStraightLine(startingPoint, Point(1, startingPoint.y), team);

      if(negXMoves.size() > 0){
        moves.insert(moves.end(), negXMoves.begin(), negXMoves.end());
      }

      std::vector<Move> posYMoves = getPathFromStraightLine(startingPoint, Point(startingPoint.x, 8), team);

      if(posYMoves.size() > 0){
        moves.insert(moves.end(), posYMoves.begin(), posYMoves.end());
      }

      std::vector<Move> negYMoves = getPathFromStraightLine(startingPoint, Point(startingPoint.x, 1), team);

      if(negYMoves.size() > 0){
        moves.insert(moves.end(), negYMoves.begin(), negYMoves.end());
      }

      return moves;
    }

    std::vector<Move> generateDiagnalMoves(Team team, Point startingPoint){
      std::vector<Move> moves;

      std::vector<Move> qOne = getPathFromDiagnalLine(startingPoint, 1, 1, team);

      if(qOne.size() > 0){
        moves.insert(moves.end(), qOne.begin(), qOne.end());
      }

      std::vector<Move> qTwo = getPathFromDiagnalLine(startingPoint, -1, 1, team);

      if(qTwo.size() > 0){
        moves.insert(moves.end(), qTwo.begin(), qTwo.end());
      }

      std::vector<Move> qThree = getPathFromDiagnalLine(startingPoint, -1, -1, team);

      if(qThree.size() > 0){
        moves.insert(moves.end(), qThree.begin(), qThree.end());
      }

      std::vector<Move> qFour = getPathFromDiagnalLine(startingPoint, 1, -1, team);

      if(qFour.size() > 0){
        moves.insert(moves.end(), qFour.begin(), qFour.end());
      }

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

        if(isOccupied(testPoints[i])){

          testPieceIndex = getPiecIndexAtPoint(testPoints[i]);

          if((pieceRegistry.reg[testPieceIndex].piece.team != currentTeam) && (testPoints[i].inBounds()) ){

            if(!isOccupiedByOpponentKing(pieceRegistry.reg[testPieceIndex].point, team)){

              moves.push_back(Move(testPoints[i], Move::CONTESTED));
            }
          }
        }
      }

      if((!isOccupied(testPoints[2])) && (testPoints[2].inBounds())){

        moves.push_back(Move(testPoints[2], Move::UNCONTESTED));

        if((!isOccupied(testPoints[3])) && (isFirstMove)){

          moves.push_back(Move(testPoints[3], Move::UNCONTESTED));  
        }
      }

      return moves;
    }

    std::vector<Move> getRookMoves(Team team, Point point){
      std::vector<Move> moves = generateStraightMoves(team, point);
      return moves;
    }

    std::vector<Move> getBishopMoves(Team team, Point point){
      std::vector<Move> moves = generateDiagnalMoves(team, point);

      return moves;
    }

    std::vector<Move> getQueenMoves(Team team, Point point){
      std::vector<Move> moves;

      std::vector<Move> straightMoves = generateStraightMoves(team, point);

      if(straightMoves.size() > 0){
        moves.insert(moves.end(), straightMoves.begin(), straightMoves.end());
      }

      std::vector<Move> diagnalMoves = generateDiagnalMoves(team, point);

      if(diagnalMoves.size() > 0){
        moves.insert(moves.end(), diagnalMoves.begin(), diagnalMoves.end());
      }

      return moves;
    }

    std::vector<Move> getKnightMoves(Team team, Point point){
      std::vector<Move> moves;
      int x = point.x;
      int y = point.y;

      std::vector<Point> testPoints;

      testPoints.push_back(Point(x + 1, y + 2));
      testPoints.push_back(Point(x + 2, y + 1));
      testPoints.push_back(Point(x + 1, y - 2));
      testPoints.push_back(Point(x + 2, y - 1));

      testPoints.push_back(Point(x - 1, y + 2));
      testPoints.push_back(Point(x - 2, y + 1));
      testPoints.push_back(Point(x - 1, y - 2));
      testPoints.push_back(Point(x - 2, y - 1));

      for(int i = 0; i < testPoints.size(); i++){

        if(testPoints[i].inBounds()){

          if(isOccupied(testPoints[i])){

            if(isOccupiedByOpponent(testPoints[i], team)){
              moves.push_back(Move(testPoints[i], Move::CONTESTED));
            }
          }else{
            moves.push_back(Move(testPoints[i], Move::UNCONTESTED));
          }
        }
      }

      return moves;
    }

    std::vector<Move> getKingMoves(Team team, Point point){
      std::vector<Move> moves;
      int x = point.x;
      int y = point.y;

      std::vector<Point> testPoints;

      testPoints.push_back(Point(x,y + 1));
      testPoints.push_back(Point(x + 1,y + 1));
      testPoints.push_back(Point(x + 1,y));
      testPoints.push_back(Point(x + 1,y - 1));
      testPoints.push_back(Point(x,y - 1));
      testPoints.push_back(Point(x - 1,y - 1));
      testPoints.push_back(Point(x - 1,y));
      testPoints.push_back(Point(x - 1,y + 1));

      for(int i = 0; i < testPoints.size(); i++){

        if(testPoints[i].inBounds()){

          if(isOccupied(testPoints[i])){

            if(isOccupiedByOpponent(testPoints[i], team)){
              moves.push_back(Move(testPoints[i], Move::CONTESTED));
            }
          }else{
            moves.push_back(Move(testPoints[i], Move::UNCONTESTED));
          }
        }
      }

      return moves;
    }

    void clearMoves(){
      for(int i = 0; i < pieceRegistry.reg.size(); i++){
        pieceRegistry.reg[i].piece.removePath();
      }
    }

    void generateStandardPieceMoves(){
      for(int i = 0; i < pieceRegistry.reg.size(); i++){

        Piece& piece = pieceRegistry.reg[i].piece;
        Point position = pieceRegistry.reg[i].point;

        if(piece.team == currentTeam){

          switch(piece.type){

            case Piece::PAWN:

              piece.moves = getPawnMoves(piece.team, position, piece.isFirstMove);

            break;

            case Piece::ROOK:

              piece.moves = getRookMoves(piece.team, position);

            break;

            case Piece::BISHOP:

              piece.moves = getBishopMoves(piece.team, position);

            break;

            case Piece::QUEEN:

              piece.moves = getQueenMoves(piece.team, position);
         
            break;

            case Piece::KNIGHT:

              piece.moves = getKnightMoves(piece.team, position);

            break;

            case Piece::KING:

            piece.moves = getKingMoves(piece.team, position);

            break;
          }
        }
      }
    }

    bool ifDiagnal(Piece::Type pieceType){
      if((pieceType == Piece::Type::BISHOP) || (pieceType == Piece::Type::QUEEN)){
        return true;
      }else{
        return false;
      }
    }

    bool ifStraight(Piece::Type pieceType){
      if((pieceType == Piece::Type::ROOK) || (pieceType == Piece::Type::QUEEN)){
        return true;
      }else{
        return false;
      }
    }

    void getStraightPinnedPath(std::vector<Point> path){

      Chess::PieceRegistry::Entry* blockingPiece;
      Chess::PieceRegistry::Entry* attackingPiece;
      int friendlyPieceCount = 0;

      for(int i = 0; i < path.size(); i++){

        if(isOccupied(path[i])){

          if(!isOccupiedByOpponent(path[i], currentTeam)){

            friendlyPieceCount = friendlyPieceCount + 1;

            blockingPiece = getPieceAtPoint(path[i]);
          }

          if(isOccupiedByOpponent(path[i], currentTeam) && (friendlyPieceCount == 1)){

            Chess::Piece::Type pieceType = getPieceType(path[i]);

            if( ifStraight(pieceType)){

              attackingPiece = getPieceAtPoint(path[i]);

              std::vector<Move> newPath;

              for(int i = 0; i < path.size(); i++){

                for(int j = 0; j < blockingPiece->piece.moves.size(); j++){

                  if(path[i] == blockingPiece->piece.moves[j].position){

                    newPath.push_back(blockingPiece->piece.moves[j]);
                  }
                }
              }

              blockingPiece->piece.moves = newPath;
            }else{
              break;
            }
          }
        }
      }
    }

    void getDiagnalPinnedPath(std::vector<Point> path){

      Chess::PieceRegistry::Entry* blockingPiece;
      Chess::PieceRegistry::Entry* attackingPiece;
      int friendlyPieceCount = 0;

      for(int i = 0; i < path.size(); i++){

        if(isOccupied(path[i])){

          if(!isOccupiedByOpponent(path[i], currentTeam)){

            friendlyPieceCount = friendlyPieceCount + 1;

            blockingPiece = getPieceAtPoint(path[i]);
          }

          if(isOccupiedByOpponent(path[i], currentTeam) && (friendlyPieceCount == 1)){

            Chess::Piece::Type pieceType = getPieceType(path[i]);

            if( ifDiagnal(pieceType)){

              attackingPiece = getPieceAtPoint(path[i]);

              std::vector<Move> newPath;

              for(int i = 0; i < path.size(); i++){

                for(int j = 0; j < blockingPiece->piece.moves.size(); j++){

                  if(path[i] == blockingPiece->piece.moves[j].position){

                    newPath.push_back(blockingPiece->piece.moves[j]);
                  }
                }
              }

              blockingPiece->piece.moves = newPath;
            }else{
              break;
            }
          }
        }
      }
    }

    void getPinMoves(){

      Point kingPos;

      if(currentTeam == Team::RED){

        kingPos = redKing.point;
      }else{

        kingPos = blueKing.point;
      }

      getStraightPinnedPath(generateStraightLine(kingPos, Point(kingPos.x,8), true));
      getStraightPinnedPath(generateStraightLine(kingPos, Point(kingPos.x,1), true));
      getStraightPinnedPath(generateStraightLine(kingPos, Point(8,kingPos.y), true));
      getStraightPinnedPath(generateStraightLine(kingPos, Point(1,kingPos.y), true));

      getDiagnalPinnedPath(generateDiagnalLine(kingPos, 1, 1));
      getDiagnalPinnedPath(generateDiagnalLine(kingPos, 1, -1));
      getDiagnalPinnedPath(generateDiagnalLine(kingPos, -1, 1));
      getDiagnalPinnedPath(generateDiagnalLine(kingPos, -1, -1));
    }

    void update(){
      generateStandardPieceMoves();

      //getPinMoves();
    }

    int getPieceCount(){
      return pieceRegistry.reg.size();
    }

    Point getPiecePosition(int pieceIndex){
      return pieceRegistry.reg[pieceIndex].point;
    }

    void movePieceToPoint(Point pieceCurrentPos, Point newPos){
      int pieceIndex = getPiecIndexAtPoint(pieceCurrentPos);

      if(pieceIndex >= 0){
        pieceRegistry.reg[pieceIndex].point = newPos;

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
    incorrectPiecePlacement,
    awaitingCapture
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

    for(int x = 1; x <= 8; x++){

      for(int y = 1; y <= 8; y++){

        setLedColor(Point(x,y), CRGB::Black, leds);
      }
    }
  }

  BoardState boardState = awaitingInitialPieceSetup;
  BoardState previousBoardState;
  Chess::PieceRegistry::Entry* currentPiece;
  std::vector<Point> incorrectPieceIndex;
  Chess chess;

  void changeBoardState(BoardState newBoardState){

    previousBoardState = boardState;

    boardState = newBoardState;
  }

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
        changeBoardState(awaitingPiecePickup);
        chess.update();
      }
    }
  }

  void awaitingPiecePickupRoutine(SquareState change, CRGB leds[64]){

    bool changeIsValid = true;

    bool isChangedPointOccupied = chess.isOccupied(change.point);

    if(isChangedPointOccupied && change.behavior == falling){
      bool ifPieceHasMoves = chess.ifPieceHasMoves(change.point);

      if(ifPieceHasMoves){
        changeBoardState(awaitingPiecePlacement);

        currentPiece = chess.getPieceAtPoint(change.point);

        currentPiece->piece.moves.size();

        for(int i = 0; i < currentPiece->piece.moves.size(); i++){
          currentPiece->piece.moves[i].print();
        }

        displayMoves(currentPiece->piece.moves, leds);

        FastLED.show();

      }else{
        incorrectPieceIndex.push_back(change.point);

        changeBoardState(incorrectPiecePickup);

        setLedColor(change.point, CRGB::Red, leds);

        FastLED.show();
      }
    }else{
      incorrectPieceIndex.push_back(change.point);

      changeBoardState(incorrectPiecePickup);

      setLedColor(change.point, CRGB::Red, leds);

      FastLED.show();
    }
  }

  void incorrectPiecePickupRoutine(SquareState change, CRGB leds[64]){

    for(int i = 0; i < incorrectPieceIndex.size(); i++){

      if(change.point == incorrectPieceIndex[i]){

        incorrectPieceIndex.erase(incorrectPieceIndex.begin() + i);

        changeBoardState(previousBoardState);
        setLedColor(change.point, CRGB::Black, leds);    
      }
    }
    FastLED.show();    
  }

  void completeTurnRoutine(CRGB leds[64]){
    currentPiece = NULL;
    chess.clearMoves();
    chess.changeTeam();    
    chess.update();
    changeBoardState(awaitingPiecePickup);
    clearLeds(leds);
    FastLED.show();
  }

  void awaitingCaptureRoutine(SquareState change, CRGB leds[64]){

    if((change.point == currentPiece->point) && (change.behavior == rising)){

      completeTurnRoutine(leds);
    }else{

      incorrectPieceIndex.push_back(change.point);

      changeBoardState(incorrectPiecePickup);

      setLedColor(change.point, CRGB::Red, leds);

      FastLED.show();
    }
  }

  void awaitingPiecePlacementRoutine(SquareState change, CRGB leds[64]){

    if(currentPiece != NULL){

      bool moveIsValid = false;

      for(int moveIndex = 0; moveIndex < currentPiece->piece.moves.size(); moveIndex++){

        if((change.point == currentPiece->piece.moves[moveIndex].position || change.point == currentPiece->point)){

          moveIsValid = true;

          if(change.point == currentPiece->point){
            changeBoardState(awaitingPiecePickup);
            clearLeds(leds);
            FastLED.show();

          }else if((currentPiece->piece.moves[moveIndex].type == Chess::Move::UNCONTESTED)){
            chess.movePieceToPoint(currentPiece->point, change.point);
            completeTurnRoutine(leds);
            break;
          }else if((currentPiece->piece.moves[moveIndex].type == Chess::Move::CONTESTED)){
            chess.movePieceToPoint(currentPiece->point, change.point);
            changeBoardState(awaitingCapture);
            chess.removeTakenPiece(change.point);
            clearLeds(leds);
            setLedColor(change.point, CRGB::Green, leds);
            FastLED.show();
            break;
          }
        }
      }

      if(!moveIsValid){

        incorrectPieceIndex.push_back(change.point);

        changeBoardState(incorrectPiecePickup);

        setLedColor(change.point, CRGB::Red, leds);

        FastLED.show();
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

        case awaitingCapture:

          awaitingCaptureRoutine(change, leds);

        break;
      }
    }
  }
};

class Display{
  public:

  struct Pixel{
    public:
    Point point;
    CRGB color;

    Pixel(Point _point, CRGB _color){

      point = _point;
      color = _color;
    }
  };

  struct Frame{
    public:
    std::vector<Pixel> pixelList;
  };

  struct Animation{
    public:
    std::vector<Frame> frameList;

    Animation(Point point, std::vector<Point> path){
      std::vector<std::vector<Point>> ring = generateConcentricRings(point);

      frameList.resize(ring.size());

      for(int i = 0; i < ring.size(); i++){

        for(int j = 0; j < ring[i].size(); j++){

          for(int k = 0; k < path.size(); k++){

            if(ring[i][j] == path[k]){

              frameList[i].pixelList.push_back(Pixel(path[k], CRGB::Red));
            }
          }
        }
      }
    }
  };

  unsigned long animationDelay = 1000;
  unsigned long previousMillis;
  int frameCounter = 0;
  std::deque<Animation> animation;
  bool isValid = false;


  void addAnimation(Point point, std::vector<Point> path){
    std::vector<std::vector<Point>> ring = generateConcentricRings(point);

    animation.push_back(Animation(point, path));    

    isValid = true;
  }

  void displayFrame(int frameNum, Board board,  CRGB leds[64]){

    int pixelCount = animation[0].frameList[frameNum].pixelList.size();

    if(pixelCount > 0){

      for(int i = 0; i < pixelCount; i++){

        board.setLedColor(animation[0].frameList[frameNum].pixelList[i].point, animation[0].frameList[frameNum].pixelList[i].color, leds);
      }
    }
  }

  void run(Board board, CRGB leds[64], unsigned long currentMillis){
    if(((currentMillis - previousMillis) > animationDelay) && (isValid)){

      previousMillis = currentMillis;

      displayFrame(frameCounter, board, leds);

      FastLED.show();

      if(frameCounter == (animation[0].frameList.size() - 1)){
        isValid = false;
      }

      frameCounter = frameCounter + 1;
    }
  }

  void clear(){
  }
};

std::array<std::bitset<8>, 8> currentHallArrayState;
std::array<std::bitset<8>, 8> previousHallArrayState;

HallArray hallArray;
Board board;
CRGB leds[64];
Display display;


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
  FastLED.setBrightness(10);

  delay(150);

  int ledIterator = 0;

  for(int x = 0; x < 8; x++){//sets all leds to black
    for(int y = 0; y < 8; y++){

      leds[ledIterator] = CRGB::Black;
      ledIterator = ledIterator + 1;
    }
  }
 
  FastLED.show();

  //board.awaitingInitialPieceSetupRoutine(leds, currentHallArrayState);

  Serial.println("Setup complete");

  /*
  
  std::vector<std::vector<Point>> rings = generateConcentricRings(Point(2,7));

  std::vector<CRGB> colors = {CRGB::Red, CRGB::Orange, CRGB::Yellow, CRGB::Blue, CRGB::Green, CRGB::Red, CRGB::Orange, CRGB::Yellow, CRGB::Blue, CRGB::Green};

  for(int i = 0; i < rings.size(); i++){

    for(int j = 0; j < rings[i].size(); j++){

      board.setLedColor(rings[i][j], colors[i], leds);        
    }

    FastLED.show();

    delay(100);
  }*/

  delay(800);   

  Point point = Point(3,3);
  std::vector<Point> path;

  std::vector<Point> pathA = generateStraightLine(Point(3,3), Point(3,8), true);
  std::vector<Point> pathB = generateStraightLine(Point(3,3), Point(3,1), true);
  std::vector<Point> pathC = generateStraightLine(Point(3,3), Point(1,3), true);
  std::vector<Point> pathD = generateStraightLine(Point(3,3), Point(8,3), true);

  path.insert(path.end(), pathA.begin(), pathA.end());
  path.insert(path.end(), pathB.begin(), pathB.end());
  path.insert(path.end(), pathC.begin(), pathC.end());
  path.insert(path.end(), pathD.begin(), pathD.end());

  display.addAnimation(point, path);
  //display.run(board, leds);
}

void loop(){/*  
  currentHallArrayState = hallArray.read();

  if(previousHallArrayState != currentHallArrayState){

    board.processInput(currentHallArrayState, previousHallArrayState, leds);

    previousHallArrayState = currentHallArrayState;
  }*/

  display.run(board, leds, millis());
}
