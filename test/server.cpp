#include <iostream>

#include "httplib.h"

int main() {
    httplib::Server svr;
    svr.Post("/alarm/report", [](const httplib::Request &req, httplib::Response &res) {
        std::cout << req.body << std::endl;
        res.set_content("Hello World!", "text/plain");
    });
    svr.listen("0.0.0.0", 8080);
}