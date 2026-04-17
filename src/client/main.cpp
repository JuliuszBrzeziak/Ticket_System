#include <iostream>
#include <httplib.h>

int main()
{
    httplib::Client cli("localhost", 8080);

    auto res = cli.Get("/ping");
    if (res && res->status == 200)
    {
        std::cout << "Odpowiedz serwera: " << res->body << std::endl;
    }
    else
    {
        std::cout << "Blad polaczenia z serwerem!" << std::endl;
    }
    return 0;
}