#include <stdlib.h>
#include <ncurses.h>
#include <assert.h>

#include "keyboard.h"
#include "card.h"
#include "stack.h"
#include "game.h"
#include "cursor.h"
#include "gui.h"
#include "common.h"

static void handle_term_resize() {
  clear();
  refresh();
  if (term_size_ok()) {
    draw_deck(deck);
    draw_cursor(cursor);
  } else {
    mvprintw(1, 1, SMALL_TERM_MSG);
  }
}

/* FIXME: this function does not work on stacks with no marked cards.
 * In that case it returns the stack's length. */
static int marked_cards_count(struct stack *stack) {
  if (stack_length(stack) == 1) {
    if (stack->card->frame->begin_y > MANEUVRE_BEGIN_Y) {
      return(1);
    }
  } else if (stack_length(stack) > 1) {
    for (int marked_cards_count = 0; stack; stack = stack->next) {
      marked_cards_count++;
      if (!stack->next || (stack->card->frame->begin_y - stack->next->card->frame->begin_y) > 1) {
        return(marked_cards_count);
      }
    }
  }
  return(0);
}

static void unmark_cards(struct stack *stack) {
  int _marked_cards_count = marked_cards_count(stack);
  for (int i = 0; i < _marked_cards_count; stack = stack->next, i++) {
    card_unmark(stack->card);
  }
}

static void handle_card_movement(struct cursor *cursor) {
  struct stack **origin = cursor_stack(cursor);
  int key;

  if (cursor_on_invalid_spot(cursor) || stack_empty(*origin)) {
    return;
  }
  if (maneuvre_stack(*origin)) {
    erase_stack(*origin);
    card_mark((*origin)->card);
    draw_stack(*origin);
    cursor->y++;
  }
  erase_cursor(cursor);
  cursor_mark(cursor);
  draw_cursor(cursor);

  int step_factor = 1;

  for (;;) {
    if ((key = getch()) == 'q' || key == 'Q') {
      endwin();
      game_end();
      exit(0);
    }
    if (term_size_ok()) {
      switch (key) {
      case '1': step_factor = 1; break;
      case '2': step_factor = 2; break;
      case '3': step_factor = 3; break;
      case '4': step_factor = 4; break;
      case '5': step_factor = 5; break;
      case '6': step_factor = 6; break;
      case '7': step_factor = 7; break;
      case '8': step_factor = 8; break;
      case '9': step_factor = 9; break;
      case '0': step_factor = 10; break;

      case 'h':
      case 'j':
      case 'k':
      case 'l':
      case KEY_LEFT:
      case KEY_DOWN:
      case KEY_UP:
      case KEY_RIGHT:
      case '^':
      case '$':
        erase_cursor(cursor);
        cursor_move(cursor, cursor_direction(key), step_factor);
        draw_cursor(cursor);
        step_factor = 1;
        break;
      case 'm':
        if (origin == cursor_stack(cursor) && maneuvre_stack(*origin)) {
          for (struct stack *i = *origin; i && i->next; i = i->next) {
            if (i->next->card->face == EXPOSED &&
                  (i->card->frame->begin_y - i->next->card->frame->begin_y) > 1) {
              erase_stack(*origin);
              card_mark(i->next->card);
              draw_stack(*origin);
              break;
            }
          }
        }
        step_factor = 1;
        break;
      case 'M':
        if (origin == cursor_stack(cursor) && maneuvre_stack(*origin)) {
          for (struct stack *i = *origin; i && i->next; i = i->next) {
            if (i->next->card->face == EXPOSED &&
                  (i->card->frame->begin_y - i->next->card->frame->begin_y) > 1) {
              erase_stack(*origin);
              card_mark(i->next->card);
              draw_stack(*origin);
            }
          }
        }
        step_factor = 1;
        break;
      case 'n':
        if (origin == cursor_stack(cursor) && maneuvre_stack(*origin)) {
          for (struct stack *i = (*origin)->next; i; i = i->next) {
            if (i->next) {
              if ((i->card->frame->begin_y - i->next->card->frame->begin_y) > 1) {
                erase_stack(*origin);
                card_unmark(i->card);
                draw_stack(*origin);
                break;
              }
            } else {
              if (i->card->frame->begin_y == (MANEUVRE_BEGIN_Y + 1)) {
                erase_stack(*origin);
                card_unmark(i->card);
                draw_stack(*origin);
                break;
              }
            }
          }
        }
        step_factor = 1;
        break;
      case 'N':
        if (origin == cursor_stack(cursor) && maneuvre_stack(*origin)) {
          erase_stack(*origin);
          unmark_cards(*origin);
          card_mark((*origin)->card);
          draw_stack(*origin);
        }
        step_factor = 1;
        break;

      case KEY_SPACEBAR:;
        /* http://www.mail-archive.com/gcc-bugs@gcc.gnu.org/msg259382.html */
        struct stack **destination = cursor_stack(cursor);
        int _marked_cards_count = marked_cards_count(*origin);
        if (maneuvre_stack(*origin) && _marked_cards_count > 0) {
          erase_stack(*origin);
          unmark_cards(*origin);
          draw_stack(*origin);
        }
        if (destination) {
          erase_stack(*origin);
          erase_cursor(cursor);
          if (_marked_cards_count > 1 &&
                maneuvre_stack(*origin) &&
                maneuvre_stack(*destination)) {
            struct stack *block = *origin;
            for (int i = 1; i < _marked_cards_count; block = block->next, i++)
              ;
            if (valid_move(block, *destination)) {
              move_block(origin, destination, _marked_cards_count);
            }
          } else {
            if (valid_move(*origin, *destination)) {
              if (maneuvre_stack(*destination)) {
                cursor->y++;
              }
              move_card(origin, destination);
            }
          }
          draw_stack(*origin);
          draw_stack(*destination);
          if (maneuvre_stack(*origin) && *origin == *destination) {
            erase_cursor(cursor);
            cursor->y--;
          }
        }
        cursor_unmark(cursor);
        draw_cursor(cursor);
        step_factor = 1;
        return;

      case KEY_ESCAPE:
        if (cursor_stack(cursor) == origin && maneuvre_stack(*origin)) {
          erase_cursor(cursor);
          cursor->y--;
        }
        if (marked_cards_count(*origin) > 0 && maneuvre_stack(*origin)) {
          erase_stack(*origin);
          unmark_cards(*origin);
          draw_stack(*origin);
        }
        if (cursor->marked) {
          cursor_unmark(cursor);
          draw_cursor(cursor);
        }
        step_factor = 1;
        return;

      case KEY_RESIZE:
        handle_term_resize();
        step_factor = 1;
        break;

      case 'q':
      case 'Q':
        endwin();
        game_end();
        exit(0);

      default:
        step_factor = 1;
        break;
      }
    } else if (key == KEY_RESIZE) {
      handle_term_resize();
      step_factor = 1;
    }
  }
}

void keyboard_event(int key) {
  if (key == 'q' || key == 'Q') {
    endwin();
    game_end();
    exit(0);
  }

  static int step_factor = 1;

  if (term_size_ok()) {
    switch (key) {
    case '1': step_factor = 1; break;
    case '2': step_factor = 2; break;
    case '3': step_factor = 3; break;
    case '4': step_factor = 4; break;
    case '5': step_factor = 5; break;
    case '6': step_factor = 6; break;
    case '7': step_factor = 7; break;
    case '8': step_factor = 8; break;
    case '9': step_factor = 9; break;
    case '0': step_factor = 10; break;

    case 'h':
    case 'j':
    case 'k':
    case 'l':
    case KEY_LEFT:
    case KEY_DOWN:
    case KEY_UP:
    case KEY_RIGHT:
    case '^':
    case '$':
      erase_cursor(cursor);
      cursor_move(cursor, cursor_direction(key), step_factor);
      draw_cursor(cursor);
      step_factor = 1;
      break;

    case KEY_SPACEBAR:
      if (cursor_on_stock(cursor)) {
        if (stack_empty(deck->stock)) {
          if (game.passes_through_deck_left >= 1 || game.passes_through_deck_left < 0) {
            while (!stack_empty(deck->waste_pile)) {
              move_card(&(deck->waste_pile), &(deck->stock));
              card_cover(deck->stock->card);
            }
            draw_stack(deck->stock);
            draw_stack(deck->waste_pile);
          }
        } else {
          move_card(&(deck->stock), &(deck->waste_pile));
          if (stack_empty(deck->stock) && game.passes_through_deck_left > 0) {
            game.passes_through_deck_left--;
          }
          card_expose(deck->waste_pile->card);
          erase_stack(deck->waste_pile);
          draw_stack(deck->stock);
          draw_stack(deck->waste_pile);
        }
      } else {
        struct card *card;
        if (cursor_stack(cursor) &&
              (card = (*cursor_stack(cursor))->card)->face == COVERED) {
          card_expose(card);
          draw_card(card);
        } else {
          handle_card_movement(cursor);
        }
      }
      step_factor = 1;
      break;

    case KEY_RESIZE:
      handle_term_resize();
      step_factor = 1;
      break;

    default:
      step_factor = 1;
      break;
    }
  } else if (key == KEY_RESIZE) {
    handle_term_resize();
    step_factor = 1;
  }
}
