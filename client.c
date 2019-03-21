/* CSci4061 F2018 Assignment 2
* section: 001
* date: 11/08/18
* name: Yuanzhe Liu
* id: yuanz002  5038957 */

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include "comm.h"

/* -------------------------Main function for the client ----------------------*/
void main(int argc, char * argv[]) {

	int pipe_child_to_user[2], pipe_user_to_child[2];

	// You will need to get user name as a parameter, argv[1].

	if(connect_to_server("TEKKAMAN", argv[1], pipe_child_to_user, pipe_user_to_child) == -1) {
		exit(-1);
	}
	/* -------------- YOUR CODE STARTS HERE -----------------------------------*/
	// close unnecessary pipe fd first
	if (close(pipe_child_to_user[1]) < 0 || close(pipe_user_to_child[0]) < 0)
		perror("Failed to close pipe 001\n");

	/* variable declariation */
	int nbytes;
	char buf[MAX_USERIDMSG];
	// setting stdin and the reading fd of pipe as non_blocking
	fcntl(STDIN_FILENO, F_SETFL, fcntl(STDIN_FILENO,F_GETFL,0) | O_NONBLOCK);
	fcntl(pipe_child_to_user[0], F_SETFL, fcntl(pipe_child_to_user[0], F_GETFL, 0) | O_NONBLOCK);


  while(1){
		// Poll stdin (input from the terminal) and send it to server (child process) via pipe
		nbytes = read(STDIN_FILENO, buf, MAX_MSG);
		if (nbytes < 0 && errno != EAGAIN) {
			// if read returns error
			perror("Reading error 002\n");
		} else if (nbytes > 0) {
			// if read something from the user
			if (strcmp (buf, "\\seg\n") == 0) {
				perror("Creating seg fault...\n");
 				int i[1];
				i[2] = 1;
			}else{
				if (write(pipe_user_to_child[1], buf, nbytes) < 0)
					perror("Failed to write to server 004\n");
			}
		}	/* if (read...) */

		memset(buf, '\0', MAX_MSG);
		usleep(10000);
		nbytes = 0;

		// Poll pipe retrieved and print it to sdiout
		nbytes = read(pipe_child_to_user[0], buf, MAX_USERIDMSG);
		// fprintf(stderr, "server read = %d\n", nbytes);
		if (nbytes < 0 && errno != EAGAIN) {
			// if read return error
			perror("Reading error 003 \n");
		}	else if (nbytes > 0) {
			// if read something from the server
			if (write(STDOUT_FILENO, buf, nbytes) < 0)
				perror("Failed to write to terminal 005\n");
		} else if (nbytes == 0) {
			// pipe is broken, means the child process has terminated
			break;
		} /* if (read...) */
		usleep (10000);
		nbytes = 0;
		memset(buf, '\0', MAX_USERIDMSG);
	} /* while(1) */

	fprintf(stderr, "Connection to server has terminated. Exiting...\n");
	if (close(pipe_child_to_user[0]) < 0 || close(pipe_user_to_child[1]) < 0)
		perror("Failed to close pipe 004");
	exit(0);
	/* -------------- YOUR CODE ENDS HERE -----------------------------------*/
}

/*--------------------------End of main for the client --------------------------*/
