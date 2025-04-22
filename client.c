#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <termios.h>
#include <sys/ioctl.h>

int PORT = 2864; // Default port
const int SIZE = 1024;
const int MAX_USERNAME = 50;
const int INPUT_SIZE = 1024;
const int HISTORY_SIZE = 5000;
#define ROOM_ID_LENGTH 6

typedef struct sockaddr *SPTR;

struct termios orig_termios;

char username[50];
char room_id[ROOM_ID_LENGTH + 1];
char room_name[50];
char input_buffer[1024] = {0};
int message_alignment[5000] = {0};
char message_history[5000][1024];

int _INPUTX_ = 0;
int _MSGCNT_ = 0;
int _ROWS_ = 24;
int _COLS_ = 80;

void timestamp(char *timestamp)
{
  time_t now = time(NULL);
  strftime(timestamp, 20, "%I:%M %p", localtime(&now));
}

void get_terminal_size()
{
  struct winsize w;
  ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
  _ROWS_ = w.ws_row;
  _COLS_ = w.ws_col;
}

void disable_raw_mode()
{
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
}

void enable_raw_mode()
{
  tcgetattr(STDIN_FILENO, &orig_termios);
  atexit(disable_raw_mode);

  struct termios raw = orig_termios;
  raw.c_lflag &= ~(ECHO | ICANON);
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

void clear_screen()
{
  printf("\e[2J");
  printf("\e[H");
}

void move_cursor(int row, int col)
{
  printf("\e[%d;%dH", row, col);
  fflush(stdout);
}

void input()
{
  move_cursor(_ROWS_ - 1, 1);
  printf("\e[1;37m>>> \e[0m%s", input_buffer);
  printf("\e[K");
  fflush(stdout);
}

void redraw_screen()
{
  clear_screen();
  move_cursor(1, 1);

  int start = (_MSGCNT_ > _ROWS_ - 3) ? _MSGCNT_ - (_ROWS_ - 3) : 0;
  for (int i = start; i < _MSGCNT_; i++)
  {
    int padding = (message_alignment[i] == 1) ? _COLS_ - strlen(message_history[i]) : 0;
    if (padding < 0)
      padding = 0;
    printf("%*s%s\n", padding, "", message_history[i]);
  }

  input();
  move_cursor(_ROWS_ - 1, 10 + _INPUTX_);
}

void push(const char *message, int align_right)
{
  if (_MSGCNT_ < HISTORY_SIZE)
  {
    strncpy(message_history[_MSGCNT_], message, SIZE - 1);
    message_alignment[_MSGCNT_] = align_right;
    _MSGCNT_++;
  }
  else
  {
    for (int i = 7; i < HISTORY_SIZE - 1; i++)
    {
      strcpy(message_history[i], message_history[i + 1]);
      message_alignment[i] = message_alignment[i + 1];
    }
    strncpy(message_history[HISTORY_SIZE - 1], message, SIZE - 1);
    message_alignment[HISTORY_SIZE - 1] = align_right;
  }
  redraw_screen();
}

void welcome()
{
  clear_screen();
  int padding = (_COLS_ - 50) / 2;
  printf("\n");
  printf("%*s╔══════════════════════════════════════════════════╗\n", padding, "");
  printf("%*s║                Multi-Room Chat App               ║\n", padding, "");
  printf("%*s╚══════════════════════════════════════════════════╝\n\n", padding, "");
}

void welcome_room()
{
  clear_screen();
  int padding = (_COLS_ - 50) / 2;
  printf("\n");
  printf("%*s╔══════════════════════════════════════════════════╗\n", padding, "");
  char welcome_msg[100];
  snprintf(welcome_msg, sizeof(welcome_msg), "║            Welcome to %s            ║", room_name);
  printf("%*s%s\n", padding, "", welcome_msg);
  printf("%*s╚══════════════════════════════════════════════════╝\n\n", padding, "");
}

// Generate a random room ID (6 alphanumeric characters)
void generate_room_id(char *id)
{
  const char charset[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
  srand(time(NULL));

  for (int i = 0; i < ROOM_ID_LENGTH; i++)
  {
    int key = rand() % (sizeof(charset) - 1);
    id[i] = charset[key];
  }
  id[ROOM_ID_LENGTH] = '\0';
}

int main(int argc, char *argv[])
{
  int sfd;
  char recv_buffer[SIZE];
  int action = -1;                  // 0 = create room, 1 = join room
  int connection_mode = -1;         // 0 = local, 1 = online
  char server_ip[50] = "127.0.0.1"; // Default to localhost

  welcome();
  printf("\e[1mEnter your username:\e[0m ");
  fgets(username, MAX_USERNAME, stdin);
  username[strcspn(username, "\n")] = 0;

  // Ask for connection mode
  while (connection_mode != 0 && connection_mode != 1)
  {
    printf("\e[1mDo you want to chat:\e[0m\n");
    printf("1. Locally (on this computer)\n");
    printf("2. Online (connect to remote server)\n");
    printf("Enter your choice (1 or 2): ");

    char mode_choice[10];
    fgets(mode_choice, sizeof(mode_choice), stdin);
    mode_choice[strcspn(mode_choice, "\n")] = 0;

    if (strcmp(mode_choice, "1") == 0)
    {
      PORT = 2865;
      connection_mode = 0; // Local
      printf("\e[1;32mConnecting to local server...\e[0m\n");
    }
    else if (strcmp(mode_choice, "2") == 0)
    {
      connection_mode = 1;            // Online
      strcpy(server_ip, "127.0.0.1"); // Always connect to localhost
      printf("\e[1;32mConnecting to online server...\e[0m\n");
    }
    else
    {
      printf("\e[1;31mInvalid choice. Please enter 1 or 2.\e[0m\n");
    }
  }

  while (action != 0 && action != 1)
  {
    printf("\e[1mDo you want to:\e[0m\n");
    printf("1. Create a new chat room\n");
    printf("2. Join an existing chat room\n");
    printf("Enter your choice (1 or 2): ");

    char choice[10];
    fgets(choice, sizeof(choice), stdin);
    choice[strcspn(choice, "\n")] = 0;

    if (strcmp(choice, "1") == 0)
    {
      action = 0; // Create room
      generate_room_id(room_id);
      printf("\e[1;32mCreating room with ID: %s\e[0m\n", room_id);
    }
    else if (strcmp(choice, "2") == 0)
    {
      action = 1; // Join room
      printf("\e[1mEnter the room ID to join:\e[0m ");
      fgets(room_id, ROOM_ID_LENGTH + 1, stdin);
      room_id[strcspn(room_id, "\n")] = 0;
    }
    else
    {
      printf("\e[1;31mInvalid choice. Please enter 1 or 2.\e[0m\n");
    }
  }

  struct sockaddr_in server;
  sfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sfd == -1)
  {
    printf("Socket Creation failed!\n");
    exit(1);
  }

  memset(&server, 0, sizeof(server));
  server.sin_addr.s_addr = inet_addr(server_ip);
  server.sin_port = htons(PORT);
  server.sin_family = AF_INET;

  printf("Connecting to server...\n");

  // Try to connect multiple times with a delay
  int max_retries = 3;
  int retry_count = 0;
  int connected = 0;

  while (retry_count < max_retries && !connected)
  {
    if (connect(sfd, (SPTR)&server, sizeof(server)) == -1)
    {
      printf("Connection attempt %d failed. Retrying...\n", retry_count + 1);
      retry_count++;
      sleep(2); // Wait 2 seconds before retrying

      // Create a new socket for the next attempt
      if (retry_count < max_retries)
      {
        close(sfd);
        sfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sfd == -1)
        {
          printf("Socket Creation failed!\n");
          exit(1);
        }
      }
    }
    else
    {
      connected = 1;
    }
  }

  if (!connected)
  {
    printf("\e[1;31mConnection failed after %d attempts!\e[0m\n", max_retries);
    printf("\e[1;33mPlease make sure the server is running:\e[0m\n");
    printf("1. For local mode: Run the C server (./s.out)\n");
    printf("2. For online mode: Run the Node.js server\n");
    printf("   - cd node\n");
    printf("   - node index.js\n");
    exit(1);
  }

  // Send connection data (format: "username|roomID|action")
  char connection_data[SIZE];
  sprintf(connection_data, "%s|%s|%d", username, room_id, action);
  send(sfd, connection_data, strlen(connection_data), 0);

  // Wait for server response
  int n = recv(sfd, recv_buffer, SIZE, 0);
  recv_buffer[n] = '\0';

  // Parse response
  char *token = strtok(recv_buffer, "|");
  if (token && strcmp(token, "ERROR") == 0)
  {
    token = strtok(NULL, "|");
    printf("\e[1;31mError: %s\e[0m\n", token);
    close(sfd);
    exit(1);
  }
  else if (token && strcmp(token, "SUCCESS") == 0)
  {
    token = strtok(NULL, "|");
    if (token)
    {
      strncpy(room_name, token, sizeof(room_name) - 1);
    }
    else
    {
      sprintf(room_name, "Room %s", room_id);
    }
  }

  enable_raw_mode();
  get_terminal_size();
  welcome_room();

  char connected_msg[SIZE];
  sprintf(connected_msg, "\e[1;34mConnected as: \e[1;32m\e[1m%s\e[0m in room: \e[1;35m%s\e[0m\n", username, room_id);
  push(connected_msg, 0);
  push("\e[1mGeneral Instructions:\e[0m", 0);
  push(" (1) Type your message and press \e[1;32mEnter\e[0m to send", 0);
  push(" (2) Type '\e[1;31m\\EXIT\e[0m' to quit", 0);
  push(" (3) Be respectful and courteous to others", 0);
  push(" (4) Avoid sharing personal information", 0);
  push("", 0);

  fd_set fds;
  int maxfd;

  while (1)
  {
    FD_ZERO(&fds);
    FD_SET(STDIN_FILENO, &fds);
    FD_SET(sfd, &fds);

    maxfd = (sfd > STDIN_FILENO) ? sfd : STDIN_FILENO;

    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 100000;

    select(maxfd + 1, &fds, NULL, NULL, &timeout);

    if (FD_ISSET(STDIN_FILENO, &fds))
    {
      char c;
      if (read(STDIN_FILENO, &c, 1) == 1)
      {
        if (c == '\n')
        {
          input_buffer[_INPUTX_] = '\0';

          if (strlen(input_buffer) > 0)
          {
            if (strcmp(input_buffer, "\\EXIT") == 0)
            {
              disable_raw_mode();
              clear_screen();
              printf("Goodbye!\n");
              close(sfd);
              exit(0);
            }

            char time[20];
            timestamp(time);
            char self_msg[SIZE];
            char truncated_input[SIZE / 2];
            strncpy(truncated_input, input_buffer, sizeof(truncated_input) - 1);
            truncated_input[sizeof(truncated_input) - 1] = '\0';
            snprintf(self_msg, SIZE - 1, "You: \e[1;32m%s\e[0m \e[1;37m%s\e[0m [%s]", truncated_input, "", time);
            push(self_msg, 1);

            _INPUTX_ = 0;
            send(sfd, input_buffer, strlen(input_buffer), 0);
            memset(input_buffer, 0, INPUT_SIZE);
          }
        }
        else if (c == 127 && _INPUTX_ > 0)
          input_buffer[--_INPUTX_] = '\0';
        else if (_INPUTX_ < INPUT_SIZE - 1)
          input_buffer[_INPUTX_++] = c;
        input();
      }
    }

    if (FD_ISSET(sfd, &fds))
    {
      int n = recv(sfd, recv_buffer, SIZE, 0);
      if (n > 0)
      {
        recv_buffer[n] = '\0';
        push(recv_buffer, 0);
      }
      else if (n == 0)
      {
        disable_raw_mode();
        clear_screen();
        printf("\nDisconnected from server.\n");
        close(sfd);
        exit(0);
      }
    }

    get_terminal_size();
    input();
  }
}