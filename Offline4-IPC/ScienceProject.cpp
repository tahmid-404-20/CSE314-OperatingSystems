#include "ScienceProject.h"
#include "time.h"
#include <chrono>
#include <fstream>
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

#define SLEEP_MULTIPLIER 1000

// locks
// a mutex lock which deals with printing, changes the state
// of printers, students (shared resource)
pthread_mutex_t printing_lock;

// binding lock
sem_t binding_lock;

// submission lock
pthread_mutex_t submit_mutex;
pthread_mutex_t read_write_lock;
int rc = 0;

// used to sleep the main thread till all students complete their all jobs
sem_t exit_lock;

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
  sem_init(&exit_lock, 0, 0);

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

// used to wake up sleeping leader, waiting for others to finish printing
// called from group.add_printed_count(),  which is called from end_printing()
void enable_binding(int student_id) {
  sem_post(&(students[student_id - 1].lock));
}

void *student_activities(void *arg) {
  Student *student = (Student *)arg;

  usleep(student->writing_time * SLEEP_MULTIPLIER); // student is writing

  start_printing(student);
  usleep(printing_time * SLEEP_MULTIPLIER);
  end_printing(student);

  if (student->is_leader()) {
    cout << "Student " << student->id << " leader of group "
         << student->group_id
         << " is waiting for others to complete printing at time " << get_time()
         << endl;

    sem_wait(&student->lock); // waiting for others to complete printing

    cout << "Group " << student->group_id << " has finished printing at time "
         << get_time() << endl;

    // just a bit of time organizing printed docs
    usleep((get_random_number() % 3 + 1) * SLEEP_MULTIPLIER);

    // binding code
    sem_wait(&binding_lock);
    cout << "Group " << student->group_id << " has started binding at time "
         << get_time() << endl;
    usleep(binding_time * SLEEP_MULTIPLIER);
    cout << "Group " << student->group_id << " has finished binding at time "
         << get_time() << endl;
    sem_post(&binding_lock);

    // walking to library
    usleep((get_random_number() % 4 + 1) * SLEEP_MULTIPLIER);

    // submission_code - students are writers
    pthread_mutex_lock(&read_write_lock);
    cout << "Group " << student->group_id << " has started submitting at time "
         << get_time() << endl;
    usleep(read_write_time * SLEEP_MULTIPLIER);
    n_submissions++;
    cout << "Group " << student->group_id << " has finished submitting at time "
         << get_time() << endl;
    pthread_mutex_unlock(&read_write_lock);
  }
}

void *staff_activities(void *arg) {
  int staff_id = *(int *)arg;

  int starting_time = get_random_number() % 3 + 1;    // 1-3 secs
  int reading_interval = get_random_number() % 4 + 2; // 2-5 secs

  usleep(starting_time * SLEEP_MULTIPLIER);

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

    usleep(read_write_time * SLEEP_MULTIPLIER);

    pthread_mutex_lock(&submit_mutex);
    cout << "Staff " << staff_id << " has finished reading at time "
         << get_time() << ". No. of submission = " << n_submissions << endl;
    rc--;
    // all readers gone
    if (rc == 0) {
      pthread_mutex_unlock(&read_write_lock);
    }
    pthread_mutex_unlock(&submit_mutex);

    //  all the groups have submitted, so exit
    if (n_submissions == N / M) {
      sem_post(&exit_lock);
    }
    usleep(reading_interval * SLEEP_MULTIPLIER);
  }
}

int main(int argc, char *argv[]) {

  if (argc != 3) {
    cout << "Usage: ./a.out <input_file> <output_file>" << endl;
    return 0;
  }

  std::ifstream inputFile(argv[1]);
  std::streambuf *cinBuffer = std::cin.rdbuf();
  std::cin.rdbuf(inputFile.rdbuf());

  std::ofstream outputFile(argv[2]);
  std::streambuf *coutBuffer = std::cout.rdbuf();
  std::cout.rdbuf(outputFile.rdbuf());

  cin >> N;
  cin >> M;
  cin >> printing_time;
  cin >> binding_time;
  cin >> read_write_time;

  pthread_t student_threads[N];

  int staff_id[NSTAFFS];
  pthread_t staff_threads[NSTAFFS];

  initialize();

  // start staff threads
  for (int i = 0; i < NSTAFFS; i++) {
    staff_id[i] = i + 1;
    pthread_create(&staff_threads[i], NULL, staff_activities, &staff_id[i]);
  }

  int remainingStudents = N;
  bool taken[N];

  // start student threads
  while (remainingStudents) {
    int randomStudent = get_random_number() % N;
    if (!taken[randomStudent]) {
      taken[randomStudent] = true;
      pthread_create(&student_threads[randomStudent], NULL, student_activities,
                     &students[randomStudent]);
      remainingStudents--;
      if (get_time() >
          7000) { // if more than 7 seconds is passed, initialize the rest
        break;
      }
    }
  }

  // after waiting for 7 secs, start the remaining students
  for (int i = 0; i < N; i++) {
    if (!taken[i]) {
      pthread_create(&student_threads[i], NULL, student_activities,
                     &students[i]);
    }
  }

  sem_wait(&exit_lock);

  for (int i = 0; i < NSTAFFS; i++) {
    pthread_cancel(staff_threads[i]);
  }

  // Restore cin and cout to the original buffers (console)
  std::cin.rdbuf(cinBuffer);
  std::cout.rdbuf(coutBuffer);
  return 0;
}