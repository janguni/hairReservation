#include <signal.h>
#include <sys/mman.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define SNAME  "hbsocket"
struct day{
	int month;
	int date;
	int time;
};

int main(int argc, char **argv) {
	int sd, len;
	char buf[256];
	int bufr[256];
	char bufr2[256];
	int month;
	int date;
	int time;

	struct sockaddr_un server;

	// 소캣 생성	
	if ((sd = socket(AF_UNIX, SOCK_STREAM,0)) == -1) {
		perror("socket");
		exit(1);
	}

	// 소캣 주소 구체 초기화 후 값 지정
	memset((char *)&server, '\0', sizeof(server));
	server.sun_family = AF_UNIX;
	strcpy(server.sun_path, SNAME);
	len = sizeof(server.sun_family) + strlen(server.sun_path);

	// 서버에게 연결 요청
	if (connect(sd, (struct sockaddr *)&server, len)<0) {
		perror("bind");
		exit(1);
	}

	strcpy(buf,"예약준비 완료");
	if (send(sd, buf, sizeof(buf),0)== -1) {
		perror("send");
		exit(1);
	}


	key_t key;
	int shmid;
	void *shmaddr;
	char input_menu;
	char buf3[2];
	int menu;

	printf("==============미용실 예약 서비스==============\n");
	printf("1. 예약하기\n2. 예약내역 확인\n3. 예약취소\n4. 리뷰등록\n5. 리뷰목록 확인\n6. 별점확인\n");
	
	// 사용자에게 메뉴 번호 받기
	scanf("%c", &input_menu);

	// 버퍼에 menu번호 넣기	
	buf3[1] = '\0';
	buf3[0] = input_menu;

	// menu를 공유메모리를 통해 서버에게 보내기	
	key = ftok("hair", 23);
	shmid = shmget(key, 2, 0);
	
	shmaddr = shmat(shmid, NULL, 0);
	strcpy(shmaddr, buf3); // 공유메모리게 버퍼내용 담기

	kill(atoi(argv[1]), SIGUSR1); // 서버에게 시그널 보내기
	

	//공유 메모리 제거	
	sleep(1);
	shmdt(shmaddr);
	shmctl(shmid, IPC_RMID, NULL);

	
	menu = atoi(&input_menu); // 메뉴 번호 정수로 변환
	switch (menu) {
		case 1:	
	
			/*예약*/

			printf("[예약]\n");
			// 사용자 정보 입력 (이름, 번호)
			char name[6];
			printf("이름: ");
			scanf("%s", name);

			// 서버로 이름 전송
			if (send(sd, name, sizeof(name),0)==-1) {
				perror("name send");
				exit(1);
			}

			char phone[12];
			printf("ex) 01012345678\n");
			printf("번호: ");
                        scanf("%s", phone);

			//서버로 핸드폰 번호 전송
                        if (send(sd, phone, sizeof(phone),0)==-1) {
                                perror("phone send");
                                exit(1);
                        }


			// 날짜 정보 입력 (월, 일)
			printf("월/일을 입력하시면 예약 가능한 시간대를 알려드리겠습니다.\n");
			printf("월: ");
			scanf("%d", &month);

			// 월의 자료형을 배열로 변경
			char send_month[3];
			send_month[2]='\0';
			sprintf(send_month, "%d", month);
	
			// 서버로 월 전송
			if(send(sd, send_month, sizeof(256),0)==-1) {
				perror("day send");
				exit(1);
			}

			printf("일: ");
			scanf("%d", &date);

			// 일의 자료형을 배열로 변경
			char send_date[3];
			send_date[2]='\0';
			sprintf(send_date, "%d", date);

			// 서버로 일 전송
			if(send(sd, send_date, sizeof(256),0)==-1) {
				perror("date send");
				exit(1);
			}
			

			// 예약현황받기	
			printf("해당 미용실의 %d월 %d일 예약현황입니다.\n",month, date);
			if (recv(sd, bufr, sizeof(bufr), 0)==-1) {
				perror("reservation receive");
				exit(1);
			}

			// 예약 현황 출력
			for (int i=9; i<23; i++) {
				if (bufr[i]==1) 
					printf("%d시 예약 불가능\n",i);
				else
					printf("%d시 예약 가능!\n", i);
			}

			
			// 예약할 시간대 입력
			printf("예약 가능한 시간을 입력해주세요 (ex) 10시~11시 예약시 10을 입력)");
			scanf("%d", &time);

			// 시간의 자료형을 배열로 변경
			char send_time[3];
			send_time[2]='\0';
			sprintf(send_time, "%d", time);

			// 서버로 시간 전송
			if(send(sd, send_time, sizeof(256),0)==-1) {
				perror("time send");
				exit(1);
			}

			printf("[예약완료]\n");	
			break;


		case 2: 
			{
				/*예약 내역 확인*/

				printf("[예약내역 확인]\n");
                   
                        	// 사용자 정보 받기 (이름, 번호)
                        	char name[6];
                        	printf("이름: ");
                        	scanf("%s", name);
                        	if (send(sd, name, sizeof(name),0)==-1) {
                        	        perror("name send");
                        	        exit(1);
                        	}
                        	char phone[12];
                        	printf("ex) 01012345678");
                        	printf("번호: ");
                        	scanf("%s", phone);
                        	if (send(sd, phone, sizeof(phone),0)==-1) {
                        	        perror("phone send");
                        	        exit(1);
                        	}
				
				//예약 횟수 server로부터 받기		
				if (recv(sd, bufr2, sizeof(bufr2), 0)==-1) {
                                	perror("reservation receive");
                                	exit(1);
                        	}
				int reser_count;
				reser_count = atoi(bufr2); // 예약 횟수 정수형으로 전환

				printf("%s님의 예약 횟수는 총 %d번 입니다.\n", name, reser_count);
				

				//예약 횟수대로 월/일/시간 데이버를 서버로 부터 받기
				struct day d;
				int month;
				int date;
				int time;				
				for(int i=0;i<reser_count; i++) {
					sleep(2);

					if (recv(sd, (struct day*)&d, sizeof(d),0)==-1) {
						perror("day receive");
						exit(1);
					}	
					printf("(%d) %d월 %d일 %d시~%d시\n", (i+1), d.month, d.date, d.time, d.time+1);
				}
				break;
			}

		case 3: 
			{	/*예약 취소*/

				printf("[예약취소]\n");
                                // 사용자 정보 받기 (이름, 번호)
                                char name[6];
                                printf("이름: ");
                                scanf("%s", name);
                                if (send(sd, name, sizeof(name),0)==-1) {
                                        perror("name send");
                                        exit(1);
                                }
                                char phone[12];
                                printf("ex) 01012345678");
                                printf("번호: ");
                                scanf("%s", phone);
                                if (send(sd, phone, sizeof(phone),0)==-1) {
                                        perror("phone send");
                                        exit(1);
                                }
				
				// 사용자에게 취소할 날짜 받고 server에 전송
				int month;
				int date;
				int time;

				printf("취소하고자 하는 날짜와 시간을 입력해주세요\n");
				printf("월: ");
				scanf("%d", &month);

                        	char send_month[3];
                        	send_month[2]='\0';
                        	sprintf(send_month, "%d", month);

				// 서버에게 월 전송
	                       	if(send(sd, send_month, sizeof(256),0)==-1) {
                        	        perror("day send");
                        	        exit(1);
                        	}

                        	printf("일: ");
                        	scanf("%d", &date);

                        	char send_date[3];
                        	send_date[2]='\0';
                        	sprintf(send_date, "%d", date);

				// 서버에게 일 전송
                        	if(send(sd, send_date, sizeof(256),0)==-1) {
                        	        perror("date send");
                        	        exit(1);
                        	}
				printf("ex) 14~15시의 예약을 취소하고자 한다면 14를 입력해주세요.\n");
				printf("시간: ");
                                scanf("%d", &time);

                                char send_time[3];
                                send_date[2]='\0';
                                sprintf(send_time, "%d", time);

				// 서버에게 시간대 전송
                                if(send(sd, send_time, sizeof(256),0)==-1) {
                                        perror("time send");
                                        exit(1);
                                }

				printf("(예약취소 완료)");
				break;
			}

		
		case 4: 
			{	
				/*리뷰 작성*/

				int star; // 별점
				printf("[리뷰등록]\n");
				printf("별점 (1점~5점): ");
				
				// 별점 받기
				scanf("%d", &star);
				getchar();

				char send_star[3];
                                send_star[2]='\0';
                                sprintf(send_star, "%d", star);

				// 서버에게 별점 전송
                                if(send(sd, send_star, sizeof(256),0)==-1) {
                                        perror("star send");
                                        exit(1);
                                }

				char review[256]; // 리뷰 메세지
				printf("리뷰을 작성해주세요: ");
				scanf("%[^\n]s", review);

				// 서버에게 리뷰 메세지 전송
                                if (send(sd, review, sizeof(review),0)==-1) {
                                        perror("review send");
                                        exit(1);
                                }
				printf("(리뷰가 등록되었습니다)\n");
				break;	
			}


		case 5: 	
			{
				/*리뷰 목록 확인*/
	
				//리뷰 횟수 server로부터 받기
                                if (recv(sd, bufr2, sizeof(bufr2), 0)==-1) {
                                        perror("review receive");
                                        exit(1);
                                }
                                int review_count;
                                review_count = atoi(bufr2);
                                printf("리뷰 총 %d개 입니다.\n", review_count);

				// 리뷰 횟수 만큼 서버로부터 리뷰 메세지를 받기				
				for(int i=0;i<review_count; i++) {
                                     
                                        if (recv(sd, bufr2, sizeof(bufr2), 0)==-1) {
                                                perror("reservation receive");
                                                exit(1);
                                        }
					printf("(%d) %s\n", (i+1), bufr2);	
					sleep(1);
				}

				break;
			}


		case 6: 		
			{
				/*별점 확인*/

				printf("잠시만 기다려 주세요.....\n");
				sleep(2);	
				//별점 평균 server로부터 받기		
				if (recv(sd, bufr2, sizeof(bufr2), 0)==-1) {
	                                        perror("review receive");
	                                        exit(1);
	                        }

	                        int star_avg; //별점 평균
	                        star_avg = atoi(bufr2); // 자료형을 정수로 변경
	                        printf("평점 평균은 %d입니다.\n", star_avg);
			}

		default:
			break;

	close(sd);
	}
}
