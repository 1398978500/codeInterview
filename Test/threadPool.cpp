#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <deque>
#include <iostream>

using namespace std;

static inline void ERR_EXIT_THREAD(int err, const char* msg)
{
    fprintf(stderr, "%s:%s", strerror(err), msg);
    exit(EXIT_FAILURE);
}

class ThreadPool;

// 任务队列
struct TaskEle {
public:
    void (*taskCallBack)(void* arg);  // 回调函数
    string user_data;                 // 函数参数

    void setFunc(void (*tcb)(void* arg))
    {
        taskCallBack = tcb;
    }
};

// 执行队列节点数据结构
struct ExecEle {
public:
    pthread_t tid;       // 线程id
    bool usable = true;  // 当前线程是否可用
    ThreadPool* pool;    // 指向中枢管理的指针

    static void* start(void* arg);
};

// 线程中枢管理
struct ThreadPool {
public:
    // 任务队列和执行队列
    deque<TaskEle*> task_queue;
    deque<ExecEle*> exec_queue;

    // 条件变量
    pthread_cond_t cont;

    // 互斥锁
    pthread_mutex_t mutex;

    // 线程池大小
    int thread_count;

    // 构造函数
    ThreadPool(int thread_count) : thread_count(thread_count)
    {
        pthread_cond_init(&cont, NULL);
        pthread_mutex_init(&mutex, NULL);
    }

    // 创建线程池
    void createPool()
    {
        int ret;

        // 初始化执行队列
        for (int i = 0; i < thread_count; ++i) {
            ExecEle* ee = new ExecEle;
            ee->pool = const_cast<ThreadPool*>(this);
            if (ret = pthread_create(&(ee->tid), NULL, ee->start, ee)) {
                delete ee;
                ERR_EXIT_THREAD(ret, "pthread_create");
            }

            // 线程分离
            for (int i = 0; i < exec_queue.size(); ++i) {
                pthread_detach(exec_queue[i]->tid);
            }

            fprintf(stdout, "create thread %d\n", i);
            exec_queue.push_back(ee);
        }
        fprintf(stdout, "create pool finish ...\n");
    }

    // 加入任务
    void push_task(void (*tcb)(void* arg), int i)
    {
        TaskEle* te = new TaskEle;
        te->setFunc(tcb);
        te->user_data = "Task " + to_string(i) + " run in thread";

        // 加锁
        pthread_mutex_lock(&mutex);

        task_queue.push_back(te);

        // 通知执行队列
        pthread_cond_signal(&cont);

        // 解锁
        pthread_mutex_unlock(&mutex);
    }

    // 析构
    ~ThreadPool()
    {
        for (int i = 0; i < exec_queue.size(); ++i) {
            exec_queue[i]->usable = false;
        }
        pthread_mutex_lock(&mutex);

        // 清空队列
        task_queue.clear();

        // 广播给每个执行线程令其推出
        pthread_cond_broadcast(&cont);

        pthread_mutex_unlock(&mutex);

        // 清空执行队列
        exec_queue.clear();

        // 销毁
        pthread_cond_destroy(&cont);
        pthread_mutex_destroy(&mutex);
    }
};

void* ExecEle::start(void* arg)
{
    ExecEle* ee = (ExecEle*)arg;

    while (true) {
        pthread_mutex_lock(&(ee->pool->mutex));
        while (ee->pool->task_queue.empty()) {  // 队列为空 等待新任务
            if (!ee->usable) {
                break;
            }
            pthread_cond_wait(&(ee->pool->cont), &(ee->pool->mutex));
        }

        if (!ee->usable) {
            pthread_mutex_unlock(&(ee->pool->mutex));
            break;
        }

        TaskEle* te = ee->pool->task_queue.front();
        ee->pool->task_queue.pop_front();

        pthread_mutex_unlock(&(ee->pool->mutex));

        // 执行回调
        te->user_data += to_string(pthread_self());
        te->taskCallBack(te);
    }

    delete ee;
    fprintf(stdout, "destroy thread %ld\n", pthread_self());

    return NULL;
}

// 线程执行的业务函数
void execFunc(void* arg)
{
    TaskEle* te = (TaskEle*)arg;
    fprintf(stdout, "%s\n", te->user_data.c_str());
}

int main(int argc, char const* argv[])
{
    if (argc < 2) {
        cout << "确少参数" << endl;
        cout << "格式 " << argv[0] << " 线程数" << endl;
        exit(EXIT_FAILURE);
    }

    int num = atoi(argv[1]);
    ThreadPool pool(num);
    pool.createPool();
    // 创建任务
    for (int i = 0; i < 10; ++i) {
        pool.push_task(&execFunc, i);
    }

    sleep(1);

    return 0;
}
