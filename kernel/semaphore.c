#include <stdint.h>
#include <stddef.h>
#include <dev/registers.h>
#include <kernel/sched.h>
#include <kernel/fault.h>

#include <kernel/semaphore.h>

static void held_semaphores_insert(struct semaphore *list[], volatile struct semaphore *semaphore) __attribute__((section(".kernel")));
void held_semaphores_remove(struct semaphore *list[], volatile struct semaphore *semaphore) __attribute__((section(".kernel")));
static void deadlock_check(struct task_ctrl *task) __attribute__((section(".kernel")));

void acquire(volatile struct semaphore *semaphore) {
    if (!task_switching) {
        semaphore->lock = 1;
        semaphore->held_by = (task_ctrl *) 0x0badf00d;
        return;
    }

    int success = 0;

    while (!success) {
        success = SVC_ARG(SVC_ACQUIRE, (void *) semaphore);
    }
}

/* Acquire semaphore, but remove from held semaphores list so that it can be freed. */
void acquire_for_free(volatile struct semaphore *semaphore) {
    acquire(semaphore);
    held_semaphores_remove(curr_task->held_semaphores, semaphore);
}

int8_t try_lock(volatile uint8_t *l) {
    uint8_t taken = 1;
    uint8_t ret;
    uint8_t tmp = 0;
    __asm__("\
        ldrexb      %[tmp], [%[addr]]             \r\n\
        cmp         %[tmp], #0                    \r\n\
        ITT         EQ                            \r\n\
        strexbeq    %[tmp], %[taken], [%[addr]]   \r\n\
        cmpeq       %[tmp], #0                    \r\n\
        ITE         EQ                            \r\n\
        moveq       %[ret], #1                    \r\n\
        movne       %[ret], #0\
        "
        :[ret] "=l" (ret)
        :[addr] "l" (l), [tmp] "l" (tmp), [taken] "l" (taken)
        :"cc", "memory");
    return ret;
}


/* Called by svc_handler */
int get_lock(volatile struct semaphore *semaphore) {
    if (try_lock(&semaphore->lock)) {
        semaphore->held_by = curr_task;
        held_semaphores_insert(curr_task->held_semaphores, semaphore);
        curr_task->waiting = NULL;
        return 1;
    }
    else {
        if (semaphore->held_by != NULL) {
            deadlock_check(semaphore->held_by);

            /* Add to waitlist if higher priority */
            if (semaphore->waiting) {
                if (semaphore->waiting->priority < curr_task->priority) {
                    semaphore->waiting = curr_task;
                    curr_task->waiting = (struct semaphore *) semaphore;
                }
            }
            else {
                semaphore->waiting = curr_task;
                curr_task->waiting = (struct semaphore *) semaphore;
            }

            return 0;
        }
        else {
            panic_print("Semaphore (0x%x) not available, but held_by unset.", semaphore);
        }
    }

    return 0;
}

void release(volatile struct semaphore *semaphore) {
    if (!semaphore->lock) { /* WTF, don't release an unlocked semaphore */
        semaphore->held_by = NULL;
        semaphore->waiting = NULL;
        return;
    }

    if (!task_switching) {
        semaphore->lock = 0;
        semaphore->held_by = NULL;
        return;
    }

    SVC_ARG(SVC_RELEASE, (void *) semaphore);
}

static void held_semaphores_insert(struct semaphore *list[], volatile struct semaphore *semaphore) {
    for (int i = 0; i < HELD_SEMAPHORES_MAX; i++) {
        if (list[i] == NULL) {
            list[i] = (struct semaphore *) semaphore;
            return;
        }
    }

    panic_print("Too many semaphores already held in list (0x%x).", list);
}

void held_semaphores_remove(struct semaphore *list[], volatile struct semaphore *semaphore) {
    for (int i = 0; i < HELD_SEMAPHORES_MAX; i++) {
        if (list[i] == semaphore) {
            list[i] = NULL;
            return;
        }
    }
    /* Not found, but this may be fine, as kernel_task frees semaphores on behalf of the deceased */
}

static void deadlock_check(struct task_ctrl *task) {
    if (task->waiting) {
        for (int i = 0; i < HELD_SEMAPHORES_MAX; i++) {
            if (curr_task->held_semaphores[i] == task->waiting) {
                panic_print("Deadlock!  Task (0x%x, fptr: 0x%x) is waiting on semaphore 0x%x, but curr_task (0x%x, fptr: 0x%x) holds it.", task, task->fptr, task->waiting, curr_task, curr_task->fptr);
            }
        }
    }
}
