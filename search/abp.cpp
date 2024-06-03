#include <iostream>
#include <limits>
#include <vector>

using namespace std;
using Value = int;  // 평가값 타입 (int)
using Depth = int;  // 깊이 타입 (int)

constexpr Value INF       = numeric_limits<Value>::max();  // 최대 평가값: 무한대(inf)
constexpr Depth MAX_DEPTH = 5;                             // 최대 깊이: 5 (임시)

enum Piece { EMPTY = 0, BLACK = 1, WHITE = 2 };

// 보드 (구조체)
struct Board
{
    vector<vector<int>> grid;           // 보드 상태 (2차원 vector)
    int                 currentPlayer;  // 현재 플레이어 (int)

    // Board(생성자): 보드 상태(15*15 영벡터)/현재 플레이어(1) 초기화
    Board() : grid(15, vector<int>(15, 0)), currentPlayer(BLACK) {}// 바깥 테두리 0으로 초기화 해야 하는거로 알고있어서 15 + 2 로 초기화 해야할듯

    // generateMoves: 수를 생성 가능한 위치를 검사하는 함수
    vector<pair<int, int>> generateMoves()
    {
        vector<pair<int, int>> moves;   // 수를 생성 가능한 위치를 저장할 벡터
        for (int i = 0; i < 15; ++i) {  // 보드의 모든 행, 열에 대해 반복
            for (int j = 0; j < 15; ++j) {
                if (grid[i][j] == 0) {  // 보드의 해당 위치가 비어 있으면(수 생성 가능), moves에 추가
                    moves.push_back({i, j});
                }
            }
        }
        return moves;  // 수를 생성 가능한 위치 반환
    }

    // makeMove: 수를 생성하는 함수
    void makeMove(int x, int y)
    {
        grid[x][y]    = currentPlayer;      // 현재 플레이어의 수 생성
        currentPlayer = 3 - currentPlayer;  // 플레이어 교체
    }

    // undoMove: 수를 취소하는 함수
    void undoMove(int x, int y)
    {
        grid[x][y]    = 0;                  // 해당 위치를 비움
        currentPlayer = 3 - currentPlayer;  // 플레이어 교체
    }

    // isGameOver: 게임 종료 여부를 확인하는 함수
    bool isGameOver()
    {
        // TODO: 게임 종료 여부 확인 함수 구현
        return false;  // false 반환 (임시)
    }

    // evaluate: 평가 함수
    Value evaluate()
    {
        // TODO: 평가 함수 구현
        return 0;  // 0 반환 (임시)
    }
};

// 알파-베타 프루닝
Value alphaBeta(Board &board, Depth depth, Value alpha, Value beta, bool maximizingPlayer)
{
    if (depth == 0 || board.isGameOver()) {  // 깊이가 0이거나(재귀 종료), 게임이 종료되면, 보드의 평가값 반환
        return board.evaluate();
    }

    vector<pair<int, int>> moves = board.generateMoves();  // 수가 생성 가능한 위치 검사

    if (maximizingPlayer) { // 최대화 플레이어 차례
        Value maxEval = -INF;                           // 최대 평가값 초기화 (-inf)
        for (const auto &move : moves) {
            board.makeMove(move.first, move.second);    // 모든 가능한 수 반복 생성
            Value eval = alphaBeta(board, depth - 1, alpha, beta, false);  // 재귀적으로 최소화 플레이어 호출
            board.undoMove(move.first, move.second);    // 수 취소
            maxEval = max(maxEval, eval);               // 최대 평가값 업데이트
            if (maxEval >= beta) {                      // 조건(maxEval >= beta) 확인 후, 프루닝
                break;
            }
            alpha   = max(alpha, maxEval);              // 알파 값 업데이트
        }
        return maxEval;     // 최대 평가 값 반환
    }
    else {                  // 최소화 플레이어 차례
        Value minEval = INF;                            // 최소 평가값 초기화 (inf)
        for (const auto &move : moves) {
            board.makeMove(move.first, move.second);    // 모든 가능한 수 반복 생성
            Value eval = alphaBeta(board, depth - 1, alpha, beta, true);  // 재귀적으로 최대화 플레이어 호출
            board.undoMove(move.first, move.second);    // 수 취소
            minEval = min(minEval, eval);               // 최소 평가값 업데이트
            if (minEval <= alpha) {                     // 조건(minEval <= alpha) 확인 후, 프루닝
                break;
            }
            beta    = min(beta, minEval);               // 베타 값 업데이트
        }
        return minEval;     // 최소 평가 값 반환
    }
}

// 메인 함수 (테스트)
int main()
{   
    Board board = Board();
    Value bestValue = alphaBeta(board, MAX_DEPTH, -INF, INF, true); // 알파-베타 프루닝 알고리즘 호출
    cout << "bestValue: " << bestValue << endl;                     // 최적 평가값 출력
    return 0;
}