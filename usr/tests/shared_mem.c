/*
 * Copyright (C) 2013, 2014 F4OS Authors
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is furnished to do
 * so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <stdio.h>
#include <string.h>
#include <kernel/sched.h>
#include <dev/char.h>
#include <dev/shared_mem.h>
#include "test.h"

#define IPC_MESSAGE "'Forty-two,' said Deep Thought, with infinite majesty and calm."

volatile int passed = 0;
struct char_device *shared_mem;

static void memreader(void) {
    char buf[100] = {'\0'};

    read(shared_mem, buf, 99);

    obj_put(&shared_mem->obj);

    if (strncmp(buf, IPC_MESSAGE, 100) == 0) {
        passed = 1;
    }
    else {
        passed = -1;
    }
}

static int shared_mem_test(char *message, int len) {
    shared_mem = shared_mem_create();
    if (!shared_mem) {
        strncpy(message, "Unable to open shared mem.", len);
        return FAILED;
    }

    /* Make a second reservation for the other task */
    obj_get(&shared_mem->obj);

    fputs(shared_mem, IPC_MESSAGE);
    new_task(&memreader, 1, 0);

    int count = 100000;
    while (!passed && (count-- > 0));

    if (passed == 1) {
        return PASSED;
    }
    else if (passed == -1) {
        strncpy(message, "Message mismatch", len);
    }
    else {
        strncpy(message, "Timed out", len);
    }

    return FAILED;
}
DEFINE_TEST("Shared memory resource", shared_mem_test);
