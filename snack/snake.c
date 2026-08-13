#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>

#define WIDTH 20
#define HEIGHT 20
#define MAX_LENGTH 100

typedef struct {
    int x, y;
} Point;

typedef struct {
    Point body[MAX_LENGTH];
    int length;
    int direction;
} Snake;

typedef struct {
    Point position;
    int active;
} Food;

// 游戏状态
Snake snake;
Food food;
int score = 0;
int game_over = 0;
struct termios original_termios;

// 初始化终端设置
void setup_terminal() {
    struct termios new_termios;
    tcgetattr(STDIN_FILENO, &original_termios);
    new_termios = original_termios;
    new_termios.c_lflag &= ~(ICANON | ECHO);
    new_termios.c_cc[VMIN] = 0;
    new_termios.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSANOW, &new_termios);
}

// 恢复终端设置
void restore_terminal() {
    tcsetattr(STDIN_FILENO, TCSANOW, &original_termios);
}

// 清屏
void clear_screen() {
    printf("\033[2J\033[H");
}

// 移动光标到指定位置
void move_cursor(int x, int y) {
    printf("\033[%d;%dH", y + 1, x + 1);
}

// 初始化游戏
void init_game() {
    // 初始化蛇
    snake.length = 3;
    snake.direction = 1; // 1=右, 2=下, 3=左, 4=上
    for (int i = 0; i < snake.length; i++) {
        snake.body[i].x = WIDTH / 2 - i;
        snake.body[i].y = HEIGHT / 2;
    }
    
    // 初始化食物
    food.active = 0;
    score = 0;
    game_over = 0;
    
    // 生成第一个食物
    srand(time(NULL));
    food.position.x = rand() % (WIDTH - 2) + 1;
    food.position.y = rand() % (HEIGHT - 2) + 1;
    food.active = 1;
}

// 生成食物
void spawn_food() {
    if (!food.active) {
        food.position.x = rand() % (WIDTH - 2) + 1;
        food.position.y = rand() % (HEIGHT - 2) + 1;
        food.active = 1;
    }
}

// 检查碰撞
int check_collision() {
    // 检查墙壁碰撞
    if (snake.body[0].x <= 0 || snake.body[0].x >= WIDTH - 1 ||
        snake.body[0].y <= 0 || snake.body[0].y >= HEIGHT - 1) {
        return 1;
    }
    
    // 检查自身碰撞
    for (int i = 1; i < snake.length; i++) {
        if (snake.body[0].x == snake.body[i].x &&
            snake.body[0].y == snake.body[i].y) {
            return 1;
        }
    }
    
    return 0;
}

// 移动蛇
void move_snake() {
    // 移动身体
    for (int i = snake.length - 1; i > 0; i--) {
        snake.body[i] = snake.body[i - 1];
    }
    
    // 移动头部
    switch (snake.direction) {
        case 1: // 右
            snake.body[0].x++;
            break;
        case 2: // 下
            snake.body[0].y++;
            break;
        case 3: // 左
            snake.body[0].x--;
            break;
        case 4: // 上
            snake.body[0].y--;
            break;
    }
}

// 检查是否吃到食物
void check_food() {
    if (snake.body[0].x == food.position.x &&
        snake.body[0].y == food.position.y) {
        snake.length++;
        score += 10;
        food.active = 0;
        spawn_food();
    }
}

// 绘制游戏界面
void draw() {
    clear_screen();
    
    // 绘制边框
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            move_cursor(x, y);
            if (y == 0 || y == HEIGHT - 1 || x == 0 || x == WIDTH - 1) {
                printf("#");
            } else {
                printf(" ");
            }
        }
    }
    
    // 绘制蛇
    for (int i = 0; i < snake.length; i++) {
        move_cursor(snake.body[i].x, snake.body[i].y);
        if (i == 0) {
            printf("@"); // 蛇头
        } else {
            printf("o"); // 蛇身
        }
    }
    
    // 绘制食物
    if (food.active) {
        move_cursor(food.position.x, food.position.y);
        printf("*");
    }
    
    // 显示分数
    move_cursor(WIDTH + 2, 0);
    printf("Score: %d", score);
    
    move_cursor(WIDTH + 2, 1);
    printf("Length: %d", snake.length);
    
    move_cursor(WIDTH + 2, 3);
    printf("Controls:");
    move_cursor(WIDTH + 2, 4);
    printf("W/↑: Up");
    move_cursor(WIDTH + 2, 5);
    printf("S/↓: Down");
    move_cursor(WIDTH + 2, 6);
    printf("A/←: Left");
    move_cursor(WIDTH + 2, 7);
    printf("D/→: Right");
    move_cursor(WIDTH + 2, 8);
    printf("Q: Quit");
    
    fflush(stdout);
}

// 处理输入
void process_input() {
    char c;
    if (read(STDIN_FILENO, &c, 1) == 1) {
        switch (c) {
            case 'w':
            case 'W':
                if (snake.direction != 2) snake.direction = 4;
                break;
            case 's':
            case 'S':
                if (snake.direction != 4) snake.direction = 2;
                break;
            case 'a':
            case 'A':
                if (snake.direction != 1) snake.direction = 3;
                break;
            case 'd':
            case 'D':
                if (snake.direction != 3) snake.direction = 1;
                break;
            case 'q':
            case 'Q':
                game_over = 1;
                break;
        }
    }
}

// 游戏主循环
void game_loop() {
    while (!game_over) {
        draw();
        process_input();
        move_snake();
        check_food();
        
        if (check_collision()) {
            game_over = 1;
        }
        
        usleep(100000); // 控制游戏速度 (100ms)
    }
}

// 显示游戏结束信息
void show_game_over() {
    clear_screen();
    move_cursor(WIDTH / 2 - 5, HEIGHT / 2);
    printf("GAME OVER!");
    move_cursor(WIDTH / 2 - 5, HEIGHT / 2 + 1);
    printf("Final Score: %d", score);
    move_cursor(WIDTH / 2 - 5, HEIGHT / 2 + 2);
    printf("Snake Length: %d", snake.length);
    move_cursor(WIDTH / 2 - 5, HEIGHT / 2 + 4);
    printf("Press any key to exit...");
    fflush(stdout);
    
    // 等待用户按键
    char c;
    if (read(STDIN_FILENO, &c, 1) < 0) {}
}

int main() {
    setup_terminal();
    init_game();
    game_loop();
    show_game_over();
    restore_terminal();
    return 0;
}