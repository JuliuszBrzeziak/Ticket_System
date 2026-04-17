#include <iostream>
#include <httplib.h>

int main()
{
    httplib::Server svr;

    svr.Get("/ping", [](const httplib::Request &, httplib::Response &res)
            { res.set_content("pong", "text/plain"); });

    std::cout << "Serwer dziala na porcie 8080..." << std::endl;
    svr.listen("0.0.0.0", 8080);
    return 0;
}