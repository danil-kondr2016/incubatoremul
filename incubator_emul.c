#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include <unistd.h>
#include <time.h>
#include <pthread.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#define MAX_REQUEST_BUFFER_LENGTH 8192
#define MAX_RESPONSE_BUFFER_LENGTH 65536

#define LEFT -1
#define NEUTRAL 0
#define RIGHT 1
#define ERROR 2

#define ON  1
#define OFF 0

#define N_PROGRAMS 1

#include "strings.h"

char request_buffer[MAX_REQUEST_BUFFER_LENGTH];
char response_buffer[MAX_RESPONSE_BUFFER_LENGTH];

pthread_t incubator;
float c_temperature = 36.00f;
float c_humidity = 50.00f;

float n_temperature = 37.50f;
float n_humidity = 50.00f;

int rot_per_day = 4320;

int current_program = 0;

time_t last_rotation = 0;
time_t begin_timer = 0;
time_t set_coef = 0;

int heating = OFF;
int wetting = OFF;
int cooling = ON;
int chamber = NEUTRAL;
int overheat = OFF;
int changed = 0;

int n_req = 0;

int recv_cmd(int);
int short_pause();

void process_command(char*);
void process_temperature();
void process_humidity();
void process_chamber();

void* incubator_thread(void*);

int main(int argc, char** argv) {
  int fd_listen, fd_connect = -1;
  struct sockaddr_in address;
  int recv_result;

  address.sin_family = AF_INET;
  address.sin_port = htons(80);
  address.sin_addr.s_addr = htonl(INADDR_ANY);

  if (argc >= 2) {
    if (strcmp(argv[1], "-h") == 0) {
      printf("Usage: %s [ip address] [port]\n", argv[0]);
      return 0;
    }
    address.sin_addr.s_addr = inet_addr(argv[1]);
  }
  if (argc == 3)
    address.sin_port = htons(atoi(argv[2]));

  printf("IoT-incubator emulator\n");
  pthread_create(&incubator, NULL, incubator_thread, NULL);
  for (int i = 0; i < MAX_REQUEST_BUFFER_LENGTH; i++)
    request_buffer[i] = 0;

  fd_listen = socket(AF_INET, SOCK_STREAM, 0);
  if (fd_listen == -1) {
    perror("Socket opening failed");
    return -1;
  } else {
    printf("Socket was opened\n");
  }
  if (bind(fd_listen, (struct sockaddr*)&address, sizeof(address)) == -1) {
    perror("Socket binding failed");
    return -1;
  } else {
    printf("Socket was bound to IP address %s:%hu\n", 
          inet_ntoa(address.sin_addr), ntohs(address.sin_port));
  }
  if (listen(fd_listen, 10) == -1) {
    perror("Socket listening failed");
    return -1;
  }

  last_rotation = time(NULL);
  begin_timer = time(NULL);

  while (1) {
    fd_connect = accept(fd_listen, (struct sockaddr*)NULL, NULL);
    if (fd_connect == -1) {
      perror("Connection failed");
      continue;
    } 
    recv_result = recv_cmd(fd_connect);
    close(fd_connect);
    short_pause();
  }

  return 0;
}

int recv_cmd(int fd) {
  int len_buf = 0, header_size = 0;
  int n_headers = 1, len_line = 1;
  char** headers;

  len_buf = recv(fd, request_buffer, MAX_REQUEST_BUFFER_LENGTH, 0);
  if (len_buf == -1)
    return -1;
  else if (len_buf == 0)
    return 0;
  else if (len_buf > MAX_REQUEST_BUFFER_LENGTH) {
    snprintf(response_buffer, MAX_RESPONSE_BUFFER_LENGTH,
            "HTTP/1.1 413 Payload Too Large\r\n"
            "Server: IncubatorEmulator\r\n"
            "Content-Type: text/html; charset=utf-8\r\n"
            "Content-Length: %d\r\n"
            "\r\n\r\n%s",
            strlen(msg413) + 2, msg413);
    send(fd, response_buffer, MAX_RESPONSE_BUFFER_LENGTH, MSG_EOR);
    return 413;
  }
  if (len_buf < MAX_REQUEST_BUFFER_LENGTH)
    request_buffer[len_buf] = '\0';

  headers = (char**)malloc(n_headers*sizeof(char*));
  headers[0] = (char*)malloc(len_line*sizeof(char));
  for (int i = 0; i < len_buf; i++) {
    if (request_buffer[i] == '\r') 
      continue;
    if (request_buffer[i] == '\0') 
      break;
    if (request_buffer[i] == '\n') {
      if (strlen(headers[n_headers-1]) == 0) {
        header_size = i+1;
        break;
      }
      n_headers++;
      len_line = 1;
      headers = (char**)realloc(headers, n_headers*sizeof(char*));
      headers[n_headers-1] = (char*)malloc(len_line*sizeof(char));
      headers[n_headers-1][0] = '\0';
    } else {
      headers[n_headers-1][len_line-1] = request_buffer[i];
      len_line++;
      headers[n_headers-1] = (char*)realloc(headers[n_headers-1],
                                            len_line*sizeof(char));
      headers[n_headers-1][len_line-1] = '\0';
    }
  }

  n_req++;

  if (strstr(headers[0], "GET / ") || strstr(headers[0], "POST / ")) {
    snprintf(response_buffer, MAX_RESPONSE_BUFFER_LENGTH,
            "HTTP/1.1 200 OK\r\n"
            "Server: IncubatorEmulator\r\n"
            "Content-Type: text/html; charset=utf-8\r\n"
            "Content-Length: %d\r\n"
            "\r\n%s",
            strlen(msgWelcome), msgWelcome);
  } else if (strstr(headers[0], "POST /control")) {
    process_command(request_buffer + header_size);
  } else if (strstr(headers[0], "GET /control")) {
    snprintf(response_buffer, MAX_RESPONSE_BUFFER_LENGTH,
             "HTTP/1.1 200 OK\r\n"
             "Server: IncubatorEmulator\r\n"
             "Content-Type: text/html; charset=utf-8\r\n"
             "Content-Length: 12\r\n"
             "\r\nmethod_get\r\n");
  } else {
    snprintf(response_buffer, MAX_RESPONSE_BUFFER_LENGTH,
            "HTTP/1.1 404 Not Found\r\n"
            "Server: IncubatorEmulator\r\n"
            "Content-Type: text/html; charset=utf-8\r\n"
            "Content-Length: %d\r\n"
            "\r\n\r\n%s",
            strlen(msg404) + 2, msg404);
  }

  free(headers);
  send(fd, response_buffer, MAX_RESPONSE_BUFFER_LENGTH, MSG_EOR);
  return 200;
}

int short_pause() {
  struct timespec time;
  time.tv_sec = 0;
  time.tv_nsec = 100000000;
  return nanosleep(&time, &time);
}

void process_command(char* req) {
  char cmd_answer[1024] = {0};
  int argc = 1, len_arg = 1;
  char** argv;

  argv = (char**)malloc(sizeof(char*) * argc);
  argv[0] = (char*)malloc(sizeof(char) * len_arg);
  argv[0][0] = '\0';
  for (int i = 0; i < strlen(req); i++) {
    if (req[i] == '\r')
      continue;
    if (req[i] == '\n')
      break;
    if (req[i] == '\t' || req[i] == ' ') {
      argc++; len_arg = 1;
      argv = (char**)realloc(argv, sizeof(char*) * argc);
      argv[argc - 1] = (char*)malloc(sizeof(char) * len_arg);
      argv[argc - 1][0] = '\0';
    } else {
      argv[argc - 1][len_arg - 1] = req[i];
      len_arg++;
      argv[argc - 1] = (char*)realloc(argv[argc - 1], sizeof(char) * len_arg);
      argv[argc - 1][len_arg - 1] = '\0';
    }
  }

  if (strcmp(argv[0], "request_state") == 0) {
    snprintf(cmd_answer, 1024,
             "current_temp %.2f\r\n"
             "current_humid %.2f\r\n"
             "heater %d\r\n"
             "cooler %d\r\n"
             "wetter %d\r\n"
             "chamber %d\r\n"
             "uptime %lld\r\n"
             "%s\r\n" 
             "%s",
             c_temperature, c_humidity, heating, cooling, wetting, chamber,
             time(NULL) - begin_timer + set_coef, 
             (overheat ? "overheat\r\n" : ""),
             (changed ? "changed\r\n" : ""));
    changed = 0;
    wetting = OFF;
    printf("C: T=%f, Hd=%f, H%dW%dC%dCh%d\n", 
           c_temperature, c_humidity, heating, wetting, cooling, chamber);
  } else if (strcmp(argv[0], "request_config") == 0) {
    snprintf(cmd_answer, 1024,
             "needed_temp %.2f\r\n"
             "needed_humid %.2f\r\n"
             "rotations_per_day %d\r\n"
             "number_of_programs %d\r\n"
             "current_program %d\r\n",
             n_temperature, n_humidity, rot_per_day, N_PROGRAMS,
             current_program);
    printf("N: T=%f, H=%f\n", n_temperature, n_humidity);
  } else if (strcmp(argv[0], "needed_temp") == 0) {
    if (current_program != 0)
      return;
    n_temperature = atof(argv[1]);
    changed = 1;
    strncpy(cmd_answer, "success\r\n", 1024);
    printf("N: T=%f, H=%f, R=%d\n", n_temperature, n_humidity, rot_per_day);
  } else if (strcmp(argv[0], "needed_humid") == 0) {
    if (current_program != 0)
      return;
    n_humidity = atof(argv[1]);
    changed = 1;
    strncpy(cmd_answer, "success\r\n", 1024);
    printf("N: T=%f, H=%f, R=%d\n", n_temperature, n_humidity, rot_per_day);
  } else if (strcmp(argv[0], "rotations_per_day") == 0) {
    if (current_program != 0)
      return;
    rot_per_day = atoi(argv[1]);
    changed = 1;
    strncpy(cmd_answer, "success\r\n", 1024);
    printf("N: T=%f, H=%f\n", n_temperature, n_humidity);
  } else if (strcmp(argv[0], "toggle_alarm") == 0) {
    overheat = ((overheat) ? OFF : ON);
    strncpy(cmd_answer, "success\r\n", 1024);
  } else if (strcmp(argv[0], "toggle_cooler") == 0) {
    cooling = ((cooling) ? OFF : ON);
    strncpy(cmd_answer, "success\r\n", 1024);
  } else if (strcmp(argv[0], "chamber_error") == 0) {
    chamber = ((chamber != ERROR) ? ERROR : NEUTRAL);
    strncpy(cmd_answer, "success\r\n", 1024);
  } else if (strcmp(argv[0], "switch_to_program") == 0) {
    current_program = atoi(argv[1]);
    changed = 1;
    strncpy(cmd_answer, "success\r\n", 1024);
  } else if (strcmp(argv[0], "set_uptime") == 0) {
    time_t new_uptime = atoll(argv[1]);
    set_coef = new_uptime - (time(NULL) - begin_timer);
    strncpy(cmd_answer, "success\r\n", 1024);
  } else if (strcmp(argv[0], "rotate_to") == 0) {
    chamber = atoi(argv[1]);
    last_rotation = time(NULL);
    strncpy(cmd_answer, "success\r\n", 1024);
  }

  snprintf(response_buffer, MAX_RESPONSE_BUFFER_LENGTH,
           "HTTP/1.1 200 OK\r\n"
           "Server: IncubatorEmulator\r\n"
           "Content-Type: text/plain; charset=utf-8\r\n"
           "Content-Length: %d\r\n"
           "\r\n%s",
           strlen(cmd_answer), cmd_answer);

  free(argv);
}

void process_temperature() {
  if (heating) {
    c_temperature += 0.005;
  }
  c_temperature -= 0.001;
  if (c_temperature  <= n_temperature - 0.5) {
    heating = ON;
  } else if (c_temperature >= n_temperature) {
    heating = OFF;
  }
}

void process_humidity() {
  c_humidity -= 0.01;
  if (c_humidity  <= n_humidity - 10) {
    c_humidity = n_humidity; 
    wetting = ON;
  }
}

void process_chamber() {
  time_t period;
  if (rot_per_day == 0) {
    chamber = NEUTRAL;
    return;
  }

  period = 86400 / rot_per_day;

  if ((time(NULL) - last_rotation) < period)
    return;
  if (chamber == NEUTRAL || chamber == LEFT)
    chamber = RIGHT;
  else if (chamber == RIGHT)
    chamber = LEFT;
  last_rotation = time(NULL);
}

void* incubator_thread(void* args) {
  while (1) {
    process_temperature();
    process_humidity();
    process_chamber();
    short_pause();
  }
  return NULL;
}
