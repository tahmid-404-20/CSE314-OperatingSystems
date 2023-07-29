#include <chrono>
#include <iostream>
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <vector>
#include "random.h"

#define NPRINTERS 4
#define NBINDERS 2
#define NSTAFFS 2

#define MAX_WRITING_TIME 4

int M, N;
int printing_time, binding_time, read_write_time;
int n_submissions = 0;

enum printer_state { FREE, OCCUPIED };

class Printer {
public:
  int id;
  printer_state state;

  Printer(int id) {
    this->id = id;
    this->state = FREE;
  }
};

enum student_state {
  WRITING_REPORT,
  WAITING_FOR_PRINTING,
  PRINTING,
  PRINTING_DONE,
  WAITING_FOR_BINDING,
  BINDING
};

class Student {
public:
  int id;
  int group_id;
  int printer_id;
  int writing_time;
  student_state state;

  sem_t lock; // binary semaphore

  Student(int id) {
    this->id = id;
    this->group_id = (this->id - 1) / M + 1;
    this->printer_id = this->id % NPRINTERS + 1;
    this->state = WRITING_REPORT;
    this->writing_time = get_random_number() % MAX_WRITING_TIME + 1;

    sem_init(&lock, 0, 0);
  }

  bool is_leader() { return this->id % M == 0; }

  int get_group_start_id() { return (this->group_id - 1) * M + 1; }
  int get_group_end_id() { return get_group_start_id() + M - 1; }
};

extern void enable_binding(int);

class Group {
public:
  int id;
  int leader_id;
  int printed_count;

  Group(int id) {
    this->id = id;
    this->leader_id = (this->id - 1) * M + M;
    this->printed_count = 0;
  }

  void add_printed_count() {
    this->printed_count++;
    if (this->printed_count == M) {
      enable_binding(this->leader_id);
    }
  }
};