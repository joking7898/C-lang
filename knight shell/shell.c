#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#define MAX_LEN 100 /* The maximum length command */
#define WRITEONPIPE 1              //파이프의 입력
#define READONPIPE 0 

int index_of_history = 0;
char history[100][MAX_LEN];
int size;

int fetch_input(char *a) { // 버퍼내용의 입력된 글자 수를 리턴 아무것도 입력 안된 상태를 분별하는 기능.
	char p;
	int num  = 0;

	while (((p = getchar()) != '\n') && (num < MAX_LEN)) {
		a[num++] = p;
	}

	if (num == MAX_LEN && p != '\n') {
		perror("length error");
		num = -1;
	} else {
		a[num] = '\0';
	}
	while (p != '\n') p = getchar();
	return num; // 버퍼 글자수 리턴
}

int parse(char *buffer, int length, char* args[]) { // args에 버퍼에 입력된 명령을 옮겨주는 역할.
	int args_num = 0, last = -1, i;
	args[0] = NULL;
	for (i = 0; i <= length; ++i) {
		if (buffer[i] != ' ' && buffer[i] != '\t' && buffer[i]) {
			continue;
		}
        else {
			if (last != i-1) {
				args[args_num] = (char*)malloc(i-last);
				if (args[args_num] == NULL) {
					perror("error\n"); // 명령이 넘어온게 없을때 error
					return 1;
				}
				memcpy(args[args_num], &buffer[last+1], i-last-1);
				args[args_num][i-last] = 0;
				args[++args_num] = NULL;
			}
			last = i;
		}
	}
	return args_num;
}

int historys(char* buffer){ // history에서 index_of_history 의 카운팅을 하지 않는 조건을 절차적으로 설정
    if(buffer[0] == '\0'){
        return 0;
    }
	else if(strcmp(buffer,"history")==0){ // history 명령 안넘김
            return 0;
        }
	else if(strcmp(buffer,"!!")==0){ // !! 명령 안넘김
		return 0;
    }
    else if(buffer[0]==33){ //	!로 시작하는 명령 안넘김
		return 0;
	}
    else{    
        strcpy(history[index_of_history],buffer); // 버퍼 내용 history 안으로 복사
		index_of_history++; // 복사 후 인덱스 1 증가
        return 0;
        }
    
    return 1;
}

void history_print(){
    int j;
    for(j=index_of_history;j>0;j--){
        printf("[%d]  %s\n",j, history[j-1] );
    }
	printf("\n");
}

int history_function(){
    if(index_of_history=0){ //not_input
        printf("no history\n");
    }
    return 0;
}

int set_args(char** ch, char* buffer) { // arg, buffer 받은것.
	int i = 0;

	ch[i++] = strtok(buffer, " ");

	while (ch[i - 1] != NULL)
		ch[i++] = strtok(NULL, " ");

	return i - 1; // 여기서 ch의 값을 반환 시켜줌.
}

// 버퍼에서 받은 명령을 파이프 기준으로 바꾸고 왼쪽 명령을 오른쪽 명령으로 넘기는 형식을 구현\
void div_buffer(char** args, char* operator, char** buf_left,	char** buf_right) { // 명령 | 명령2 이런 형식을 명령 따로 명령2 따로 구분시켜줌.
	//함수(나눌 문자열 ,파이프라인,왼쪽 명령, 오른쪽 명령) 의 형태
	int i =0, j = 0;
	while (strcmp(operator, args[i]) != 0) { // 파이프 라인을 기준으로 좌우 구분
		buf_left[i] = args[i];
		i++;
	}
	buf_left[i] = NULL;
	i++;
	while (args[i] != NULL)
		buf_right[j++] = args[i++];
	buf_right[j] = NULL;
}

void pipe_exe(char** args) {
	int status;	// execvp 확인하는 것 1일경우 정상 0일경우 에러
	pid_t pid;	// fork 사용
	char* buf_left[MAX_LEN / 2 + 1];	//파이프 연산자 왼쪽 명령
	char* buf_right[MAX_LEN / 2 + 1];	//파이프 연산자 오른쪽 명령
	int fd[2];			//파이프함수 호출시 사용되는 파일디스크립터

	//'|'를 기준으로 왼쪽과 오른쪽 연산으로 나누어 준다.
	div_buffer(args, "|", buf_left, buf_right);
	pipe(fd); // 파일 디스크립터 들어감. 파일의 위치를 알려주는 기능
	pid = fork();
	if (pid < 0) {
		fprintf(stderr, "Fork가 실패하였습니다.");
		return;
	} 
	else if (pid == 0) {		//자식 프로세스인 경우
		dup2(fd[WRITEONPIPE], STDOUT_FILENO);		//표준출력을 파이프의 쓰기 채널을 가르키게 한다.
		//이때 파이프의 write와 read부분을 모두 닫아주어, 표준 출력만이 파이프에 입력되도록 설정한다.
		close(fd[READONPIPE]);
		close(fd[WRITEONPIPE]);
		status = execvp(buf_left[0], buf_left);
		if (status == -1) {
			fprintf(stderr, "%s : 명령어를 찾을 수 없습니다.", buf_left[0]);
			exit(1);
		}
	} 
	else {
		waitpid(pid, NULL, 0);      //자식프로세스의 종료를 기다림
		pid = fork();               //한번 더 fork()를 해준다.
		if (pid < 0) {
			fprintf(stderr, "Fork fsizeailed");
			exit(1);
		} 
		else if (pid == 0) {
			dup2(fd[READONPIPE], STDIN_FILENO); 
			//파이프 연산이 실행될때 파이프에  왼쪽 명령어의 출력, 오른쪽 명령어의 입력으로 들어가집니다.
			close(fd[WRITEONPIPE]);
			close(fd[READONPIPE]);

			status = execvp(buf_right[0], buf_right);
			if (status == -1) {
				fprintf(stderr, "%s : 명령어를 찾을 수 없습니다.", buf_right[0]);
				exit(1);
			}
		} 
		else {
			close(fd[READONPIPE]);
			close(fd[WRITEONPIPE]);
			waitpid(pid, NULL, 0);
		}
	}
}

int is_pipe(char** ch, int size) {
	int i = 0;

	for (i = 0; i < size; i++)
		if (ch[i][0] == '|'){
			return 1; // pipe 가 존재합니다.!!!
		}
	return 0; // piep 가 존재 하지 않습니다.!!!!
}

int main(void) {

	char *args[MAX_LEN/2 + 1]; /* command line arguments */
	int should_run = 1; /* flag to determine when to exit program */
	int size;
	char buffer[MAX_LEN + 1];
	memset(buffer, 0, sizeof(buffer));
	int length, args_num,input;

	while (should_run) {
		printf("knight_shell >>");
		fflush(stdout);

        length = fetch_input(buffer);
        if(length == -1){
            continue ;
        }

        historys(buffer);
        args_num = parse(buffer,length,args);
		//("버퍼 : %s\n", buffer);   // test용 실험 출력
		//printf("args : %s\n", args[0]);
		//printf("%d\n",buffer[0]);
		//printf("%d\n",buffer[1]);
        printf("히스토리 :%d\n",index_of_history);
        if (args_num == 0)
            continue;

        if(strcmp(args[0],"exit")==0){ // 입력하였을 경우 쉘 종료
            should_run =0;
            continue;
        }
        if(strcmp(args[0],"history")==0){ // history 입력시 히스토리 내역 출력
            history_print();
            continue;
        }
		size = set_args(args, buffer);	
		if (is_pipe(args, size) > 0) {
			pipe_exe(args);
			printf("\n");
			continue;
		}

        if(buffer[0] == '!' && buffer[1] == '!'){
			if(index_of_history<1){
				printf("no history\n");
			}
			else{
				length = strlen(history[(index_of_history-1)]);
				args_num = parse(history[(index_of_history-1)], length, args);
				// strcpy(args[0],history[(index_of_history-1)]);
				// printf("%s\n",args[0]); // 실행명령을 이전 실행 명령문으로 대체해버림.
            }
		}

		// ascii code using paser      // ascii 코드를 사용하여 히스토리 구현 하려고 시도. (결과적으로 성공, 하지만 다른 ! 명령을 처리할 경우 에러 발생)
		// if(buffer[0]==33 && (buffer[1]>47 && buffer[1]<58)){//아스키코드 사용 ! 33번 0~9번이 48~57번 이므로 버퍼로 따로 처리함. 
		// 	printf("%d\n",buffer[1]-48);
		// 	strcpy(args[0],history[(buffer[1]-48)-1]);
		// }

		if (buffer[0] == '!' && buffer[1] != '!')  // 개선된 !!와 !n의 명령 구현 부.
		{
			if(index_of_history<1){
				printf("no history\n");
			}
			else{
			length = strlen(history[(buffer[1]-'0')-1]);
			args_num = parse(history[(buffer[1]-'0')-1], length, args);
			}
		}

		int background = 0;
		if (strcmp(args[args_num-1], "&") == 0) { // & 가 입력되었는지를 비교해서 맞을경우 밑의 조건분기 판별.
			args[args_num - 1] = NULL;
			background = 1;
		}

		pid_t pid = fork();

		if (pid < 0) { // 
			perror("Fork error\n");
			exit(0);
		}

		int status;

		// printf("test: %s, %d\n", args[0], strlen(args[0]));

		if (pid == 0) {
			//printf("waiting for child, not a background process\n");
			status = execvp(args[0], args);
			
			return 0;
			//printf("child process complte\n");
		}
		else{
			if(pid >0) {
				if (!background) {
					waitpid(pid, &status, 0);
					//printf("\n");
					printf("child process complte\n");
				}
				else if(background) {
					//exit(0);
					printf("background process\n");
				}
			}
		}
	    
		not_input:;	
	}

	return 0;
}
