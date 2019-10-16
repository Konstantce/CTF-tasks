#include <webserver.h>
#include <base64.h>

struct MHD_Daemon *httpdaemon;
struct session* sessions[MAX_SESSION_COUNT];
struct page_struct pages[PAGE_COUNT];

void load_page(char* file, struct page_struct* ptr)
{
    FILE* fd = fopen(file, "rb");
    if (!fd)
    {
        perror("File opening error: ");
        printf("%s\n", file);
        raise(SIGQUIT);
    }
    fseek(fd, 0, SEEK_END);
    size_t size = ftell(fd);
    fseek(fd, 0, SEEK_SET);
    ptr->page = (char*)malloc(size + 1);
    if (fread(ptr->page, 1, size, fd) != size)
    {
        perror("File opening error: ");
        raise(SIGQUIT);
    }
    ptr->size = size;
}

int main()
{
    struct sigevent event;
    memset(&event, 0, sizeof(event));
    event.sigev_notify = SIGEV_THREAD;
    event.sigev_notify_function = reset_cookies;
    timer_t timerid;
    timer_create(CLOCK_REALTIME, &event, &timerid);

    struct sigaction sig_manager;
    sig_manager.sa_handler = graceful_quit;
    sigfillset(&sig_manager.sa_mask);
    sigaction(SIGQUIT,&sig_manager,NULL);

    load_page(MAIN_PAGE, &(pages[main_page]));
    load_page(CHALLENGE_PAGE, &pages[challenge_page]);
    load_page(ERROR_PAGE, &pages[error_page]);
    load_page(LOGO, &pages[favicon]);
    load_page(THEME, &pages[theme]);
    load_page(CLOCK_JS, &pages[clock_js]);
    load_page(CLOCK_CSS, &pages[clock_css]);

    httpdaemon = MHD_start_daemon (MHD_USE_EPOLL_INTERNALLY_LINUX_ONLY| MHD_USE_EPOLL_LINUX_ONLY | MHD_USE_TCP_FASTOPEN, PORT, &policy_callback, NULL,
                                   &answer_to_connection, NULL, MHD_OPTION_PER_IP_CONNECTION_LIMIT, 6, MHD_OPTION_NOTIFY_COMPLETED,
                                   request_completed_callback, NULL, MHD_OPTION_END);
    if (!httpdaemon)
        return 1;
    getchar ();
    raise(SIGQUIT);
    return 0;
}

void graceful_quit(int signum)
{
    UNUSED(signum);
    if (httpdaemon)
        MHD_stop_daemon(httpdaemon);

    for(int i=0; i<PAGE_COUNT; i++)
        free(pages[i].page);
    for(int i=0; i<MAX_SESSION_COUNT; i++)
        free(sessions[i]);

    base64_cleanup();
    exit(0);
}

void reset_cookies(union sigval val)
{
    UNUSED(val);
    for(int i=0; i<MAX_SESSION_COUNT; i++)
        if (sessions[i])
        {
            if (sessions[i]->recently_used)
                sessions[i]->recently_used = 0;
            else
            {
                free(sessions[i]);
                sessions[i] = NULL;
            }
        }
}

int policy_callback(void* cls, const struct sockaddr* addr, socklen_t addrlen)
{
    /*UNUSED(cls); UNUSED(addrlen);
    struct sockaddr_in* psockaddr_in = (struct sockaddr_in*)addr;
    unsigned long ipv4 = psockaddr_in->sin_addr.s_addr;
    char* str_address = inet_ntoa(psockaddr_in->sin_addr);
    printf("New client has been arrived: %s %d\n", str_address, ntohs(psockaddr_in->sin_port));

    sqlite3* db;
    sqlite3_stmt* stmt;
    const char* errmsg;
    int result;
    if (sqlite3_open(WEBSERVER_DATABASE, &db) || sqlite3_prepare_v2(db, IP_QUERY, -1, & stmt, NULL) || sqlite3_bind_int64(stmt, 1, ipv4) ||
            (sqlite3_step(stmt) != SQLITE_ROW))
    {
        errmsg = sqlite3_errmsg(db);
        printf("Sqlite3 error: %s\n", errmsg);
        raise(SIGQUIT);
    }
    result = sqlite3_column_int(stmt, 1);
    if (sqlite3_finalize(stmt) || sqlite3_close(db))
    {
        errmsg = sqlite3_errmsg(db);
        printf("Sqlite3 error: %s\n", errmsg);
        raise(SIGQUIT);
    }
    return result;*/
    return 1;
}

void request_completed_callback(void* cls, struct MHD_Connection* connection, void** con_cls, enum MHD_RequestTerminationCode toe)
{
    UNUSED(cls); UNUSED(toe); UNUSED(connection);
    struct connection_info* con_info = *con_cls;
    if (con_info)
    {
        if (con_info->postprocessor)
            MHD_destroy_post_processor(con_info->postprocessor);
        free(con_info);
    }
}

int post_iterator(void* cls, enum MHD_ValueKind kind, const char* key, const char* filename, const char* content_type,
                  const char* transfer_encoding, const char* data, uint64_t off, size_t size)
{
    UNUSED(kind); UNUSED(filename); UNUSED(transfer_encoding); UNUSED(off);
    struct connection_info* con_info =cls;
    if (strcmp(key, FORM_NAME) || strcmp(content_type, TEXT))
        return MHD_NO;
    if ((size) && (size <= CHALLENGE_SIZE))
    {
        memset(con_info->session->user_answer, 0, CHALLENGE_SIZE);
        memcpy(con_info->session->user_answer, data, size);
        return MHD_YES;
    }
    return MHD_NO;
}

int check(struct session* session)
{
    time_t current_time = time(NULL);
    int result = challenge(session->challenge, session->user_answer) && (current_time - session->challenge_time <= TIME_LIMIT);
    return result;
}

int validate_cookies(const char* cookie)
{
    sqlite3* db;
    sqlite3_stmt* stmt;
    const char* errmsg;
    if (sqlite3_open(WEBSERVER_DATABASE, &db) || sqlite3_prepare_v2(db, VALIDATE_COOKIE_QUERY, -1, &stmt, NULL) ||
            sqlite3_bind_text(stmt, 1, cookie, -1, NULL) || (sqlite3_step(stmt) != SQLITE_ROW))
    {
        errmsg = sqlite3_errmsg(db);
        printf("Sqlite3 error: %s\n", errmsg);
        raise(SIGQUIT);
    }
    int result = sqlite3_column_int(stmt, 1);
    if (sqlite3_finalize(stmt) || sqlite3_close(db))
    {
        errmsg = sqlite3_errmsg(db);
        printf("Sqlite3 error: %s\n", errmsg);
        raise(SIGQUIT);
    }
    return result;
}

int send_page(struct MHD_Connection* connection, int status_code, struct page_struct html_page, int count, char* new_cookie, ...)
{
    int ret;
    struct MHD_Response* response;
    response = MHD_create_response_from_buffer(html_page.size, (void*)html_page.page, MHD_RESPMEM_PERSISTENT);
    if (!response)
        return MHD_NO;

    if (new_cookie)
    {
        char* cookies = (char*)malloc(COOKIEBUFFERSIZE);
        snprintf(cookies,COOKIEBUFFERSIZE, "%s=%s", COOKIE_NAME, new_cookie);
        MHD_add_response_header(response, MHD_HTTP_HEADER_SET_COOKIE, cookies);
        free(cookies);
    }

    if (count)
    {
        va_list ap;
        va_start(ap, new_cookie);
        int i;
        for(i=0; i<count; i++)
        {
            char* header = va_arg(ap, char*);
            char* value = va_arg(ap, char*);
            MHD_add_response_header(response, header, value);
        }
        va_end(ap);
    }

    ret = MHD_queue_response(connection, status_code, response);
    MHD_destroy_response(response);
    return ret;
}

int fill_random_buf(char* buf, unsigned int buf_size)
{
    FILE* fr = fopen("/dev/urandom", "r");
    if (!fr)
        return MHD_NO;
    memset(buf, 0, buf_size);
    if (fread(buf, sizeof(char), buf_size, fr) != buf_size)
        return MHD_NO;
    fclose(fr);
    return MHD_YES;
}

int create_challenge(char* buf)
{
    return fill_random_buf(buf, CHALLENGE_SIZE);
}

char* create_cookies_db(char* cookie)
{
    sqlite3* db;
    sqlite3_stmt* stmt;
    if (sqlite3_open(WEBSERVER_DATABASE, &db) || sqlite3_prepare_v2(db, COOKIE_INSERT_QUERY, -1, &stmt, NULL) ||
            (sqlite3_bind_text(stmt, 1, cookie, CHALLENGE_SIZE, NULL) || sqlite3_step(stmt) != SQLITE_DONE) || sqlite3_finalize(stmt) || sqlite3_close(db))
    {
        const char* errmsg = sqlite3_errmsg(db);
        printf("Sqlite3 error: %s\n", errmsg);
        raise(SIGQUIT);
    }
    return cookie;
}

struct session* create_new_session(char** pnew_cookie)
{
    for(int i=0; i<MAX_SESSION_COUNT; i++)
        if (!sessions[i])
        {
            char buf[COOKIE_I_SIZE];
            if (!fill_random_buf(buf, COOKIE_I_SIZE))
                return NULL;
            size_t output_len;
            char* _cookie = base64_encode((unsigned char*)buf, COOKIE_I_SIZE, &output_len);
            if (output_len != CHALLENGE_SIZE)
                return NULL;
            sessions[i] = calloc(sizeof(struct session), 1);
            if (!sessions[i])
                return NULL;
            memcpy(sessions[i]->cookies, _cookie, CHALLENGE_SIZE);
            sessions[i]->recently_used = 1;
            *pnew_cookie = sessions[i]->cookies;
            free(_cookie);
            return sessions[i];
        }
    return (struct session*)-1;
}

struct session* find_create_session(struct MHD_Connection *connection, char** pnew_cookies)
{
    const char* cookie = MHD_lookup_connection_value(connection, MHD_COOKIE_KIND, COOKIE_NAME);
    if (!cookie)
        return create_new_session(pnew_cookies);
    else
    {
        for(int i=0; i<MAX_SESSION_COUNT; i++)
            if (sessions[i] && !strcmp(sessions[i]->cookies, cookie))
            {
                sessions[i]->recently_used = 1;
                return sessions[i];
            }
        return create_new_session(pnew_cookies);
    }
}

int answer_to_connection (void *cls, struct MHD_Connection *connection, const char *url, const char *method,
    const char *version, const char *upload_data,size_t *upload_data_size, void **con_cls)
{
    UNUSED(version); UNUSED(cls);
    if (!*con_cls)
    {
        MHD_set_connection_option(connection, MHD_CONNECTION_OPTION_TIMEOUT, CONNECTION_TIMEOUT);
        struct connection_info* con_info = calloc(sizeof(struct connection_info), 1);
        if(!strcmp(method, POST))
            con_info->postprocessor = MHD_create_post_processor(connection, POSTBUFFERSIZE, post_iterator, (void*)con_info);
        *con_cls = (void*)con_info;

        return MHD_YES;
    }

    char local_buf[CHALLENGE_SIZE];
    strcpy(local_buf, "Target page is not found on the server: ");
    char* new_cookie = NULL;
    struct session* current_session = find_create_session(connection, &new_cookie);

    if (strcmp(url, URI_BASE) && strcmp(url, URI_SECRET) && strcmp(url, URI_CHALLENGE) && strcmp(url, URI_FAVICON) && strcmp(url, URI_THEME) &&
            strcmp(url, URI_CLOCK_CSS) && strcmp(url, URI_CLOCK_JS))
    {
        snprintf(local_buf+40, 24, url);
        current_session->ajax_parameter = "Target page is not found";
        return send_page(connection, MHD_HTTP_NOT_FOUND, pages[error_page], 0, new_cookie);
    }
    if (strcmp(method,GET) && strcmp(method, POST) && strcmp(method, HEAD))
    {
        current_session->ajax_parameter = "The method is not implemented";
        return send_page(connection, MHD_HTTP_NOT_IMPLEMENTED, pages[error_page], 0, new_cookie);
    }


    if (!strcmp(url, URI_SECRET) && (!strcmp(method, GET) || !strcmp(method, HEAD)))
    {
        if (current_session->authorized && validate_cookies(current_session->cookies))
        {
            load_page(SECRET_PAGE, &pages[secret_page]);
            int result = send_page(connection, MHD_HTTP_OK, pages[secret_page], 0, new_cookie);
            free(pages[secret_page].page);
            return result;
        }
        else
        {
            current_session->ajax_parameter = "This action is forbidden! You must pass the challenge first!";
            return send_page(connection, MHD_HTTP_FORBIDDEN, pages[error_page], 0, new_cookie);
        }
    }

    if (!strcmp(url, URI_BASE) && (!strcmp(method, GET) || !strcmp(method, HEAD)))
    {
        int param_count = current_session->ajax_parameter ? 1 : 0;
        int result = send_page(connection, MHD_HTTP_OK, pages[main_page], param_count, new_cookie, AJAX_HEADER, current_session->ajax_parameter);
        free(current_session->encoded_challenge);
        current_session->encoded_challenge = NULL;
        if (current_session->authorized)
            current_session->ajax_parameter = "TRUE";
        else
            current_session->ajax_parameter = "FALSE";
        return result;
    }

    char buf[LOCAL_BUF_SIZE];
    if (!strcmp(url, URI_FAVICON) && (!strcmp(method, GET) || !strcmp(method, HEAD)))
    {
        sprintf(buf, "%u", pages[favicon].size);
        return send_page(connection, MHD_HTTP_OK, pages[favicon], 2, new_cookie, MHD_HTTP_HEADER_CONTENT_LENGTH, buf,
                         MHD_HTTP_HEADER_CONTENT_TYPE, "image/vnd.microsoft.icon");
    }

    if (!strcmp(url, URI_THEME) && (!strcmp(method, GET) || !strcmp(method, HEAD)))
    {
        sprintf(buf, "%u", pages[theme].size);
        return send_page(connection, MHD_HTTP_OK, pages[theme], 2, new_cookie, MHD_HTTP_HEADER_CONTENT_LENGTH, buf,
                         MHD_HTTP_HEADER_CONTENT_TYPE, "image/jpeg");
    }

    if (!strcmp(url, URI_CLOCK_CSS) && (!strcmp(method, GET) || !strcmp(method, HEAD)))
    {
        sprintf(buf, "%u", pages[clock_css].size);
        return send_page(connection, MHD_HTTP_OK, pages[clock_css], 2, new_cookie, MHD_HTTP_HEADER_CONTENT_LENGTH, buf,
                         MHD_HTTP_HEADER_CONTENT_TYPE, "text/css");
    }

    if (!strcmp(url, URI_CLOCK_JS) && (!strcmp(method, GET) || !strcmp(method, HEAD)))
    {
        sprintf(buf, "%u", pages[clock_js].size);
        return send_page(connection, MHD_HTTP_OK, pages[clock_js], 2, new_cookie, MHD_HTTP_HEADER_CONTENT_LENGTH, buf,
                         MHD_HTTP_HEADER_CONTENT_TYPE, "text/javascript");
    }

    if (!strcmp(url, URI_CHALLENGE))
    {
        if (!strcmp(method, GET) || !strcmp(method, HEAD))
        {
            current_session->challenge_time = time(NULL);
            if (!create_challenge(current_session->challenge))
            {
                current_session->ajax_parameter = "Internal server error";
                return send_page(connection, MHD_HTTP_INTERNAL_SERVER_ERROR, pages[error_page], 0, new_cookie);
            }
            size_t output_len;
            current_session->encoded_challenge = (char*)base64_encode((const unsigned char*)current_session->challenge, CHALLENGE_SIZE, &output_len);
            int result = send_page(connection, MHD_HTTP_OK, pages[challenge_page], 0, new_cookie);
            current_session->ajax_parameter = current_session->encoded_challenge;
            return result;
        }
        else
            if (*upload_data_size)
            {
                struct connection_info* con_info = (struct connection_info*)con_cls;
                con_info->session = current_session;
                MHD_post_process(con_info->postprocessor, upload_data, *upload_data_size);
                *upload_data_size = 0;
                if (check(current_session))
                {
                    create_cookies_db(current_session->cookies);
                    current_session->authorized = 1;
                    return send_page(connection, MHD_HTTP_OK, pages[main_page], 1, new_cookie, AJAX_HEADER, "TRUE");
                }
                else
                    return send_page(connection, MHD_HTTP_OK, pages[main_page], 1, new_cookie, AJAX_HEADER, "FALSE");
            }
    }
    current_session->ajax_parameter = "Server can't process the request";
    return send_page(connection, MHD_HTTP_BAD_REQUEST, pages[error_page], 0, new_cookie);
}




