#ifndef WEBSERVER_H
#define WEBSERVER_H
#define _POSIX_C_SOURCE 199309L

#include <sys/types.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <microhttpd.h>
#include <netinet/in.h>
#include <stdio.h>
#include <signal.h>
#include <time.h>
#include <string.h>
#include <sqlite3.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <malloc.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>
#include <errno.h>
#include <base64.h>

#define UNUSED(x) (void)(x)

#define COOKIE_NAME "session"
#define WEBSERVER_DATABASE "webserver.db"

#define IP_QUERY "SELECT COUNT(country_id) FROM ip WHERE ? BETWEEN ip_range_start AND ip_range_end AND country_id=\"AU\""
#define VALIDATE_COOKIE_QUERY "SELECT COUNT(cookie) FROM sessions WHERE cookie == ?"
#define COOKIE_INSERT_QUERY "INSERT INTO sessions (cookie) VALUES (?)"

#define MAIN_PAGE "main_page.html"
#define CHALLENGE_PAGE "challenge_page.html"
#define SECRET_PAGE "secret_page.html"
#define ERROR_PAGE "error_page.html"
#define LOGO "img/logo.ico"
#define THEME "img/theme.jpg"
#define CLOCK_JS "flipclock.min.js"
#define CLOCK_CSS "flipclock.css"

#define PORT 8888
#define CHALLENGE_SIZE 64
#define COOKIE_I_SIZE 48
#define TIME_LIMIT 15 // in seconds
#define COOKIE_LIFETIME 300 //in seconds
#define CONNECTION_TIMEOUT 60 //in seconds
#define POSTBUFFERSIZE 256
#define COOKIEBUFFERSIZE 128
#define MAX_SESSION_COUNT 256 //the maximum number of concurrent sessions
#define LOCAL_BUF_SIZE 8

#define TEXT "text"
#define FORM_NAME "response"

#define URI_BASE "/"
#define URI_SECRET "/secret"
#define URI_CHALLENGE "/challenge"
#define URI_FAVICON "/favicon.ico"
#define URI_THEME "/theme.jpg"
#define URI_CLOCK_JS "/clock.js"
#define URI_CLOCK_CSS "/clock.css"

#define GET "GET"
#define POST "POST"
#define HEAD "HEAD"
#define AJAX_HEADER "ajax-parameter"


typedef unsigned long long uint64;

struct session
{
    char cookies[CHALLENGE_SIZE + 1];
    char challenge[CHALLENGE_SIZE + 1];
    char user_answer[CHALLENGE_SIZE + 1];
    time_t challenge_time;
    int recently_used; //define if we should delete this session on the next iteration
    int authorized;
    char* ajax_parameter;
    char* encoded_challenge;
};

struct connection_info
{
    struct MHD_PostProcessor* postprocessor;
    struct session* session;
};

struct page_struct
{
    char* page;
    size_t size;
};
enum pages{main_page, challenge_page, secret_page, error_page, favicon, theme, clock_js, clock_css};
#define PAGE_COUNT 8

int main();
int policy_callback(void* cls, const struct sockaddr* addr, socklen_t addrlen);
void reset_cookies(union sigval);
int answer_to_connection(void* cls, struct MHD_Connection* connection, const char* url, const char* method,
    const char *version, const char *upload_data,size_t *upload_data_size, void **con_cls);
void graceful_quit(int signum);
void request_completed_callback(void* cls, struct MHD_Connection* connection, void** con_cls, enum MHD_RequestTerminationCode toe);
int post_iterator(void* cls, enum MHD_ValueKind kind, const char* key, const char* filename, const char* content_type,
                  const char* transfer_encoding, const char* data, uint64_t off, size_t size);
int send_page(struct MHD_Connection* connection, int status_code, struct page_struct html_page, int count, char* new_cookie, ...);
int validate_cookies(const char* cookie);
int check(struct session* session);
int create_challenge(char* buf);
char* create_cookies_db(char* cookie);
void load_page(char* file, struct page_struct* ptr);
extern int challenge(char*, char*);
struct session* find_create_session(struct MHD_Connection *connection, char** pnew_cookie);
int fill_random_buf(char* buf, unsigned int buf_size);
struct session* create_new_session(char** pnew_cookie);

#endif // WEBSERVER_H

