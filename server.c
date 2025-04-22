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

const int PORT = 2865;
const int SIZE = 1024;
const int MAX_CLIENTS = 50;
const int MAX_ROOMS = 20;
#define MAX_USERNAME 50
#define ROOM_ID_LENGTH 6

typedef struct sockaddr *SPTR;

typedef struct
{
  int sockfd;
  char username[MAX_USERNAME];
  char room_id[ROOM_ID_LENGTH + 1];
  struct sockaddr_in addr;
} Client;

typedef struct
{
  char id[ROOM_ID_LENGTH + 1];
  char name[50];
  int client_count;
} Room;

void timestamp(char *_time)
{
  time_t now = time(NULL);
  strftime(_time, 20, "%I:%M %p", localtime(&now));
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

// Check if a room exists
int room_exists(Room *rooms, int room_count, const char *room_id)
{
  for (int i = 0; i < room_count; i++)
  {
    if (strcmp(rooms[i].id, room_id) == 0)
    {
      return i; // Return the room index
    }
  }
  return -1; // Room not found
}

void welcome()
{
  printf("\033[H\033[J");
  printf("\n\033[1mMulti-Room Chat Server Running\033[0m\n\n");
}

int main(int argc, char *argv[])
{
  fd_set fds;
  int sfd, maxfd, numclient = 0, numrooms = 0;
  char buffer[SIZE];
  Client clients[MAX_CLIENTS];
  Room rooms[MAX_ROOMS];

  // Initialize clients and rooms
  memset(clients, 0, sizeof(clients));
  memset(rooms, 0, sizeof(rooms));

  struct sockaddr_in server;
  sfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sfd == -1)
  {
    printf("Socket Creation failed!\n");
    exit(1);
  }

  int opt = 1;
  setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

  bzero(&server, sizeof(server));
  server.sin_addr.s_addr = htonl(INADDR_ANY);
  server.sin_port = htons(PORT);
  server.sin_family = AF_INET;

  if (bind(sfd, (SPTR)&server, sizeof(server)) == -1)
  {
    printf("Bind failed!\n");
    exit(1);
  }
  if (listen(sfd, 5) == -1)
  {
    printf("Listen failed!\n");
    exit(1);
  }
  welcome();

  while (1)
  {
    FD_ZERO(&fds);
    FD_SET(sfd, &fds);
    maxfd = sfd;

    for (int i = 0; i < MAX_CLIENTS; i++)
    {
      if (clients[i].sockfd > 0)
      {
        FD_SET(clients[i].sockfd, &fds);
        maxfd = (maxfd > clients[i].sockfd) ? maxfd : clients[i].sockfd;
      }
    }

    select(maxfd + 1, &fds, NULL, NULL, NULL);

    if (FD_ISSET(sfd, &fds) && numclient < MAX_CLIENTS)
    {
      socklen_t len = sizeof(struct sockaddr_in);
      int newfd = accept(sfd, (SPTR)&clients[numclient].addr, &len);

      // Receive connection data (format: "username|roomID|action")
      // action: 0 = create room, 1 = join room
      char connection_data[SIZE];
      int n = recv(newfd, connection_data, SIZE, 0);
      connection_data[n] = '\0';

      // Parse connection data
      char username[MAX_USERNAME];
      char room_id[ROOM_ID_LENGTH + 1];
      int action;

      char *token = strtok(connection_data, "|");
      if (token)
        strncpy(username, token, MAX_USERNAME - 1);

      token = strtok(NULL, "|");
      if (token)
        strncpy(room_id, token, ROOM_ID_LENGTH);

      token = strtok(NULL, "|");
      if (token)
        action = atoi(token);

      // Handle room creation or joining
      int room_idx = room_exists(rooms, numrooms, room_id);
      char response[SIZE];

      if (action == 0)
      { // Create room
        if (room_idx != -1)
        {
          // Room already exists
          sprintf(response, "ERROR|Room with ID %s already exists", room_id);
          send(newfd, response, strlen(response), 0);
          close(newfd);
          continue;
        }

        // Create new room
        if (numrooms < MAX_ROOMS)
        {
          strcpy(rooms[numrooms].id, room_id);
          sprintf(rooms[numrooms].name, "Room %s", room_id);
          rooms[numrooms].client_count = 0;
          room_idx = numrooms;
          numrooms++;
        }
        else
        {
          sprintf(response, "ERROR|Maximum number of rooms reached");
          send(newfd, response, strlen(response), 0);
          close(newfd);
          continue;
        }
      }
      else
      { // Join room
        if (room_idx == -1)
        {
          // Room doesn't exist
          sprintf(response, "ERROR|Room with ID %s doesn't exist", room_id);
          send(newfd, response, strlen(response), 0);
          close(newfd);
          continue;
        }
      }

      // Add client to room
      clients[numclient].sockfd = newfd;
      strncpy(clients[numclient].username, username, MAX_USERNAME);
      strncpy(clients[numclient].room_id, room_id, ROOM_ID_LENGTH + 1);
      rooms[room_idx].client_count++;

      // Send success response with room name
      sprintf(response, "SUCCESS|%s", rooms[room_idx].name);
      send(newfd, response, strlen(response), 0);

      char *IP = inet_ntoa(clients[numclient].addr.sin_addr);
      int port = ntohs(clients[numclient].addr.sin_port);

      char time[20];
      timestamp(time);
      printf("[%s] %s joined room %s\n", time, username, room_id);

      char join[SIZE];
      sprintf(join, "\033[1m[%s]\033[0m \033[1;32m%s\033[0m joined the chat!", time, username);

      // Notify only users in the same room
      for (int i = 0; i < numclient; i++)
      {
        if (clients[i].sockfd > 0 && strcmp(clients[i].room_id, room_id) == 0)
        {
          send(clients[i].sockfd, join, strlen(join), 0);
        }
      }
      numclient++;
    }

    for (int i = 0; i < MAX_CLIENTS; i++)
    {
      if (clients[i].sockfd > 0 && FD_ISSET(clients[i].sockfd, &fds))
      {
        int currfd = clients[i].sockfd;
        int status = recv(currfd, buffer, SIZE, 0);

        if (status > 0)
        {
          buffer[status] = '\0';
          char time[20];
          timestamp(time);

          char formatted_msg[SIZE];
          sprintf(formatted_msg, "\033[1m[%s]\033[0m \033[1;36m%s\033[0m: %s", time, clients[i].username, buffer);
          printf("%s (Room: %s)\n", formatted_msg, clients[i].room_id);

          // Send message only to clients in the same room
          for (int j = 0; j < MAX_CLIENTS; j++)
          {
            if (clients[j].sockfd <= 0)
              continue;
            if (clients[j].sockfd == currfd)
              continue;
            if (strcmp(clients[j].room_id, clients[i].room_id) != 0)
              continue; // Skip clients in different rooms
            send(clients[j].sockfd, formatted_msg, strlen(formatted_msg), 0);
          }
        }
        else if (status == 0)
        {
          char time[20];
          timestamp(time);
          printf("[%s] %s left room %s\n", time, clients[i].username, clients[i].room_id);

          char left[SIZE];
          sprintf(left, "\033[1m[%s]\033[0m 👋 \033[1;31m%s\033[0m left the chat!", time, clients[i].username);

          // Get the room index
          int room_idx = room_exists(rooms, numrooms, clients[i].room_id);
          if (room_idx != -1)
          {
            rooms[room_idx].client_count--;

            // If room is empty, remove it
            if (rooms[room_idx].client_count <= 0)
            {
              printf("Room %s is now empty and will be removed\n", rooms[room_idx].id);

              // Shift rooms to fill the gap
              if (room_idx < numrooms - 1)
              {
                for (int k = room_idx; k < numrooms - 1; k++)
                {
                  memcpy(&rooms[k], &rooms[k + 1], sizeof(Room));
                }
              }
              numrooms--;
            }
          }

          // Notify only users in the same room
          for (int j = 0; j < MAX_CLIENTS; j++)
          {
            if (clients[j].sockfd <= 0)
              continue;
            if (clients[j].sockfd == currfd)
              continue;
            if (strcmp(clients[j].room_id, clients[i].room_id) != 0)
              continue; // Skip clients in different rooms
            send(clients[j].sockfd, left, strlen(left), 0);
          }

          close(clients[i].sockfd);
          clients[i].sockfd = 0;
          memset(clients[i].username, 0, MAX_USERNAME);
          // numclient--;
        }
      }
    }
  }
}