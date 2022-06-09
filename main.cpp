#include <bits/stdc++.h>
#include <ncurses.h>
#include <unistd.h>

using namespace std;

typedef struct {
  short id;       /* ID to distinguish multiple devices */
  int x, y, z;    /* event coordinates */
  mmask_t bstate; /* button state bits */
} mouseevent;

const int SPEED = 100000;
int score = 0;
int WIDTH = 60, HEIGHT = 20;
typedef pair<int, int> coord;

int direction = 0;

deque<coord> snake;
set<coord> food;
set<pair<char, coord>> specials;
int turbo = 0;

void new_snake();
void new_food();
void gameloop();

void new_game() {
  score = 0;
  turbo = 0;
  new_snake();
  food.clear();
  specials.clear();
  for (int i = 0; i < 10; i++) new_food();
  gameloop();
}

void display() {
  getmaxyx(stdscr, HEIGHT, WIDTH);
  clear();
  box(stdscr, 0, 0);
  attron(COLOR_PAIR(1));
  for (auto &x : snake) mvaddch(x.second, x.first, ' ');
  attroff(COLOR_PAIR(1));

  attron(COLOR_PAIR(2));
  for (auto x : food) mvaddch(x.second, x.first, 'O');
  for (auto x : specials) mvaddch(x.second.second, x.second.first, x.first);
  attroff(COLOR_PAIR(2));

  attron(COLOR_PAIR(3));
  mvprintw(0, 0, "Score: %d", score);
  attroff(COLOR_PAIR(3));

  if (turbo > 0) {
    attron(COLOR_PAIR(4));
    mvaddch(snake[0].second, snake[0].first, ' ');
    for (int i = 0; i < turbo; i++) mvaddch(HEIGHT - 1, i, ' ');
    attroff(COLOR_PAIR(4));
    turbo--;
  }

  refresh();
}

coord get_next_square() {
  coord front = snake.front();
  if (direction == 0) front.first++;
  if (direction == 1) front.second--;
  if (direction == 2) front.first--;
  if (direction == 3) front.second++;
  return front;
}

void game_over() {
  mvprintw(HEIGHT / 2, WIDTH / 2, "Game Over!");
  refresh();
  sleep(2);
  new_game();
}

void add_head() {
  coord front = get_next_square();
  if (front.first < 0 || front.first >= WIDTH || front.second < 0 ||
      front.second >= HEIGHT ||
      find(snake.begin(), snake.end(), front) != snake.end()) {
    game_over();
  } else if (food.find(front) != food.end()) {
    food.erase(front);
    score++;
    add_head();
    new_food();
  } else if (specials.find({'T', front}) != specials.end()) {
    turbo += 50;
    specials.erase({'T', front});
  }

  snake.push_front(front);
}

void move_snake() {
  add_head();
  snake.pop_back();
}

void new_food() {
  coord front = {rand() % (WIDTH - 2) + 1, rand() % (HEIGHT - 2) + 1};
  food.insert(front);
  if (rand() % 10 == 0) {
    specials.insert({'T', front});
  }
}

void get_direction(int x = 99) {
  MEVENT event;

  int ch = getch();
  // if (x == 0) ch = KEY_UP;
  // if (x == 1) ch = KEY_LEFT;
  // if (x == 2) ch = KEY_DOWN;
  // if (x == 3) ch = KEY_RIGHT;
  if (ch == KEY_UP && direction != 3) direction = 1;
  if (ch == KEY_LEFT && direction != 0) direction = 2;
  if (ch == KEY_DOWN && direction != 1) direction = 3;
  if (ch == KEY_RIGHT && direction != 2) direction = 0;
  if (ch == KEY_MOUSE && getmouse(&event) == OK)
    if (event.bstate & BUTTON1_CLICKED) food.insert({event.x, event.y});
}

void gameloop() {
  while (1) {
    display();
    get_direction();
    move_snake();
    if (turbo > 0) {
      usleep(SPEED / 2);
      if (direction == 1 || direction == 3) usleep(SPEED / 2 / 2.5);
    } else {
      usleep(SPEED);
      if (direction == 1 || direction == 3) usleep(SPEED / 2.5);
    }
  }
}

void new_snake() {
  coord front = {WIDTH / 2, HEIGHT / 2};
  snake.clear();
  for (int i = 0; i < 10; i++) {
    snake.push_front(front);
    front.first++;
  }
}

void init_ncurses() {
  initscr();
  cbreak();
  noecho();
  keypad(stdscr, TRUE);  // make keys work
  curs_set(0);           // hide cursor

  /* Get all the mouse events */
  mousemask(ALL_MOUSE_EVENTS, NULL);
  timeout(100);
}

int main() {
  srand(time(0));
  init_ncurses();
  nodelay(stdscr, TRUE);
  start_color();
  init_pair(1, COLOR_GREEN, COLOR_GREEN);
  init_pair(2, COLOR_RED, COLOR_BLACK);
  init_pair(3, COLOR_YELLOW, COLOR_BLACK);
  init_pair(4, COLOR_YELLOW, COLOR_YELLOW);

  new_game();

  delwin(stdscr);
  endwin();
  nocbreak();
}