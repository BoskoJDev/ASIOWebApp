#include <iostream>

#ifdef _WIN64
    #define _WIN64_WINNT 0x0601
#endif

#define ASIO_STANDALONE
#include "asio.hpp" // Core of ASIO library
#include "asio/ts/buffer.hpp" // Part of ASIO which handles memory movement
#include "asio/ts/internet.hpp" // Part of ASIO which handles network communication

using namespace asio;

//I created a vector of reasonable size because I don't know how much data I'll receive
static std::vector<char> netBuffer(20 * 1024);

static void GetDataAsync(ip::tcp::socket& s)
{
    s.async_read_some(buffer(netBuffer.data(), netBuffer.size()),
        [&](error_code ec, size_t size)
        {
            if (ec)
                return;

            std::cout << "\n\nReading " << size << " bytes of data.\n\n";
            for (int i = 0; i < size; i++)
                std::cout << netBuffer[i];

            // This function will be recursively called until all incoming data is read
            GetDataAsync(s);
        });
}

int main()
{
    error_code ec;

    io_context context; // This is platform specific interface

    // Creating some fake work for the context
    io_context::work fakeWork(context);

    std::thread contextThread([&context]() { context.run(); });

    //Getting the adress of somewhere we wish to connect to
    ip::tcp::endpoint endpoint(ip::make_address("51.38.81.49", ec), 80);

    // Creation of socket using context to deliver implementation
    ip::tcp::socket socket(context);
    socket.connect(endpoint, ec);
    if (ec)
    {
        std::cout << "Failed to connect: " << ec.message() << '\n';
        return 1;
    }

    std::cout << "Connected!\n\n";

    if (!socket.is_open())
        return 1;

    GetDataAsync(socket);

    std::string_view httpRequest =
        "GET /index.html HTTP/1.1\r\n"
        "Host: example.com\r\n"
        "Connection: close\r\n\r\n";

    // Sending HTTP request
    socket.write_some(buffer(httpRequest.data(), httpRequest.size()), ec);

    std::cin.get();

    context.stop();
    contextThread.join();

    return 0;
}