#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

#include <graphics.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <vector>
#include <algorithm>
#include <random>
#include <locale>
#include <map>
#include <ctime>
#include <stack>
#include <utility>
#include <cmath>
#include <string>
#include <queue>
#include <unordered_set>

using namespace std;

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

const int WIN_W = 900;
const int WIN_H = 850;

enum ProgramState { STATE_MENU, STATE_GAME };
ProgramState currentState = STATE_MENU;

int N = 4;
int CELL_SIZE = 120;
int BOARD_LEFT, BOARD_TOP, BOARD_WIDTH, BOARD_HEIGHT;
vector<vector<int>> board;
vector<vector<bool>> hasShadow;
vector<vector<int>> initialBoard;
vector<vector<bool>> initialShadow;
int stepCount = 0;
bool gameWin = false;
int difficulty = 1;
clock_t startTime = 0;
bool timing = false;
double currentTime = 0.0;
map<int, double> bestTimeMap;
map<int, double> blindObserveBest;
map<int, double> blindRecoverBest;
map<int, double> blindTotalBest;
bool victoryEffectDrawn = false;

bool blindMode = false;
bool blindObserving = true;
bool blindFailed = false;
double observeTime = 0.0;
double recoverTime = 0.0;
clock_t observeStart = 0;
clock_t recoverStart = 0;

bool showHint = false;
int hintDir = -1;
const double HINT_CALC_MAX_TIME = 2.0;

struct UndoStep {
    vector<vector<int>> boardState;
    vector<vector<bool>> shadowState;
    int stepCount;
};
stack<UndoStep> undoStack;
const int MAX_UNDO = 50;

int menuStep = 0;
int selectedSize = 4;

int activePage = 0;
int visiblePage = 1;

void findBlank(int &row, int &col);
bool moveTile(int row, int col);
bool checkWin();
void startTiming();
void stopTiming();
void resetTiming();
void clearUndo();
void pushUndoState();
void undoLastMove();
void updateBoardLayout();
void initGame(int size, int diff, bool blind);
wstring getDifficultyStars();
void drawGame();
void drawMenu();
void handleMenuKey(int key);
void handleGameKey(int key);
int countShadows(int n, int diff);
void placeShadows();
void generateRandomState();
void initBoardWithShadows();
int getHintDirection();
string boardToString(const vector<vector<int>>& b);
int calcManhattan(const vector<vector<int>>& b);

void findBlank(int &row, int &col) {
    for (int i = 0; i < N; ++i)
        for (int j = 0; j < N; ++j)
            if (board[i][j] == 0) { row = i; col = j; return; }
}

bool checkWin() {
    int expected = 1;
    for (int i = 0; i < N; ++i)
        for (int j = 0; j < N; ++j) {
            if (i == N - 1 && j == N - 1) return board[i][j] == 0;
            if (board[i][j] != expected) return false;
            expected++;
        }
    return true;
}

string boardToString(const vector<vector<int>>& b) {
    string res;
    for (auto& row : b) {
        for (int num : row) {
            res += to_string(num) + ",";
        }
    }
    return res;
}

int calcManhattan(const vector<vector<int>>& b) {
    int sum = 0;
    int n = b.size();
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) {
            int val = b[i][j];
            if (val == 0) continue;
            int targetR = (val - 1) / n;
            int targetC = (val - 1) % n;
            sum += abs(i - targetR) + abs(j - targetC);
        }
    }
    return sum;
}

int getHintDirection() {
    if (N != 3 && N != 4) return -1;
    if (difficulty != 1) return -1;
    if (gameWin || blindMode || blindFailed) return -1;

    clock_t calcStart = clock();
    int br, bc;
    findBlank(br, bc);
    int dr[] = {-1, 1, 0, 0};
    int dc[] = {0, 0, -1, 1};
    int targetBr = N-1, targetBc = N-1;

    int greedyDir = -1;
    int currentScore = calcManhattan(board);
    int bestGreedyScore = currentScore;

    for (int d = 0; d < 4; ++d) {
        int nr = br + dr[d];
        int nc = bc + dc[d];
        if (nr < 0 || nr >= N || nc < 0 || nc >= N) continue;
        swap(board[br][bc], board[nr][nc]);
        int newScore = calcManhattan(board);
        swap(board[br][bc], board[nr][nc]);
        if (newScore < bestGreedyScore) {
            bestGreedyScore = newScore;
            greedyDir = d;
        }
    }

    if (greedyDir == -1) {
        if (bc < targetBc) greedyDir = 3;
        else if (bc > targetBc) greedyDir = 2;
        else if (br < targetBr) greedyDir = 1;
        else if (br > targetBr) greedyDir = 0;
    }

    struct BFSNode {
        vector<vector<int>> board;
        int blankR, blankC;
        vector<int> path;
    };

    queue<BFSNode> q;
    unordered_set<string> visited;
    BFSNode startNode;
    startNode.board = board;
    startNode.blankR = br;
    startNode.blankC = bc;
    q.push(startNode);
    visited.insert(boardToString(board));

    int bestDir = -1;
    bool foundOptimal = false;

    while (!q.empty()) {
        double elapsed = (clock() - calcStart) / (double)CLOCKS_PER_SEC;
        if (elapsed >= HINT_CALC_MAX_TIME) {
            return greedyDir;
        }

        BFSNode curr = q.front();
        q.pop();

        if (calcManhattan(curr.board) == 0) {
            if (!curr.path.empty()) {
                bestDir = curr.path[0];
                foundOptimal = true;
            }
            break;
        }

        for (int d = 0; d < 4; ++d) {
            int nr = curr.blankR + dr[d];
            int nc = curr.blankC + dc[d];
            if (nr < 0 || nr >= N || nc < 0 || nc >= N) continue;

            BFSNode next = curr;
            swap(next.board[next.blankR][next.blankC], next.board[nr][nc]);
            next.blankR = nr;
            next.blankC = nc;
            string key = boardToString(next.board);

            if (visited.find(key) == visited.end()) {
                visited.insert(key);
                next.path.push_back(d);
                q.push(next);
            }
        }
    }

    return foundOptimal ? bestDir : greedyDir;
}

int countShadows(int n, int diff) {
    if (diff == 7) return 0;
    int total = n * n;
    if (diff == 1) return 0;

    int shadowCount = 0;
    if (diff == 2)
        shadowCount = total * (10 + rand() % 11) / 100;
    else if (diff == 3)
        shadowCount = total * (30 + rand() % 11) / 100;
    else if (diff == 4)
        shadowCount = total * (50 + rand() % 11) / 100;
    else if (diff == 5)
        shadowCount = total * (70 + rand() % 11) / 100;
    else // diff == 6
        shadowCount = total - 1;

    // 保证难度2~6时至少有1个阴影格
    if (shadowCount == 0 && diff >= 2 && diff <= 6)
        shadowCount = 1;

    return shadowCount;
}

void placeShadows() {
    int shadowCount = countShadows(N, difficulty);
    hasShadow.assign(N, vector<bool>(N, false));
    vector<pair<int, int>> pos;
    for (int i = 0; i < N; ++i)
        for (int j = 0; j < N; ++j)
            if (board[i][j] != 0) pos.emplace_back(i, j);
    shuffle(pos.begin(), pos.end(), mt19937{random_device{}()});
    for (int k = 0; k < shadowCount && k < (int)pos.size(); ++k) {
        auto [r, c] = pos[k];
        hasShadow[r][c] = true;
    }
}

//generate here
void generateRandomState() {
    board.assign(N, vector<int>(N, 0));
    int idx = 1;
    for (int i = 0; i < N; ++i)
        for (int j = 0; j < N; ++j)
            if (i == N - 1 && j == N - 1) board[i][j] = 0;
            else board[i][j] = idx++;
    int moves = 500;
    int dr[] = {-1, 1, 0, 0}, dc[] = {0, 0, -1, 1};
    for (int m = 0; m < moves; ++m) {
        int br, bc;
        findBlank(br, bc);
        vector<int> dirs;
        for (int d = 0; d < 4; ++d) {
            int nr = br + dr[d], nc = bc + dc[d];
            if (nr >= 0 && nr < N && nc >= 0 && nc < N) dirs.push_back(d);
        }
        if (dirs.empty()) continue;
        int d = dirs[rand() % dirs.size()];
        int nr = br + dr[d], nc = bc + dc[d];
        swap(board[br][bc], board[nr][nc]);
    }
    placeShadows();
    initialBoard = board;
    initialShadow = hasShadow;
}

void initBoardWithShadows() { generateRandomState(); }

bool moveTile(int row, int col) {
    if (row < 0 || row >= N || col < 0 || col >= N) return false;
    if (board[row][col] <= 0) return false;
    int br, bc;
    findBlank(br, bc);
    if ((row == br && abs(col - bc) == 1) || (col == bc && abs(row - br) == 1)) {
        pushUndoState();
        swap(board[row][col], board[br][bc]);
        stepCount++;
        return true;
    }
    return false;
}

void startTiming() {
    if (!timing && !gameWin) { timing = true; startTime = clock(); }
}

void stopTiming() {
    if (timing) {
        timing = false;
        currentTime = (clock() - startTime) / (double)CLOCKS_PER_SEC;
        if (!blindMode) {
            int key = N * 100 + difficulty;
            auto it = bestTimeMap.find(key);
            if (it == bestTimeMap.end() || currentTime < it->second)
                bestTimeMap[key] = currentTime;
        }
    }
}

void resetTiming() { timing = false; currentTime = 0.0; }

void clearUndo() { while (!undoStack.empty()) undoStack.pop(); }

void pushUndoState() {
    if (undoStack.size() >= MAX_UNDO) {
        stack<UndoStep> tmp;
        while (undoStack.size() > 1) { tmp.push(undoStack.top()); undoStack.pop(); }
        undoStack.pop();
        while (!tmp.empty()) { undoStack.push(tmp.top()); tmp.pop(); }
    }
    undoStack.push({board, hasShadow, stepCount});
}

void undoLastMove() {
    if (gameWin || blindFailed || undoStack.empty()) return;
    UndoStep prev = undoStack.top(); undoStack.pop();
    board = prev.boardState; hasShadow = prev.shadowState; stepCount = prev.stepCount;
    gameWin = false; victoryEffectDrawn = false;
}

void updateBoardLayout() {
    int usableHeight = WIN_H - 220;
    int usableWidth = WIN_W - 120;
    int maxCell = min(usableWidth / N, usableHeight / N);
    CELL_SIZE = max(40, maxCell);
    BOARD_WIDTH = N * CELL_SIZE;
    BOARD_HEIGHT = N * CELL_SIZE;
    BOARD_LEFT = (WIN_W - BOARD_WIDTH) / 2;
    BOARD_TOP = 50;
}

wstring getDifficultyStars() {
    if (blindMode) return L"盲棋";
    wstring stars;
    for (int i = 0; i < difficulty; ++i) stars += L"★";
    return stars;
}

void initGame(int size, int diff, bool blind) {
    N = size; blindMode = blind;
    if (blind) difficulty = 7; else difficulty = diff;
    blindFailed = false;
    showHint = false;
    hintDir = -1;
    updateBoardLayout();
    initBoardWithShadows();
    stepCount = 0; gameWin = false; victoryEffectDrawn = false;
    clearUndo();
    if (blindMode) {
        blindObserving = true; observeTime = 0.0; recoverTime = 0.0;
        observeStart = clock(); timing = false; currentTime = 0.0;
    } else {
        blindObserving = false; resetTiming();
    }
    wchar_t cap[100];
    if (blindMode)
        swprintf(cap, 100, L"盲棋模式 - %dx%d (观察阶段)", N, N);
    else
        swprintf(cap, 100, L"数字华容道 - %dx%d %ls", N, N, getDifficultyStars().c_str());
    setcaption(cap);
}

void drawGame() {
    setactivepage(activePage);
    cleardevice();
    setbkcolor(EGERGB(240, 240, 240));
    cleardevice();

    // 外边框阴影
    setcolor(EGERGB(220, 220, 220));
    setlinestyle(PS_SOLID, 0, 5);
    rectangle(BOARD_LEFT + 3, BOARD_TOP + 3, BOARD_LEFT + BOARD_WIDTH + 3, BOARD_TOP + BOARD_HEIGHT + 3);
    // 外边框主框
    setcolor(EGERGB(50, 50, 50));
    setlinestyle(PS_SOLID, 0, 4);
    rectangle(BOARD_LEFT, BOARD_TOP, BOARD_LEFT + BOARD_WIDTH, BOARD_TOP + BOARD_HEIGHT);
    // 内部格子线
    setcolor(EGERGB(200, 200, 200));
    setlinestyle(PS_SOLID, 0, 1);
    for (int i = 1; i < N; ++i) {
        int x = BOARD_LEFT + i * CELL_SIZE;
        line(x, BOARD_TOP, x, BOARD_TOP + BOARD_HEIGHT);
        int y = BOARD_TOP + i * CELL_SIZE;
        line(BOARD_LEFT, y, BOARD_LEFT + BOARD_WIDTH, y);
    }
    setlinestyle(PS_SOLID, 0, 1);

    int fontSize = min(36, CELL_SIZE / 3);
    setfont(fontSize, 0, L"SimSun");
    setbkmode(TRANSPARENT);

    for (int i = 0; i < N; ++i)
        for (int j = 0; j < N; ++j) {
            int val = board[i][j];
            bool shadow = hasShadow[i][j];
            int x = BOARD_LEFT + j * CELL_SIZE, y = BOARD_TOP + i * CELL_SIZE;
            
            // 处理空格
            if (val == 0) {
                // 盲棋复原阶段：空格始终显示为白色背景（无文字）
                if (blindMode && !blindObserving && !blindFailed) {
                    setfillcolor(WHITE);
                    bar(x + 2, y + 2, x + CELL_SIZE - 2, y + CELL_SIZE - 2);
                } else if (shadow) {
                    // 普通模式或观察阶段：空格+阴影重叠显示红色背景 + 白色NKU
                    setfillcolor(EGERGB(255, 0, 0));
                    bar(x + 2, y + 2, x + CELL_SIZE - 2, y + CELL_SIZE - 2);
                    setcolor(WHITE);
                    int nkuFontSize = min(36, CELL_SIZE / 3);
                    setfont(nkuFontSize, 0, L"SimSun");
                    wchar_t nkuTxt[] = L"NKU";
                    int tw = textwidth(nkuTxt), th = textheight(nkuTxt);
                    outtextxy(x + (CELL_SIZE - tw) / 2, y + (CELL_SIZE - th) / 2, nkuTxt);
                }
                continue;
            }

            // 非空格格子
            bool realVisible = (!blindMode) || (!blindObserving && !blindFailed) || blindFailed;
            if (blindMode && blindObserving && !blindFailed) realVisible = true;
            if (blindMode && !blindObserving && !blindFailed) realVisible = false;

            if (realVisible) {
                // 可见阶段（普通模式、观察阶段、失败后）
                if (shadow) {
                    // 阴影数字：紫色背景 + 白色NKU
                    setfillcolor(EGERGB(148, 0, 211));
                    bar(x + 2, y + 2, x + CELL_SIZE - 2, y + CELL_SIZE - 2);
                    setcolor(WHITE);
                    int nkuFontSize = min(36, CELL_SIZE / 3);
                    setfont(nkuFontSize, 0, L"SimSun");
                    wchar_t nkuTxt[] = L"NKU";
                    int tw = textwidth(nkuTxt), th = textheight(nkuTxt);
                    outtextxy(x + (CELL_SIZE - tw) / 2, y + (CELL_SIZE - th) / 2, nkuTxt);
                } else {
                    // 普通数字：蓝色背景 + 白色数字
                    setfillcolor(EGERGB(70, 130, 200));
                    bar(x + 2, y + 2, x + CELL_SIZE - 2, y + CELL_SIZE - 2);
                    setcolor(WHITE);
                    wchar_t txt[4];
                    swprintf(txt, 4, L"%d", val);
                    int tw = textwidth(txt), th = textheight(txt);
                    outtextxy(x + (CELL_SIZE - tw) / 2, y + (CELL_SIZE - th) / 2, txt);
                }
            } else {
                // 盲棋复原阶段：所有数字格统一显示紫色NKU（不分阴影）
                setfillcolor(EGERGB(148, 0, 211));
                bar(x + 2, y + 2, x + CELL_SIZE - 2, y + CELL_SIZE - 2);
                setcolor(WHITE);
                int nkuFontSize = min(36, CELL_SIZE / 3);
                setfont(nkuFontSize, 0, L"SimSun");
                wchar_t nkuTxt[] = L"NKU";
                int tw = textwidth(nkuTxt), th = textheight(nkuTxt);
                outtextxy(x + (CELL_SIZE - tw) / 2, y + (CELL_SIZE - th) / 2, nkuTxt);
            }
        }

    // 提示箭头（标准模式，仅3/4难度1）
    if (showHint && hintDir != -1 && !gameWin && !blindFailed && !blindMode) {
        int br, bc;
        findBlank(br, bc);
        int blankX = BOARD_LEFT + bc * CELL_SIZE + CELL_SIZE / 2;
        int blankY = BOARD_TOP + br * CELL_SIZE + CELL_SIZE / 2;
        int arrowX = blankX, arrowY = blankY;
        int arrowSize = CELL_SIZE / 4;
        switch (hintDir) {
            case 0: arrowY -= arrowSize; break;
            case 1: arrowY += arrowSize; break;
            case 2: arrowX -= arrowSize; break;
            case 3: arrowX += arrowSize; break;
            default: showHint = false; hintDir = -1; break;
        }
        setcolor(EGERGB(255, 0, 0));
        setlinestyle(PS_SOLID, 0, 3);
        line(blankX, blankY, arrowX, arrowY);
        int headSize = 10;
        double angle = atan2(arrowY - blankY, arrowX - blankX);
        int x1 = arrowX - headSize * cos(angle - M_PI/6);
        int y1 = arrowY - headSize * sin(angle - M_PI/6);
        int x2 = arrowX - headSize * cos(angle + M_PI/6);
        int y2 = arrowY - headSize * sin(angle + M_PI/6);
        line(arrowX, arrowY, x1, y1);
        line(arrowX, arrowY, x2, y2);
        setlinestyle(PS_SOLID, 0, 1);
    }

    int infoBaseY = BOARD_TOP + BOARD_HEIGHT + 20;
    if (infoBaseY + 150 > WIN_H) infoBaseY = WIN_H - 150;
    if (infoBaseY < BOARD_TOP + BOARD_HEIGHT + 5) infoBaseY = BOARD_TOP + BOARD_HEIGHT + 5;

    setfont(26, 0, L"SimSun");
    setcolor(EGERGB(50, 50, 50));
    wchar_t stepStr[50];
    swprintf(stepStr, 50, L"步数: %d", stepCount);
    outtextxy(60, infoBaseY, stepStr);

    if (blindMode && blindObserving && !blindFailed) {
        observeTime = (clock() - observeStart) / (double)CLOCKS_PER_SEC;
        wchar_t obsStr[80];
        swprintf(obsStr, 80, L"观察时间: %.3f 秒", observeTime);
        outtextxy(280, infoBaseY, obsStr);
    } else if (blindMode && !blindObserving && !gameWin && !blindFailed) {
        recoverTime = (clock() - recoverStart) / (double)CLOCKS_PER_SEC;
        wchar_t recStr[80];
        swprintf(recStr, 80, L"复原时间: %.3f 秒", recoverTime);
        outtextxy(280, infoBaseY, recStr);
    } else {
        if (timing && !gameWin) currentTime = (clock() - startTime) / (double)CLOCKS_PER_SEC;
        wchar_t timeStr[80];
        if (timing || currentTime > 0.0 || gameWin) {
            swprintf(timeStr, 80, L"用时: %.3f 秒", currentTime);
            outtextxy(280, infoBaseY, timeStr);
        } else if (!gameWin && !blindMode) {
            setfont(22, 0, L"SimSun");
            setcolor(RGB(200, 80, 80));
            outtextxy(280, infoBaseY, L"按方向键开始");
        }
    }

    if (!blindMode) {
        int key = N * 100 + difficulty;
        auto it = bestTimeMap.find(key);
        if (it != bestTimeMap.end()) {
            wchar_t bestStr[80];
            swprintf(bestStr, 80, L"最佳: %.3f 秒", it->second);
            setfont(22, 0, L"SimSun");
            setcolor(RGB(200, 100, 0));
            outtextxy(580, infoBaseY, bestStr);
        }
    } else {
        int key = N * 100 + 7;
        setfont(20, 0, L"SimSun");
        setcolor(RGB(200, 100, 0));
        if (blindObserveBest.find(key) != blindObserveBest.end()) {
            wchar_t oStr[80];
            swprintf(oStr, 80, L"观察最佳: %.2f", blindObserveBest[key]);
            outtextxy(600, infoBaseY, oStr);
        }
        if (blindRecoverBest.find(key) != blindRecoverBest.end()) {
            wchar_t rStr[80];
            swprintf(rStr, 80, L"复原最佳: %.2f", blindRecoverBest[key]);
            outtextxy(600, infoBaseY + 25, rStr);
        }
        if (blindTotalBest.find(key) != blindTotalBest.end()) {
            wchar_t tStr[80];
            swprintf(tStr, 80, L"总最佳: %.2f", blindTotalBest[key]);
            outtextxy(600, infoBaseY + 50, tStr);
        }
    }

    setfont(20, 0, L"SimSun");
    setcolor(EGERGB(80, 80, 80));
    int startY = infoBaseY + 35, lh = 28;
    if (blindMode) {
        if (blindObserving && !blindFailed) {
            outtextxy(60, startY, L"P 开始复原   Q 返回菜单");
        } else if (!blindObserving && !gameWin && !blindFailed) {
            outtextxy(60, startY, L"WASD 移动   P 提交答案   Q 返回菜单");
            outtextxy(60, startY + lh, L"U 撤销上一步");
        } else if (blindFailed) {
            outtextxy(60, startY, L"复原失败！按 Q 返回菜单");
        }
    } else {
        outtextxy(60, startY, L"WASD 移动   R 重置   Q 返回菜单");
        outtextxy(60, startY + lh, L"U 撤销上一步   P 洗牌（新局）");
        outtextxy(60, startY + 2 * lh, L"H 显示提示（仅3*3/4*4难度1，持续到下一步操作）");
    }
    outtextxy(60, startY + 3 * lh, L"难度: ");
    setcolor(RGB(200, 100, 0));
    if (!blindMode) {
        wstring stars = getDifficultyStars();
        outtextxy(150, startY + 3 * lh, stars.c_str());
    } else {
        outtextxy(150, startY + 3 * lh, L"盲棋");
    }
    setcolor(EGERGB(80, 80, 80));
    outtextxy(280, startY + 3 * lh, L"阴影格: 紫色背景(NKU), 空格+阴影显示红色");
    if (blindFailed) {
        setfont(32, 0, L"SimSun");
        setcolor(RGB(200, 50, 50));
        outtextxy(WIN_W / 2 - 120, BOARD_TOP + BOARD_HEIGHT / 3, L"复原失败！按 Q 返回");
    }
    if (gameWin) {
        showHint = false;
        hintDir = -1;
        setfont(56, 0, L"SimSun");
        setcolor(RED);
        wchar_t win[] = L"胜利！";
        int tw = textwidth(win);
        outtextxy((WIN_W - tw) / 2, BOARD_TOP + BOARD_HEIGHT / 3, win);
        setfont(32, 0, L"SimSun");
        setcolor(RGB(0, 150, 0));
        wchar_t info[100];
        if (blindMode) {
            double total = observeTime + recoverTime;
            swprintf(info, 100, L"观察: %.3f 秒  复原: %.3f 秒  总计: %.3f 秒",
                     observeTime, recoverTime, total);
        } else {
            swprintf(info, 100, L"完成时间: %.3f 秒", currentTime);
        }
        int tx = textwidth(info);
        outtextxy((WIN_W - tx) / 2, BOARD_TOP + BOARD_HEIGHT / 3 + 70, info);
        if (!victoryEffectDrawn) {
            setcolor(RGB(255, 215, 0));
            for (int i = 0; i < 40; i++) {
                int x = BOARD_LEFT + rand() % BOARD_WIDTH;
                int y = BOARD_TOP + rand() % BOARD_HEIGHT;
                circle(x, y, 4);
                setfillcolor(RGB(255, 255, 0));
                fillcircle(x, y, 4);
            }
            victoryEffectDrawn = true;
        }
    }

    setvisualpage(activePage);
    activePage = 1 - activePage;
    visiblePage = 1 - visiblePage;
}

//------
void drawMenu() {
    setactivepage(activePage);
    cleardevice();
    setbkcolor(EGERGB(240, 240, 240));
    cleardevice();

    int startY = 40;
    setfont(40, 0, L"SimSun");
    setcolor(BLACK);
    outtextxy(60, startY, L"数字华容道");

    setfont(28, 0, L"SimSun");
    setcolor(RGB(200, 80, 0));
    if (menuStep == 0) outtextxy(40, startY + 90, L"▶");
    else outtextxy(40, startY + 180, L"▶");

    setcolor(BLACK);
    setfont(24, 0, L"SimSun");
    outtextxy(80, startY + 90, L"1. 选择棋盘大小（按数字键 3-8）");
    outtextxy(80, startY + 180, L"2. 选择难度（按数字键 1-6，按0盲棋模式）");

    int sizes[] = {3, 4, 5, 6, 7, 8};
    int x1 = 80, y1 = startY + 140, stepX = 90;
    for (int i = 0; i < 6; i++) {
        wchar_t line[20];
        swprintf(line, 20, L"%d x %d", sizes[i], sizes[i]);
        outtextxy(x1 + i * stepX, y1, line);
    }

    int diffY = startY + 230, diffStep = 35;
    outtextxy(80, diffY, L"1 - ★ (无阴影)");
    outtextxy(80, diffY + diffStep, L"2 - ★★ (10-20%阴影)");
    outtextxy(80, diffY + 2 * diffStep, L"3 - ★★★ (30-40%阴影)");
    outtextxy(80, diffY + 3 * diffStep, L"4 - ★★★★ (50-60%阴影)");
    outtextxy(80, diffY + 4 * diffStep, L"5 - ★★★★★ (70-80%阴影)");
    outtextxy(80, diffY + 5 * diffStep, L"6 - ★★★★★★ (仅一个非阴影)");
    outtextxy(80, diffY + 6 * diffStep, L"0 - 盲棋模式（观察+复原计时）");

    wchar_t szText[50];
    swprintf(szText, 50, L"当前选中大小: %d x %d", selectedSize, selectedSize);
    setcolor(RGB(0, 0, 150));
    outtextxy(80, startY + 470, szText);

    setfont(20, 0, L"SimSun");
    setcolor(RGB(100, 100, 100));
    outtextxy(80, startY + 510, L"当前尺寸最佳记录：");
    int yOff = startY + 540;
    for (int diff = 1; diff <= 6; diff++) {
        int key = selectedSize * 100 + diff;
        auto it = bestTimeMap.find(key);
        wchar_t line[120];
        wstring stars = L"";
        for (int i = 0; i < diff; i++) stars += L"★";
        if (it != bestTimeMap.end())
            swprintf(line, 120, L"难度 %d (%ls) : %.3f 秒", diff, stars.c_str(), it->second);
        else
            swprintf(line, 120, L"难度 %d (%ls) : 无记录", diff, stars.c_str());
        outtextxy(100, yOff + (diff - 1) * 22, line);
    }

    int blindKey = selectedSize * 100 + 7;
    auto itO = blindObserveBest.find(blindKey);
    auto itR = blindRecoverBest.find(blindKey);
    auto itT = blindTotalBest.find(blindKey);
    if (itO != blindObserveBest.end() || itR != blindRecoverBest.end() || itT != blindTotalBest.end()) {
        setfont(20, 0, L"SimSun");
        setcolor(RGB(150, 50, 150));
        outtextxy(80, yOff + 150, L"盲棋模式最佳记录：");
        if (itO != blindObserveBest.end()) {
            wchar_t oStr[100];
            swprintf(oStr, 100, L"观察: %.3f 秒", itO->second);
            outtextxy(100, yOff + 180, oStr);
        }
        if (itR != blindRecoverBest.end()) {
            wchar_t rStr[100];
            swprintf(rStr, 100, L"复原: %.3f 秒", itR->second);
            outtextxy(300, yOff + 180, rStr);
        }
        if (itT != blindTotalBest.end()) {
            wchar_t tStr[100];
            swprintf(tStr, 100, L"总: %.3f 秒", itT->second);
            outtextxy(550, yOff + 180, tStr);
        }
    }

    int ruleStartX = 480;
    int ruleStartY = startY + 230;
    setfont(26, 0, L"SimSun");
    setcolor(RGB(0, 0, 0));
    outtextxy(ruleStartX, ruleStartY, L"游戏规则");
    setfont(20, 0, L"SimSun");
    setcolor(EGERGB(60, 60, 60));
    const wchar_t *rules[] = {
        L"1. 目标：按顺序排列数字（1~N*N-1，空格右下角）",
        L"2. 阴影格：紫色背景(NKU)，空格+阴影显示红色",
        L"3. 操作：WASD移动，R重置（本局初态），P洗牌（新局）",
        L"4. 盲棋模式：观察阶段按P开始复原，复原阶段按P提交",
        L"5. 失败后显示盘面，按Q返回菜单",
        L"6. H键提示：3*3/4*4难度1下，显示最优移动方向"
    };
    for (int i = 0; i < 6; i++) {
        outtextxy(ruleStartX, ruleStartY + 40 + i * 28, rules[i]);
    }

    setfont(22, 0, L"SimSun");
    setcolor(RGB(150, 150, 150));
    outtextxy(60, WIN_H - 50, L"提示：按数字键3-8选大小，再按1-6选难度或0盲棋；按Q重置选择");

    setvisualpage(activePage);
    activePage = 1 - activePage;
    visiblePage = 1 - visiblePage;
}

void handleMenuKey(int key) {
    if (key == 'Q' || key == 'q') {
        menuStep = 0; selectedSize = 4;
        showHint = false;
        hintDir = -1;
        setcaption(L"数字华容道 - 请选择大小 (3-8)");
        return;
    }
    if (menuStep == 0) {
        if (key >= '3' && key <= '8') {
            selectedSize = key - '0';
            menuStep = 1;
            setcaption(L"数字华容道 - 请选择难度 1-6 或 0盲棋");
        }
    } else {
        if (key == 8 || key == 27) {
            menuStep = 0;
            setcaption(L"数字华容道 - 请选择大小 (3-8)");
        } else if (key >= '1' && key <= '6') {
            int diff = key - '0';
            initGame(selectedSize, diff, false);
            currentState = STATE_GAME;
        } else if (key == '0') {
            initGame(selectedSize, 1, true);
            currentState = STATE_GAME;
        }
    }
}

void handleGameKey(int key) {
    if (key == 'Q' || key == 'q') {
        currentState = STATE_MENU; menuStep = 0; selectedSize = 4;
        showHint = false;
        hintDir = -1;
        setcaption(L"数字华容道 - 菜单");
        return;
    }
    if (blindFailed) return;
    if (blindMode && !gameWin) {
        if (blindObserving) {
            if (key == 'P' || key == 'p') {
                blindObserving = false; recoverStart = clock();
                wchar_t cap[100];
                swprintf(cap, 100, L"盲棋模式 - %dx%d (复原阶段)", N, N);
                setcaption(cap);
                return;
            }
            return;
        } else {
            if (key == 'P' || key == 'p') {
                if (checkWin()) {
                    gameWin = true; stopTiming();
                    showHint = false; hintDir = -1;
                    double total = observeTime + recoverTime;
                    int bkey = N * 100 + 7;
                    if (blindObserveBest.find(bkey) == blindObserveBest.end() || observeTime < blindObserveBest[bkey])
                        blindObserveBest[bkey] = observeTime;
                    if (blindRecoverBest.find(bkey) == blindRecoverBest.end() || recoverTime < blindRecoverBest[bkey])
                        blindRecoverBest[bkey] = recoverTime;
                    if (blindTotalBest.find(bkey) == blindTotalBest.end() || total < blindTotalBest[bkey])
                        blindTotalBest[bkey] = total;
                } else {
                    blindFailed = true; timing = false;
                    showHint = false; hintDir = -1;
                }
                return;
            }
        }
    }
    if (gameWin) {
        if (key == 'R' || key == 'r') {
            board = initialBoard; hasShadow = initialShadow; stepCount = 0; gameWin = false;
            showHint = false; hintDir = -1;
            if (blindMode) {
                blindObserving = true; blindFailed = false; observeStart = clock();
                wchar_t cap[100]; swprintf(cap, 100, L"盲棋模式 - %dx%d (观察阶段)", N, N); setcaption(cap);
                resetTiming();
            } else resetTiming();
            victoryEffectDrawn = false; clearUndo();
        }
        return;
    }

    if (key == 'H' || key == 'h') {
        if ((N == 3 || N == 4) && difficulty == 1 && !blindMode && !gameWin && !blindFailed) {
            hintDir = getHintDirection();
            if (hintDir != -1) showHint = true;
        }
        return;
    }

    switch (key) {
    case 'R': case 'r':
        board = initialBoard; hasShadow = initialShadow; stepCount = 0; gameWin = false;
        showHint = false; hintDir = -1;
        if (blindMode) {
            blindObserving = true; blindFailed = false; observeStart = clock();
            wchar_t cap[100]; swprintf(cap, 100, L"盲棋模式 - %dx%d (观察阶段)", N, N); setcaption(cap);
            resetTiming();
        } else resetTiming();
        victoryEffectDrawn = false; clearUndo();
        break;
    case 'U': case 'u':
        undoLastMove();
        showHint = false; hintDir = -1;
        break;
    case 'P': case 'p':
        if (!blindMode) {
            generateRandomState(); stepCount = 0; gameWin = false;
            showHint = false; hintDir = -1;
            resetTiming(); victoryEffectDrawn = false; clearUndo();
        }
        break;
    default: {
        if (blindMode && (blindObserving || blindFailed)) return;
        int br, bc; findBlank(br, bc);
        int tr = br, tc = bc;
        bool moved = false;
        switch (key) {
        case 'W': case 'w': tr = br - 1; moved = true; break;
        case 'S': case 's': tr = br + 1; moved = true; break;
        case 'A': case 'a': tc = bc - 1; moved = true; break;
        case 'D': case 'd': tc = bc + 1; moved = true; break;
        default: return;
        }
        if (moved && tr >= 0 && tr < N && tc >= 0 && tc < N) {
            if (!blindMode && !timing && !gameWin) startTiming();
            if (blindMode && !blindObserving && !timing && !gameWin && !blindFailed) {
                timing = true; startTime = clock();
            }
            if (moveTile(tr, tc)) {
                showHint = false; hintDir = -1;
                if (!blindMode && checkWin()) { gameWin = true; stopTiming(); }
            }
        }
    }
    }
}

int main() {
    setlocale(LC_ALL, "zh_CN.UTF-8");
    srand((unsigned)time(NULL));
    initgraph(WIN_W, WIN_H);
    setcaption(L"数字华容道 - 菜单");
    activePage = 0; visiblePage = 1;
    setactivepage(activePage); setvisualpage(visiblePage);
    for (; is_run(); delay_fps(20)) {
        if (currentState == STATE_MENU) {
            drawMenu();
            while (kbhit()) handleMenuKey(getch());
        } else {
            drawGame();
            while (kbhit()) handleGameKey(getch());
        }
    }
    closegraph();
    return 0;
}

#pragma GCC diagnostic pop