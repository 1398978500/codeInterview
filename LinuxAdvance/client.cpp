#include <arpa/inet.h>
#include <assert.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>

using namespace std;
#define BUF_SIZE (10240)

int main(int argc, char const* argv[])
{
    const char* ip = "127.0.0.1";
    int port = 8080;
    int backlog = 100;

    int sockFd = socket(PF_INET, SOCK_STREAM, 0);
    assert(sockFd >= 0);

    // 创建ipv4socket地址
    sockaddr_in stAddress;
    bzero(&stAddress, sizeof(stAddress));
    stAddress.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &stAddress.sin_addr);
    stAddress.sin_port = htons(port);

    int ret = connect(sockFd, (sockaddr*)&stAddress, sizeof(stAddress));
    if (ret < 0) {
        printf("connect failed\n");
    }
    else {
        // const char* oob = "abc";
        const char* normal = "123";

        send(sockFd, normal, strlen(normal), 0);
        // sleep(1);
        // send(sockFd, oob, strlen(oob), 0);
        // sleep(1);
        // send(sockFd, normal, strlen(normal), 0);
        char buf[BUF_SIZE] = "";

        memset(buf, 0, BUF_SIZE);
        ret = recv(sockFd, buf, BUF_SIZE - 1, 0);
        printf("got %d bytes of normal data '%s' \n", ret, buf);
    }

    close(sockFd);
    return 0;
}
