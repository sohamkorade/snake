#include <bits/stdc++.h>
#include <ncurses.h>
#include <unistd.h>

using namespace std;

struct mouseevent {
  short id;        // ID to distinguish multiple devices
  int x, y, z;     // event coordinates
  mmask_t bstate;  // button state bits
};

enum ItemType { Food, Poison, Turbo, Magnet };
enum Direction { East, North, West, South };

struct Pixel {
  int x, y;
};

struct Item {
  char type;
  Pixel pixel;
};

struct Snake {
  deque<Pixel> body;
  ItemType powerup = Food;
  int turbo = 0;
  bool magnet = false;
  int score = 0;
  int speed = 100000;
  Direction direction = East;

  void reset() {
    body.clear(), turbo = 0, score = 0, magnet = false;

    Pixel front = {WIDTH / 2, HEIGHT / 2};
    for (int i = 0; i < 10; i++) {
      body.push_front(front);
      front.x++;
    }
  }

  Pixel get_next_pixel() {
    Pixel front = body.front();
    if (direction == 0) front.x++;
    if (direction == 1) front.y--;
    if (direction == 2) front.x--;
    if (direction == 3) front.y++;
    return front;
  }
};

int WIDTH = 60, HEIGHT = 20;

struct Field {
  Snake snake;
  set<Item> items;

  void reset() {
    snake.score = 0;
    snake.reset();
    items.clear();
    for (int i = 0; i < 10; i++) spawn_item();
    gameloop();
  }

  void game_over() {
    mvprintw(HEIGHT / 2, WIDTH / 2 - 10, "Game Over!");
    refresh();
    sleep(2);
    reset();
  }

  void display() {
    getmaxyx(stdscr, HEIGHT, WIDTH);
    clear();
    box(stdscr, 0, 0);
    attron(COLOR_PAIR(1));
    for (auto &part : snake.body) mvaddch(part.y, part.x, ' ');
    attroff(COLOR_PAIR(1));

    attron(COLOR_PAIR(2));
    for (auto item : items) mvaddch(item.type, item.pixel.x, item.pixel.y);
    attroff(COLOR_PAIR(2));

    attron(COLOR_PAIR(3));
    mvprintw(0, 0, "Score: %d", snake.score);
    attroff(COLOR_PAIR(3));

    if (snake.turbo > 0) {
      attron(COLOR_PAIR(4));
      mvaddch(snake.body[0].y, snake.body[0].x, ' ');
      for (int i = 0; i < snake.turbo; i++) mvaddch(HEIGHT - 1, i, ' ');
      attroff(COLOR_PAIR(4));
      snake.turbo--;
    }

    refresh();
  }

  void add_head() {
    Pixel front = snake.get_next_pixel();
    if (front.x < 0 || front.x >= WIDTH || front.y < 0 || front.y >= HEIGHT ||
        find(snake.body.begin(), snake.body.end(), front) != snake.body.end()) {
      game_over();
    } else if (items.find({'O', front}) != items.end()) {
      items.erase({'O', front});
      snake.score++;
      add_head();
      spawn_item();
    } else if (items.find({'T', front}) != items.end()) {
      snake.turbo += 50;
      items.erase({'T', front});
    }

    for (auto &item : items) {
      if (item.type == 'M' && distance(item.pixel, front) <= 5) {
        auto temp(item);
        temp.pixel.x += front.x - temp.pixel.x;
        temp.pixel.y += front.y - temp.pixel.y;

        items.erase(item);
        items.insert(temp);
      }
    }

    snake.body.push_front(front);
  }

  void move_snake() {
    add_head();
    snake.body.pop_back();
  }

  void spawn_item() {
    Pixel front = {rand() % (WIDTH - 2) + 1, rand() % (HEIGHT - 2) + 1};

    int random = rand() % 10;
    if (random == 0)
      items.insert({'T', front});
    else if (random == 1)
      items.insert({'M', front});
    else
      items.insert({'O', front});
  }
};

Field game;

int distance(Pixel a, Pixel b) { return abs(a.x - b.x) + abs(a.y - b.y); }

  void handle_events() {
    MEVENT event;
    int ch = getch();
    if (ch == KEY_UP && snake.direction != South) snake.direction = North;
    if (ch == KEY_LEFT && snake.direction != East) snake.direction = West;
    if (ch == KEY_DOWN && snake.direction != North) snake.direction = South;
    if (ch == KEY_RIGHT && snake.direction != West) snake.direction = East;
    if (ch == KEY_MOUSE && getmouse(&event) == OK)
      if (event.bstate & BUTTON1_CLICKED)
        items.insert({Food, {event.x, event.y}});
    if (ch == 'q') snake.alive = false;
    if (ch == 'p') pause_game();

    if (CHEATS) {
      if (ch == 'f') snake.fast += 50;
      if (ch == 's') snake.slow += 50;
      if (ch == 'm') snake.magnet += 50;
      if (ch == 'h') snake.shield += 50;
      if (ch == 'r') snake.reverse();
    }
  }

  int ch = getch();
  if (ch == KEY_UP && direction != South) direction = North;
  if (ch == KEY_LEFT && direction != East) direction = West;
  if (ch == KEY_DOWN && direction != North) direction = South;
  if (ch == KEY_RIGHT && direction != West) direction = East;
  if (ch == KEY_MOUSE && getmouse(&event) == OK)
    if (event.bstate & BUTTON1_CLICKED) items.insert({'O', {event.x, event.y}});
}

void gameloop() {
  while (1) {
    display();
    get_direction();
    move_snake();
    if (snake.turbo > 0) {
      usleep(speed / 2);
      if (direction == 1 || direction == 3) usleep(speed / 2 / 2.5);
    } else {
      usleep(speed);
      if (direction == 1 || direction == 3) usleep(speed / 2.5);
    }
  }
};

void init_ncurses() {
  initscr();
  cbreak();              // no line buffering
  noecho();              // don't echo keypresses
  keypad(stdscr, TRUE);  // make keys work
  curs_set(0);           // hide cursor
  halfdelay(1);          // wait for 1 char

  mousemask(ALL_MOUSE_EVENTS, NULL);  // Get all the mouse events
  timeout(100);

  nodelay(stdscr, TRUE);
  start_color();
  init_pair(1, COLOR_GREEN, COLOR_GREEN);
  init_pair(2, COLOR_RED, COLOR_BLACK);
  init_pair(3, COLOR_YELLOW, COLOR_BLACK);
  init_pair(4, COLOR_YELLOW, COLOR_YELLOW);
}

int main() {
  srand(time(0));
  init_ncurses();
  new_game();

  Field game;
  game.reset();
  game.gameloop();

  close_ncurses();
}