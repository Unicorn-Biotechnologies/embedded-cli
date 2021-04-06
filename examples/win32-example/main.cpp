/**
 * Simple example of using embedded-cli in win32 application.
 * Create emulator of terminal which prints entered commands and args
 */

#include <iostream>
#include <thread>
#include <string>
#include <chrono>
#include <Windows.h>

#include "embedded_cli.h"

static bool exitFlag = false;

void onCommand(const std::string &name, char *tokens);

void onExit(EmbeddedCli *cli, char *args, void *context);

void onHello(EmbeddedCli *cli, char *args, void *context);

int main() {
    HANDLE hConsole = GetStdHandle(STD_INPUT_HANDLE);

    if (hConsole == INVALID_HANDLE_VALUE) {
        std::cerr << "Can't get handle to console\n";
        return -1;
    }

    EmbeddedCli *cli = embeddedCliNewDefault();

    cli->onCommand = [](EmbeddedCli *embeddedCli, CliCommand *command) {
        embeddedCliTokenizeArgs(command->args);
        onCommand(command->name == nullptr ? "" : command->name, command->args);
    };
    cli->writeChar = [](EmbeddedCli *embeddedCli, char c) {
        std::cout << c;
    };

    CliCommandBinding bindings[] = {
            {
                    "exit",
                    "Stop CLI and exit",
                    false,
                    nullptr,
                    onExit
            },
            {
                    "hello",
                    "Print hello message",
                    false,
                    (void *) "World",
                    onHello
            },
    };
    embeddedCliSetBindings(cli, bindings, 2);

    std::cout << "Cli is running. Press 'Esc' to exit\n";

    auto lastUpdate = std::chrono::steady_clock::now();
    auto t0 = std::chrono::steady_clock::now();

    while (!exitFlag) {
        auto now = std::chrono::steady_clock::now();
        auto passed = std::chrono::duration_cast<std::chrono::milliseconds>(now - t0).count();
        if (std::chrono::duration_cast<std::chrono::seconds>(now - lastUpdate).count() > 1) {
            std::string str = std::to_string(passed) + "ms passed";
            embeddedCliPrint(cli, str.c_str());
            lastUpdate = now;
        }

        INPUT_RECORD record;
        DWORD read;
        GetNumberOfConsoleInputEvents(hConsole, &read);

        if (read == 0) {
            using namespace std::chrono_literals;
            std::this_thread::sleep_for(10ms);
            continue;
        }

        ReadConsoleInput(hConsole, &record, 1, &read);

        if (read > 0 && record.EventType == KEY_EVENT) {
            KEY_EVENT_RECORD event = record.Event.KeyEvent;
            if (!event.bKeyDown)
                continue;
            char aChar = event.uChar.AsciiChar;

            // escape code
            if (aChar == 27)
                break;

            if (aChar > 0)
                embeddedCliReceiveChar(cli, aChar);

            embeddedCliProcess(cli);
        }
    }

    return 0;
}

void onCommand(const std::string &name, char *tokens) {
    std::cout << "Received command: " << name << "\n";

    for (int i = 0; i < embeddedCliGetTokenCount(tokens); ++i) {
        std::cout << "Arg " << i << ": " << embeddedCliGetToken(tokens, i) << "\n";
    }
}

void onExit(EmbeddedCli *cli, char *args, void *context) {
    exitFlag = true;
    std::cout << "Cli will shutdown now...\r\n";
}

void onHello(EmbeddedCli *cli, char *args, void *context) {
    std::cout << "Hello, ";
    if (embeddedCliGetTokenCount(args) == 0)
        std::cout << (const char *) context;
    else
        std::cout << embeddedCliGetToken(args, 0);
    std::cout << "\r\n";
}