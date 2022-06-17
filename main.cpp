#include <bits/stdc++.h>
#include <ncurses.h>
#include <unistd.h>

using namespace std;

const int FPS = 60;
const bool CHEATS = true;

struct mouseevent {
  short id;        // ID to distinguish multiple devices
  int x, y, z;     // event coordinates
  mmask_t bstate;  // button state bits
};

enum ItemType {
  Food = 'O' | COLOR_PAIR(5),     // magenta-magenta
  Slow = '-' | COLOR_PAIR(1),     // black-red
  Fast = '+' | COLOR_PAIR(3),     // black-yellow
  Magnet = 'M' | COLOR_PAIR(6),   // black-cyan
  Shield = 'S' | COLOR_PAIR(7),   // black-white
  Wall = '#' | COLOR_PAIR(7),     // black-white
  Reverse = 'R' | COLOR_PAIR(2),  // green-black
};
enum Direction { East, North, West, South };

struct Pixel {
  int x, y;
  bool operator==(const Pixel &other) const {
    return x == other.x && y == other.y;
  }
  bool operator<(const Pixel &other) const {
    return (x < other.x) || ((!(other.x < x)) && (y < other.y));
  }
};

struct Item {
  ItemType type;
  Pixel pixel;
  bool operator<(const Item &other) const { return pixel < other.pixel; }
};

void init_color_pairs() {
  init_pair(0, COLOR_BLACK, COLOR_BLACK);
  init_pair(1, COLOR_RED, COLOR_BLACK);
  init_pair(2, COLOR_GREEN, COLOR_BLACK);
  init_pair(3, COLOR_YELLOW, COLOR_BLACK);
  init_pair(4, COLOR_BLUE, COLOR_BLACK);
  init_pair(5, COLOR_MAGENTA, COLOR_BLACK);
  init_pair(6, COLOR_CYAN, COLOR_BLACK);
  init_pair(7, COLOR_WHITE, COLOR_BLACK);

  init_pair(50, COLOR_BLACK, COLOR_BLACK);
  init_pair(51, COLOR_BLACK, COLOR_RED);
  init_pair(52, COLOR_BLACK, COLOR_GREEN);
  init_pair(53, COLOR_BLACK, COLOR_YELLOW);
  init_pair(54, COLOR_BLACK, COLOR_BLUE);
  init_pair(55, COLOR_BLACK, COLOR_MAGENTA);
  init_pair(56, COLOR_BLACK, COLOR_CYAN);
  init_pair(57, COLOR_BLACK, COLOR_WHITE);
}

int choice_menu(string title, vector<string> choices) {
  int choice = 0;
  int choice_count = choices.size();
  int choice_width = 30;
  int choice_height = choice_count + 2;
  int choice_x = (COLS - choice_width) / 2;
  int choice_y = (LINES - choice_height) / 2;
  int title_x = (choice_width - title.length()) / 2;

  WINDOW *choice_win = newwin(choice_height, choice_width, choice_y, choice_x);
  int ch;
  while (true) {
    if (ch == KEY_UP) choice--;
    if (ch == KEY_DOWN) choice++;
    if (ch == '\n') break;
    if (ch == 'q') return -1;
    if (choice < 0) choice = choice_count - 1;
    if (choice >= choice_count) choice = 0;
    werase(choice_win);
    box(choice_win, 0, 0);
    mvwaddnstr(choice_win, 0, title_x, title.c_str(), choice_width - 2);
    for (int i = 0; i < choice_count; i++) {
      if (i == choice) wattron(choice_win, A_REVERSE);
      mvwprintw(choice_win, i + 1, 1, choices[i].c_str());
      if (i == choice) wattroff(choice_win, A_REVERSE);
    }
    wrefresh(choice_win);
    ch = getch();
  }
  // refresh();
  delwin(choice_win);
  return choice;
}

struct Snake {
  deque<Pixel> body;
  int score = 0;
  int speed = 30;  // range is [0, FPS]
  int magnet_range = 4;
  int fast = 0, slow = 0, magnet = 0, shield = 0;
  bool alive = true;
  Direction direction = East;

  void reset() {
    body.clear();
    alive = true;
    fast = 0, slow = 0, score = 0, magnet = false, direction = East;
  }

  Pixel get_next_pixel() {
    Pixel front = body.front();
    if (direction == East) front.x++;
    if (direction == North) front.y--;
    if (direction == West) front.x--;
    if (direction == South) front.y++;
    return front;
  }

  void reverse() {
    std::reverse(body.begin(), body.end());
    // calculate new direction
    Pixel front = body.front();
    Pixel front2 = body[1];
    if (front2.x != front.x) direction = front2.x > front.x ? West : East;
    if (front2.y != front.y) direction = front2.y > front.y ? North : South;
  }

  bool is_in_body(Pixel pixel) {
    return find(body.begin(), body.end(), pixel) != body.end();
  }
};

struct Field {
  int width = 60, height = 20;
  int maxpower = 50;
  int num_powers = 4;
  Snake snake;
  set<Item> items;

  Field() { reset(); }

  void reset() {
    getmaxyx(stdscr, height, width);
    snake.reset();
    Pixel front = {width / 2, height / 2};
    snake.body = {
        {front.x, front.y}, {front.x, front.y + 1}, {front.x, front.y + 2}};
    items.clear();
  }

  void new_game() {
    reset();
    for (int i = 0; i < 3; i++) spawn_item();  // spawn 3 items
    gameloop();
  }

  void display_bar(int powerleft, int color_pair, int y, int &x) {
    Pixel front = snake.body.front();
    if (powerleft > 0) {
      attron(color_pair);  // black-yellow
      mvaddch(front.y, front.x, ' ');
      for (int i = 0; i < powerleft * width / maxpower / num_powers; i++, x++)
        mvaddch(height - 1, x, ' ');
      attroff(color_pair);
    }
  }

  void display() {
    getmaxyx(stdscr, height, width);
    clear();
    box(stdscr, 0, 0);

    // draw snake
    attron(COLOR_PAIR(52));  // black-green
    for (auto &part : snake.body) mvaddch(part.y, part.x, ' ');
    attroff(COLOR_PAIR(52));

    // draw items
    for (auto &item : items) mvaddch(item.pixel.y, item.pixel.x, item.type);

    // draw score text
    attron(COLOR_PAIR(2));  // green-black
    mvprintw(0, 0, "Score: %d", snake.score);
    attroff(COLOR_PAIR(2));

    // color snake head and display powerup countdown bar
    int bar_x = 0, bar_y = height - 1;
    display_bar(snake.fast, COLOR_PAIR(53), bar_y, bar_x);    // black-yellow
    display_bar(snake.slow, COLOR_PAIR(51), bar_y, bar_x);    // black-red
    display_bar(snake.magnet, COLOR_PAIR(56), bar_y, bar_x);  // black-cyan
    display_bar(snake.shield, COLOR_PAIR(57), bar_y, bar_x);  // black-white

    Pixel front = snake.body.front();
    if (snake.magnet > 0)
      // dim items around snake head within magnet range
      for (int i = 1; i < width - 1; i++)
        for (int j = 1; j < height - 1; j++)
          if (!is_offscreen(Pixel({i, j})))
            if (distance(front, Pixel({i, j})) <= snake.magnet_range)
              mvaddch(j, i, mvinch(j, i) | A_DIM);  // dim

    refresh();
  }

  int distance(Pixel a, Pixel b) { return abs(a.x - b.x) + abs(a.y - b.y); }

  bool is_offscreen(Pixel p) {
    return p.x <= 0 || p.x >= width - 1 || p.y <= 0 || p.y >= height - 1;
  }

  void add_head() {
    Pixel front = snake.get_next_pixel();
    // snake comes from opposite side
    if (is_offscreen(front)) {
      if (front.x == 0)
        front.x = width - 2;
      else if (front.x == width - 1)
        front.x = 1;
      else if (front.y == 0)
        front.y = height - 2;
      else if (front.y == height - 1)
        front.y = 1;
    }

    // snake dies if it touches itself (except for shield)
    if (!snake.shield && snake.is_in_body(front)) {
      snake.alive = false;
      choice_menu("Game Over", {"Your score was: " + to_string(snake.score)});
    }

    snake.body.push_front(front);  // add new head
  }

  void eat_food() {
    snake.score += 1;
    add_head();
    spawn_item();
  }

  void pickup_items() {
    Pixel front = snake.body.front();
    for (auto item = items.begin(); item != items.end();)
      if (item->pixel == front) {
        if (item->type == Fast) snake.fast += maxpower;
        if (item->type == Slow)
          if (!snake.shield) snake.slow += maxpower;
        if (item->type == Magnet) snake.magnet += maxpower;
        if (item->type == Shield) snake.shield += maxpower;
        if (item->type == Reverse) snake.reverse();
        if (item->type == Food) eat_food();
        item = items.erase(item);
      } else
        item++;
  }

  void handle_powerups() {
    auto front = snake.body.front();
    if (snake.magnet > 0) {
      for (auto item = items.begin(); item != items.end();)
        if (item->type == Food &&
            distance(item->pixel, front) <= snake.magnet_range) {
          auto temp = *item;
          item = items.erase(item);
          // move food closer to snake head
          if (temp.pixel.x != front.x)
            temp.pixel.x < front.x ? temp.pixel.x++ : temp.pixel.x--;
          if (temp.pixel.y != front.y)
            temp.pixel.y < front.y ? temp.pixel.y++ : temp.pixel.y--;

          if (is_offscreen(temp.pixel)) continue;
          if (snake.is_in_body(temp.pixel))
            eat_food();
          else
            items.insert(temp);
        } else
          item++;
    }
  }

  void countdown() {
    if (snake.magnet > 0) snake.magnet--;
    if (snake.fast > 0) snake.fast--;
    if (snake.slow > 0) snake.slow--;
    if (snake.shield > 0) snake.shield--;
  }

  void slither() {
    add_head();
    snake.body.pop_back();
  }

  Pixel find_empty_pixel() {
    while (true) {
      Pixel front = {rand() % (width - 2) + 1, rand() % (height - 2) + 1};
      if (mvinch(front.y, front.x) == ' ' && !snake.is_in_body(front))
        return front;
    }
  }

  void spawn_item() {
    items.insert({Food, find_empty_pixel()});

    ItemType item;
    int random = rand() % 10;
    if (random == 0)
      item = Slow;
    else if (random == 1)
      item = Fast;
    else if (random == 2)
      item = Magnet;
    else if (random == 3)
      item = Shield;
    else if (random == 4)
      item = Reverse;
    else if (random == 5)
      item = Food;
    else
      return;
    items.insert({item, find_empty_pixel()});
  }

  void pause_game() {
    int choice;
    choice = choice_menu("Game Paused", {"Continue", "New Game", "Quit"});
    if (choice == 0)
      ;  // continue game
    else if (choice == 1)
      new_game();
    else if (choice == 2) {
      snake.alive = false;
      choice_menu("Game Quitted",
                  {"Your score was: " + to_string(snake.score), "", ""});
    }
  }

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

  void wait_for_next_frame(chrono::nanoseconds diff) {
    auto us = chrono::duration_cast<chrono::microseconds>(diff).count();
    double us_per_frame = 1e6 / FPS;
    // slowdown vertical movement as the cell is a tall rectangle
    if (snake.direction == North || snake.direction == South)
      us_per_frame *= 1.91;
    if (us_per_frame > us) usleep(us_per_frame - us);
  }

  void gameloop() {
    int frame = 0;
    while (snake.alive) {
      auto start = chrono::high_resolution_clock::now();
      handle_events();
      // skip frames according to snake speed
      int speed = snake.speed;
      if (snake.fast > 0) speed *= 2;
      if (snake.slow > 0) speed /= 2;
      if (frame * speed % FPS == 0) {
        slither();
        handle_powerups();
        pickup_items();
      }
      if (frame * 10 % FPS == 0)  // every 0.1s
        countdown();
      display();
      auto end = chrono::high_resolution_clock::now();
      wait_for_next_frame(end - start);
      frame++;
      if (frame > FPS) frame = 0;
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

  mousemask(ALL_MOUSE_EVENTS, NULL);  // get all the mouse events
  timeout(100);

  nodelay(stdscr, TRUE);  // non-blocking getch
  start_color();
  init_color_pairs();
}

void close_ncurses() {
  delwin(stdscr);
  nocbreak();  // line buffering
  echo();      // echo keypresses
  endwin();
}

int main() {
  srand(time(0));
  atexit(close_ncurses);
  init_ncurses();

  Field game;

  // menu loop
  while (true) {
    int choice = choice_menu("Snake", {"New Game", "Quit"});
    if (choice == 0) {
      game.new_game();
    } else if (choice == 1)
      break;
  }
}