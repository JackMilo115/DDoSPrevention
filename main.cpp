#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <iostream>
#include <fstream>
#include <sqlite3.h>

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = net::ip::tcp;

void initialize_database() {
    sqlite3* db;
    char* err_msg = 0;
    int rc = sqlite3_open("ips.db", &db);

    if (rc != SQLITE_OK) {
        std::cerr << "Cannot open database: " << sqlite3_errmsg(db) << std::endl;
        sqlite3_close(db);
        return;
    }

    std::string sql = "CREATE TABLE IF NOT EXISTS ips ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "ip TEXT UNIQUE, "
        "count INTEGER DEFAULT 1);";

    rc = sqlite3_exec(db, sql.c_str(), 0, 0, &err_msg);
    if (rc != SQLITE_OK) {
        std::cerr << "SQL error: " << err_msg << std::endl;
        sqlite3_free(err_msg);
    }
    else {
        std::cout << "Table created or already exists.\n";
    }

    sqlite3_close(db);
}

void log_ip(const std::string& ip) {
    sqlite3* db;
    char* err_msg = 0;
    int rc = sqlite3_open("ips.db", &db);

    if (rc != SQLITE_OK) {
        std::cerr << "Cannot open database: " << sqlite3_errmsg(db) << std::endl;
        sqlite3_close(db);
        return;
    }

    std::string sql_select = "SELECT count FROM ips WHERE ip = '" + ip + "';";
    sqlite3_stmt* stmt;
    rc = sqlite3_prepare_v2(db, sql_select.c_str(), -1, &stmt, nullptr);

    if (rc != SQLITE_OK) {
        std::cerr << "Failed to fetch IP: " << sqlite3_errmsg(db) << std::endl;
        sqlite3_finalize(stmt);
        sqlite3_close(db);
        return;
    }

    rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) {
        int count = sqlite3_column_int(stmt, 0);
        count++;
        std::string sql_update = "UPDATE ips SET count = " + std::to_string(count) + " WHERE ip = '" + ip + "';";
        rc = sqlite3_exec(db, sql_update.c_str(), 0, 0, &err_msg);

        if (rc != SQLITE_OK) {
            std::cerr << "SQL error: " << err_msg << std::endl;
            sqlite3_free(err_msg);
        }
        else {
            std::cout << "IP count updated successfully\n";
        }
    }
    else {
        std::string sql_insert = "INSERT INTO ips (ip, count) VALUES ('" + ip + "', 1);";
        rc = sqlite3_exec(db, sql_insert.c_str(), 0, 0, &err_msg);

        if (rc != SQLITE_OK) {
            std::cerr << "SQL error: " << err_msg << std::endl;
            sqlite3_free(err_msg);
        }
        else {
            std::cout << "IP logged successfully\n";
        }
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);
}


void handle_request(http::request<http::string_body>& req, const std::string& ip)
{
    //if (req.method() == http::verb::get && req.target() == "/log_ip") {
    log_ip(ip);
    //}
}

void do_session(tcp::socket socket)
{
    bool close = false;
    beast::error_code ec;

    beast::flat_buffer buffer;
    http::request<http::string_body> req;

    http::read(socket, buffer, req, ec);
    if (ec == http::error::end_of_stream)
        return;

    auto ip = socket.remote_endpoint().address().to_string();

    handle_request(req, ip);

    http::response<http::string_body> res{ http::status::ok, req.version() };
    res.set(http::field::server, "Test Server");
    res.set(http::field::content_type, "text/plain");
    res.keep_alive(req.keep_alive());
    res.body() = "IP logged";
    res.prepare_payload();

    http::write(socket, res, ec);

    socket.shutdown(tcp::socket::shutdown_send, ec);
}

int main() {
    try {
        initialize_database();
        auto const address = net::ip::make_address("0.0.0.0");
        unsigned short port = static_cast<unsigned short>(8080);

        net::io_context ioc{ 1 };

        tcp::acceptor acceptor{ ioc, {address, port} };
        for (;;) {
            tcp::socket socket{ ioc };
            acceptor.accept(socket);

            std::thread(do_session, std::move(socket)).detach();
        }
    }
    catch (std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
}