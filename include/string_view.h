#ifndef   STRING_VIEW_H_
#define   STRING_VIEW_H_

#include <stddef.h>
#include <stdio.h>

typedef struct {
    char  *items;
    size_t len;
} String_View;

String_View sv_chop_left (String_View *sv, char c);
String_View sv_chop_right(String_View *sv, char c);
String_View sv_get_line(String_View sv, size_t line);

#endif // STRING_VIEW_H_

#ifdef STRING_VIEW_IMPLEMENTATION

String_View sv_get_line(String_View sv, size_t line)
{
    size_t line_left  = 1;
    size_t count_left = 0;
    while (line_left < line) {
        while (count_left < sv.len) {
            if (sv.items[count_left] == '\n') {
                count_left++; // chop the delimiter
                break;
            }
            count_left++;
        }
        line_left++;
    }

    size_t line_size = 0;
    while (count_left + line_size < sv.len) {
        if (sv.items[count_left + line_size] == '\n') {
            // dont want the new line
            break;
        }
        line_size++; // add the delimiter
    }

    return (String_View) {
        .items = sv.items + count_left,
        .len   = line_size,
    };
}

String_View sv_chop_left(String_View *sv, char c)
{
    size_t count = 0;
    while (count < sv->len) {
        if (sv->items[count] == c) {
            count++; // chop the delimiter
            break;
        }
        count++;
    }

    return (String_View) {
        .items = sv->items + count,
        .len   = sv->len - count
    };
}
String_View sv_chop_right(String_View *sv, char c)
{
    size_t count = sv->len - 1;
    while (count >= 0) {
        if (sv->items[count] == c) {
            break;
        }
        count--;
    }

    return (String_View) {
        .items = sv->items,
        .len   = count
    };
}

#endif // STRING_VIEW_IMPLEMENTATION 

