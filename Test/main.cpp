#include <stdlib.h>
#include <iostream>
#include "mysql/mysql.h"
#include "redis++.h"

#include <mysql_connection.h>

#include <cppconn/driver.h>
#include <cppconn/exception.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>

#include <string>
#include <thread>

using namespace std;
using namespace sw::redis;

void mysqlTest();
void redisTest();
int mysqlppTest();

void threadTest(string& msg);

class Fctor {
public:
    void operator()(const string& msg)
    {
        cout << __FUNCTION__ << " " << msg << endl;
        // msg = "world";
    }
};

int main(int argc, char const* argv[])
{
#ifdef REDIS_TEST
    redisTest();
#endif

    string msg = "hello";
    thread t1(Fctor(), move(msg));
    t1.join();

    cout << __FUNCTION__ << "  " << msg << endl;

    return 0;
}

void threadTest(string& msg)
{
    cout << __FUNCTION__ << "   " << msg << endl;
    msg = "world";
}



int mysqlppConn()
{
    cout << endl;
    cout << "Running 'SELECT " << endl;

    try {
        sql::Driver* driver;
        sql::Connection* con;
        sql::Statement* stmt;
        sql::ResultSet* res;

        /* Create a connection */
        driver = get_driver_instance();
        con = driver->connect("tcp://127.0.0.1:3306", "longwei", "123456");
        /* Connect to the MySQL test database */
        con->setSchema("db_test");

        stmt = con->createStatement();

        res = stmt->executeQuery("SELECT * from t_test;");
        auto field = res->getMetaData();
        int iSz = field->getColumnCount();

        for (int i = 1; i <= iSz; i++)
            cout << field->getColumnName(i) << "\t" << endl;

        while (res->next()) {
            // cout << " \t iSz ===" << iSz << endl;

            for (int i = 1; i <= iSz; i++)
                cout << "\t" << res->getString(i) << endl;
        }
        delete res;
        delete stmt;
        delete con;
    }
    catch (sql::SQLException& e) {
        cout << "# ERR: SQLException in " << __FILE__;
        cout << "(" << __FUNCTION__ << ") on line " << __LINE__ << endl;
        cout << "# ERR: " << e.what();
        cout << " (MySQL error code: " << e.getErrorCode();
        cout << ", SQLState: " << e.getSQLState() << " )" << endl;
    }

    cout << endl;

    return EXIT_SUCCESS;
}

void redisTest()
{
    try {
        auto redis = Redis("tcp://127.0.0.1:6379");
        redis.set("key", "val");
        auto val = redis.get("key");  // val is of type OptionalString. See 'API Reference' section for details.
        if (val) {
            // Dereference val to get the returned value of std::string type.
            std::cout << *val << std::endl;
        }  // else key doesn't exist.
    }
    catch (const Error& e) {
        // Error handling.
        cout << e.what() << endl;
    }
}

void mysqlTest()
{
    MYSQL mysql;
    MYSQL_RES* res;
    MYSQL_ROW row;
    char query[] = "select * from t_test where id > 0;";
    int t, r;
    mysql_init(&mysql);
    if (!mysql_real_connect(&mysql, "localhost", "longwei", "123456", "db_test", 3306, NULL, 0)) {
        printf("Error connecting to database:%s\n", mysql_error(&mysql));
    }
    else {
        printf("Connected........\n");
    }

    t = mysql_query(&mysql, query);
    if (t != 0) {
        printf("Error making query:%s\n", mysql_error(&mysql));
    }
    else {
        printf("Query made ....\n");
        res = mysql_use_result(&mysql);
        if (res) {
            int iField = mysql_field_count(&mysql);
            cout << iField << endl;

            for (r = 0; r <= iField; r++) {
                row = mysql_fetch_row(res);
                if (row < 0) {
                    break;
                }

                for (t = 0; t < mysql_num_fields(res); t++) {
                    printf("%s ", row[t]);
                }

                printf("\n");
            }
        }
        mysql_free_result(res);
    }
    mysql_close(&mysql);
}
