#include "ScienceProject.h"
#include "time.h"
#include <chrono>
#include <iostream>
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <vector>
using namespace std;

vector<Student> students;
vector<Printer> printers;
vector<Group> groups;

// locks
pthread_mutex_t
    printing_lock; // a mutex lock which deals with printing, changes the state
                   // of printers, students (shared resource)
sem_t binding_lock;

pthread_mutex_t submit_mutex;
pthread_mutex_t read_write_lock;
int rc = 0;

void initialize() {

  // initialize students
  for (int i = 1; i <= N; i++) {
    students.emplace_back(Student(i));
  }

  // initialize groups
  for (int i = 1; i <= N / M; i++) {
    groups.emplace_back(Group(i));
  }

  // initialize printers
  for (int i = 1; i <= NPRINTERS; i++) {
    printers.emplace_back(Printer(i));
  }

  // initialize locks
  pthread_mutex_init(&printing_lock, NULL);
  sem_init(&binding_lock, 0, NBINDERS);
  pthread_mutex_init(&submit_mutex, NULL);
  pthread_mutex_init(&read_write_lock, NULL);

  // initialize tine
  start_time = std::chrono::high_resolution_clock::now();
}

// all the printing procedures
void provoke_to_print(int student_id) {
  int student_index = student_id - 1;

  Student *student = &students[student_index];
  Printer *printer = &printers[student->printer_id - 1];

  if (student->state == WAITING_FOR_PRINTING && printer->state == FREE) {
    student->state = PRINTING;
    printer->state = OCCUPIED;
    cout << "Student " << student_id << " has started printing at time "
         << get_time() << endl;
    sem_post(&student->lock);
  }
}

void provoke_groupmates(Student *student) {
  int group_start_std_id = student->get_group_start_id();
  int group_end_std_id = student->get_group_end_id();

  for (int id = group_start_std_id; id <= group_end_std_id; id++) {
    if (id == student->id)
      continue; // skip the student itself

    Student *groupmate = &students[id - 1];
    if (groupmate->printer_id != student->printer_id) {
      continue; // skip the groupmate if he/she is using another printer
    }
    provoke_to_print(groupmate->id);
  }
}

void provoke_other_group_persons(Student *student) {
  int group_start_std_id = student->get_group_start_id();
  int group_end_std_id = student->get_group_end_id();

  for (int id = 1; id <= N; id++) {
    if (id >= group_start_std_id && id <= group_end_std_id)
      continue; // skip the group itself

    Student *other_group_person = &students[id - 1];
    if (other_group_person->printer_id != student->printer_id) {
      continue; // skip the other-person if he/she is using another printer
    }
    provoke_to_print(other_group_person->id);
  }
}

void start_printing(Student *student) {
  int student_id = student->id;

  sleep(student->writing_time);

  pthread_mutex_lock(&printing_lock);
  student->state = WAITING_FOR_PRINTING;
  cout << "Student " << student_id
       << " has arrived at the print station at time " << get_time() << endl;
  provoke_to_print(student_id);
  pthread_mutex_unlock(&printing_lock);

  sem_wait(&student->lock);
}

void end_printing(Student *student) {
  int student_id = student->id;
  int student_index = student_id - 1;

  pthread_mutex_lock(&printing_lock);
  Printer *printer = &printers[student->printer_id - 1];

  printer->state = FREE;
  student->state = PRINTING_DONE;

  cout << "Student " << student_id << " has finished printing at time "
       << get_time() << endl;

  groups[student->group_id - 1].add_printed_count();
  provoke_groupmates(student);
  provoke_other_group_persons(student);
  pthread_mutex_unlock(&printing_lock);
}

void enable_binding(int student_id) {
  sem_post(&(students[student_id - 1].lock));
}

void *student_activities(void *arg) {
  Student *student = (Student *)arg;

  start_printing(student);
  sleep(printing_time);
  end_printing(student);

  if (student->is_leader()) {
    cout << "Student " << student->id << " leader of group "
         << student->group_id
         << " is waiting for others to complete printing at time " << get_time()
         << endl;

    sem_wait(&student->lock); // waiting for others to complete printing

    cout << "Group " << student->group_id << " has finished printing at time "
         << get_time() << endl;

    // binding code
    sem_wait(&binding_lock);
    cout << "Group " << student->group_id << " has started binding at time "
         << get_time() << endl;
    sleep(binding_time);
    cout << "Group " << student->group_id << " has finished binding at time "
         << get_time() << endl;
    sem_post(&binding_lock);

    // submission_code
    pthread_mutex_lock(&read_write_lock);
    cout << "Group " << student->group_id << " has started submitting at time "
         << get_time() << endl;
    sleep(read_write_time);
    n_submissions++;
    cout << "Group " << student->group_id << " has finished submitting at time "
         << get_time() << endl;
    pthread_mutex_unlock(&read_write_lock);
  }
}

void *staff_activities(void *arg) {
  int staff_id = *(int *)arg;

  int starting_time = rand() % 3 + 1;    // 1-3 secs
  int reading_interval = rand() % 2 + 1; // 1-2 secs

  sleep(starting_time);

  while (true) {
    pthread_mutex_lock(&submit_mutex);
    rc++;
    // first reader
    if (rc == 1) {
      pthread_mutex_lock(&read_write_lock);
    }
    cout << "Staff " << staff_id << " has started reading at time "
         << get_time() << ". No. of submission = " << n_submissions << endl;
    pthread_mutex_unlock(&submit_mutex);

    sleep(read_write_time);

    pthread_mutex_lock(&submit_mutex);
    cout << "Staff " << staff_id << " has finished reading at time "
         << get_time() << ". No. of submission = " << n_submissions << endl;
    rc--;
    // all readers gone
    if (rc == 0) {
      pthread_mutex_unlock(&read_write_lock);
    }
    pthread_mutex_unlock(&submit_mutex);

    sleep(reading_interval);
  }
}

int main() {

  N = 15;
  M = 5;
  printing_time = 10;
  binding_time = 8;
  read_write_time = 3;

  pthread_t student_threads[N];

  int staff_id[NSTAFFS];
  pthread_t staff_threads[NSTAFFS];

  initialize();

  int remainingStudents = N;
  bool taken[N];

  // start staff threads
  for (int i = 0; i < NSTAFFS; i++) {
    staff_id[i] = i + 1;
    pthread_create(&staff_threads[i], NULL, staff_activities, &staff_id[i]);
  }

  // start student threads
  while (remainingStudents) {
    int randomStudent = rand() % N;
    if (!taken[randomStudent]) {
      taken[randomStudent] = true;
      pthread_create(&student_threads[randomStudent], NULL, student_activities,
                     &students[randomStudent]);
      remainingStudents--;
      if (get_time() > 50000) {
        break;
      }
    }
  }

  // after waiting for 50 secs, start the remaining students
  for (int i = 0; i < N; i++) {
    if (!taken[i]) {
      pthread_create(&student_threads[i], NULL, student_activities,
                     &students[i]);
    }
  }

  // join threads
  for (int i = 0; i < N; i++) {
    pthread_join(student_threads[i], NULL);
  }

  for(int i = 0; i < NSTAFFS; i++) {
    pthread_join(staff_threads[i], NULL);
  }
  return 0;
}