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

using namespace std;

// 固定窗口大小
const int WIN_W = 900;
const int WIN_H = 850;

// 程序状态
enum ProgramState
{
    STATE_MENU,
    STATE_GAME
};
ProgramState currentState = STATE_MENU;

// 游戏变量
int N = 4;
int CELL_SIZE = 120;
int BOARD_LEFT, BOARD_TOP;
int BOARD_WIDTH, BOARD_HEIGHT;
vector<vector<int>> board;
int stepCount = 0;
bool gameWin = false;

// 计时相关
clock_t startTime = 0;
bool timing = false;
double currentTime = 0.0;
map<int, double> bestTimeMap;

// 胜利特效标志
bool victoryEffectDrawn = false;

// 函数声明
void initGame(int size);
void initBoard();
void findBlank(int &row, int &col);
bool moveTile(int row, int col);
bool checkWin();
void drawMenu();
void drawGame();
void handleMenuKey(int key);
void handleGameKey(int key);
bool isSolvable(const vector<int> &nums, int n);
int countInversions(const vector<int> &nums);
void startTiming();
void stopTiming();
void resetTiming();
void updateBoardLayout();

// 逆序数计算
int countInversions(const vector<int> &nums)
{
    int inv = 0;
    int len = nums.size();
    for (int i = 0; i < len; ++i)
    {
        if (nums[i] == 0)
            continue;
        for (int j = i + 1; j < len; ++j)
        {
            if (nums[j] == 0)
                continue;
            if (nums[i] > nums[j])
                ++inv;
        }
    }
    return inv;
}

bool isSolvable(const vector<int> &nums, int n)
{
    int inv = countInversions(nums);
    int blankRow;
    for (int i = 0; i < n * n; ++i)
    {
        if (nums[i] == 0)
        {
            blankRow = i / n;
            break;
        }
    }
    if (n % 2 == 1)
        return (inv % 2 == 0);
    else
        return ((inv + (n - blankRow)) % 2 == 1);
}

// 动态布局
void updateBoardLayout()
{
    int usableHeight = WIN_H - 180;
    int usableWidth = WIN_W - 120;
    int maxCellByWidth = usableWidth / N;
    int maxCellByHeight = usableHeight / N;
    CELL_SIZE = min(maxCellByWidth, maxCellByHeight);
    if (CELL_SIZE < 40)
        CELL_SIZE = 40;
    BOARD_WIDTH = N * CELL_SIZE;
    BOARD_HEIGHT = N * CELL_SIZE;
    BOARD_LEFT = (WIN_W - BOARD_WIDTH) / 2;
    BOARD_TOP = 70;
}

void initBoard()
{
    vector<int> nums;
    for (int i = 1; i < N * N; ++i)
        nums.push_back(i);
    nums.push_back(0);
    random_device rd;
    mt19937 g(rd());
    do
    {
        shuffle(nums.begin(), nums.end(), g);
    } while (!isSolvable(nums, N));
    for (int i = 0; i < N; ++i)
        for (int j = 0; j < N; ++j)
            board[i][j] = nums[i * N + j];
}

void startTiming()
{
    if (!timing && !gameWin)
    {
        timing = true;
        startTime = clock();
    }
}

void stopTiming()
{
    if (timing)
    {
        timing = false;
        currentTime = (clock() - startTime) / (double)CLOCKS_PER_SEC;
        auto it = bestTimeMap.find(N);
        if (it == bestTimeMap.end() || currentTime < it->second)
        {
            bestTimeMap[N] = currentTime;
        }
    }
}

void resetTiming()
{
    timing = false;
    currentTime = 0.0;
}

void initGame(int size)
{
    N = size;
    updateBoardLayout();
    board.assign(N, vector<int>(N, 0));
    initBoard();
    stepCount = 0;
    gameWin = false;
    resetTiming();
    victoryEffectDrawn = false;
    setcaption(L"数字华容道 - 游戏中");
}

void findBlank(int &row, int &col)
{
    for (int i = 0; i < N; ++i)
        for (int j = 0; j < N; ++j)
            if (board[i][j] == 0)
            {
                row = i;
                col = j;
                return;
            }
}

bool moveTile(int row, int col)
{
    int blankRow, blankCol;
    findBlank(blankRow, blankCol);
    if ((row == blankRow && abs(col - blankCol) == 1) ||
        (col == blankCol && abs(row - blankRow) == 1))
    {
        swap(board[row][col], board[blankRow][blankCol]);
        stepCount++;
        return true;
    }
    return false;
}

bool checkWin()
{
    int expected = 1;
    for (int i = 0; i < N; ++i)
    {
        for (int j = 0; j < N; ++j)
        {
            if (i == N - 1 && j == N - 1)
                return board[i][j] == 0;
            if (board[i][j] != expected)
                return false;
            expected++;
        }
    }
    return true;
}

void drawGame()
{
    cleardevice();
    setbkcolor(EGERGB(240, 240, 240));
    cleardevice();

    // 绘制网格
    setcolor(EGERGB(100, 100, 100));
    for (int i = 0; i <= N; ++i)
    {
        int x = BOARD_LEFT + i * CELL_SIZE;
        int y1 = BOARD_TOP;
        int y2 = BOARD_TOP + BOARD_HEIGHT;
        line(x, y1, x, y2);
        line(BOARD_LEFT, BOARD_TOP + i * CELL_SIZE, BOARD_LEFT + BOARD_WIDTH, BOARD_TOP + i * CELL_SIZE);
    }

    // 绘制数字块
    int fontSize = min(36, CELL_SIZE / 3);
    setfont(fontSize, 0, L"SimSun");
    setbkmode(TRANSPARENT);
    for (int i = 0; i < N; ++i)
    {
        for (int j = 0; j < N; ++j)
        {
            int val = board[i][j];
            if (val == 0)
                continue;
            int x = BOARD_LEFT + j * CELL_SIZE;
            int y = BOARD_TOP + i * CELL_SIZE;
            setfillcolor(EGERGB(70, 130, 200));
            bar(x + 2, y + 2, x + CELL_SIZE - 2, y + CELL_SIZE - 2);
            setcolor(WHITE);
            wchar_t text[4];
            swprintf(text, 4, L"%d", val);
            int tw = textwidth(text);
            int th = textheight(text);
            outtextxy(x + (CELL_SIZE - tw) / 2, y + (CELL_SIZE - th) / 2, text);
        }
    }

    // 信息栏位置
    int infoY = BOARD_TOP + BOARD_HEIGHT + 25;
    if (infoY + 100 > WIN_H)
        infoY = WIN_H - 100;

    // 步数
    setfont(26, 0, L"SimSun");
    setcolor(EGERGB(50, 50, 50));
    wchar_t stepStr[50];
    swprintf(stepStr, 50, L"步数: %d", stepCount);
    outtextxy(60, infoY, stepStr);

    // 当前耗时
    if (timing)
        currentTime = (clock() - startTime) / (double)CLOCKS_PER_SEC;
    wchar_t timeStr[80];
    if (timing || currentTime > 0.0)
    {
        swprintf(timeStr, 80, L"用时: %.3f 秒", currentTime);
        outtextxy(280, infoY, timeStr);
    }
    else if (!gameWin)
    {
        setfont(22, 0, L"SimSun");
        setcolor(RGB(200, 80, 80));
        outtextxy(280, infoY, L"按方向键开始");
    }

    // 最佳记录
    auto it = bestTimeMap.find(N);
    if (it != bestTimeMap.end())
    {
        wchar_t bestStr[80];
        swprintf(bestStr, 80, L"最佳: %.3f 秒", it->second);
        setfont(22, 0, L"SimSun");
        setcolor(RGB(200, 100, 0));
        outtextxy(580, infoY, bestStr);
    }

    // 操作提示
    setfont(20, 0, L"SimSun");
    setcolor(EGERGB(80, 80, 80));
    outtextxy(60, infoY + 45, L"WASD 移动   R 重置   Q 返回菜单");

    // 胜利界面
    if (gameWin)
    {
        setfont(56, 0, L"SimSun");
        setcolor(RED);
        wchar_t winText[] = L"胜利！";
        int tw = textwidth(winText);
        int winX = (WIN_W - tw) / 2;
        int winY = BOARD_TOP + BOARD_HEIGHT / 3;
        outtextxy(winX, winY, winText);

        setfont(32, 0, L"SimSun");
        setcolor(RGB(0, 150, 0));
        wchar_t timeInfo[100];
        swprintf(timeInfo, 100, L"完成时间: %.3f 秒", currentTime);
        int tx = textwidth(timeInfo);
        outtextxy((WIN_W - tx) / 2, winY + 70, timeInfo);

        if (!victoryEffectDrawn)
        {
            setcolor(RGB(255, 215, 0));
            for (int i = 0; i < 40; i++)
            {
                int x = BOARD_LEFT + rand() % BOARD_WIDTH;
                int y = BOARD_TOP + rand() % BOARD_HEIGHT;
                circle(x, y, 4);
                setfillcolor(RGB(255, 255, 0));
                fillcircle(x, y, 4);
            }
            victoryEffectDrawn = true;
        }
    }
}

void drawMenu()
{
    cleardevice();
    setbkcolor(EGERGB(240, 240, 240));
    cleardevice();

    // 标题
    setfont(40, 0, L"SimSun");
    setcolor(BLACK);
    outtextxy(60, 30, L"数字华容道");

    // 左侧：尺寸选择
    setfont(24, 0, L"SimSun");
    outtextxy(60, 100, L"选择棋盘大小（按数字键）：");

    int x1 = 70, y_start = 160;
    int lineHeight = 50;
    int sizes[] = {3, 4, 5, 6, 7, 8};
    for (int i = 0; i < 6; ++i)
    {
        int sz = sizes[i];
        int y = y_start + i * lineHeight;
        wchar_t line[120];
        auto it = bestTimeMap.find(sz);
        if (it != bestTimeMap.end())
        {
            swprintf(line, 120, L"%d x %d   最佳: %.3f 秒", sz, sz, it->second);
        }
        else
        {
            swprintf(line, 120, L"%d x %d   暂无记录", sz, sz);
        }
        outtextxy(x1, y, line);
    }

    // 右侧：规则介绍
    setfont(26, 0, L"SimSun");
    setcolor(EGERGB(0, 0, 0));
    outtextxy(480, 100, L"游戏规则");

    setfont(20, 0, L"SimSun");
    setcolor(EGERGB(60, 60, 60));
    const wchar_t *rules[] = {
        L"1. 目标：将数字按顺序排列",
        L"   （从小到大，空格在右下角）",
        L"2. 操作：WASD 移动数字块",
        L"3. 计时：首次移动后开始计时",
        L"4. 重置：按 R 重置当前棋盘",
        L"5. 返回：按 Q 返回主菜单",
        L"6. 最佳：每个尺寸独立记录",
        L"   （仅当前会话有效）"};
    int ruleY = 160;
    for (int i = 0; i < 8; ++i)
    {
        outtextxy(480, ruleY + i * 32, rules[i]);
    }

    // 底部提示
    setfont(22, 0, L"SimSun");
    setcolor(RGB(150, 150, 150));
    outtextxy(60, WIN_H - 70, L"按对应数字键开始游戏");
}

void handleMenuKey(int key)
{
    if (key >= '3' && key <= '8')
    {
        initGame(key - '0');
        currentState = STATE_GAME;
    }
}

void handleGameKey(int key)
{
    if (key == 'Q' || key == 'q')
    {
        currentState = STATE_MENU;
        setcaption(L"数字华容道 - 菜单");
        return;
    }
    if (gameWin)
    {
        if (key == 'R' || key == 'r')
        {
            initBoard();
            stepCount = 0;
            gameWin = false;
            resetTiming();
            victoryEffectDrawn = false;
        }
        return;
    }
    switch (key)
    {
    case 'R':
    case 'r':
        initBoard();
        stepCount = 0;
        gameWin = false;
        resetTiming();
        victoryEffectDrawn = false;
        break;
    default:
    {
        int blankRow, blankCol;
        findBlank(blankRow, blankCol);
        int targetRow = blankRow, targetCol = blankCol;
        bool moved = false;
        switch (key)
        {
        case 'W':
        case 'w':
            targetRow = blankRow - 1;
            moved = true;
            break;
        case 'S':
        case 's':
            targetRow = blankRow + 1;
            moved = true;
            break;
        case 'A':
        case 'a':
            targetCol = blankCol - 1;
            moved = true;
            break;
        case 'D':
        case 'd':
            targetCol = blankCol + 1;
            moved = true;
            break;
        default:
            return;
        }
        if (moved && targetRow >= 0 && targetRow < N && targetCol >= 0 && targetCol < N)
        {
            if (!timing && !gameWin)
                startTiming();
            moveTile(targetRow, targetCol);
            if (checkWin())
            {
                gameWin = true;
                stopTiming();
            }
        }
    }
    }
}

int main()
{
    setlocale(LC_ALL, "zh_CN.UTF-8");
    srand((unsigned)time(NULL));
    initgraph(WIN_W, WIN_H);
    setcaption(L"数字华容道 - 菜单");

    for (; is_run(); delay_fps(20))
    { // 帧率降到 20，减少重绘频率，缓解闪烁
        if (currentState == STATE_MENU)
        {
            drawMenu();
            while (kbhit())
            {
                int key = getch();
                handleMenuKey(key);
            }
        }
        else if (currentState == STATE_GAME)
        {
            drawGame();
            while (kbhit())
            {
                int key = getch();
                handleGameKey(key);
            }
        }
    }
    closegraph();
    return 0;
}