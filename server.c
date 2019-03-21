/* CSci4061 F2018 Assignment 2
* section: 001
* date: 11/08/18
* name: Yuanzhe Liu
* id: yuanz002  5038957 */

#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include "comm.h"
#include "util.h"

/* -----------Functions that implement server functionality -------------------------*/

/*
 * Returns the empty slot on success, or -1 on failure
 */
int find_empty_slot(USER * user_list) {
	// iterate through the user_list and check m_status to see if any slot is EMPTY
	// return the index of the empty slot
    int i = 0;
	for(i=0;i<MAX_USER;i++) {
    	if(user_list[i].m_status == SLOT_EMPTY) {
			return i;
		}
	}
	return -1;
}

/*
 * list the existing users on the server shell
 */
int list_users(int idx, USER * user_list)
{
	// iterate through the user list
	// if you find any slot which is not empty, print that m_user_id
	// if every slot is empty, print "<no users>""
	// If the function is called by the server (that is, idx is -1), then printf the list
	// If the function is called by the user, then send the list to the user using write() and passing m_fd_to_user
	// return 0 on success
	int i, flag = 0;
	char buf[MAX_MSG] = {}, *s = NULL;

	/* construct a list of user names */
	s = buf;
	strncpy(s, "---connecetd user list---\n", strlen("---connecetd user list---\n"));
	s += strlen("---connecetd user list---\n");
	for (i = 0; i < MAX_USER; i++) {
		if (user_list[i].m_status == SLOT_EMPTY)
			continue;
		flag = 1;
		strncpy(s, user_list[i].m_user_id, strlen(user_list[i].m_user_id));
		s = s + strlen(user_list[i].m_user_id);
		strncpy(s, "\n", 1);
		s++;
	}
	if (flag == 0) {
		strcpy(buf, "<no users>");
	} else {
		s--;
		strncpy(s, "\0", 1);
	}

	if(idx < 0) {
		printf("%s", buf);
		printf("\n");
	} else {
		/* write to the given pipe fd */
    strcat(buf,"\n");
		if (write(user_list[idx].m_fd_to_user, buf, strlen(buf) + 1) < 0)
			perror("writing to server shell 001");
	}

	return 0;
}

/*
 * add a new user
 */
int add_user(int idx, USER * user_list, int pid, char * user_id, int pipe_to_child, int pipe_to_parent)
{
	// populate the user_list structure with the arguments passed to this function
	// return the index of user added
  user_list[idx].m_pid = pid;
  strcpy(user_list[idx].m_user_id, user_id);
  // parent should read from fd_to_server and write to fd_to_user
  user_list[idx].m_fd_to_user = pipe_to_child;
  user_list[idx].m_fd_to_server = pipe_to_parent;
  user_list[idx].m_status = SLOT_FULL;
  return 0;
}

/*
 * Kill a user
 */
void kill_user(int idx, USER * user_list) {
  int status =0;
  kill(user_list[idx].m_pid, SIGKILL);
  waitpid(user_list[idx].m_pid,&status,0);
}

/*
 * Perform cleanup actions after the used has been killed
 */
void cleanup_user(int idx, USER * user_list) {
	// int values set back to -1
	// m_user_id set to zero, using memset()
	// close all the fd
  if (close(user_list[idx].m_fd_to_user) < 0 || close(user_list[idx].m_fd_to_server) < 0 ) {
    perror("Failed to close pipe 002\n");
  }

  memset(user_list[idx].m_user_id, '\0', MAX_USER_ID);
  user_list[idx].m_pid = -1;
  user_list[idx].m_fd_to_user = -1;
  user_list[idx].m_fd_to_server = -1;
  user_list[idx].m_status = SLOT_EMPTY;
}

/*
 * Kills the user and performs cleanup
 */
void kick_user(int idx, USER * user_list) {
  kill_user(idx, user_list);
  cleanup_user(idx, user_list);
}

/*
 * broadcast message to all users
 */
int broadcast_msg(USER * user_list, char *buf, char *sender)
{
  int nbytes = 0;
  for (int i = 0; i < MAX_USER; i++) {
    if (user_list[i].m_status == SLOT_EMPTY) continue;
    if (strcmp (user_list[i].m_user_id, sender) == 0) continue;
    char buf_temp[MAX_USERIDMSG];
    strcpy(buf_temp, sender);
    strcat(buf_temp, ": ");
    strcat(buf_temp, buf);
    nbytes = strlen(buf_temp);
    if (write(user_list[i].m_fd_to_user, buf_temp, nbytes) < 0)
      perror("Failed to broadcast msg 003\n");
  } /* for (all users...) */
	return 0;
}

/*
 * Cleanup user chat boxes
 */
void cleanup_users(USER * user_list)
{
  for (int i = 0; i < MAX_USER; i++) {
    if (user_list[i].m_status == SLOT_FULL)
      kick_user(i, user_list);
  } /* for (all users) */
}

/*
 * find user index for given user name
 */
int find_user_index(USER * user_list, char * user_id)
{
	// go over the  user list to return the index of the user which matches the argument user_id
	// return -1 if not found
	int i, user_idx = -1;

	if (user_id == NULL) {
		fprintf(stderr, "NULL name passed.\n");
		return user_idx;
	}
	for (i=0;i<MAX_USER;i++) {
		if (user_list[i].m_status == SLOT_EMPTY)
			continue;
		if (strcmp(user_list[i].m_user_id, user_id) == 0) {
			return i;
		}
	}

	return -1;
}

/*
 * given a command's input buffer, extract name
 */
int extract_name(char * buf, char * user_name)
{
	char inbuf[MAX_MSG];
    char * tokens[16];
    strcpy(inbuf, buf);

    int token_cnt = parse_line(inbuf, tokens, " ");

    if(token_cnt >= 2) {
        strcpy(user_name, tokens[1]);
        return 0;
    }

    return -1;
}

int extract_text(char *buf, char * text)
{
    char inbuf[MAX_MSG];
    char * tokens[16];
    char * s = NULL;
    strcpy(inbuf, buf);

    int token_cnt = parse_line(inbuf, tokens, " ");

    if(token_cnt >= 3) {
        //Find " "
        s = strchr(buf, ' ');
        s = strchr(s+1, ' ');

        strcpy(text, s+1);
        return 0;
    }

    return -1;
}

/*
 * send personal message
 */
void send_p2p_msg(int idx, USER * user_list, char *buf)
{
  char targetname[MAX_USER_ID];
  int nbytes = 0;

  extract_name (buf, targetname);
  for (int i = 0; i < MAX_USER; i++) {
    if (strcmp (user_list[i].m_user_id, targetname) == 0) {
      char buf_temp[MAX_USERIDMSG];
      char buf_temp2[MAX_USERIDMSG];
      strcpy(buf_temp, "<p2p>");
      strcat(buf_temp, user_list[idx].m_user_id);
      strcat(buf_temp, ": ");
      extract_text(buf, buf_temp2);
      strcat(buf_temp, buf_temp2);
      nbytes += strlen(buf_temp);
      if (write(user_list[i].m_fd_to_user, buf_temp, nbytes) < 0)
        perror("Failed to send p2p msg 004\n");
      return;
    }
  }   /* for (all users...) */
  if (write(user_list[idx].m_fd_to_user, "User not found\n", 15) < 0)
    perror ("Failed to write to user\n 005");
}

/*
 * Populates the user list initially
 */
void init_user_list(USER * user_list) {
	//iterate over the MAX_USER
	//memset() all m_user_id to zero
	//set all fd to -1
	//set the status to be EMPTY
	int i=0;
	for(i=0;i<MAX_USER;i++) {
		user_list[i].m_pid = -1;
		memset(user_list[i].m_user_id, '\0', MAX_USER_ID);
		user_list[i].m_fd_to_user = -1;
		user_list[i].m_fd_to_server = -1;
		user_list[i].m_status = SLOT_EMPTY;
	}
}

/* ---------------------End of the functions that implementServer functionality -----------------*/


/* ---------------------Start of the Main function ----------------------------------------------*/
int main(int argc, char * argv[])
{
	int nbytes;
	setup_connection("TEKKAMAN"); // Specifies the connection point as argument.

	USER user_list[MAX_USER];
	init_user_list(user_list);   // Initialize user list

	char buf[MAX_MSG];
	fcntl(0, F_SETFL, fcntl(0, F_GETFL)| O_NONBLOCK);
	print_prompt("admin");

	//
		/* ------------------------YOUR CODE FOR MAIN--------------------------------*/

		// Handling a new connection using get_connection
	int pipe_child_to_server[2];
	int pipe_server_to_child[2];
	char user_id[MAX_USER_ID];
  int pipe_user_to_child[2];
  int pipe_child_to_user[2];
  int pid;
  int idx;
  int prompt = NONEED;

  while(1) {

    if(prompt == NEED) {
      print_prompt("admin");
      prompt = NONEED;
    }

    // first check if there is incoming connection
    if (get_connection(user_id,pipe_child_to_user, pipe_user_to_child) != -1) {
      //if new connection to be established
      if (find_user_index(user_list, user_id) == -1) {
        //if the user does not exist
        if ((idx = find_empty_slot(user_list)) != -1) {
          //if there is space for new user
          if (strcmp(user_id, "Notice") == 0 ) {
            if (write(pipe_child_to_user[1], "Illegal user name!\n", 19) < 0)
              perror("writing to server shell 006\n");
            if (close(pipe_child_to_user[0]) < 0  // writing fd of child to server
              || close(pipe_child_to_user[1]) < 0  // writing fd of child to server
              || close(pipe_user_to_child[0]) < 0 // writing fd of child to server
              || close(pipe_user_to_child[1]) < 0)  // writing fd of child to server
              perror("Failed to close pipe 021\n");
            continue;
          }
          if (pipe(pipe_server_to_child) < 0 || pipe(pipe_child_to_server) < 0) {
      			perror("Failed to create pipe 007\n");
            prompt = NEED;
            continue;
      		}

          // child process should break from server polling loop
          pid = fork();
          if (pid < 0) perror("Failed to fork child process 008\n");
          if (pid == 0) break;
          if (write(pipe_child_to_user[1], "Connected to server!\n", 21) < 0)
            perror("writing to server shell 009\n");
          // add a new user to the user list
          add_user(idx, user_list, pid, user_id,
            pipe_server_to_child[1], pipe_child_to_server[0]);
          // set reading fd of child to server as non_blocking
          if (fcntl(pipe_child_to_server[0], F_SETFL,
            fcntl(pipe_child_to_server[0], F_GETFL, 0) | O_NONBLOCK) < 0)
            perror("Non-blocking read for pipe failed 010\n");


          if (close(pipe_child_to_server[1]) < 0 // writing fd of child to server
          || close(pipe_server_to_child[0]) < 0 )  // reading fd of server to child
          perror("Failed to close pipe 011\n");
        } else {
          if (write(pipe_child_to_user[1], "Server Full!\n", 13) < 0)
            perror("writing to server shell 012\n");
        }/* if (empty_slot) */
      } else {
        if (write(pipe_child_to_user[1], "Duplicated user name!\n", 22) < 0)
          perror("writing to server shell 013\n");
      }   /* if (user does not exit) */
      if ( close(pipe_child_to_user[0]) < 0  // writing fd of child to server
        || close(pipe_child_to_user[1]) < 0  // writing fd of child to server
        || close(pipe_user_to_child[0]) < 0 // writing fd of child to server
        || close(pipe_user_to_child[1]) < 0)  // writing fd of child to server
        perror("Failed to close pipe 014\n");
    } /* if (new connection) */

    // then, poll from every user in user_list
    for (int i = 0; i < MAX_USER; i++) {
      if (user_list[i].m_status == SLOT_EMPTY) continue;
      usleep(1000);
      nbytes = read(user_list[i].m_fd_to_server, buf, MAX_MSG);
      if(nbytes > 0) {  // read some data

        // handle user commands
        if (strncmp(buf, "\\p2p", 4) == 0) {
          send_p2p_msg(i, user_list, buf);
        }
        else if (strcmp (buf , "\\list\n") == 0) {
          list_users(i, user_list);
        }
        else if(strcmp (buf , "\\exit\n") == 0) {
          kick_user(i, user_list);
        }
        else {
          broadcast_msg(user_list, buf, user_list[i].m_user_id);
        }
        // printf ("Server: %.*s\n", nbytes, buf);
        memset(buf, '\0', MAX_MSG);
      }
      else if(nbytes == 0) {   // pipe is closed
        kick_user(i, user_list);
      }
      else if (nbytes < 0 && errno != EAGAIN) {  // read encounters errors
        fprintf(stderr, "errorno %d   022\n", errno);
        continue;
      } /* if ... */
    } /* for (users...) */

    // finally, check stdin for server admin
    nbytes = read(0, buf, MAX_MSG);
    if(nbytes > 0) {
      prompt = NEED;
      // read some data
      // handle admin commands
      if (strcmp (buf , "\\list\n") == 0) {
        list_users(-1, user_list);
      }
      else if (strncmp(buf, "\\kick", 5) == 0) {
        char kickname[MAX_USER_ID];
        extract_name(buf, kickname);
        int len = strlen(kickname) - 1;
        for(int i = 0; i < MAX_USER; i++) {
          if(strncmp(kickname, user_list[i].m_user_id, len) == 0) {
            kick_user(i,user_list);
          }
        }  /*for (users...)*/
      }
      else if (strcmp (buf , "\\exit\n") == 0) {
        cleanup_users(user_list);
        exit(0);
      }
      else {
        broadcast_msg(user_list, buf, "Notice");
      }
      memset(buf, '\0', MAX_MSG);
    } /* if (nbytes >0) */

  } /* while(1) for server (parent)*/

  // only child process should reach here
  // 4 pipe connecting to child, 4 fds needs to be closed
  if (close(pipe_child_to_server[0]) < 0  // reading fd of child to server
    || close(pipe_server_to_child[1]) < 0  // writing fd of server to child
    || close(pipe_child_to_user[0]) < 0  // reading fd of child to user
    || close(pipe_user_to_child[1]) < 0)   // writing fd of user to child
    perror("Failed to close pipe 016\n");
  // non_blocking for two reading fd of child
  if ( fcntl(pipe_user_to_child[0], F_SETFL,
        fcntl(pipe_user_to_child[0], F_GETFL, 0) | O_NONBLOCK) < 0
    || fcntl(pipe_server_to_child[0], F_SETFL,
        fcntl(pipe_server_to_child[0], F_GETFL, 0) | O_NONBLOCK) < 0 )
        perror ("Non-blocking read for pipe failed 017\n");

  while (1) {   // polling for child
    // read from server
    nbytes = read(pipe_server_to_child[0], buf, MAX_MSG);
    if (nbytes > 0) {  // read some data
      if (write(pipe_child_to_user[1], buf, nbytes) < 0)
        perror("Failed to write to pile (child 1) 018\n");
      memset(buf, '\0', MAX_MSG);
    }
    else if (nbytes == 0) {  // pipe is closed
      exit(0);
    }
    else if (nbytes < 0 && errno != EAGAIN) {   //encounter error
      perror("Reading error 019\n");
    }

    // read from user
    nbytes = read(pipe_user_to_child[0], buf, MAX_MSG);
    if (nbytes > 0) {  // read some data
      if (write(pipe_child_to_server[1], buf, nbytes) < 0 )
        perror("Failed to write to pile (child 2) 020\n");
      memset(buf, '\0', MAX_MSG);
    }
    else if (nbytes == 0) {  // pipe is closed
      exit(0);
    }
    else if (nbytes < 0 && errno != EAGAIN) {   //encounter error
      fprintf(stderr, "errorno %d   023\n", errno);
    }

  } /* while(1) for child */
		/* ------------------------YOUR CODE FOR MAIN--------------------------------*/

}

/* --------------------End of the main function ----------------------------------------*/
