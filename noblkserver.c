#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

void error(const char *msg)
{
    perror(msg);
    exit(0);
}

struct node {
	int data;
	struct node *next;
};

struct node* createList(int data) {
	int size_of_data = sizeof(data);
	struct node *head = (struct node*) malloc(size_of_data);
	head->data = data;
	head->next = NULL;
	return head;
}

void pushToList(struct node *head_ref, int data) {
	int size_of_data = sizeof(data);
	struct node *new_node = (struct node*) malloc(size_of_data);
	new_node->data = data;
	struct node *temp_node = head_ref->next;
	head_ref->next = new_node;
	new_node->next = temp_node;
}

int main(int argc, char const *argv[])
{
	// Step 1: Set up socket
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) 
        error("ERROR opening socket");
    int portno = 8080;

	// Step 2: Bind
	struct sockaddr_in host;
	host.sin_family = AF_INET;
	host.sin_port = htons(portno);
	host.sin_addr.s_addr = INADDR_ANY;
	const struct sockaddr *host_ptr = (struct sockaddr*) &host;
	int bound = bind(sockfd, host_ptr, sizeof(host));
	if (bound < 0)
		error("ERROR binding to socket");

	// Step 3: Listen
	int listening = listen(sockfd, 2);
	if (listening < 0)
		error("ERROR listener failed");

	fd_set read_fds;
	FD_SET(sockfd, &read_fds);
	struct node *head = createList(0);

	int maxsockfd = sockfd + 1;
	while (1) {
		printf("%s\n", "Attempting to Select . . .");
		int selected = select(maxsockfd, &read_fds, NULL, NULL, NULL);
		if (selected >= 0) {
			printf("%s\n", "Connection Selected");
			struct node *cur_node = head;
			int echoed = 0;
			while (cur_node != NULL) {
				int fd = cur_node->data;

				if (FD_ISSET(fd, &read_fds)) {
					printf("%s\n", "Echoing Data");
					echoed = 1;
					// Step 5: Echo
					char buffer[256];
					bzero(buffer, 256);
					int recvd = recv(fd, buffer, 255, 0);
					if (recvd < 0)
						error("ERROR data not received");
					int sent = send(fd, buffer, strlen(buffer), 0);
					if (recvd < 0)
						error("ERROR data not sent");
					close(fd);
					FD_CLR(fd, &read_fds);
					FD_SET(sockfd, &read_fds);
				}
				cur_node = cur_node->next;
			}

			if (echoed == 0) {
				// Step 4: Accecpt
				struct sockaddr_in remote;
				struct sockaddr *remote_ptr = (struct sockaddr*) &remote;
				socklen_t remote_len = (socklen_t) sizeof(remote);
				printf("%s\n", "Attempting to Accept . . .");
				int remotefd = accept(sockfd, remote_ptr, &remote_len);
				if (remotefd < 0)
					error("ERROR receiving request");
				FD_SET(remotefd, &read_fds);
				maxsockfd = remotefd + 1;
				pushToList(head, remotefd);
				printf("%s\n", "Connection Accecpted");
			}
		}
		else {printf("%s\n", "Selection failed");}
	}

	// Close the connection
	printf("Closing the connection");
    close(sockfd);
	return 0;
}