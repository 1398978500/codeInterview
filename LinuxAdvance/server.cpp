#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/sendfile.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>

using namespace std;

void byteOrder();
void p2nTest();
void serverTest();
void readvTest(const char* ip, int port, const char* file_name);
void sendfileTest(const char* ip, int port, const char* file_name);
void spliceTest(const char* ip, int port);
void teeTest(const char* filename);

int main(int argc, char const* argv[])
{
    // byteOrder();

    // p2nTest();

    // serverTest();

    // readvTest("127.0.0.1", 8080, "client.cpp");

    // sendfileTest("127.0.0.1", 8080, "client.cpp");

    // spliceTest("127.0.0.1", 8080);

    teeTest("test.txt");

    return 0;
}

#define BUFFER_SIZE (1024)

void teeTest(const char* filename)
{
    int filefd = open(filename, O_CREAT | O_WRONLY | O_TRUNC, 0666);
    assert(filefd > 0);

    int pipe_stdout[2];
    int ret = pipe(pipe_stdout);
    assert(ret != -1);

    int pipe_file[2];
    ret = pipe(pipe_file);
    assert(ret != -1);

    // 标准输入 输入pipe_stdout
    ret = splice(STDIN_FILENO, NULL, pipe_stdout[1], NULL, 32768, SPLICE_F_MORE | SPLICE_F_MOVE);
    if (ret == -1) {
        perror("splicd error:");
        close(filefd);
        close(pipe_stdout[0]);
        close(pipe_stdout[1]);
        close(pipe_file[0]);
        close(pipe_file[1]);
        exit(1);
    }

    // 将pipe_stdout 输出端 定向到pipe_file
    ret = tee(pipe_stdout[0], pipe_file[1], 32768, SPLICE_F_NONBLOCK);
    if (ret == -1) {
        perror("splicd error:");
        close(filefd);
        close(pipe_stdout[0]);
        close(pipe_stdout[1]);
        close(pipe_file[0]);
        close(pipe_file[1]);
        exit(1);
    }

    // 将pipe_file 输出端 定向到filefd 即将标准输入写入文件
    ret = splice(pipe_file[0], NULL, filefd, NULL, 32768, SPLICE_F_MORE | SPLICE_F_MOVE);
    if (ret == -1) {
        perror("splicd error:");
        close(filefd);
        close(pipe_stdout[0]);
        close(pipe_stdout[1]);
        close(pipe_file[0]);
        close(pipe_file[1]);
        exit(1);
    }

    // 将pipe_stdout 输出端定向到标准输出 与写入文件内容相同
    ret = splice(pipe_stdout[0], NULL, STDOUT_FILENO, NULL, 32768, SPLICE_F_MORE | SPLICE_F_MOVE);
    if (ret == -1) {
        perror("splicd error:");
        close(filefd);
        close(pipe_stdout[0]);
        close(pipe_stdout[1]);
        close(pipe_file[0]);
        close(pipe_file[1]);
        exit(1);
    }

    close(filefd);
    close(pipe_stdout[0]);
    close(pipe_stdout[1]);
    close(pipe_file[0]);
    close(pipe_file[1]);
}

void spliceTest(const char* ip, int port)
{
    sockaddr_in address;
    bzero(&address, sizeof(address));
    address.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &address.sin_addr);
    address.sin_port = htons(port);

    int sock = socket(PF_INET, SOCK_STREAM, 0);
    assert(sock >= 0);

    int ret = bind(sock, (sockaddr*)&address, sizeof(address));
    assert(ret != -1);

    ret = listen(sock, 5);
    assert(ret != -1);

    sockaddr_in client;
    socklen_t client_len = sizeof(client);
    int connfd = accept(sock, (sockaddr*)&client, &client_len);
    if (connfd < 0) {
        perror("erron is: ");
    }
    else {
        int pipefd[2];
        ret = pipe(pipefd);
        assert(ret != -1);

        ret = splice(connfd, NULL, pipefd[1], NULL, 32768, SPLICE_F_MORE | SPLICE_F_MOVE);
        assert(ret != -1);

        ret = splice(pipefd[0], NULL, connfd, NULL, 32768, SPLICE_F_MORE | SPLICE_F_MOVE);
        assert(ret != -1);

        close(connfd);
    }

    close(sock);
}

void sendfileTest(const char* ip, int port, const char* file_name)
{
    sockaddr_in address;
    bzero(&address, sizeof(address));
    address.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &address.sin_addr);
    address.sin_port = htons(port);

    int filefd = open(file_name, O_RDONLY);
    assert(filefd > 0);
    struct stat stat_buf;
    fstat(filefd, &stat_buf);

    int sock = socket(PF_INET, SOCK_STREAM, 0);
    assert(sock >= 0);

    int ret = bind(sock, (sockaddr*)&address, sizeof(address));
    assert(ret != -1);

    ret = listen(sock, 5);
    assert(ret != -1);

    sockaddr_in client;
    socklen_t client_len = sizeof(client);
    int connfd = accept(sock, (sockaddr*)&client, &client_len);
    if (connfd < 0) {
        perror("erron is: ");
    }
    else {
        sendfile(connfd, filefd, NULL, stat_buf.st_size);
        close(connfd);
    }

    close(sock);
}

static const char* status_line[2] = {"200 ok", "500 internal server error"};
void readvTest(const char* ip, int port, const char* file_name)
{
    sockaddr_in address;
    bzero(&address, sizeof(address));
    address.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &address.sin_addr);
    address.sin_port = htons(port);

    int sock = socket(PF_INET, SOCK_STREAM, 0);
    assert(sock >= 0);

    int ret = bind(sock, (sockaddr*)&address, sizeof(address));
    assert(ret != -1);

    ret = listen(sock, 5);
    assert(ret != -1);

    sockaddr_in client;
    socklen_t client_len = sizeof(client);
    int connfd = accept(sock, (sockaddr*)&client, &client_len);
    if (connfd < 0) {
        perror("erron is: ");
    }
    else {
        char header_buf[BUFFER_SIZE];
        memset(header_buf, 0, BUFFER_SIZE);

        char* file_buf;
        struct stat file_stat;
        bool valid = true;
        int len = 0;
        if (stat(file_name, &file_stat) < 0) {
            valid = false;
        }
        else {
            if (S_ISDIR(file_stat.st_mode)) {
                valid = false;
            }
            else if (file_stat.st_mode & S_IROTH) {  // 当前用户有读取目标文件权限
                int fd = open(file_name, O_RDONLY);
                file_buf = new char[file_stat.st_size + 1];
                memset(file_buf, 0, file_stat.st_size + 1);
                if (read(fd, file_buf, file_stat.st_size) < 0) {
                    valid = false;
                }
            }
            else {
                valid = false;
            }
        }

        // 有效， 则发送正常http应答
        if (valid) {
            // 状态行、 头部字段和一个空行加入header_buf
            ret = snprintf(header_buf, BUFFER_SIZE - 1, "%s %s\r\n", "HTTP/1.1", status_line[0]);
            len += ret;

            ret = snprintf(header_buf + len, BUFFER_SIZE - 1 - len, "Content-Length: %lu\r\n", file_stat.st_size);
            len += ret;

            ret = snprintf(header_buf, BUFFER_SIZE - 1 - len, "%s", "\r\n");

            // 利用writev将 header_buf 和 file_buf 内容一并写出
            iovec iv[2];
            iv[0].iov_base = header_buf;
            iv[0].iov_len = strlen(header_buf);
            iv[1].iov_base = file_buf;
            iv[1].iov_len = file_stat.st_size;
            ret = writev(connfd, iv, 2);
        }
        // 目标文件无效 通知错误
        else {
            ret = snprintf(header_buf, BUFFER_SIZE - 1, "%s %s\r\n", "HTTP/1.1", status_line[1]);
            len += ret;

            ret = snprintf(header_buf, BUFFER_SIZE - 1 - len, "%s", "\r\n");
            send(connfd, header_buf, strlen(header_buf), 0);
        }
        close(connfd);
        delete[] file_buf;
    }
    close(sock);
}

static bool stop = false;

static void handleTerm(int sig)
{
    cout << "收到信号" << sig << endl;
    stop = true;
}

void serverTest()
{
    signal(SIGTERM, handleTerm);
    signal(SIGINT, handleTerm);

    const char* ip = "127.0.0.1";
    int port = 8082;
    int backlog = 100;

    int sockFd = socket(PF_INET, SOCK_STREAM, 0);
    assert(sockFd >= 0);

    // 创建ipv4socket地址
    sockaddr_in stAddress;
    bzero(&stAddress, sizeof(stAddress));
    stAddress.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &stAddress.sin_addr);
    stAddress.sin_port = htons(port);

    int ret = bind(sockFd, (sockaddr*)&stAddress, sizeof(stAddress));
    assert(ret != -1);

    ret = listen(sockFd, backlog);
    assert(ret != -1);

    // sleep(20);

    const int BUF_SIZE = 1024;
    sockaddr_in client;
    socklen_t clientLen = sizeof(client);
    int connfd = accept(sockFd, (sockaddr*)&client, &clientLen);
    if (connfd < 0) {
        printf("error is %d\n", errno);
    }
    else {
        char remote[INET_ADDRSTRLEN];
        printf("connected with ip: %s and port: %d \n",
               inet_ntop(AF_INET, &client.sin_addr, remote, INET_ADDRSTRLEN),
               client.sin_port);

        char buf[BUF_SIZE] = "";

        memset(buf, 0, BUF_SIZE);
        ret = recv(connfd, buf, BUF_SIZE - 1, 0);
        printf("got %d bytes of normal data '%s' \n", ret, buf);

        memset(buf, 0, BUF_SIZE);
        ret = recv(connfd, buf, BUF_SIZE - 1, 0);
        printf("got %d bytes of oob data '%s' %d \n", ret, buf, errno);

        memset(buf, 0, BUF_SIZE);
        ret = recv(connfd, buf, BUF_SIZE - 1, 0);
        printf("got %d bytes of normal data '%s' \n", ret, buf);

        close(connfd);
    }

    // while (!stop) {
    //     cout << "等待链接" << endl;
    //     sleep(1);
    // }

    close(sockFd);

    return;
}

void byteOrder()
{
    union {
        short value;
        char c[sizeof(short)];
    } test;

    test.value = 0x0102;

    if (test.c[0] == 0x01 && test.c[1] == 0x02) {
        cout << "big" << endl;
    }
    else if (test.c[0] == 0x02 && test.c[1] == 0x01) {
        cout << "small" << endl;
    }
    else {
        cout << "unknown" << endl;
    }
}

void p2nTest()
{
    in_addr dst;
    int iRet = inet_pton(AF_INET, "0.0.0.1", &dst);

    cout << dst.s_addr << endl;

    char src[16];
    inet_ntop(AF_INET, &dst, src, INET_ADDRSTRLEN);

    printf("%s\n", src);
}
