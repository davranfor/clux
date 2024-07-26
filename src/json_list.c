/*!
 *  \brief     clux - json and json-schema library for C
 *  \author    David Ranieri <davranfor@gmail.com>
 *  \copyright GNU Public License.
 */

#include <stdlib.h>
#include "clib_math.h"
#include "json_list.h"

#define MIN_SIZE 8

struct json_list
{
    const json **data;
    size_t room;
    size_t size;
};

json_list *json_list_create(size_t room)
{
    json_list *list = malloc(sizeof *list);

    if (list != NULL)
    {
        list->size = 0;
        list->room = room <= MIN_SIZE ? MIN_SIZE : next_size(room);
        list->data = malloc(sizeof *list->data * list->room);
        if (list->data == NULL)
        {
            free(list);
            list = NULL;
        }
    }
    return list;
}

int json_list_add(json_list *list, const json *data)
{
    if ((list != NULL) && (data != NULL))
    {
        if (list->size == list->room)
        {
            size_t room = list->room * 2;
            const json **temp = realloc(list->data, sizeof *temp * room);

            if (temp == NULL)
            {
                return 0;
            }
            list->data = temp;
            list->room = room;
        }
        list->data[list->size++] = data;
        return 1;
    }
    return 0;
}

const json *json_list_at(const json_list *list, size_t index)
{
    if ((list != NULL) && (index < list->size))
    {
        return list->data[index];
    }
    return NULL;
}

const json **json_list_data(const json_list *list)
{
    return list != NULL ? list->data : NULL;
}

size_t json_list_size(const json_list *list)
{
    return list != NULL ? list->size : 0;
}

// Function to merge two sorted subarrays in place
static void merge(const json **array, int top, int mid, int end,
    json_sort_callback callback)
{
    int tmp = mid + 1;

    // If the direct merge is already sorted
    if (callback(array[mid], array[tmp]) <= 0)
    {
        return;
    }
    // Two pointers to maintain top of both arrays to merge
    while ((top <= mid) && (tmp <= end))
    {
        // If element 1 is in right place
        if (callback(array[top], array[tmp]) <= 0)
        {
            top++;
        }
        else
        {
            const json *value = array[tmp];
            int index = tmp;

            // Shift all lements between element 1 and element 2 right by 1
            while (index != top)
            {
                array[index] = array[index - 1];
                index--;
            }
            array[top] = value;
            // Update all the pointers
            top++;
            mid++;
            tmp++;
        }
    }
}

// Iterative merge sort function to sort arr[0...n-1]
static void sort(const json **array, int size, json_sort_callback callback)
{
    // Merge subarrays in bottom-up manner
    for (int count = 1; count <= size - 1; count = count * 2)
    {
        // Pick starting point of different subarrays of current size
        for (int top = 0; top < size - 1; top += count * 2)
        {
            // Find ending point of left subarray
            int mid = top + count - 1;
            // Find ending point of right subarray
            int end = top + 2 * count - 1 < size - 1
                ? top + 2 * count - 1
                : size - 1;

            // Merge Subarrays arr[top...mid] & arr[mid+1...end]
            if (mid < end)
            {
                merge(array, top, mid, end, callback);
            }
        }
    }
}

void json_list_sort(json_list *list, json_sort_callback callback)
{
    if ((list != NULL) && (list->size > 1))
    {
        sort(list->data, (int)list->size, callback);
    }
}

void json_list_destroy(json_list *list)
{
    if (list != NULL)
    {
        free(list->data);
        free(list);
    }
}

