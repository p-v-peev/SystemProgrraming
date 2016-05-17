#include <stdio.h>
//For string Functions
#include <string.h>
//For calloc
#include <stdlib.h>
//For read
#include <unistd.h>
//For open
#include <fcntl.h>
//For inet_ntop and all the bulshit with the networking.
#include <arpa/inet.h>

#define MAX_ERRORS 20

//Type definitions
typedef struct sockaddr_in sockaddr_in;

//Functions

void reverse(char* s)
{
 int i, j;
 char c;

 for (i = 0, j = strlen(s)-1; i<j; i++, j--) {
     c = *(s + i);
     *(s + i) = *(s + j);
     *(s + j) = c;
 }
}

void itoa(int n, char s[])
 {
 int i, sign;

 if ((sign = n) < 0)
     n = -n;
 i = 0;
 do {
     s[i++] = n % 10 + '0';
 } while ((n /= 10) > 0);
 if (sign < 0)
     s[i++] = '-';
 s[i] = '\0';
 reverse(s);
 }

 char* get_line(int* data_fd)
 {
   char c;
   char* buffer = calloc(1, 256 * sizeof(char));
 	 int index = 0;
 	 int if_readed = 0;

 	 while(((if_readed = read(*data_fd, &c, 1)) != 0) && c!= '\n')
 	{
    *(buffer + index) = c;
 		index++;
 	}

   if (if_readed == 0) {
     return NULL;
   }

   return buffer;
 }

 void add_new_user(char* ip_address, char* credentials, char* file_name)
 {
   int data_fd = 0;
   char user_data[150] = {'\0'};

   data_fd = open(file_name, O_WRONLY | O_APPEND);

   if (data_fd < 0) {
     printf("%s\n", "Cant open file");
     return;
   }

   strcat(user_data, ip_address);
   strcat(user_data, " ");
   strcat(user_data, credentials);
   strcat(user_data, " ");
   strcat(user_data, "0");
   strcat(user_data, "\n");

   write(data_fd, user_data, strlen(user_data));

   close(data_fd);
 }

void increment_errors(char* ip_address, char* file_name, int errors)
{
    int data_fd = 0;
    int dest_fd = 0;

    char* buffer = NULL;
    char* first_space = NULL;
    char* last_space = NULL;
    char errors_as_string[3] = {'\0'};
    char current_ip[INET_ADDRSTRLEN];

    data_fd = open(file_name, O_RDONLY);

    if (data_fd < 0) {
      printf("%s\n", "Cant open file");
      return;
    }

    dest_fd = open("result.txt", O_CREAT | O_WRONLY, 0777);

    if (dest_fd < 0) {
      printf("%s\n", "Cant open file");
      close(data_fd);
      return;
    }

    while((buffer = get_line(&data_fd)) != NULL) {
      if (0 < strlen(buffer)) {
        memset(current_ip, '\0', sizeof(current_ip));
        first_space = strchr(buffer, ' ');
        strncpy(current_ip, buffer, (int)(first_space - buffer));

        if (0 == strcmp(ip_address, current_ip)) {
            last_space = strrchr(buffer, ' ');
            itoa(errors + 1, errors_as_string);
            strcpy(last_space + 1, errors_as_string);
        }
        strcat(buffer, "\n");
        write(dest_fd, buffer, strlen(buffer));
    }
  }

    close(data_fd);
    close(dest_fd);
    remove(file_name);
    rename ("result.txt", file_name);

    return;
}

char* search_for_info(const char* ip_address, int* status, const char* file_name, int* errors)
{

  int data_fd = 0;
  char* buffer = NULL;
  char* first_space = NULL;
  char* last_space = NULL;
  char* credentials = NULL;
  char current_ip[INET_ADDRSTRLEN];
  int number_of_bytes = 0;
  data_fd = open(file_name, O_RDONLY);

  if (data_fd < 0) {
    printf("%s\n", "Cant open file");
    *status = -1;
    return credentials;
  }

  while((buffer = get_line(&data_fd)) != NULL) {
    if (0 < strlen(buffer)) {
      memset(current_ip, '\0', sizeof(current_ip));
      first_space = strchr(buffer, ' ');
      strncpy(current_ip, buffer, (int)(first_space - buffer));
      if (0 == strcmp(ip_address, current_ip)) {
          last_space = strrchr(buffer, ' ');
          *errors = atoi(last_space + 1);
          if (*errors >= MAX_ERRORS) {
            free(buffer);
            close(data_fd);
            *status = -1;
            return credentials;
          }
          number_of_bytes = (int)(last_space - first_space) - 1;
          credentials = calloc(1, number_of_bytes * sizeof(char));
          strncpy(credentials, first_space + 1, number_of_bytes);
          free(buffer);
          close(data_fd);
          *status = 1;
          return credentials;
      }
    }
    free(buffer);
  }

  close(data_fd);

  *status = 0;
  return credentials;
}

void run_server(int server_fd)
{
  int client_fd;
  int client_length = 0;
  int status = 0;
  int errors = 0;
  int choose_username_length = 0;
  int enter_username_length = 0;
  char ip_address[INET_ADDRSTRLEN];
  char* credentials = NULL;
  char* choose_username = "Choose username and password";
  char* enter_username = "Enter username and password";
  char* client_message = NULL;
  sockaddr_in client_address;

  client_length = sizeof(client_address);
  choose_username_length = strlen(choose_username);
  enter_username_length = strlen(enter_username);

  printf("%s\n", "Server is running");
  listen(server_fd , 5);
  while (1) {
    memset(&client_address, 0, sizeof(client_address));
    client_fd = accept(server_fd, (struct sockaddr*) &client_address, (socklen_t *) &client_length);
    if (client_fd >= 0) {
      printf("%s\n", "Some client is connected");
      client_message = calloc(1, 100 * sizeof(char));
      memset(ip_address, '\0', INET_ADDRSTRLEN);
      inet_ntop(AF_INET, &(client_address.sin_addr), ip_address, INET_ADDRSTRLEN);
      credentials = search_for_info(ip_address, &status, "data.txt", &errors);

      if (status > 0) {
        status = write(client_fd, enter_username, enter_username_length);
        if (status != -1) {
          status = read(client_fd, client_message, 100);
          if (status != -1) {
            if (0 == strcmp(credentials, client_message)) {
              printf("%s\n", "Client can connect to server");
            } else {
              printf("%s\n", "Increment errors about this client");
              increment_errors(ip_address, "data.txt", errors);
            }
          }
        }
        free(credentials);
      } else if (status == 0) {
        status = write(client_fd, choose_username, choose_username_length);
        if (status != -1) {
          status = read(client_fd, client_message, 100);
          if (status != -1) {
            add_new_user(ip_address, client_message, "data.txt");
            printf("%s\n", "New user is aded to the server");
          }
        }
      }

      close(client_fd);
    }
    free(client_message);
  }
}

int main(int argc, char** args)
{
  int status = 0;
  int server_fd = 0;
  int port_number = 5001;
  sockaddr_in server_address;

  server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd < 0) {
    (void) printf("%s\n", "Error on creating socket");
    return -1;
  }

  memset(&server_address, 0, sizeof(server_address));
  server_address.sin_family = AF_INET;
  server_address.sin_addr.s_addr = INADDR_ANY;
  server_address.sin_port = htons(port_number);

  status = bind(server_fd, (struct sockaddr* )&server_address, sizeof(server_address));
  if (status < 0) {
    (void) printf("%s\n", "Error on binding");
    return -1;
  }

  run_server(server_fd);

  close(server_fd);
  return 0;
}
