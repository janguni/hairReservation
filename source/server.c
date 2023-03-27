#include <sys/mman.h>
#include <signal.h>
#include <sys/shm.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sqlite3.h>

#define SNAME "hbsocket"

int time_table[] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}; //예약현황 배열

void handler(int dummy) {
	;
}

struct day{ 
	int month;
	int date;
	int time;
};

int main() {
	char buf[256];
	struct sockaddr_un server, client;
	int sd, nsd, len;
	socklen_t clen;
	int retval;
	int member_id;	
	sqlite3* db;
	sqlite3_stmt* res;
	char *err_msg = 0;	
	char* sql;
	int rc = sqlite3_open("hairReservation.db", &db); // db open
	if (rc != SQLITE_OK){
		printf("db open error\n");
		exit(1);
	}
		
	unlink("hbsocke"); // 마지막 글자가 빠지는 경우가 있어서 추가했음 
	unlink(SNAME); // 소캣 파일 삭제
 
	// 소캣 생성
	if ((sd = socket(AF_UNIX, SOCK_STREAM,0)) == -1) {
		perror("create socket");
		exit(1);
	}
	memset((char *)&server, 0, sizeof(struct sockaddr_un));
	server.sun_family = AF_UNIX;
	strcpy(server.sun_path, SNAME);
	len = sizeof(server.sun_family) + strlen(server.sun_path);

	if(bind(sd, (struct sockaddr *)&server, len)) {
		perror("bind");	
		exit(1);
	}

	if (listen(sd, 5)<0){
		perror("listen");
		exit(1);
	}

	printf("Waiting...\n");
	if ((nsd = accept(sd, (struct sockaddr *)&client, &clen)) == -1) {
		perror("accept");
		exit(1);
	}

	if (recv(nsd, buf, sizeof(buf), 0)== -1) {
		perror("recv");
		exit(1);
	}

	printf("Received Message: %s\n", buf);

	
	// client에게 메뉴 번호 받기
	// 1. 예약하기	
	// 2. 예약 내역 확인	
	// 3. 예약 취소	
	// 4. 리뷰 등록
	// 5. 리뷰목록 확인
	// 6. 별점확인
	

	key_t key;
	int shmid;
	void *shmaddr;
	sigset_t mask;
	char buf3[1024];
	int menu;

	// 공유 메모리 생성
	key = ftok("hair", 23);
	shmid = shmget(key, 1024, IPC_CREAT|0666);
	
	// SIGUSR1, SIGKILL을 제외한 모든 시그널 블로킹	
	sigfillset(&mask);
	sigdelset(&mask, SIGUSR1);
	sigdelset(&mask, SIGKILL);
	signal(SIGUSR1, handler);
	
	printf("client로 부터 시그널을 기다림..\n");
	sigsuspend(&mask); // 시그널을 받을 때까지 대기
	shmaddr = shmat(shmid, NULL, 0);
	strcpy(buf3,shmaddr);
	menu = atoi(buf3); // 메뉴 번호를 정수형으로 전환

	shmdt(shmaddr); // 공유 메모리 해제

        printf("menu number: %d\n", menu);
	 
	
	// 서비스 수행
	switch (menu) {

		/* 예약하기*/
		case 1: 
			{
				// 사용자 정보 받기
				char name[6];
				char phone[12];
				
				// 이름 받기
				retval = recv(nsd, buf, sizeof(buf),0);
				if (retval == -1) {
        	        		perror("member name recv");
        	        		exit(1);
        			}
				strcpy(name,buf);
	
				// 핸드폰 번호받기
				retval = recv(nsd, buf, sizeof(buf),0);
        			if (retval == -1) {
        	        		perror("member phone recv");
        	        		exit(1);
        			}
				strcpy(phone,buf);


				// 기존 사용자인지 새로운 사용자인지 확인
				// 해당 이름과 핸드폰 번호를 가진 사용자가 1명 이상이면 기존 사용자
				// 0명이면 새로운 사용자로 구분
				sql = "select count(*) from member where member_name = ? and member_phone=?;";
				rc = sqlite3_prepare_v2(db, sql, -1, &res, 0);
				if (rc != SQLITE_OK) {
        	        	        fprintf(stderr, "SQL error: %s\n", err_msg);
        	        	        sqlite3_free(err_msg);
        	        	        sqlite3_close(db);
        	        	}
        	        	sqlite3_bind_text(res,1,name, -1, SQLITE_STATIC); //sql문에 이름을 바인딩
        	        	sqlite3_bind_text(res,2,phone, -1, SQLITE_STATIC); //sql문에 핸드폰 번호 바인딩

        	        	rc = sqlite3_step(res); // sql문 실행

				int ismemberExist = sqlite3_column_int(res,0);
				
				if (ismemberExist==0) { // 새로운 회원일 때 
					// db에 저장
        				sql = "INSERT INTO member(member_name, member_phone) VALUES (?,?);";
        				rc = sqlite3_prepare_v2(db, sql, -1, &res, 0);
        				if (rc != SQLITE_OK) {
        	       				fprintf(stderr, "SQL error: %s\n", err_msg);
        	        			sqlite3_free(err_msg);
        	        			sqlite3_close(db);
        				}
					sqlite3_bind_text(res,1,name, -1, SQLITE_TRANSIENT);
					sqlite3_bind_text(res,2,phone, -1, SQLITE_STATIC);
					sqlite3_step(res);
					member_id = sqlite3_last_insert_rowid(db); // 방금 저장한 사용자의 id값
				}
		
				else { // 기존 사용자 일 때
					// 사용자의 id를 찾음
		                	sql = "select member_id from member where member_name = ? and member_phone=?;";
		                	rc = sqlite3_prepare_v2(db, sql, -1, &res, 0);
		                	if (rc != SQLITE_OK) {
		                        	fprintf(stderr, "SQL error: %s\n", err_msg);
		                        	sqlite3_free(err_msg);
		                        	sqlite3_close(db);
		                	}
		                	sqlite3_bind_text(res,1,name, -1, SQLITE_STATIC);
		                	sqlite3_bind_text(res,2,phone, -1, SQLITE_STATIC);
		                	rc = sqlite3_step(res);
					member_id = sqlite3_column_int(res,0);
				}
				

				// 예약할 날짜 받기

				// 월 받기
				retval = recv(nsd, buf, sizeof(buf),0);
				if (retval == -1) {
					perror("day recv");
					exit(1);
				}
				int month;
				month = atoi(buf);
				printf("월: %d\n", month);
				
				// 일 받기			
				retval = recv(nsd, buf, sizeof(buf), 0);
				if (retval == -1) {
					perror("date recv");
					exit(1);
				}
				int date;
				date = atoi(buf);
				printf("일: %d\n", date);
				
				// 해당 날짜에 예약 현황 보내주기		
				sql = "select reser_time from reservation where reser_month = ? and reser_date = ?;";
				rc = sqlite3_prepare_v2(db, sql, -1, &res, 0);
				if (rc != SQLITE_OK) {
		       	        	fprintf(stderr, "SQL error: %s\n", err_msg);
		       	        	sqlite3_free(err_msg);
		       	        	sqlite3_close(db);
		       		}
		       		sqlite3_bind_int(res,1, month);
		       	 	sqlite3_bind_int(res,2, date);
				
				// 해당 날짜에 예약된 시간이 존재하면 time_table의 배열에서 해당 시간을 0에서 1로 바꾸기 (index=9 이면 9시를 뜻함)
				while ((rc = sqlite3_step(res)) == SQLITE_ROW) {
					int time = sqlite3_column_int(res,0);
					time_table[time] = 1;
				}
				
				// 예약현황 보내주기
				if(send(nsd, time_table, sizeof(time_table),0)==-1) {
					perror("time send");
					exit(1);
				}

				memset(buf, '\0', sizeof(buf));


				// 예약 시간 받기
				retval = recv(nsd, buf, sizeof(buf),0);
			        if (retval == -1) {
		        	        perror("time recv");
		                	exit(1);
		        	}
		        	int time;
		        	time = atoi(buf);
		        	printf("시간: %d\n", time);
			
				// db에 저장
			        sql = "INSERT INTO reservation(reser_month, reser_date, reser_time, member_id) VALUES (?,?,?,?);";
        			rc = sqlite3_prepare_v2(db, sql, -1, &res, 0);
        			if (rc != SQLITE_OK) {
        			        fprintf(stderr, "SQL error: %s\n", err_msg);
        		        	sqlite3_free(err_msg);
        		        	sqlite3_close(db);
        			}
        			sqlite3_bind_int(res,1, month);
        			sqlite3_bind_int(res,2, date);
				sqlite3_bind_int(res,3, time);
				sqlite3_bind_int(res,4, member_id);
        			sqlite3_step(res);
				printf("[예약] %s님 %d월 %d일 %d~%d\n", name, month, date, time, time+1);
		
				break;
			}


		case 2:
			/* 예약 내역 확인*/

			{
                                // 사용자 정보 받기
                                char name[6];
                                char phone[12];
				
				// 이름 받기
                                retval = recv(nsd, buf, sizeof(buf),0);
                                if (retval == -1) {
                                        perror("member name recv");
                                        exit(1);
                                }
                                strcpy(name,buf);
                                
				// 핸드폰 번호 받기
				retval = recv(nsd, buf, sizeof(buf),0);
                                if (retval == -1) {
                                        perror("member phone recv");
                                        exit(1);
                                }
                                strcpy(phone,buf);

				/*
				// 기존 사용자인지 확인
				sql = "select count(*) from member where member_name = ? and member_phone=?;";
                                rc = sqlite3_prepare_v2(db, sql, -1, &res, 0);
                                if (rc != SQLITE_OK) {
                                        fprintf(stderr, "SQL error: %s\n", err_msg);
                                        sqlite3_free(err_msg);
                                        sqlite3_close(db);
                                }
                                sqlite3_bind_text(res,1,name, -1, SQLITE_STATIC);
                                sqlite3_bind_text(res,2,phone, -1, SQLITE_STATIC);
                                rc = sqlite3_step(res);
                                int ismemberExist = sqlite3_column_int(res,0);
				*/
				
				
                                // 사용자 id 확인
                                sql = "select member_id from member where member_name = ? and member_phone = ?;";
                                rc = sqlite3_prepare_v2(db, sql, -1, &res, 0);
                                if (rc != SQLITE_OK) {
                                        fprintf(stderr, "SQL error: %s\n", err_msg);
                                        sqlite3_free(err_msg);
                                        sqlite3_close(db);
                                }
                                sqlite3_bind_text(res,1,name, -1, SQLITE_STATIC);
                                sqlite3_bind_text(res,2,phone, -1, SQLITE_STATIC);
                                rc = sqlite3_step(res);
                                member_id = sqlite3_column_int(res,0);
				
				printf("member_id: %d\n",member_id);

				
				//예약 횟수 구하기
				sql = "select count(*) from reservation where member_id = ?;";
                                rc = sqlite3_prepare_v2(db, sql, -1, &res, 0);
                                if (rc != SQLITE_OK) {
                                        fprintf(stderr, "SQL error: %s\n", err_msg);
                                        sqlite3_free(err_msg);
                                        sqlite3_close(db);
                                }
                                sqlite3_bind_int(res,1,member_id);
                                rc = sqlite3_step(res);
                                int reser_count = sqlite3_column_int(res,0);
				
				char send_reser_count[3];
				send_reser_count[2]='\0';
				sprintf(send_reser_count, "%d", reser_count); 
				if(send(nsd, send_reser_count, sizeof(send_reser_count),0)==-1) {
                                        perror("reservation count send");
                                        exit(1);
                                }


				// 예약내역 client에게 전송	
				sql = "select reser_month, reser_date, reser_time from reservation where member_id = ?;";
                                rc = sqlite3_prepare_v2(db, sql, -1, &res, 0);
                                if (rc != SQLITE_OK) {
                                        fprintf(stderr, "SQL error: %s\n", err_msg);
                                        sqlite3_free(err_msg);
                                        sqlite3_close(db);
                                }
                                sqlite3_bind_int(res,1, member_id);
                                
				char send_date[3];
                                while ((rc = sqlite3_step(res)) == SQLITE_ROW) {
                                        int month = sqlite3_column_int(res,0);
                                        int date = sqlite3_column_int(res,1);
					int time = sqlite3_column_int(res,2);
                                	printf("%d님은 %d월 %d일 %d시에 예약함.\n", member_id, month, date, time);
					struct day d = {month, date, time};
					
					sleep(1);
					if (send(nsd, (struct day*)&d, sizeof(d),0)== -1) {
						perror("dat send");
						exit(1);
					}
                                }
				break;
				
			}


		case 3: 
			/* 예약 취소*/
	
			{
				// 사용자 정보 받기
                                char name[6];
                                char phone[12];
				
				// 이름 받기
                                retval = recv(nsd, buf, sizeof(buf),0);
                                if (retval == -1) {
                                        perror("member name recv");
                                        exit(1);
                                }
                                strcpy(name,buf);
				
				// 핸드폰 번호 받기
                                retval = recv(nsd, buf, sizeof(buf),0);
                                if (retval == -1) {
                                        perror("member phone recv");
                                        exit(1);
                                }
                                strcpy(phone,buf);


                                // 사용자 id 확인
                                sql = "select member_id from member where member_name = ? and member_phone = ?;";
                                rc = sqlite3_prepare_v2(db, sql, -1, &res, 0);
                                if (rc != SQLITE_OK) {
                                        fprintf(stderr, "SQL error: %s\n", err_msg);
                                        sqlite3_free(err_msg);
                                        sqlite3_close(db);
                                }
                                sqlite3_bind_text(res,1,name, -1, SQLITE_STATIC);
                                sqlite3_bind_text(res,2,phone, -1, SQLITE_STATIC);
                                rc = sqlite3_step(res);
                                member_id = sqlite3_column_int(res,0);

                                printf("member_id: %d\n",member_id);

				// 취소할 월/일/시간 받기	
				int retval;
				retval = recv(nsd, buf, sizeof(buf),0);
                                if (retval == -1) {
                                        perror("day recv");
                                        exit(1);
                                }
                                int month;
                                month = atoi(buf);
                                printf("월: %d\n", month);

                                retval = recv(nsd, buf, sizeof(buf), 0);
                                if (retval == -1) {
                                        perror("date recv");
                                        exit(1);
                                }
                                int date;
                                date = atoi(buf);
                                printf("일: %d\n", date);
				
				retval = recv(nsd, buf, sizeof(buf), 0);
                                if (retval == -1) {
                                        perror("time recv");
                                        exit(1);
                                }
                                int time;
                                time = atoi(buf);
                                printf("시간: %d\n", time);	                
				

				//db에서 해당 예약건 삭제		
				sql = "delete from reservation where reser_month = ? and reser_date = ? and reser_time = ? and member_id = ?;";
                                rc = sqlite3_prepare_v2(db, sql, -1, &res, 0);
                                if (rc != SQLITE_OK) {
                                        fprintf(stderr, "SQL error: %s\n", err_msg);
                                        sqlite3_free(err_msg);
                                        sqlite3_close(db);
                                }
                                sqlite3_bind_int(res,1, month);
                                sqlite3_bind_int(res,2, date);
                                sqlite3_bind_int(res,3, time);
                                sqlite3_bind_int(res,4, member_id);
                                rc = sqlite3_step(res);
				printf("%d님 예약 취소 완료\n", member_id);				
				break;
			}


                case 4:
			/* 리뷰 등록*/

			{
				// 별점 받기		
				int retval;
                                retval = recv(nsd, buf, sizeof(buf),0);
                                if (retval == -1) {
                                        perror("star recv");
                                        exit(1);
                                }
                                int star;
                                star = atoi(buf);
                                printf("별점: %d\n", star);
				
				// 리뷰 받기	
				sleep(15);	
				char review[256];
                                retval = recv(nsd, buf, sizeof(buf),0);
                                if (retval == -1) {
                                        perror("review recv");
                                        exit(1);
                                }
                                strcpy(review,buf);
				printf("review: %s\n", review);

				//db에 리뷰 저장
				sql = "INSERT INTO review(review_star, review_text) VALUES (?,?);";
                                rc = sqlite3_prepare_v2(db, sql, -1, &res, 0);
                                if (rc != SQLITE_OK) {
                                	fprintf(stderr, "SQL error: %s\n", err_msg);
                                        sqlite3_free(err_msg);
                                        sqlite3_close(db);
                                }
                                sqlite3_bind_int(res,1, star);
                                sqlite3_bind_text(res,2,review, -1, SQLITE_TRANSIENT);
                                sqlite3_step(res);
				
				printf("리뷰가 등록되었습니다.\n");
				break;                                       
			}               


		case 5: 
			/*리뷰 목록 확인*/
		
			{
				//리뷰 총 횟수 구하기
                                sql = "select count(*) from review;";
                                rc = sqlite3_prepare_v2(db, sql, -1, &res, 0);
                                if (rc != SQLITE_OK) {
                                        fprintf(stderr, "SQL error: %s\n", err_msg);
                                        sqlite3_free(err_msg);
                                        sqlite3_close(db);
                                }
                              
                                rc = sqlite3_step(res);
                                int review_count = sqlite3_column_int(res,0);

                                char send_review_count[3];
                                send_review_count[2]='\0';
                                sprintf(send_review_count, "%d", review_count);
                                if(send(nsd, send_review_count, sizeof(send_review_count),0)==-1) {
                                        perror("review count send");
                                        exit(1);
                                }

				// 리뷰 client에게 전송
                                sql = "select review_text from review;";
                                rc = sqlite3_prepare_v2(db, sql, -1, &res, 0);
                                if (rc != SQLITE_OK) {
                                        fprintf(stderr, "SQL error: %s\n", err_msg);
                                        sqlite3_free(err_msg);
                                        sqlite3_close(db);
                                }
                               
				char* reviews;
                                while ((rc = sqlite3_step(res)) == SQLITE_ROW) {
                                        sleep(1);
					//reviews = sqlite3_column_text(res,0);
                                 	if(send(nsd, sqlite3_column_text(res,0), 256,0)==-1) {
                                                perror("review send");
                                                exit(1);
                                        }

                                           
                                }
				break;
			}
	

		case 6: 
			/*별점 확인*/

			{	
				int star_sum=0; // 별점의 합계
				int star_count=0; // 별점(리뷰) 갯수
				int star_avg; // 평균 별점

				
				sql = "select review_star from review;";
                                rc = sqlite3_prepare_v2(db, sql, -1, &res, 0);
                                if (rc != SQLITE_OK) {
                                        fprintf(stderr, "SQL error: %s\n", err_msg);
                                        sqlite3_free(err_msg);
                                        sqlite3_close(db);
                                }
				
				// 별점 평균 구하기
                                while ((rc = sqlite3_step(res)) == SQLITE_ROW) {
                                        star_sum+=sqlite3_column_int(res,0);
					star_count+=1;       
                                }

				sleep(1);
				star_avg = star_sum/star_count;
				printf("별점 평균은: %d\n", star_avg);
				char send_star_avg[3];
				send_star_avg[2] = '\0';
				sprintf(send_star_avg, "%d", star_avg);
				
				// 별점 client에게 전송
				if(send(nsd,send_star_avg, sizeof(send_star_avg),0)== -1) {
					perror("star send");
					exit(1);
				}
				break;
	
			}	
		
		default:
			break;
	}
		
	sqlite3_close(db);
	
	close(nsd);
	close(sd);
}

