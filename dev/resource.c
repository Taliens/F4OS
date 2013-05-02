#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mm/mm.h>
#include <kernel/sched.h>
#include <kernel/semaphore.h>
#include <kernel/fault.h>

#include <dev/resource.h>

/* Get resource struct from rd */
static inline resource *get_resource(rd_t rd) {
    if (rd < 0 || rd >= RESOURCE_TABLE_SIZE) {
        return NULL;
    }

    /* Before task switching begins, the default list of resources must be used.
     * Once it begins, we can get the resouece directly from the task. */
    if (task_switching) {
        return curr_task->task->resources[rd];
    }
    else {
        return default_resources[rd];
    }
}

resource *create_new_resource(void) {
    resource *ret = kmalloc(sizeof(resource));
    if (ret) {
        memset(ret, 0, sizeof(resource));
    }
    return ret;
}

/* Add new resource to task */
rd_t add_resource(task_ctrl* tcs, resource* r) {
    if (r == NULL) {
        printk("OOPS: NULL resource passed to add_resource.\r\n");
        return -1;
    }

    /* There is room at the end of the resource table, so add there */
    if (tcs->top_rd < RESOURCE_TABLE_SIZE) {
        rd_t rd = tcs->top_rd;

        tcs->resources[rd] = r;
        tcs->top_rd++;

        return rd;
    }
    /* There is no room at end of resource table, so search for space */
    else {
        /* We only search up to tcs->top_rd.  Since we are here, that
         * should be the end of the list, but just to be safe, we want
         * to make sure we don't find a space after tcs->top_rd and then
         * fail to increment tcs->top_rd */
        for (int i = 0; i < tcs->top_rd; i++) {
            /* Found an empty space! */
            if (tcs->resources[i] == NULL) {
                rd_t rd = i;

                tcs->resources[rd] = r;

                return rd;
            }
        }

        /* If we got here, nothing was found. */
        printk("OOPS: No room to add resources.\r\n");
        return -1;
    }
}

/* Properly set up resources in a new task.
 * This means copying the resources from the current task,
 * or the default resource list, if task switching has not
 * yet begun. */
void resource_setup(task_ctrl* task) {
    if (task_switching) {
        for (int i = 0; i < RESOURCE_TABLE_SIZE; i++) {
            task->resources[i] = curr_task->task->resources[i];
        }

        task->top_rd = curr_task->task->top_rd;
    }
    else {
        int top_rd = 0;

        for (int i = 0; i < RESOURCE_TABLE_SIZE; i++) {
            if (default_resources[i]) {
                task->resources[i] = default_resources[i];
                top_rd++;
            }
            else {
                task->resources[i] = NULL;
            }
        }

        task->top_rd = top_rd;
    }
}

/* Return bytes written, negative on error */
int write(rd_t rd, char* d, int n) {
    resource *resource = get_resource(rd);
    if (!resource) {
        return -1;
    }

    int tot = 0;

    acquire(resource->write_sem);

    for(int i = 0; i < n; i++) {
        int ret = resource->writer(d[i], resource->env);
        if (ret > 0) {
            tot += ret;
        }
        else {
            /* Return on error */
            tot = ret;
            break;
        }
    }

    release(resource->write_sem);

    return tot;
}

/* Return bytes written, negative on error */
int swrite(rd_t rd, char* s) {
    resource *resource = get_resource(rd);
    if (!resource) {
        return -1;
    }

    int ret = 0;

    acquire(resource->write_sem);

    if (resource->swriter) {
        ret = resource->swriter(s, resource->env);
    }
    else {
        while(*s) {
            int n = resource->writer(*s++, resource->env);
            if (n >= 0) {
                ret += n;
            }
            else {
                ret = n;
                break;
            }
        }
    }

    release(resource->write_sem);

    return ret;
}

/* Returns 0 on success, else on error */
int close(rd_t rd) {
    if (rd < 0 || rd >= RESOURCE_TABLE_SIZE) {
        return -1;
    }

    resource **resource = task_switching ? &curr_task->task->resources[rd] : &default_resources[rd];

    if (!*resource) {
        return -1;
    }

    int ret = (*resource)->closer(*resource);
    if (!ret) {
        kfree(*resource);
        *resource = NULL;
        if (task_switching && (rd == curr_task->task->top_rd - 1)) {
            curr_task->task->top_rd--;
        }
    }

    return ret;
}

/* Returns number of bytes read, or negative on error */
int read(rd_t rd, char *buf, int n) {
    resource *resource = get_resource(rd);
    if (!resource) {
        return -1;
    }

    int tot = 0;

    acquire(resource->read_sem);

    for(int i = 0; i < n; i++) {
        int error;

        buf[i] = resource->reader(resource->env, &error);

        if (!error) {
            tot += 1;
        }
        else {
            tot = error;
            break;
        }
    }

    release(resource->read_sem);

    return tot;
}
