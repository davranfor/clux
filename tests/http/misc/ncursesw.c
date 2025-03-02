/* Compile

On debian:
sudo apt install libncurses5-dev libncursesw5-dev
gcc -Wpedantic -Wall -Wextra -o demo demo.c -DNCURSES_WIDECHAR=1 -lncursesw

To launch in an external window:
sudo apt install xterm
xterm -title "einfo [Alpha 0.1]" \
      -geometry 132x43+200+0 \
      -bg "silver" -b 1 -fa Monaco -fs 16 \
      -e 'export TERM=xterm-256color; ./demo' &

'b 1' means border to 1
 
On macos:
brew install ncurses
gcc -Wpedantic -Wall -Wextra -o demo demo.c -DNCURSES_WIDECHAR=1 -lncurses
or
gcc -Wpedantic -Wall -Wextra -o demo demo.c -DNCURSES_WIDECHAR=1 $(pkg-config --cflags --libs ncursesw)

To launch in an external window:
osascript -e 'tell application "Terminal"
    activate
    set win to do script "./demo; exit"
    set current settings of win to (first settings set whose name is "Basic")
    tell win
        set font size to 16
        set number of columns to 132
        set number of rows to 43
    end tell
end tell'

*/

#include <ncurses.h>
#include <locale.h>
#include <wchar.h>

#define CUSTOM_BLACK  8
#define CUSTOM_GRAY   9
#define CUSTOM_BEIGE 10
#define CUSTOM_WHITE 11

static wint_t key;

/**
 * Fills 'input' with user input at position 'y', 'x'.
 * 'input' must be a NUL terminated wide string with space enough
 * for at least 'max_length' + 1 wide characters.
 * 'box_length' = visible length, if < 'max_length' then the text scrolls.
 * Returns the length of the wide string or -1 on error.
 */
static int input_at(WINDOW *window, int y, int x, wchar_t *input,
    int box_length, int max_length)
{
    int length = 0, old_length = 0;
    int scroll = 0, old_scroll = 0;
    int offset = 0, old_offset = 0;
    wint_t wch;

    if ((box_length == 0) || (max_length == 0))
    {
        return -1;
    }
    mousemask(ALL_MOUSE_EVENTS, NULL); // Enables all mouse event tracking
    move(y, x);
    while (1)
    {
        get_wch(&wch);
        if (wch == '\n')
        {
            // Handle enter
            key = KEY_ENTER;
            break;
        }
        if (wch == '\t')
        {
            // Handle tab key
            key = KEY_NEXT;
            break;
        }
        if (wch == 27)
        {
            // Handle escape (break on pure escape, flush otherwise)
            nodelay(window, TRUE); // Non blocking mode
            if (getch() == ERR) // Unavailable input
            {
                nodelay(window, FALSE); // Blocking mode
                key = KEY_CANCEL;
                break;
            }
            nodelay(window, FALSE);
            flushinp(); // Flush all pending input
            continue; 
        }
        if ((wch == KEY_BACKSPACE) || (wch == '\b') || (wch == 127))
        {
            // Handle backspace
            if (scroll > 0)
            {
                scroll--;
                length--;
            }
            else if (offset > 0)
            {
                offset--;
                length--;
            }
        }
        else if (wch == KEY_DC)
        {
            // Handle delete key
            if (scroll + offset < length)
            {
                length--;
            }
        }
        else if (wch == KEY_IC)
        {
            // Handle insert key
            // Do nothing for the moment
        }
        else if ((wch == KEY_UP) || (wch == KEY_DOWN))
        {
            // Handle up and down arrow
            key = wch;
            break;
        }
        else if (wch == KEY_LEFT)
        {
            // Handle left arrow
            if (offset > 0)
            {
                offset--;
            }
            else if (scroll > 0)
            {
                scroll--;
            }
        }
        else if (wch == KEY_RIGHT)
        {
            // Handle right arrow
            if (scroll + offset < length)
            {
                if ((offset == box_length - 1) &&
                    (scroll + offset != length - 1))
                {
                    scroll++;
                }
                else
                {
                    offset++;
                }
            }
        }
        else if (wch == KEY_HOME)
        {
            // Handle home key
            scroll = 0;
            offset = 0;
        }
        else if (wch == KEY_END)
        {
            // Handle end key
            if ((length > box_length) && (length - box_length != scroll))
            {
                scroll = length - box_length;
            }
            offset = length - scroll;
        }
        else if ((wch == KEY_NPAGE) || (wch == KEY_PPAGE))
        {
            // Handle page up and page down
            key = wch;
            break;
        }
        else if ((wch >= KEY_F(1)) && (wch <= KEY_F(12)))
        {
            // Handle key F1 to key F12
            key = wch;
            break;
        }
        else if (((wch >= 32) && (wch <= 126)) ||
                ((wch >= 160) && (wch <= 255)) || (wch == 8364))
        {
            // Handle ASCII, Latin-1 Supplement characters and Euro sign
            if (length < max_length)
            {
                if (offset != box_length)
                {
                    offset++;
                }
                else
                {
                    scroll++;
                }
                length++;
            }
        }
        else if (wch == KEY_MOUSE)
        {
            MEVENT event;

            // Capture mouse events
            if (getmouse(&event) == OK)
            {
                if ((event.y == y) &&
                    (event.x >= x) && (event.x <= x + length - scroll))
                {
                    offset = event.x - x;
                }
            }
        }
        if ((length != old_length) ||
            (scroll != old_scroll) ||
            (offset != old_offset))
        {
            if (length != old_length)
            {
                if (length > old_length) // if inserting
                {
                    int i = length;

                    for (; i > scroll + offset - 1; i--)
                    {
                        input[i] = input[i - 1];
                    }
                    input[i] = wch;
                }
                else // if deleting
                {
                    for (int i = scroll + offset; i <= length; i++)
                    {
                        input[i] = input[i + 1];
                    }
                }
            }
            if ((length != old_length) || (scroll != old_scroll))
            {
                int n = length - scroll;

                if (n > box_length)
                {
                    n = box_length;
                }
                mvaddnwstr(y, x, &input[scroll], n);
                if (n < box_length)
                {
                    mvaddwstr(y, x + n, L" ");
                }
            }
            old_length = length;
            old_scroll = scroll;
            old_offset = offset;
            move(y, x + offset);
            refresh();
        }
    }
    mousemask(0, NULL); // Disables all mouse event tracking
    return length;
}

int main(void)
{
    setlocale(LC_ALL, "");  // Set locale to the user's default locale
    initscr();              // Initialize the window
    cbreak();               // Disable line buffering
    noecho();               // Don't echo input characters
    keypad(stdscr, TRUE);   // Enable special keys to be processed
    mouseinterval(0);       // Disables double-click detection
    set_escdelay(50);       // Set minimum delay in milliseconds for ESCAPE key
    start_color();          // Enable colors

    if (can_change_color()) // Check if the terminal supports custom colors
    {
        // Define custom colors
        init_color(CUSTOM_BLACK, 192, 192, 192);
        init_color(CUSTOM_GRAY, 664, 664, 664);
        init_color(CUSTOM_BEIGE, 776, 712, 512);
        init_color(CUSTOM_WHITE, 1000, 1000, 1000);
        // Set pair of custom colors
        init_pair(1, CUSTOM_WHITE, CUSTOM_BLACK); // White on Black background
        init_pair(2, CUSTOM_GRAY, CUSTOM_BLACK);  // Gray on Black background
        init_pair(3, CUSTOM_BEIGE, CUSTOM_BLACK); // Beige on Black background
    }
    else
    {
        init_pair(1, COLOR_WHITE, COLOR_BLACK); // White on Black background
        init_pair(2, COLOR_GREEN, COLOR_BLACK); // Green on Black background
        init_pair(3, COLOR_BLUE, COLOR_BLACK);  // Cyan on Black background
    }
    bkgd(COLOR_PAIR(1)); // Set the background
    clear(); // Clear screen with new background

    enum { box_length = 15, max_length = 25 };
    // Buffer to store input + 1 for null terminator
    wchar_t input[max_length + 1] = L"";

    box(stdscr, 0, 0); // Draw borders around form

    int height, width;

    (void)height;
    getmaxyx(stdscr, height, width); // Get the screen's height and width
    mvaddch(2, 0, ACS_LTEE); // Draw |-
    hline(ACS_HLINE, width - 2); // Draw line
    mvaddch(2, width - 1, ACS_RTEE); // Draw -|

    attron(COLOR_PAIR(2));
    mvprintw(1, 2, "Enter text (box: %d characters, max %d characters): [%*s]",
        box_length, max_length, box_length, "");
    attroff(COLOR_PAIR(2));
    refresh();

    int y, x;

    getyx(stdscr, y, x); // Retrieve the cursor's position

    int length = input_at(stdscr, y, x - box_length - 1, input,
        box_length, max_length);

    if (length != -1)
    {
        mvprintw(3, 2, "You entered '%ls' code '%s'", input, keyname((int)key));
    }
    attron(COLOR_PAIR(3));
    mvprintw(4, 2, "Press any key to continue");
    attroff(COLOR_PAIR(3));
    curs_set(0); // Hide cursor
    refresh();
    getch(); // Wait for another key press
    endwin(); // Restore normal terminal behavior
    return 0;
}

