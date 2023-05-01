#include "types.h"
#include "stat.h"
#include "user.h"

#define MSGSIZE 8

int main(void)
{
	toggle(); 
	printf(1,"%s\n","IPC Test case");
	// char *msg = (char *)malloc(20);
	// int myid;
	// int from;	
	
int cid = fork();
	if(cid==0){
		// This is child
		char *msg = (char *)malloc(MSGSIZE);
		int stat=-1;
		while(stat==-1){
			stat = recv(msg);
		}
		printf(1,"2 CHILD: msg recv is: %s \n", msg );
		exit();
	}else{
		// This is parent
		char *msg_child = (char *)malloc(MSGSIZE);
		msg_child = "P";
		printf(1,"1 PARENT: msg sent is: %s \n", msg_child );
		send(getpid(),cid,msg_child);	
		
		
		free(msg_child);
	}
	wait();
	
	exit();
}
