#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <ncurses.h>
#include <assert.h>

#include "cursor.h"
#include "common.h"

void cursor_malloc(struct cursor **cursor) {
  if (!(*cursor = malloc(sizeof(**cursor)))) {
    tty_solitaire_generic_error(errno, __FILE__, __LINE__);
  }
  (*cursor)->window = newwin(1, 1, CURSOR_BEGIN_Y, CURSOR_BEGIN_X);
}

void cursor_init(struct cursor *cursor) {
  mvwin(cursor->window, CURSOR_BEGIN_Y, CURSOR_BEGIN_X);
  cursor->y = CURSOR_BEGIN_Y;
  cursor->x = CURSOR_BEGIN_X;
  cursor->marked = false;
}

void cursor_free(struct cursor *cursor) {
  delwin(cursor->window);
  free(cursor);
}

void cursor_mark(struct cursor *cursor) {
  cursor->marked = true;
}

void cursor_unmark(struct cursor *cursor) {
  cursor->marked = false;
}

#define CURSOR_STEP_X 8
#define CURSOR_STEP_Y 7

static void cursor_move_left(struct cursor *cursor, int goal) {
  if (cursor->x > CURSOR_BEGIN_X) {
      while (cursor->x > goal) {
        cursor->x = cursor->x - CURSOR_STEP_X;
      }
      if (cursor->y > CURSOR_BEGIN_Y) {
        cursor_move(cursor, UP);
        cursor_move(cursor, DOWN);
      }
  }
}

static void cursor_move_right(struct cursor *cursor, int goal) {
  if (cursor->x < CURSOR_END_X) {
      while (cursor->x < goal) {
        cursor->x = cursor->x + CURSOR_STEP_X;
      }
      if (cursor->y > CURSOR_BEGIN_Y) {
        cursor_move(cursor, UP);
        cursor_move(cursor, DOWN);
      }
  }
}

void cursor_move(struct cursor *cursor, enum movement movement) {
  switch (movement) {
  case LEFT:
    cursor_move_left(cursor, cursor->x - CURSOR_STEP_X);
    break;
  case DOWN:
    if (cursor->y == CURSOR_BEGIN_Y) {
      int index = (cursor->x - CURSOR_BEGIN_X) / 8;
      cursor->y = cursor->y + CURSOR_STEP_Y + stack_length(deck->maneuvre[index]);
    }
    break;
  case RIGHT:
    cursor_move_right(cursor, cursor->x + CURSOR_STEP_X);
    break;
  case UP:
    if (cursor->y > CURSOR_BEGIN_Y) {
      cursor->y = CURSOR_BEGIN_Y;
    }
    break;
  case BEGIN:
    cursor_move_left(cursor, CURSOR_BEGIN_X);
    break;

  case END:
    cursor_move_right(cursor, CURSOR_END_X);
    break;
  }
}

enum movement cursor_direction(int key) {
  switch (key) {
  case 'h':
  case KEY_LEFT:
    return(LEFT);
  case 'j':
  case KEY_DOWN:
    return(DOWN);
  case 'k':
  case KEY_UP:
    return(UP);
  case 'l':
  case KEY_RIGHT:
    return(RIGHT);
  case '^':
    return(BEGIN);
  case '$':
    return(END);
  default:
    endwin();
    assert(false && "invalid cursor direction");
  }
}

struct stack **cursor_stack(struct cursor *cursor) {
  if (cursor->y == CURSOR_BEGIN_Y) {
    switch (cursor->x) {
    case CURSOR_STOCK_X:        return(&(deck->stock));
    case CURSOR_WASTE_PILE_X:   return(&(deck->waste_pile));
    case CURSOR_FOUNDATION_0_X: return(&(deck->foundation[0]));
    case CURSOR_FOUNDATION_1_X: return(&(deck->foundation[1]));
    case CURSOR_FOUNDATION_2_X: return(&(deck->foundation[2]));
    case CURSOR_FOUNDATION_3_X: return(&(deck->foundation[3]));
    case CURSOR_INVALID_SPOT_X: return(NULL);
    default:
      endwin();
      assert(false && "invalid stack");
    }
  } else {
    int index = (cursor->x - 4) / 8;
    return(&(deck->maneuvre[index]));
  }
}

bool cursor_on_stock(struct cursor *cursor) {
  return(cursor_stack(cursor) && *cursor_stack(cursor) == deck->stock);
}

bool cursor_on_invalid_spot(struct cursor *cursor) {
  return(!cursor_stack(cursor));
}
