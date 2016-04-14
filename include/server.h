#ifndef  __server_h
#define  __server_h

#include   <stdio.h>
#include   <stdlib.h>
#include   <sys/types.h>
#include   <sys/socket.h>
#include   <sys/time.h>
#include   <arpa/inet.h>
#include   <errno.h>
#include   <fcntl.h>
#include   <signal.h>
#include   <unistd.h>
#include   <string.h>
#include   <pthread.h>
#include   <sys/types.h>
#include   <sys/wait.h>
#include   <syslog.h>
#include   <sys/ioctl.h>
#include <openssl/md5.h>




#define MAX_REQUESTS 10000
#define MAX_WORKER   100
#define SERV_PORT    5678
#define SERV_IP      "45.62.113.14"
#define PROXY_PORT   6789
#define PROXY_IP     "45.62.113.14"
#define MESG_LEN     100
#define	MAXLINE	     4096	/* max text line length */
#define	BUFSIZE	     1024	/* buffer size for reads and writes */

#define	min(a,b)	((a) < (b) ? (a) : (b))
#define	max(a,b)	((a) > (b) ? (a) : (b))

#define SA struct sockaddr        


typedef enum {GET, REGET, RGST, UPDT} mesg_type;

typedef struct {
    mesg_type type;
    char data[16];
} request_t;

typedef struct {
    uint16_t ack;
    char data[16];
} response_t;

typedef struct {
    mesg_type type;
} get;

typedef struct {
    uint16_t ack;
    uint16_t port;
    uint32_t addr;
} get_ack;

typedef struct  {
    mesg_type type;
    uint16_t port;
    uint32_t addr;
} reget;

typedef struct  {
    uint16_t ack;
    uint16_t port;
    uint32_t addr;
} reget_ack;

typedef struct {
    mesg_type type;
    uint16_t port;
    uint32_t addr;
} regist; 

typedef struct {
    uint16_t ack;
    uint32_t index;
    uint32_t passwd;
} regist_ack; 

typedef struct {
    mesg_type type;
    uint32_t index;
    uint32_t passwd;
    int load;
} update;

typedef struct {
    uint16_t ack;
} update_ack; 

typedef struct {
    const char *domain;
    const char *id;
    uint16_t   port;
    uint32_t   addr;
} server_conf;

#define DEFAULT_NWORKER 20
typedef struct {
    int passwd;
    int index;
    int load;
    int status;
    uint16_t port;
    uint32_t addr;
} worker; 

#define WKR_FLD 0
#define WKR_RDY 1
#define WKR_OVL 2

int Close(int );
void* Calloc(size_t, size_t);
void* Realloc(void *, size_t);
pid_t Fork(void);   


			/* prototypes for our stdio wrapper functions: see {Sec errors} */
void	 Fclose(FILE *);
FILE	*Fdopen(int, const char *);
char	*Fgets(char *, int, FILE *);
FILE	*Fopen(const char *, const char *);
void	 Fputs(const char *, FILE *);

			/* prototypes for our socket wrapper functions: see {Sec errors} */
int      Accept(int, struct sockaddr *, socklen_t *);
void	 Bind(int, const struct sockaddr *, socklen_t);
void	 Connect(int, const struct sockaddr *, socklen_t);
void	 Getpeername(int, struct sockaddr *, socklen_t *);
void	 Getsockname(int, struct sockaddr *, socklen_t *);
void	 Getsockopt(int, int, int, void *, socklen_t *);
void	 Listen(int, int);
ssize_t	 Recv(int, void *, size_t, int);
ssize_t	 Recvfrom(int, void *, size_t, int, struct sockaddr *, socklen_t *);
ssize_t	 Recvmsg(int, struct msghdr *, int);
int		 Select(int, fd_set *, fd_set *, fd_set *, struct timeval *);
void	 Send(int, const void *, size_t, int);
void	 Sendto(int, const void *, size_t, int, const struct sockaddr *, socklen_t);
void	 Sendmsg(int, const struct msghdr *, int);
void	 Setsockopt(int, int, int, const void *, socklen_t);
void	 Shutdown(int, int);
int		 Sockatmark(int);
int		 Socket(int, int, int);
void	 Socketpair(int, int, int, int *);

void	 err_dump(const char *, ...);
void	 err_msg(const char *, ...);
void	 err_quit(const char *, ...);
void	 err_ret(const char *, ...);
void	 err_sys(const char *, ...);

			/* prototypes for our pthread wrapper functions: see {Sec wrappthead} */

void	Pthread_create(pthread_t *, const pthread_attr_t *,
					   void * (*)(void *), void *);
void	Pthread_join(pthread_t, void **);
void	Pthread_detach(pthread_t);
void	Pthread_kill(pthread_t, int);

void	Pthread_mutexattr_init(pthread_mutexattr_t *);
void	Pthread_mutexattr_setpshared(pthread_mutexattr_t *, int);
void	Pthread_mutex_init(pthread_mutex_t *, pthread_mutexattr_t *);
void	Pthread_mutex_lock(pthread_mutex_t *);
void	Pthread_mutex_unlock(pthread_mutex_t *);

void	Pthread_cond_broadcast(pthread_cond_t *);
void	Pthread_cond_signal(pthread_cond_t *);
void	Pthread_cond_wait(pthread_cond_t *, pthread_mutex_t *);
void	Pthread_cond_timedwait(pthread_cond_t *, pthread_mutex_t *,
							   const struct timespec *);

void	Pthread_key_create(pthread_key_t *, void (*)(void *));
void	Pthread_setspecific(pthread_key_t, const void *);
void	Pthread_once(pthread_once_t *, void (*)(void));


#endif
