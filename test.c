#include "types.h"
#include "stat.h"
#include "user.h"

void print_variance(float xx)
{
 int beg=(int)(xx);
 int fin=(int)(xx*100)-beg*100;
 printf(1, "Variance of array for the file arr is %d.%d\n", beg, fin);
}

int
main(int argc, char *argv[])
{
	if(argc< 2){
		printf(1,"Need type and input filename\n");
		exit();
	}
	char *filename;
	filename=argv[2];
	int type = atoi(argv[1]);
	printf(1,"Type is %d and filename is %s\n",type, filename);

	int tot_sum = 0;	

	int size=1000;
	short arr[size];
	char c;
	int fd = open(filename, 0);
	for(int i=0; i<size; i++){
		read(fd, &c, 1);
		arr[i]=c-'0';
		read(fd, &c, 1);
	}	
  	close(fd);
  	// this is to supress warning
  	printf(1,"first elem %d\n", arr[0]);
  
  	//----FILL THE CODE HERE for unicast sum
	int main_pid = getpid();
	for(int i = 0; i < 8; ++i){
		int pid = fork();
		if(pid == 0){
			int partialSum = 0;
			for(int j = 125*i; j < 125*(i+1); ++j) partialSum += arr[j];
			send(getpid(),main_pid,(void*)&partialSum);
			exit();
		}
	}
	if(getpid()==main_pid){
		for(int i = 0; i < 8; ++i){
			int * curr = malloc(sizeof(int));
			recv(curr);
			tot_sum += *curr;
			wait();
		}
	}
	float mean = tot_sum;
	float variance = 0;
	mean /= 1000;
	int child_ids[8];
	for(int i = 0; i < 8; ++i){
		int pid = fork();
		if(pid>0) child_ids[i] = pid;
		else{
			float * temp = malloc(sizeof(float));
			recv(temp);
			float partialSquare = 0;
			for(int j = 125*i; j < 125*(i+1); ++j){
				float val = arr[j];
				partialSquare += (*temp - val)*(*temp - val);
			}
			send(getpid(),main_pid,(void*)&partialSquare);
			exit();
		}
	}
	if(getpid()==main_pid){
		send_multi(main_pid,child_ids,(void*)&mean);
		for(int i = 0; i < 8; ++i){
			float * curr = malloc(sizeof(float));
			recv(curr);
			variance += *curr;
			wait();
		}
		variance /= 1000;
	}

  	//------------------

  	if(type==0){ //unicast sum
		printf(1,"Sum of array for file %s is %d\n", filename,tot_sum);
	}else{
		print_variance(variance);
	}
	exit();
}
