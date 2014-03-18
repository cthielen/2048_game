#include <stdio.h>
#include <stdlib.h>
#include <curses.h>
#include <string.h>
#include <time.h>

#define max(a, b) ((a) > (b) ? (a) : (b))

#define NROWS 4
#define NCOLS NROWS

#ifndef uint8_t
#define uint8_t unsigned char
#endif

struct game_t {
	int turns, score, max_tile;
	uint8_t board[NROWS][NCOLS];
};

int place_tile(struct game_t *game)
{
	uint8_t *lboard = (uint8_t *)game->board;
	int i, num_zeros = 0;
	for (i = 0; i < NROWS * NCOLS; i++) {
		num_zeros += lboard[i] ? 0 : 1;
	}

	if (!num_zeros) {
		return -1;
	}

	int loc = random() % num_zeros;
	uint8_t new_val = random() % 10 ? 1 : 2;

	for (i = 0; i < NROWS * NCOLS; i++) {
		if (!lboard[i]) {
			if (!loc) {
				lboard[i] = new_val;
				return 0;
			}
			loc--;
		}
	}
	return 0;
}

void print_tile(int tile)
{
	if (tile) {
		if (tile < 6)
			attron(A_BOLD);
		int pair = COLOR_PAIR(1 + (tile % 6));
		attron(pair);
		printw("%4d", 1 << tile);
		attroff(pair);
		attroff(A_BOLD);
	}
	else {
		printw("   .");
	}
}

void print_game(struct game_t *game)
{
	int r, c;
	move(0, 0);
	printw("Score: %6d  Turns: %4d", game->score, game->turns);
	for (r = 0; r < NROWS; r++) {
		for (c = 0; c < NCOLS; c++) {
			move(r + 2, 5 * c);
			print_tile(game->board[r][c]);
		}
	}

	refresh();
}

int combine_left(struct game_t *game, uint8_t row[NCOLS])
{
	int c, did_combine = 0;
	for (c = 1; c < NCOLS; c++) {
		if (row[c] && row[c-1] == row[c]) {
			row[c-1]++;
			row[c] = 0;
			game->score += 1 << (row[c-1] - 1);
			did_combine = 1;
			game->max_tile = max(game->max_tile, row[c-1]);
		}
	}
	return did_combine;
}

int deflate_left(uint8_t row[NCOLS])
{
	uint8_t buf[NCOLS] = {0};
	uint8_t *out = buf;
	int did_compress = 0;

	int in;
	for (in = 0; in < NCOLS; in++) {
		if (row[in] != 0) {
			*(out++) = row[in];
			if (buf[in] != row[in])
				did_compress = 1;
		}
	}

	memcpy(row, buf, sizeof(buf));
	return did_compress;
}

void rotate_clockwise(struct game_t *game)
{
	uint8_t buf[NROWS][NCOLS];
	memcpy(buf, game->board, sizeof(game->board));

	int r, c;
	for (r = 0; r < NROWS; r++) {
		for (c = 0; c < NCOLS; c++) {
			game->board[r][c] = buf[NCOLS - c - 1][r];
		}
	}
}

void move_left(struct game_t *game)
{
	int r, ret = 0;
	for (r = 0; r < NROWS; r++) {
		uint8_t *row = &game->board[r][0];
		ret |= deflate_left(row);
		ret |= combine_left(game, row);
		ret |= deflate_left(row);
	}

	game->turns += ret;
}

void move_right(struct game_t *game)
{
	rotate_clockwise(game);
	rotate_clockwise(game);
	move_left(game);
	rotate_clockwise(game);
	rotate_clockwise(game);
}

void move_up(struct game_t *game)
{
	rotate_clockwise(game);
	rotate_clockwise(game);
	rotate_clockwise(game);
	move_left(game);
	rotate_clockwise(game);
}

void move_down(struct game_t *game)
{
	rotate_clockwise(game);
	move_left(game);
	rotate_clockwise(game);
	rotate_clockwise(game);
	rotate_clockwise(game);
}

int lose_game(struct game_t test_game)
{
	int start_turns = test_game.turns;
	if (0 < place_tile(&test_game)) {
		return 1;
	}

	move_left(&test_game);
	move_up(&test_game);
	move_down(&test_game);
	move_right(&test_game);
	return test_game.turns == start_turns;
}

void init_curses()
{
	initscr(); // curses init
	start_color();
	cbreak(); // curses don't wait for enter key
	noecho(); // curses don't echo the pressed key
	clear(); // curses clear screen and send cursor to (0,0)
	refresh();
	curs_set(0);

	init_pair(1, COLOR_RED, 0);
	init_pair(2, COLOR_GREEN, 0);
	init_pair(3, COLOR_YELLOW, 0);
	init_pair(4, COLOR_BLUE, 0);
	init_pair(5, COLOR_MAGENTA, 0);
	init_pair(6, COLOR_CYAN, 0);
}

int main()
{
	init_curses();

	const char *exit_msg = "";
	srand(time(NULL));

	struct game_t game = {0};
	int last_turn = game.turns;

	place_tile(&game);
	place_tile(&game);

	while (1) {
		print_game(&game);

		if (lose_game(game)) {
			exit_msg = "You lose!";
			goto lose;
		}

		last_turn = game.turns;

		switch (getch()) {
		case 'h': move_left(&game); break;
		case 'j': move_down(&game); break;
		case 'k': move_up(&game); break;
		case 'l': move_right(&game); break;
		case 'q':
			exit_msg = "Quitter!";
			goto end;
		}

		if (last_turn != game.turns)
			place_tile(&game);
	}

lose:
	move(7, 0);
	printw("You lose. Press q to quit.");
	while (getch() != 'q');
end:
	endwin();
	printf("%s %d points in %d turns, with largest tile %d\n",
		exit_msg, game.score, game.turns, 1 << game.max_tile);
	return 0;
}
